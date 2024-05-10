#ifndef __REQRES
#define __REQRES

#include <Arduino.h>
#include <CircularBuffer.hpp>
#include <memory>

struct ExpectedResponse
{
  uint8_t *data;
  int len;
  int received;
  const char *message;
  void (*onComplete)(void);
};

CircularBuffer<ExpectedResponse *, 10> expectedResponses;

void expectResponse(const uint8_t respData[], const int len, const char *message, void (*onComplete)(void))
{
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
  Serial.print("I received: ");
  Serial.println(c, HEX);

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

#endif