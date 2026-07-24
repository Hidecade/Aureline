#pragma once

#include "Engine/AnalogEngine.h"

#include <array>
#include <cstdint>

namespace aureline
{
struct PerformanceSequencerSettings
{
    bool arpeggiatorEnabled = false;
    bool chordEnabled = false;
    bool holdEnabled = false;
    double tempoBpm = 120.0;
    int rate = 1;
    int direction = 0;
    double gate = 0.75;
    int scaleRoot = 0;
};

class PerformanceSequencer
{
public:
    void prepare(double sampleRate);
    void setSettings(const PerformanceSequencerSettings& newSettings);
    void noteOn(AnalogEngine& engine, int note, int velocity);
    void noteOff(AnalogEngine& engine, int note);
    void panic(AnalogEngine& engine);
    StereoSample renderStereoSample(AnalogEngine& engine);

private:
    int chordNote(int root, int index) const;
    void addRoot(int root);
    void removeRoot(int root);
    void resetState(AnalogEngine& engine);
    int selectNextNote();

    PerformanceSequencerSettings settings;
    std::array<std::uint8_t, 128> heldNotes {};
    std::array<bool, 128> inputNotes {};
    double sampleRate = 44100.0;
    int currentNote = -1;
    int lastNote = -1;
    int samplesUntilStep = 0;
    int gateSamplesRemaining = 0;
    int velocity = 100;
    bool movingUp = true;
    bool resetRequested = false;
    std::uint32_t randomState = 0x41555245U;
};
} // namespace aureline
