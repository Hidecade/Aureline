#include "DSP/LadderFilter.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void LadderFilter::prepare(double sampleRate)
{
    currentSampleRate = std::max(1.0, sampleRate);
    reset();
}

void LadderFilter::reset()
{
    stages.fill(0.0);
}

double LadderFilter::render(double input, double cutoffHz, double resonance)
{
    constexpr double pi = 3.14159265358979323846;
    const auto cutoff = std::clamp(cutoffHz, 20.0, currentSampleRate * 0.45);
    const auto g = 1.0 - std::exp(-2.0 * pi * cutoff / currentSampleRate);
    const auto feedback = std::clamp(resonance, 0.0, 1.0) * 3.85;
    auto value = std::tanh(input - feedback * stages[3]);

    for (auto& stage : stages)
    {
        stage += g * (std::tanh(value) - std::tanh(stage));
        value = stage;
    }

    return stages[3];
}
} // namespace aureline
