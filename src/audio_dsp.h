#ifndef AUDIO_DSP_H
#define AUDIO_DSP_H

#include <Arduino.h>

// Применяет к сырому 24-битному сэмплу гейт, АРУ и лимитер. Возвращает готовые 16 бит.
int16_t applyStudioProcessing24Bit(int32_t sample24);

#endif // AUDIO_DSP_H