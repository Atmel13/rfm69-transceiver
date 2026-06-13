#include "audio_dsp.h"

int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // Округляем 3 младших бита (шум матрицы)
    sample24 = sample24 & 0xFFFFFFF8;

    // Перевод во float (-1.0 ... 1.0)
    float input_signal = (float)sample24 / 8388608.0f;

    // Вваливаем жесткое тестовое усиление x40.0, чтобы проверить сборку!
    float amplified = input_signal * 40.0f;

    // Мягкий лимитер
    float limit_threshold = 0.85f;
    if (amplified > limit_threshold) {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.1f;
    } else if (amplified < -limit_threshold) {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.1f;
    }

    // Конвертация в 16 бит
    float out_16 = amplified * 32767.0f;

    if (out_16 > 32767.0f)  out_16 = 32767.0f;
    if (out_16 < -32768.0f) out_16 = -32768.0f;

    return (int16_t)out_16;
}