#include "adpcm.h"

static const int indexTable[16] =
{
    -1,-1,-1,-1,
     2, 4, 6, 8,
    -1,-1,-1,-1,
     2, 4, 6, 8
};

static const int stepTable[89] =
{
      7,     8,     9,    10,    11,
     12,    13,    14,    16,    17,
     19,    21,    23,    25,    28,
     31,    34,    37,    41,    45,
     50,    55,    60,    66,    73,
     80,    88,    97,   107,   118,
    130,   143,   157,   173,   190,
    209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,
    544,   598,   658,   724,   796,
    876,   963,  1060,  1166,  1282,
   1411,  1552,  1707,  1878,  2066,
   2272,  2499,  2749,  3024,  3327,
   3660,  4026,  4428,  4871,  5358,
   5894,  6484,  7132,  7845,  8630,
   9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385,
  24623, 27086, 29794, 32767
};

IMAADPCMEncoder::IMAADPCMEncoder()
{
    reset();
}

void IMAADPCMEncoder::reset()
{
    predictor = 0;
    stepIndex = 0;
}

uint8_t IMAADPCMEncoder::encodeSample(int16_t sample)
{
    int step = stepTable[stepIndex];

    int diff = sample - predictor;

    uint8_t code = 0;

    if(diff < 0)
    {
        code = 8;
        diff = -diff;
    }

    int delta = step >> 3;

    if(diff >= step)
    {
        code |= 4;
        diff -= step;
        delta += step;
    }

    if(diff >= (step >> 1))
    {
        code |= 2;
        diff -= step >> 1;
        delta += step >> 1;
    }

    if(diff >= (step >> 2))
    {
        code |= 1;
        delta += step >> 2;
    }

    if(code & 8)
        predictor -= delta;
    else
        predictor += delta;

    if(predictor > 32767)
        predictor = 32767;

    if(predictor < -32768)
        predictor = -32768;

    stepIndex += indexTable[code];

    if(stepIndex < 0)
        stepIndex = 0;

    if(stepIndex > 88)
        stepIndex = 88;

    return code & 0x0F;
}

void IMAADPCMEncoder::encodeBlock(
    const int16_t* pcm,
    size_t samples,
    uint8_t* adpcm
)
{
    size_t outPos = 0;

    for(size_t i = 0; i < samples; i += 2)
    {
        uint8_t n0 = encodeSample(pcm[i]);

        uint8_t n1 = 0;

        if(i + 1 < samples)
            n1 = encodeSample(pcm[i + 1]);

        adpcm[outPos++] =
            (n0 & 0x0F) |
            ((n1 & 0x0F) << 4);
    }
}