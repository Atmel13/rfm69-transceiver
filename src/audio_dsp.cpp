#include "audio_dsp.h"

int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // 1. Перевод во float (-1.0 ... 1.0)
    float input_signal = (float)sample24 / 8388608.0f;

    // 2. ФИКСИРОВАННОЕ УСИЛЕНИЕ (Без компрессоров, гейтов и АРУ)
    // Просто умножаем сигнал в 12 раз. Это покажет реальную физику твоего микрофона.
    float amplified = input_signal * 12.0f;

    // 3. Защитный лимитер
    float limit_threshold = 0.85f;
    if (amplified > limit_threshold) {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.1f;
    } else if (amplified < -limit_threshold) {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.1f;
    }

    // 4. Конвертация в 16 бит
    float out_16 = amplified * 32767.0f;

    if (out_16 > 32767.0f)  out_16 = 32767.0f;
    if (out_16 < -32768.0f) out_16 = -32768.0f;

    return (int16_t)out_16;
}