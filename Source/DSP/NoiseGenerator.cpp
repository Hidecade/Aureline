#include "DSP/NoiseGenerator.h"

namespace aureline
{
void NoiseGenerator::reset(std::uint32_t seed)
{
    state = seed == 0 ? 0x9e3779b9U : seed;
}

double NoiseGenerator::render()
{
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return (static_cast<double>(state) / 4294967295.0) * 2.0 - 1.0;
}
} // namespace aureline
