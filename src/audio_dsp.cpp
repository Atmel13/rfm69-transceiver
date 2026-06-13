#include "audio_dsp.h"

static float gate_envelope_24 = 0.0f;

int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // 1. Перевод во float (-1.0 ... 1.0)
    float input_signal = (float)sample24 / 8388608.0f;
    float abs_val = abs(input_signal);

    // Быстрая атака (0.3) позволяет мгновенно реагировать на тихий голос
    gate_envelope_24 = (0.3f * abs_val) + (0.7f * gate_envelope_24);

    // 2. УМНЫЙ ШУМОПОДАВИТЕЛЬ (Noise Gate)
    float gate_threshold = 0.0004f; 
    if (gate_envelope_24 < gate_threshold)
    {
        input_signal *= (gate_envelope_24 / gate_threshold);
    }

    // 3. ДИНАМИЧЕСКИЙ КОМПРЕССОР (Автоматическая Регулировка Усиления)
    float target_gain = 1.0f;
    if (gate_envelope_24 > 0.0001f) {
        target_gain = 0.4f / (gate_envelope_24 + 0.005f);
        
        if (target_gain > 45.0f) target_gain = 45.0f;
        if (target_gain < 5.0f)  target_gain = 5.0f; 
    }

    float amplified = input_signal * target_gain;

    // 4. МЯГКИЙ ЛИМИТЕР (Защита от клиппинга)
    float limit_threshold = 0.8f;
    if (amplified > limit_threshold)
    {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.1f;
    }
    else if (amplified < -limit_threshold)
    {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.1f;
    }

    // 5. Конвертация в 16 бит
    float out_16 = amplified * 32767.0f;

    if (out_16 > 32767.0f)  out_16 = 32767.0f;
    if (out_16 < -32768.0f) out_16 = -32768.0f;

    return (int16_t)out_16;
}