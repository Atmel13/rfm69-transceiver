#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0
#define I2S_BCLK 10
#define I2S_WS 9
#define I2S_DIN 8
#define SAMPLE_RATE 16000

// Инициализация периферии I2S под микрофон INMP441
void setupMicrophone();

// Чтение порции сырых данных. Возвращает количество успешно прочитанных сэмплов
int readMicrophoneData(int32_t* buffer, size_t maxSamples);

#endif // MICROPHONE_H