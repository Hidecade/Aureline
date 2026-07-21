#pragma once

#include <array>

namespace aureline
{
class LadderFilter
{
public:
    void prepare(double sampleRate);
    void reset();
    double render(double input, double cutoffHz, double resonance);

private:
    double currentSampleRate = 44100.0;
    std::array<double, 4> stages {};
};
} // namespace aureline
