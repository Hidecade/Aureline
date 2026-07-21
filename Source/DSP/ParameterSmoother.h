#pragma once

#include <algorithm>
#include <cmath>

namespace aureline
{
class ParameterSmoother
{
public:
    void prepare(double sampleRate, double timeSeconds = 0.01)
    {
        const auto samples = std::max(1.0, sampleRate * timeSeconds);
        coefficient = 1.0 - std::exp(-1.0 / samples);
        initialized = false;
    }

    void reset(double value)
    {
        current = value;
        initialized = true;
    }

    double process(double target)
    {
        if (!initialized)
            reset(target);
        current += (target - current) * coefficient;
        return current;
    }

private:
    double current = 0.0;
    double coefficient = 1.0;
    bool initialized = false;
};
} // namespace aureline
