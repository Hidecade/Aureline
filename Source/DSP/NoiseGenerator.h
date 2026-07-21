#pragma once

#include <cstdint>

namespace aureline
{
class NoiseGenerator
{
public:
    void reset(std::uint32_t seed);
    double render();

private:
    std::uint32_t state = 0x9e3779b9U;
};
} // namespace aureline
