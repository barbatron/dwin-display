#ifndef __UTIL
#define __UTIL

#include <Arduino.h>

HardwareSerial dwinSerial = Serial2;

void noop()
{
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

#endif