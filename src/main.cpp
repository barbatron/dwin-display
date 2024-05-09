#include <Arduino.h>
#include <CircularBuffer.hpp>
#include <memory>

struct dwinDrawRectParam
{
  uint8_t mode;
  uint16_t color;
  uint16_t x0;
  uint16_t y0;
  uint16_t x1;
  uint16_t y1;
};

HardwareSerial dwinSerial = Serial2;

void noop()
{
}

bool handshook = false;

struct ExpectedResponse
{
  uint8_t *data;
  int len;
  int received;
  const char *message;
  void (*onComplete)(void);
};

CircularBuffer<ExpectedResponse *, 10> expectedResponses;

void expectResponse(const uint8_t respData[], const char *message, void (*onComplete)(void))
{
  int len = sizeof respData - 2;
  if (expectedResponses.isFull())
  {
    Serial.println("Response buffer full!");
    return;
  }
  Serial.printf("Adding response expectation for \"%s\" (%d bytes): ", message, len);
  for (int i = 0; i < len; i++)
  {
    Serial.print(respData[i], DEC);
    Serial.print("=");
    Serial.print(respData[i], HEX);
    Serial.print("  ");
  }
  Serial.println();
  uint8_t *dataData = (uint8_t *)std::malloc(len);
  memcpy(dataData, respData, len);
  ExpectedResponse *er = new ExpectedResponse{
      dataData,
      len,
      0,
      message,
      onComplete,
  };
  expectedResponses.push(er);
  Serial.print("First bytee of new ER: ");
  Serial.print(er->data[0], HEX);
  Serial.println();
}

void fulfillResponse()
{
  if (expectedResponses.isEmpty())
  {
    Serial.println("Attempt to fulfill response but expectedResponse empty");
    return;
  }
  ExpectedResponse *fulfilledResp = expectedResponses.shift();
  std::free(fulfilledResp->data);
  Serial.printf("Shifted fulfilled response: %s", fulfilledResp->message);

  if (fulfilledResp->onComplete)
  {
    fulfilledResp->onComplete();
  }
}

void handleReceived(uint8_t c)
{
  if ((expectedResponses.isEmpty()))
    return;

  ExpectedResponse *resp = expectedResponses.first();
  int idx = resp->received;
  uint8_t expectByte = resp->data[idx];

  Serial.printf("Expecting response for \"%s\" - awaiting %x @ %d, got %x\n",
                resp->message,
                expectByte,
                idx,
                c);

  if (c == expectByte)
  {
    resp->received = resp->received + 1;
    bool isDone = resp->received >= resp->len;
    Serial.printf("Matched expectation! i=%d len=%d str=%s %s\n",
                  resp->received,
                  resp->len,
                  resp->message,
                  isDone ? "- DONE" : "");
    if (isDone)
    {
      fulfillResponse();
    }
  }
}

void send(uint8_t *data, int len)
{
  if (!dwinSerial.availableForWrite())
  {
    Serial.println("dwinSerial not available for write - skipping handshake");
    return;
  }

  // crc.reset();
  uint8_t buf[len + 5];
  int i = 0;

  // header
  buf[i++] = 0xAA;

  // data
  for (int j = 0; j < len; j++)
  {
    buf[i++] = data[j];
  }

  // footer
  buf[i++] = 0xCC;
  buf[i++] = 0x33;
  buf[i++] = 0xC3;
  buf[i++] = 0x3C;

  dwinSerial.write(buf, i);
}

void sendHandshake()
{
  uint8_t data[] = {0x00};
  send(data, 1);
  Serial.println("Sent handshake");
}

void showJpeg(uint8_t id)
{
  uint8_t data[] = {0x00, 0x22, id};
  send(data, 3);
  Serial.printf("Showed jpeg %d\n", id);
}

void setScreenDir(uint8_t dir)
{
  uint8_t data[] = {0x34, 0x5A, 0xA5, dir};
  send(data, 4);

  const uint8_t ok[] = {0x4F, 0x4B, 0xCC, 0x33, 0xC3, 0x3C};
  expectResponse(ok, "setScreenDir OK", noop);

  Serial.printf("Set screen dir %d\n", dir);
}

void clear(uint16_t color)
{
  uint8_t data[] = {0x3D, highByte(color), lowByte(color)};
  send(data, 5);
  Serial.printf("Cleared screen color=%x\n", color);
}

void update()
{
  uint8_t data[] = {0x3D};
  send(data, 1);
  Serial.println("Updated");
}

void drawRect(
    uint8_t mode, uint16_t color,
    uint16_t x0, uint16_t y0,
    uint16_t x1, uint16_t y1)
{
  uint8_t data[] = {
      0x05,
      mode,
      highByte(color),
      lowByte(color),
      highByte(x0),
      lowByte(x0),
      highByte(y0),
      lowByte(y0),
      highByte(x1),
      lowByte(x1),
      highByte(y1),
      lowByte(y1),
  };
  send(data, 12);
  Serial.println("Drew rect");
}

void on_handshaked()
{
  Serial.println("Handshake complete");
  clear(0);
  showJpeg(0);
  setScreenDir(1);
  drawRect(0x01, 0xF00F, 10, 10, 100, 100);
  update();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  // Log serial
  Serial.begin(115200);
  Serial.println("Setup complete");

  // DWIN serial
  dwinSerial.begin(115200, SERIAL_8N1, 18, 19);
  Serial.println("DWIN serial begun, sending handshake");

  const uint8_t ok[2] = {0xC3, 0x3C};
  expectResponse(ok, "handshake OK", on_handshaked);
}

long loopno = 0;

void loop()
{
  if (!handshook && (loopno % 1500000 == 0))
  {
    Serial.println("Handshake timeout - re-sending");
    sendHandshake();
  }
  loopno++;

  while (dwinSerial.available())
  {
    uint8_t c = dwinSerial.read();
    Serial.print("I received: ");
    Serial.println(c, HEX);
    handleReceived(c);
  }
}
