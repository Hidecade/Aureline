#pragma once

#include <array>
#include <cstdint>

namespace aureline
{
// Four cascaded OTA integrators with global resonance feedback. This is an
// Aureline model of the Prophet filter architecture, not a circuit clone.
class ProphetOtaFilter
{
public:
    void prepare(double sampleRate);
    void reset();
    double render(double input, double cutoffHz, double resonance);

private:
    double processSubSample(double input, double coefficient, double resonance);

    double currentSampleRate = 44100.0;
    std::array<double, 4> stages {};
    std::uint32_t excitationState = 0x6d2b79f5u;
};
} // namespace aureline
