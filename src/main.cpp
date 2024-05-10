#include <Arduino.h>
#include <memory>

#include "util.h"
#include "reqres.h"
#include "commands.h"

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
