#pragma once

#include "Engine/AnalogPatch.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace aureline
{
struct FactoryPreset
{
    const char* name;
    AnalogPatch patch;
};

constexpr std::size_t kFactoryPresetCount = 32;

const std::array<FactoryPreset, kFactoryPresetCount>& factoryPresets();
const FactoryPreset& factoryPreset(std::size_t index);
} // namespace aureline
