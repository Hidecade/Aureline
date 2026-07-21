#pragma once

#include "Engine/AnalogPatch.h"

#include <cstdint>

namespace aureline
{
class Lfo
{
public:
    void prepare(double sampleRate);
    void reset();
    double render(double rateHz, LfoWaveform waveform);
    double render(double rateHz, int waveformMask);

private:
    double currentSampleRate = 44100.0;
    double phase = 0.0;
    double heldValue = 0.0;
    std::uint32_t randomState = 0x7f4a7c15U;
};
} // namespace aureline
