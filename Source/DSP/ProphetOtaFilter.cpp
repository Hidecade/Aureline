#include "DSP/ProphetOtaFilter.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void ProphetOtaFilter::prepare(double sampleRate)
{
    currentSampleRate = std::max(1.0, sampleRate);
    reset();
}

void ProphetOtaFilter::reset()
{
    stages.fill(0.0);
    excitationState = 0x6d2b79f5u;
}

double ProphetOtaFilter::processSubSample(double input, double coefficient,
                                          double resonance)
{
    // A tiny deterministic analogue-noise floor lets the feedback loop start
    // naturally when resonance is above its oscillation threshold.
    excitationState ^= excitationState << 13;
    excitationState ^= excitationState >> 17;
    excitationState ^= excitationState << 5;
    const auto noise = (static_cast<double>(excitationState) / 4294967295.0 - 0.5)
                     * 2.0e-8;

    const auto r = std::clamp(resonance, 0.0, 1.0);
    // The curved final portion crosses the four-pole oscillation threshold
    // near the top of the control, without making mid settings too peaky.
    const auto feedback = (3.72 * r + 0.78 * r * r * r) * stages[3];

    // Prophet-style resonance loses less apparent bass than an uncompensated
    // four-pole loop. Keep the dry drive increase modest so high resonance
    // still thins the fundamental instead of becoming louder everywhere.
    const auto inputCompensation = 1.0 + 0.72 * r;
    auto value = std::tanh(input * inputCompensation - feedback + noise);

    // Each stage represents an OTA integrator. Saturating the differential
    // voltage of each cell gives a softer, less ladder-like overload.
    for (auto& stage : stages)
    {
        const auto differential = std::clamp(value - stage, -3.0, 3.0);
        stage += coefficient * std::tanh(differential);
        stage = std::clamp(stage, -1.35, 1.35);
        value = stage;
    }

    return stages[3];
}

double ProphetOtaFilter::render(double input, double cutoffHz, double resonance)
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double oversampling = 2.0;
    const auto cutoff = std::clamp(cutoffHz, 20.0, currentSampleRate * 0.42);
    const auto coefficient = 1.0 - std::exp(
        -2.0 * pi * cutoff / (currentSampleRate * oversampling));

    processSubSample(input, coefficient, resonance);
    return processSubSample(input, coefficient, resonance);
}
} // namespace aureline
