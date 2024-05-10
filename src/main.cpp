#include <Arduino.h>
#include <memory>

#include "reqres.h"

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
  expectResponse(ok, 6, "setScreenDir OK", noop);

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
  handshook = true;
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

  expectResponse(ok, 2, "handshake OK", on_handshaked);
  sendHandshake();
}

long loopno = 0;

void loop()
{
  while (dwinSerial.available())
  {
    uint8_t c = dwinSerial.read();
    handleReceived(c);
  }
}
