#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <Arduino.h>
#include <driver/i2s_std.h> // Современный драйвер

#define I2S_PORT I2S_NUM_0
#define I2S_BCLK 10
#define I2S_WS 9
#define I2S_DIN 8
#define SAMPLE_RATE 16000

void setupMicrophone();
int readMicrophoneData(int32_t* buffer, size_t maxSamples);

#endif // MICROPHONE_H