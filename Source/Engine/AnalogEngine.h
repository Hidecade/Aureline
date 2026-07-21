#pragma once

#include "Engine/AnalogPatch.h"
#include "Engine/AnalogVoice.h"
#include "DSP/Lfo.h"
#include "DSP/ParameterSmoother.h"

#include <array>
#include <cstdint>

namespace aureline
{
constexpr int kVoiceCount = 8;

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

private:
    AnalogVoice& selectVoice();

    AnalogPatch patch;
    std::array<AnalogVoice, kVoiceCount> voices;
    std::array<bool, 128> keyDownNotes {};
    std::array<bool, 128> sustainedNotes {};
    Lfo lfo;
    ParameterSmoother masterGainSmoother;
    std::uint64_t voiceAge = 0;
    double pitchBend = 0.0;
    double pitchBendRange = 2.0;
    double modWheel = 0.0;
    bool sustainPedalDown = false;
    int lastPlayedNote = -1;
};
} // namespace aureline
