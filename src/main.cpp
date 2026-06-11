#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>
#include <driver/i2s.h>
#include "radio.h"

// --- Пины и настройки I2S ---
#define I2S_PORT        I2S_NUM_0
#define I2S_BCLK        10
#define I2S_WS          9
#define I2S_DIN         8
#define SAMPLE_RATE     16000

// --- Структура пакета ADPCM ---
struct AdpcmBlock {
    int16_t predictor;      // 2 байта
    uint8_t step_index;     // 1 байт
    uint8_t samples[58];    // 58 байт (вмещает 116 сэмплов по 4 бита)
} __attribute__((packed));

static_assert(sizeof(AdpcmBlock) == 61, "Ошибка: размер AdpcmBlock должен быть ровно 61 байт!");

// --- Глобальные переменные ---
AdpcmBlock currentBlock;
int sampleCount = 0;       
int16_t adpcm_predictor = 0; 
int8_t adpcm_step_index = 0; 

// --- Переменные для статистики ---
uint32_t txPacketCount = 0;
uint32_t lastTxTime = 0;

// --- Таблицы IMA-ADPCM ---
const int8_t indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

const uint16_t stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// --- Инициализация I2S ---
void setupI2S() {
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
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DIN
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

// --- Кодирование одного сэмпла ---
uint8_t encodeSample(int16_t sample) {
    int32_t diff = sample - adpcm_predictor;
    uint8_t nibble = 0;
    int step = stepSizeTable[adpcm_step_index];

    if (diff < 0) {
        nibble = 8;
        diff = -diff;
    }

    int diffq = step >> 3;
    if (diff >= step) {
        nibble |= 4;
        diff -= step;
        diffq += step;
    }
    step >>= 1;
    if (diff >= step) {
        nibble |= 2;
        diff -= step;
        diffq += step;
    }
    step >>= 1;
    if (diff >= step) {
        nibble |= 1;
        diffq += step;
    }

    if (nibble & 8) {
        adpcm_predictor -= diffq;
    } else {
        adpcm_predictor += diffq;
    }

    if (adpcm_predictor > 32767) adpcm_predictor = 32767;
    else if (adpcm_predictor < -32768) adpcm_predictor = -32768;

    adpcm_step_index += indexTable[nibble];
    if (adpcm_step_index < 0) adpcm_step_index = 0;
    else if (adpcm_step_index > 88) adpcm_step_index = 88;

    return nibble;
}

// --- Обработка входящего аудио ---
void processPcmSample(int16_t pcm) {
    if (sampleCount == 0) {
        currentBlock.predictor = adpcm_predictor;
        currentBlock.step_index = adpcm_step_index;
        memset(currentBlock.samples, 0, sizeof(currentBlock.samples));
    }

    uint8_t nibble = encodeSample(pcm);

    int byteIndex = sampleCount / 2;
    if (sampleCount % 2 == 0) {
        currentBlock.samples[byteIndex] = nibble;
    } else {
        currentBlock.samples[byteIndex] |= (nibble << 4);
    }

    sampleCount++;

    if (sampleCount == 116) {
        radio.send(2, (const void*)&currentBlock, sizeof(AdpcmBlock));
        sampleCount = 0;
        
        // Увеличиваем счетчик отправленных пакетов
        txPacketCount++;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupRadio();
    setupI2S();

    Serial.println("Setup complete. Starting audio transmission...");
    
    // Инициализируем таймер
    lastTxTime = millis();
}

void loop()
{
    int32_t i2s_data[64];
    size_t bytes_read = 0;

    esp_err_t result = i2s_read(I2S_PORT, &i2s_data, sizeof(i2s_data), &bytes_read, 0);

    if (result == ESP_OK && bytes_read > 0) {
        int samples_read = bytes_read / sizeof(int32_t);
        
        for (int i = 0; i < samples_read; i++) {
            int16_t pcm = (int16_t)(i2s_data[i] >> 16);
            processPcmSample(pcm);
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