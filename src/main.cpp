#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>

#include "radio.h"
#include "microphone.h"
#include "audio_dsp.h"
#include "audio_encoder.h"

uint32_t txPacketCount = 0;
uint32_t lastTxTime = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupRadio();
    setupMicrophone();
    resetAudioEncoder();

    Serial.println("Setup complete. Starting modular 24-bit audio processing...");
    lastTxTime = millis();
}

void loop()
{
    int32_t i2s_data[64];
    
    // Читаем порцию данных с микрофона
    int samples_read = readMicrophoneData(i2s_data, 64);

    for (int i = 0; i < samples_read; i++)
    {
        // 1. Извлекаем чистые знаковые 24 бита из 32-битного контейнера I2S
        int32_t raw_24 = i2s_data[i] >> 8;

        // 2. Студийное АРУ + компрессия + лимитер
        int16_t processed_pcm_16 = applyStudioProcessing24Bit(raw_24);

        // 3. Кодирование в 4-битный ADPCM и отправка по заполнению
        processPcmSample(processed_pcm_16);
    }

    // Каждую секунду выводим статистику PPS в Serial монитор
    if (millis() - lastTxTime >= 1000)
    {
        Serial.printf("TX Packets per second: %u\n", txPacketCount);
        txPacketCount = 0;
        lastTxTime = millis();
    }
}