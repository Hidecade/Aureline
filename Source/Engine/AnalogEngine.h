#pragma once

#include "Engine/AnalogPatch.h"
#include "Engine/AnalogVoice.h"
#include "DSP/Lfo.h"
#include "DSP/ParameterSmoother.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace aureline
{
constexpr int kVoiceCount = 8;
constexpr int kUnisonVoiceCount = 5;

class AnalogEngine
{
public:
    void prepare(double sampleRate);
    void setPatch(const AnalogPatch& newPatch);
    const AnalogPatch& getPatch() const { return patch; }
    void noteOn(int midiNote, int velocity);
    void noteOff(int midiNote);
    void setSustainPedal(bool down);
    void setPitchBend(double normalized);
    void setPitchBendRange(double semitones);
    void setModWheel(double normalized);
    void panic();
    StereoSample renderStereoSample();
    double renderSample();
    void renderBlock(float* left, float* right, int numSamples);
    int activeVoiceCount() const;
    double currentLfoValue() const
    {
        return displayedLfoValue.load(std::memory_order_relaxed);
    }

private:
    AnalogVoice& selectVoice();
    double nextUnisonPhase();

    AnalogPatch patch;
    std::array<AnalogVoice, kVoiceCount> voices;
    std::array<bool, 128> keyDownNotes {};
    std::array<bool, 128> sustainedNotes {};
    std::array<std::uint64_t, 128> notePriority {};
    Lfo lfo;
    ParameterSmoother masterGainSmoother;
    std::uint64_t voiceAge = 0;
    std::uint64_t notePriorityCounter = 0;
    std::uint32_t unisonPhaseState = 0x8a5cd789U;
    double pitchBend = 0.0;
    double pitchBendRange = 2.0;
    double modWheel = 0.0;
    std::atomic<double> displayedLfoValue { 0.0 };
    bool sustainPedalDown = false;
    int lastPlayedNote = -1;
};
} // namespace aureline
