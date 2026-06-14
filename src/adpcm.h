#pragma once

#include <Arduino.h>

class IMAADPCMEncoder
{
public:
    IMAADPCMEncoder();

    void reset();

    void encodeBlock(
        const int16_t* pcm,
        size_t samples,
        uint8_t* adpcm
    );

    int16_t predictor;
    uint8_t stepIndex;

private:
    uint8_t encodeSample(int16_t sample);
};