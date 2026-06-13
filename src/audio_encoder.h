#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include <Arduino.h>

// Структура пакета ADPCM (строго 61 байт)
struct AdpcmBlock
{
    int16_t predictor;   // 2 байта
    uint8_t step_index;  // 1 байт
    uint8_t samples[58]; // 58 байт (вмещает 116 сэмплов по 4 бита)
} __attribute__((packed));

// Принимает обработанный 16-битный сэмпл, кодирует его и при заполнении блока отправляет по радио
void processPcmSample(int16_t pcm);

// Сброс состояния кодера (полезно при инициализации)
void resetAudioEncoder();

#endif // AUDIO_ENCODER_H