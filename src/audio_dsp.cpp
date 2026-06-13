#include "audio_dsp.h"

int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // 1. Перевод во float (-1.0 ... 1.0)
    // Извлекаем чистый сигнал из 24-битного контейнера микрофона
    float input_signal = (float)sample24 / 8388608.0f;

    // 2. АБСОЛЮТНО ЛИНЕЙНОЕ УСИЛЕНИЕ (Без гейтов, без АРУ, без компрессоров)
    // Увеличиваем коэффициент до 20.0f. Это поднимем общую чувствительность тракта,
    // чтобы были слышны шаги и вороны за окном, но сохранит линейность и прозрачность.
    float amplified = input_signal * 20.0f;

    // 3. Мягкое ограничение пиков (только для защиты от жесткого хрипа, если крикнуть в микрофон)
    float limit_threshold = 0.9f;
    if (amplified > limit_threshold) {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.1f;
    } else if (amplified < -limit_threshold) {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.1f;
    }

    // 4. Конвертация в 16 бит напрямую
    float out_16 = amplified * 32767.0f;

    if (out_16 > 32767.0f)  out_16 = 32767.0f;
    if (out_16 < -32768.0f) out_16 = -32768.0f;

    return (int16_t)out_16;
}