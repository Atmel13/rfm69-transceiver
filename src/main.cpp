#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>
#include <driver/i2s_std.h>

#include "radio.h"
#include "adpcm.h"

#define I2S_BCLK 10
#define I2S_WS   9
#define I2S_DIN  8

#define SAMPLE_RATE 16000

#define PCM_BLOCK_SAMPLES 512
#define ADPCM_PER_PACKET  54
#define PCM_PER_PACKET    (ADPCM_PER_PACKET * 2)

struct __attribute__((packed)) RadioPacket
{
    uint16_t blockId;
    uint8_t packetId;

    int16_t predictor;
    uint8_t stepIndex;

    uint8_t payload[54];
};

static_assert(
    sizeof(RadioPacket) == 60,
    "RadioPacket must be 60 bytes"
);

static IMAADPCMEncoder encoder;

static int16_t pcmBlock[PCM_BLOCK_SAMPLES];

static uint16_t blockCounter = 0;

static i2s_chan_handle_t rx_chan = NULL;

static void setupMic()
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(
            I2S_NUM_0,
            I2S_ROLE_MASTER
        );

    i2s_new_channel(
        &chan_cfg,
        NULL,
        &rx_chan
    );

    i2s_std_config_t cfg =
    {
        .clk_cfg =
            I2S_STD_CLK_DEFAULT_CONFIG(
                SAMPLE_RATE
            ),

        .slot_cfg =
        {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,

            // INMP441 обычно сидит в левом канале
            .slot_mask = I2S_STD_SLOT_LEFT,

            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false
        },

        .gpio_cfg =
        {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK,
            .ws   = (gpio_num_t)I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)I2S_DIN,
        }
    };

    i2s_channel_init_std_mode(
        rx_chan,
        &cfg
    );

    i2s_channel_enable(rx_chan);
}

static bool captureBlock()
{
    int32_t raw[PCM_BLOCK_SAMPLES];

    size_t bytesRead = 0;

    esp_err_t err =
        i2s_channel_read(
            rx_chan,
            raw,
            sizeof(raw),
            &bytesRead,
            portMAX_DELAY
        );

    if(err != ESP_OK)
    {
        Serial.printf(
            "I2S read error: %d\n",
            err
        );

        return false;
    }

    if(bytesRead != sizeof(raw))
    {
        Serial.printf(
            "I2S short read: %u / %u\n",
            (unsigned)bytesRead,
            (unsigned)sizeof(raw)
        );

        return false;
    }

    for(int i = 0; i < PCM_BLOCK_SAMPLES; i++)
    {
        pcmBlock[i] =
            (int16_t)(raw[i] >> 13);
    }

    // Debug: print peak level once per second
    static uint32_t t = millis();

    if(millis() - t > 1000)
    {
        int16_t peak = 0;

        for(int i = 0; i < PCM_BLOCK_SAMPLES; i++)
        {
            int16_t v = abs(pcmBlock[i]);

            if(v > peak)
                peak = v;
        }

        Serial.printf(
            "MIC peak = %d\n",
            peak
        );

        t = millis();
    }

    return true;
}

static void sendPCMChunk(
    const int16_t* pcm,
    size_t samples,
    uint8_t packetId
)
{
    RadioPacket pkt;

    pkt.blockId = blockCounter;
    pkt.packetId = packetId;

    pkt.predictor = encoder.predictor;
    pkt.stepIndex = encoder.stepIndex;

    memset(pkt.payload, 0, sizeof(pkt.payload));

    encoder.encodeBlock(
        pcm,
        samples,
        pkt.payload
    );

    radio.send(
        2,
        &pkt,
        sizeof(pkt)
    );
}

static void sendBlock()
{
    uint8_t packetId = 0;

    size_t offset = 0;

    while(offset < PCM_BLOCK_SAMPLES)
    {
        size_t count = PCM_PER_PACKET;

        if(offset + count > PCM_BLOCK_SAMPLES)
        {
            count =
                PCM_BLOCK_SAMPLES - offset;
        }

        sendPCMChunk(
            &pcmBlock[offset],
            count,
            packetId
        );

        offset += count;
        packetId++;
    }

    blockCounter++;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupRadio();

    setupMic();

    encoder.reset();

    Serial.println("Audio TX started");
}

void loop()
{
    static uint32_t packetsSent = 0;
    static uint32_t lastPrint = 0;

    if(!captureBlock())
        return;

    sendBlock();

    packetsSent += 5;

    if(millis() - lastPrint >= 1000)
    {
        Serial.printf(
            "TX packets/sec = %u, blocks/sec = %u\n",
            packetsSent,
            packetsSent / 5
        );

        packetsSent = 0;
        lastPrint = millis();
    }
}