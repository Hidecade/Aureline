#pragma once

namespace aureline
{
enum class Waveform
{
    saw,
    pulse,
    triangle
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
    double masterGain = 0.25;
};

struct StereoSample
{
    double left = 0.0;
    double right = 0.0;
};

AnalogPatch normalizePatch(const AnalogPatch& patch);
double midiNoteToFrequency(double midiNote);
} // namespace aureline
