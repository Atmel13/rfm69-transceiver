#include "audio_dsp.h"

static float dsp_envelope = 0.0f;
static float current_gain = 12.0f;  // Кэшированное значение усиления
static int sample_counter = 0;      // Счетчик сэмплов для прореживания расчетов

int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // 1. Быстрое побитовое округление младших бит
    sample24 = sample24 & 0xFFFFFFF8;

    // 2. Перевод во float
    float input_signal = (float)sample24 / 8388608.0f;
    float abs_val = abs(input_signal);

    // Расчет огибающей уровня звука
    if (abs_val > dsp_envelope) {
        dsp_envelope = (0.2f * abs_val) + (0.8f * dsp_envelope);
    } else {
        dsp_envelope = (0.005f * abs_val) + (0.995f * dsp_envelope);
    }

    // ОПТИМИЗАЦИЯ: Считаем тяжелое деление АРУ только один раз на 16 сэмплов!
    // Для звука это незаметно, но разгружает ядро ESP32-C3 на 93%
    sample_counter++;
    if (sample_counter >= 16) {
        sample_counter = 0;
        
        if (dsp_envelope > 0.00005f) {
            current_gain = 0.35f / (dsp_envelope + 0.006f);
            if (current_gain > 40.0f) current_gain = 40.0f;
            if (current_gain < 8.0f)  current_gain = 8.0f;
        } else {
            current_gain = 40.0f; // В полной тишине выкручиваем на максимум под шаги
        }
    }

    // Применяем к текущему сэмплу уже готовое, быстрое умножение
    float amplified = input_signal * current_gain;

    // 3. Мягкий лимитер
    float limit_threshold = 0.85f;
    if (amplified > limit_threshold) {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.1f;
    } else if (amplified < -limit_threshold) {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.1f;
    }

    // 4. Быстрая конвертация в 16 бит
    float out_16 = amplified * 32767.0f;

    if (out_16 > 32767.0f)  out_16 = 32767.0f;
    if (out_16 < -32768.0f) out_16 = -32768.0f;

    return (int16_t)out_16;
}