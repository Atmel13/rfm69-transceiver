#include "audio_encoder.h"
#include "radio.h"

static AdpcmBlock currentBlock;
static int sampleCount = 0;
static int16_t adpcm_predictor = 0;
static int8_t adpcm_step_index = 0;

extern uint32_t txPacketCount; // Будем инкрементировать счетчик для main.cpp

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

void resetAudioEncoder()
{
    sampleCount = 0;
    adpcm_predictor = 0;
    adpcm_step_index = 0;
    memset(&currentBlock, 0, sizeof(AdpcmBlock));
}

static uint8_t encodeSample(int16_t sample)
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

    if (nibble & 8)  adpcm_predictor -= diffq;
    else             adpcm_predictor += diffq;

    if (adpcm_predictor > 32767)      adpcm_predictor = 32767;
    else if (adpcm_predictor < -32768) adpcm_predictor = -32768;

    adpcm_step_index += indexTable[nibble];
    if (adpcm_step_index < 0)       adpcm_step_index = 0;
    else if (adpcm_step_index > 88)  adpcm_step_index = 88;

    return nibble;
}

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
        // Отправляем пакет на ноду приемника (ID = 2)
        radio.send(2, (const void *)&currentBlock, sizeof(AdpcmBlock));
        sampleCount = 0;
        txPacketCount++;
    }
}