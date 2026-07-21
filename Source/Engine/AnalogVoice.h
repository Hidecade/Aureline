#pragma once

#include "DSP/Envelope.h"
#include "DSP/LadderFilter.h"
#include "DSP/NoiseGenerator.h"
#include "DSP/Oscillator.h"
#include "DSP/ParameterSmoother.h"
#include "Engine/AnalogPatch.h"

#include <cstdint>

namespace aureline
{
class AnalogVoice
{
public:
    void prepare(double sampleRate, int voiceIndex);
    void start(int midiNote, int velocity, std::uint64_t age,
               double startNote, double glideSeconds, double detuneCents = 0.0,
               double initialPhase = 0.0);
    void retarget(int midiNote, double glideSeconds);
    void release();
    void reset();
    double render(const AnalogPatch& patch, double pitchBendSemitones,
                  double lfoValue, double modWheel);
    bool isActive() const { return amplifierEnvelope.isActive(); }
    bool isReleasing() const { return releasing; }
    int note() const { return currentNote; }
    double level() const { return amplifierEnvelope.level(); }
    std::uint64_t age() const { return startAge; }
    double panPosition() const { return voicePanPosition; }
    double vintagePanPosition() const { return vintagePanVariation; }

private:
    double oscillatorFrequency(const OscillatorParams& oscillator,
                               double pitchBendSemitones,
                               double vintageTuneCents) const;
    void configureGlide(double startNote, double targetNote, double glideSeconds);

    Oscillator oscillatorA;
    Oscillator oscillatorB;
    Envelope filterEnvelope;
    Envelope amplifierEnvelope;
    LadderFilter filter;
    NoiseGenerator noise;
    ParameterSmoother oscillatorALevelSmoother;
    ParameterSmoother oscillatorBLevelSmoother;
    ParameterSmoother pulseWidthASmoother;
    ParameterSmoother pulseWidthBSmoother;
    ParameterSmoother noiseLevelSmoother;
    ParameterSmoother cutoffSmoother;
    ParameterSmoother resonanceSmoother;
    int currentNote = -1;
    double currentPitchNote = 60.0;
    double targetPitchNote = 60.0;
    double glideStepPerSample = 0.0;
    double unisonDetuneCents = 0.0;
    double currentSampleRate = 44100.0;
    double voicePanPosition = 0.0;
    double velocityGain = 0.0;
    double voiceTuneCents = 0.0;
    double oscillatorATuneVariation = 0.0;
    double oscillatorBTuneVariation = 0.0;
    double filterVariation = 0.0;
    double envelopeVariation = 0.0;
    double gainVariation = 0.0;
    double vintagePanVariation = 0.0;
    double driftPhase = 0.0;
    double driftRateHz = 0.1;
    std::uint64_t noteSamples = 0;
    std::uint64_t startAge = 0;
    bool releasing = false;
};
} // namespace aureline
