#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>
#include <driver/i2s.h>
#include "radio.h"

// --- Пины и настройки I2S ---
#define I2S_PORT I2S_NUM_0
#define I2S_BCLK 10
#define I2S_WS 9
#define I2S_DIN 8
#define SAMPLE_RATE 16000

// --- Структура пакета ADPCM ---
struct AdpcmBlock
{
    int16_t predictor;   // 2 байта
    uint8_t step_index;  // 1 байт
    uint8_t samples[58]; // 58 байт (вмещает 116 сэмплов по 4 бита)
} __attribute__((packed));

static_assert(sizeof(AdpcmBlock) == 61, "Ошибка: размер AdpcmBlock должен быть ровно 61 байт!");

// --- Глобальные переменные ---
AdpcmBlock currentBlock;
int sampleCount = 0;
int16_t adpcm_predictor = 0;
int8_t adpcm_step_index = 0;

uint32_t txPacketCount = 0;
uint32_t lastTxTime = 0;

// --- Глобальная переменная для Шумоподавителя (теперь под 24 бита) ---
float gate_envelope_24 = 0.0f;

// --- Таблицы IMA-ADPCM ---
const int8_t indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8};

const uint16_t stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

// --- Инициализация I2S ---
void setupI2S()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DIN};

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

// --- 24-битная студийная обработка с конвертацией в 16 бит ---
int16_t applyStudioProcessing24Bit(int32_t sample24)
{
    // 1. ПРЕДУСИЛИТЕЛЬ
    // Конвертируем в float без потери точности (мантисса float покрывает 24 бита)
    float gain = 10.0f;
    float amplified = (float)sample24 * gain;

    // Считаем огибающую для 24-битного сигнала
    float abs_val = abs(amplified);
    gate_envelope_24 = (0.01f * abs_val) + (0.99f * gate_envelope_24);

    // 2. ШУМОПОДАВИТЕЛЬ (Noise Gate)
    // Порог увеличен в 256 раз (по сравнению с 16-битной версией)
    float gate_threshold = 102400.0f;
    if (gate_envelope_24 < gate_threshold)
    {
        amplified *= (gate_envelope_24 / gate_threshold);
    }

    // 3. МЯГКИЙ ЛИМИТЕР (Soft Clipper)
    // Порог срабатывания лимитера (24000 * 256)
    float limit_threshold = 6144000.0f;
    if (amplified > limit_threshold)
    {
        amplified = limit_threshold + (amplified - limit_threshold) * 0.3f;
    }
    else if (amplified < -limit_threshold)
    {
        amplified = -limit_threshold + (amplified + limit_threshold) * 0.3f;
    }

    // 4. КОНВЕРТАЦИЯ В 16 БИТ
    // Делим на 2^8 (256), чтобы пропорционально сжать 24-битный сигнал до 16-битного
    float out_16 = amplified / 256.0f;

    // 5. ЖЕСТКАЯ ЗАЩИТА (Clipping prevention на уровне 16 бит)
    if (out_16 > 32767.0f)
        out_16 = 32767.0f;
    if (out_16 < -32768.0f)
        out_16 = -32768.0f;

    return (int16_t)out_16;
}

// --- Кодирование одного сэмпла ---
uint8_t encodeSample(int16_t sample)
{
    int32_t diff = sample - adpcm_predictor;
    uint8_t nibble = 0;
    int step = stepSizeTable[adpcm_step_index];

    if (diff < 0)
    {
        nibble = 8;
        diff = -diff;
    }

    int diffq = step >> 3;
    if (diff >= step)
    {
        nibble |= 4;
        diff -= step;
        diffq += step;
    }
    step >>= 1;
    if (diff >= step)
    {
        nibble |= 2;
        diff -= step;
        diffq += step;
    }
    step >>= 1;
    if (diff >= step)
    {
        nibble |= 1;
        diffq += step;
    }

    if (nibble & 8)
    {
        adpcm_predictor -= diffq;
    }
    else
    {
        adpcm_predictor += diffq;
    }

    if (adpcm_predictor > 32767)
        adpcm_predictor = 32767;
    else if (adpcm_predictor < -32768)
        adpcm_predictor = -32768;

    adpcm_step_index += indexTable[nibble];
    if (adpcm_step_index < 0)
        adpcm_step_index = 0;
    else if (adpcm_step_index > 88)
        adpcm_step_index = 88;

    return nibble;
}

// --- Обработка входящего аудио ---
void processPcmSample(int16_t pcm)
{
    if (sampleCount == 0)
    {
        currentBlock.predictor = adpcm_predictor;
        currentBlock.step_index = adpcm_step_index;
        memset(currentBlock.samples, 0, sizeof(currentBlock.samples));
    }

    uint8_t nibble = encodeSample(pcm);

    int byteIndex = sampleCount / 2;
    if (sampleCount % 2 == 0)
    {
        currentBlock.samples[byteIndex] = nibble;
    }
    else
    {
        currentBlock.samples[byteIndex] |= (nibble << 4);
    }

    sampleCount++;

    if (sampleCount == 116)
    {
        radio.send(2, (const void *)&currentBlock, sizeof(AdpcmBlock));
        sampleCount = 0;
        txPacketCount++;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupRadio();
    setupI2S();

    Serial.println("Setup complete. Starting 24-bit audio processing and transmission...");

    lastTxTime = millis();
}

void loop()
{
    int32_t i2s_data[64];
    size_t bytes_read = 0;

    esp_err_t result = i2s_read(I2S_PORT, &i2s_data, sizeof(i2s_data), &bytes_read, 0);

    if (result == ESP_OK && bytes_read > 0)
    {
        int samples_read = bytes_read / sizeof(int32_t);

        for (int i = 0; i < samples_read; i++)
        {
            // 1. ИЗВЛЕКАЕМ 24 БИТА
            // Сдвигаем на 8 вправо, чтобы убрать пустые биты контейнера.
            // При этом сохраняется знак (число остается отцентрованным вокруг нуля).
            int32_t raw_24 = i2s_data[i] >> 8;

            // 2. ПРИМЕНЯЕМ ОБРАБОТКУ
            // Функция считает всё в 24 битах и отдает чистые 16 бит
            int16_t processed_pcm_16 = applyStudioProcessing24Bit(raw_24);

            // 3. ОТПРАВЛЯЕМ В КОДЕР
            processPcmSample(processed_pcm_16);
        }
    }

    // Каждую секунду выводим результат в консоль
    if (millis() - lastTxTime >= 1000)
    {
        Serial.printf("TX Packets per second: %u\n", txPacketCount);
        txPacketCount = 0;
        lastTxTime = millis();
    }
}