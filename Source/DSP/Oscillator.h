#pragma once

#include "Engine/AnalogPatch.h"

namespace aureline
{
class Oscillator
{
public:
    void prepare(double sampleRate);
    void reset(double initialPhase = 0.0);
    double render(double frequencyHz, Waveform waveform, double pulseWidth);
    double render(double frequencyHz, bool sawEnabled, bool triangleEnabled,
                  bool pulseEnabled, double pulseWidth);
    double render(double frequencyHz, bool sawEnabled, bool triangleEnabled,
                  bool pulseEnabled, double pulseWidth, bool waveMemoryEnabled,
                  const WaveMemoryData& waveMemory, WaveMemoryCharacter character);
    double renderPhaseModulated(double frequencyHz, bool sawEnabled, bool triangleEnabled,
                                bool pulseEnabled, double pulseWidth, bool waveMemoryEnabled,
                                const WaveMemoryData& waveMemory, WaveMemoryCharacter character,
                                double phaseOffsetCycles);
    bool wrappedLastSample() const { return didWrap; }

private:
    double polyBlep(double phase, double phaseIncrement) const;

    double currentSampleRate = 44100.0;
    double phase = 0.0;
    bool didWrap = false;
};
} // namespace aureline
