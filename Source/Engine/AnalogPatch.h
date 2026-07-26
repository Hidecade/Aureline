#pragma once

#include <array>
#include <cstdint>

namespace aureline
{
constexpr std::size_t kWaveMemorySize = 32;
constexpr std::size_t kWaveMemoryFactoryCount = 16;
using WaveMemoryData = std::array<std::uint8_t, kWaveMemorySize>;

enum class WaveMemoryCharacter
{
    fiveBit,
    fourBit,
    smooth
};

inline const std::array<WaveMemoryData, kWaveMemoryFactoryCount>& waveMemoryFactoryBank()
{
    static const std::array<WaveMemoryData, kWaveMemoryFactoryCount> bank {{
        {{16,19,22,25,27,29,30,31,31,31,30,29,27,25,22,19,16,12,9,6,4,2,1,0,0,0,1,2,4,6,9,12}},
        {{16,22,27,30,31,30,27,22,16,9,4,1,0,1,4,9,16,22,27,30,31,30,27,22,16,9,4,1,0,1,4,9}},
        {{0,3,6,9,12,15,18,21,24,27,30,31,4,7,10,13,16,19,22,25,28,31,2,5,8,11,14,17,20,23,26,29}},
        {{4,6,9,13,18,24,29,31,28,22,16,11,8,7,9,13,18,23,27,28,25,20,14,9,6,5,7,11,16,21,24,22}},
        {{16,24,28,24,16,8,4,8,16,22,26,22,16,10,6,10,16,20,24,20,16,12,8,12,16,18,22,18,16,14,10,14}},
        {{16,18,22,28,31,25,12,3,0,6,19,29,27,15,5,8,20,30,25,11,2,7,22,31,21,6,4,18,30,24,9,1}},
        {{0,31,4,27,8,23,12,19,16,15,20,11,24,7,28,3,31,0,27,4,23,8,19,12,15,16,11,20,7,24,3,28}},
        {{5,8,14,23,30,31,27,19,12,9,11,17,24,28,25,17,9,5,7,14,23,29,28,21,13,8,9,15,22,25,20,11}},
        {{9,13,20,27,31,29,22,14,8,6,10,18,25,28,24,16,10,8,12,20,27,29,24,16,10,7,11,19,25,26,20,13}},
        {{2,2,4,6,10,15,22,29,31,25,18,12,8,6,5,5,4,4,6,9,14,20,27,30,26,18,11,7,5,4,3,2}},
        {{0,0,4,4,8,8,12,12,16,16,20,20,24,24,28,28,31,31,27,27,23,23,19,19,15,15,11,11,7,7,3,3}},
        {{0,6,14,24,31,28,18,8,2,5,15,27,30,21,10,3,7,18,29,27,16,6,4,12,25,31,23,11,2,8,20,28}},
        {{0,0,8,8,15,15,7,7,22,22,30,30,18,18,10,10,31,31,20,20,12,12,25,25,5,5,16,16,2,2,27,27}},
        {{3,5,9,15,23,30,31,25,15,7,4,8,17,27,30,23,12,5,6,14,25,31,27,17,8,4,10,20,29,29,20,10}},
        {{0,0,0,31,31,31,8,8,8,24,24,24,4,4,4,28,28,28,12,12,12,20,20,20,2,2,30,30,16,16,6,26}},
        {{1,27,5,31,9,19,3,24,14,29,0,17,7,26,11,22,4,30,13,20,2,25,8,18,6,28,10,23,12,21,15,16}}
    }};
    return bank;
}

inline const std::array<const char*, kWaveMemoryFactoryCount>& waveMemoryFactoryNames()
{
    static const std::array<const char*, kWaveMemoryFactoryCount> names {{
        "SOFT SINE", "HOLLOW", "BRIGHT 5", "REED", "ORGAN", "BELL", "METAL",
        "VOCAL A", "VOCAL O", "BASS STEP", "ARCADE 1", "ARCADE 2", "MAZE",
        "TOWER", "PULSE MIX", "NOISY EDGE"
    }};
    return names;
}

enum class Waveform
{
    saw,
    pulse,
    triangle,
    waveMemory
};

enum class LfoWaveform
{
    triangle,
    sawUp,
    sawDown,
    square,
    sampleAndHold
};

enum class VoiceMode
{
    poly,
    mono,
    unison
};

struct EnvelopeParams
{
    double attackSeconds = 0.01;
    double decaySeconds = 0.25;
    double sustainLevel = 0.75;
    double releaseSeconds = 0.4;
};

struct OscillatorParams
{
    Waveform waveform = Waveform::saw;
    bool sawEnabled = true;
    bool triangleEnabled = false;
    bool pulseEnabled = false;
    bool waveMemoryEnabled = false;
    int waveMemoryIndex = 0;
    WaveMemoryCharacter waveMemoryCharacter = WaveMemoryCharacter::fiveBit;
    WaveMemoryData waveMemoryData = waveMemoryFactoryBank()[0];
    bool lowFrequencyMode = false;
    bool keyboardTracking = true;
    double octave = 0.0;
    double semitones = 0.0;
    double fineCents = 0.0;
    double pulseWidth = 0.5;
    double level = 0.5;
};

struct AnalogPatch
{
    VoiceMode voiceMode = VoiceMode::poly;
    OscillatorParams oscillatorA;
    OscillatorParams oscillatorB = []
    {
        OscillatorParams oscillator;
        oscillator.fineCents = 7.0;
        return oscillator;
    }();
    bool oscillatorSync = false;
    EnvelopeParams filterEnvelope { 0.01, 0.3, 0.4, 0.5 };
    EnvelopeParams amplifierEnvelope;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.1;
    double filterEnvelopeAmount = 0.25;
    double filterKeyboardTracking = 0.0;
    double filterVelocityAmount = 0.0;
    double noiseLevel = 0.0;
    LfoWaveform lfoWaveform = LfoWaveform::triangle;
    int lfoWaveformMask = 2;
    double lfoRateHz = 5.0;
    double lfoInitialAmount = 0.0;
    double lfoWheelAmount = 0.35;
    double lfoDelaySeconds = 0.0;
    double lfoFadeSeconds = 0.0;
    bool lfoRetrigger = false;
    double lfoPitchDepthASemitones = 0.0;
    double lfoPitchDepthBSemitones = 0.0;
    double lfoFilterDepthOctaves = 0.0;
    double lfoPulseWidthDepthA = 0.0;
    double lfoPulseWidthDepthB = 0.0;
    double polyModOscillatorBToPitch = 0.0;
    double polyModFilterEnvelopeToPitch = 0.0;
    double polyModOscillatorBToPulseWidthA = 0.0;
    double polyModFilterEnvelopeToPulseWidthA = 0.0;
    double polyModOscillatorBToFilter = 0.0;
    double polyModFilterEnvelopeToFilter = 0.0;
    double glideSeconds = 0.0;
    bool glideLegatoOnly = false;
    double masterTuneCents = 0.0;
    double unisonDetuneCents = 14.0;
    double stereoSpread = 0.0;
    double vintageAmount = 0.0;
    double masterGain = 0.8;
};

struct StereoSample
{
    double left = 0.0;
    double right = 0.0;
};

AnalogPatch normalizePatch(const AnalogPatch& patch);
double midiNoteToFrequency(double midiNote);
} // namespace aureline
