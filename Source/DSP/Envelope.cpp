#include "DSP/Envelope.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void Envelope::prepare(double sampleRate)
{
    currentSampleRate = std::max(1.0, sampleRate);
}

void Envelope::noteOn()
{
    stage = Stage::attack;
}

void Envelope::noteOff()
{
    if (stage != Stage::idle)
        stage = Stage::release;
}

void Envelope::reset()
{
    stage = Stage::idle;
    currentLevel = 0.0;
}

double Envelope::coefficient(double seconds) const
{
    return 1.0 - std::exp(-1.0 / (std::max(0.0001, seconds) * currentSampleRate));
}

double Envelope::render(const EnvelopeParams& parameters)
{
    switch (stage)
    {
        case Stage::idle:
            currentLevel = 0.0;
            break;
        case Stage::attack:
            currentLevel += (1.0 - currentLevel) * coefficient(parameters.attackSeconds) * 6.0;
            if (currentLevel >= 0.999)
            {
                currentLevel = 1.0;
                stage = Stage::decay;
            }
            break;
        case Stage::decay:
            currentLevel += (parameters.sustainLevel - currentLevel) * coefficient(parameters.decaySeconds) * 6.0;
            if (std::abs(currentLevel - parameters.sustainLevel) < 0.0001)
            {
                currentLevel = parameters.sustainLevel;
                stage = Stage::sustain;
            }
            break;
        case Stage::sustain:
            currentLevel = parameters.sustainLevel;
            break;
        case Stage::release:
            currentLevel += (0.0 - currentLevel) * coefficient(parameters.releaseSeconds) * 6.0;
            if (currentLevel < 0.00001)
                reset();
            break;
    }
    return currentLevel;
}

bool Envelope::isActive() const
{
    return stage != Stage::idle;
}
} // namespace aureline
