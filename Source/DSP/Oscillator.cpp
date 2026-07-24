#include "DSP/Oscillator.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void Oscillator::prepare(double sampleRate)
{
    currentSampleRate = std::max(1.0, sampleRate);
}

void Oscillator::reset(double initialPhase)
{
    phase = initialPhase - std::floor(initialPhase);
    didWrap = false;
}

double Oscillator::polyBlep(double t, double dt) const
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

double Oscillator::render(double frequencyHz, Waveform waveform, double pulseWidth)
{
    return render(frequencyHz, waveform == Waveform::saw, waveform == Waveform::triangle,
                  waveform == Waveform::pulse, pulseWidth);
}

double Oscillator::render(double frequencyHz, bool sawEnabled, bool triangleEnabled,
                          bool pulseEnabled, double pulseWidth)
{
    return render(frequencyHz, sawEnabled, triangleEnabled, pulseEnabled, pulseWidth,
                  false, waveMemoryFactoryBank()[0], WaveMemoryCharacter::fiveBit);
}

double Oscillator::render(double frequencyHz, bool sawEnabled, bool triangleEnabled,
                          bool pulseEnabled, double pulseWidth, bool waveMemoryEnabled,
                          const WaveMemoryData& waveMemory, WaveMemoryCharacter character)
{
    const auto frequency = std::clamp(frequencyHz, 0.0, currentSampleRate * 0.45);
    const auto increment = frequency / currentSampleRate;
    const auto width = std::clamp(pulseWidth, 0.02, 0.98);
    double sample = 0.0;

    int waveformCount = 0;
    if (sawEnabled)
    {
        sample += 2.0 * phase - 1.0 - polyBlep(phase, increment);
        ++waveformCount;
    }
    if (triangleEnabled)
    {
        sample += 1.0 - 4.0 * std::abs(phase - 0.5);
        ++waveformCount;
    }
    if (pulseEnabled)
    {
        auto pulse = phase < width ? 1.0 : -1.0;
        pulse += polyBlep(phase, increment);
        auto fallingPhase = phase - width;
        if (fallingPhase < 0.0)
            fallingPhase += 1.0;
        pulse -= polyBlep(fallingPhase, increment);
        sample += pulse;
        ++waveformCount;
    }
    if (waveMemoryEnabled)
    {
        const auto tablePosition = phase * static_cast<double>(kWaveMemorySize);
        const auto index = static_cast<std::size_t>(tablePosition) % kWaveMemorySize;
        auto quantized = [](std::uint8_t step, WaveMemoryCharacter mode)
        {
            if (mode == WaveMemoryCharacter::fourBit)
                return static_cast<double>(step >> 1U) / 7.5 - 1.0;
            return static_cast<double>(step) / 15.5 - 1.0;
        };
        auto memorySample = quantized(waveMemory[index], character);
        if (character == WaveMemoryCharacter::smooth)
        {
            const auto next = (index + 1U) % kWaveMemorySize;
            const auto fraction = tablePosition - std::floor(tablePosition);
            const auto nextSample = quantized(waveMemory[next], character);
            memorySample += (nextSample - memorySample) * fraction;
        }
        sample += memorySample;
        ++waveformCount;
    }
    if (waveformCount == 0)
        sample = 0.0;
    else
        sample /= std::sqrt(static_cast<double>(waveformCount));

    phase += increment;
    didWrap = phase >= 1.0;
    phase -= std::floor(phase);
    return sample;
}
} // namespace aureline
