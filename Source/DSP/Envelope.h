#pragma once

#include "Engine/AnalogPatch.h"

namespace aureline
{
class Envelope
{
public:
    void prepare(double sampleRate);
    void noteOn();
    void noteOff();
    void reset();
    double render(const EnvelopeParams& parameters);
    bool isActive() const;
    double level() const { return currentLevel; }

private:
    enum class Stage { idle, attack, decay, sustain, release };
    double coefficient(double seconds) const;

    Stage stage = Stage::idle;
    double currentSampleRate = 44100.0;
    double currentLevel = 0.0;
};
} // namespace aureline
