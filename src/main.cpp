#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>
#include "radio.h"

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupRadio();
    Serial.println("Setup complete.");
}

static uint32_t packetCounter = 0;

void loop()
{
    radio.send(
        2,
        (const void*)&packetCounter,61
    );

    packetCounter++;
}