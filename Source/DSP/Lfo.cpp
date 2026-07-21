#include "DSP/Lfo.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void Lfo::prepare(double sampleRate)
{
    currentSampleRate = std::max(1.0, sampleRate);
    reset();
}

void Lfo::reset()
{
    phase = 0.0;
    heldValue = 0.0;
    randomState = 0x7f4a7c15U;
}

double Lfo::render(double rateHz, LfoWaveform waveform)
{
    double value = 0.0;
    switch (waveform)
    {
        case LfoWaveform::triangle: value = 1.0 - 4.0 * std::abs(phase - 0.5); break;
        case LfoWaveform::sawUp: value = 2.0 * phase - 1.0; break;
        case LfoWaveform::sawDown: value = 1.0 - 2.0 * phase; break;
        case LfoWaveform::square: value = phase < 0.5 ? 1.0 : -1.0; break;
        case LfoWaveform::sampleAndHold: value = heldValue; break;
    }

    phase += std::clamp(rateHz, 0.01, 30.0) / currentSampleRate;
    if (phase >= 1.0)
    {
        phase -= std::floor(phase);
        randomState ^= randomState << 13U;
        randomState ^= randomState >> 17U;
        randomState ^= randomState << 5U;
        heldValue = (static_cast<double>(randomState) / 4294967295.0) * 2.0 - 1.0;
    }
    return value;
}

double Lfo::render(double rateHz, int waveformMask)
{
    double value = 0.0;
    int selectedWaveforms = 0;
    if ((waveformMask & 1) != 0)
    {
        value += 2.0 * phase - 1.0;
        ++selectedWaveforms;
    }
    if ((waveformMask & 2) != 0)
    {
        value += 1.0 - 4.0 * std::abs(phase - 0.5);
        ++selectedWaveforms;
    }
    if ((waveformMask & 4) != 0)
    {
        value += phase < 0.5 ? 1.0 : -1.0;
        ++selectedWaveforms;
    }
    if ((waveformMask & 8) != 0)
    {
        value += 1.0 - 2.0 * phase;
        ++selectedWaveforms;
    }
    if ((waveformMask & 16) != 0)
    {
        value += heldValue;
        ++selectedWaveforms;
    }
    if (selectedWaveforms > 0)
        value /= static_cast<double>(selectedWaveforms);

    phase += std::clamp(rateHz, 0.01, 30.0) / currentSampleRate;
    if (phase >= 1.0)
    {
        phase -= std::floor(phase);
        randomState ^= randomState << 13U;
        randomState ^= randomState >> 17U;
        randomState ^= randomState << 5U;
        heldValue = (static_cast<double>(randomState) / 4294967295.0) * 2.0 - 1.0;
    }
    return value;
}
} // namespace aureline
