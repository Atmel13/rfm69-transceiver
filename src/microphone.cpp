#include "microphone.h"

// Глобальный дескриптор I2S канала для чтения
static i2s_chan_handle_t rx_handle = NULL;

void setupMicrophone()
{
    // 1. Создаем конфигурацию шины и Rx-канала (Master роль)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    
    // Выделяем аппаратные ресурсы под RX канал
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    // 2. Настраиваем аудио-поток строго по стандарту PHILIPS I2S (как было в старом драйвере)
    // Это восстановит сдвиг на 1 такт, и микрофон INMP441 снова начнет отдавать чистые данные.
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        
        // ВАЖНО: Используем PHILIPS конфигурацию слотов вместо MSB
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK,
            .ws = (gpio_num_t)I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    // Привязываем чтение к левому каналу (INMP441 по умолчанию)
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    // Инициализируем стандартный режим
    i2s_channel_init_std_mode(rx_handle, &std_cfg);

    // 3. Активируем канал I2S
    i2s_channel_enable(rx_handle);
    
    Serial.println("Modern I2S Philips Driver initialized successfully.");
}

int readMicrophoneData(int32_t* buffer, size_t maxSamples)
{
    size_t bytes_read = 0;
    
    // Читаем данные из буфера I2S DMA. 
    // Даем таймаут 10 мс, чтобы дать задаче FreeRTOS «подышать» между чтениями
    esp_err_t result = i2s_channel_read(rx_handle, buffer, maxSamples * sizeof(int32_t), &bytes_read, pdMS_TO_TICKS(10));
    
    if (result == ESP_OK && bytes_read > 0) {
        return bytes_read / sizeof(int32_t);
    }
    return 0;
}