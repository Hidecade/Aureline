#include "Engine/AnalogPatch.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
namespace
{
double clamp(double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}

void normalizeEnvelope(EnvelopeParams& envelope)
{
    envelope.attackSeconds = clamp(envelope.attackSeconds, 0.0001, 30.0);
    envelope.decaySeconds = clamp(envelope.decaySeconds, 0.0001, 30.0);
    envelope.sustainLevel = clamp(envelope.sustainLevel, 0.0, 1.0);
    envelope.releaseSeconds = clamp(envelope.releaseSeconds, 0.0001, 30.0);
}

void normalizeOscillator(OscillatorParams& oscillator)
{
    if (!oscillator.sawEnabled && !oscillator.triangleEnabled && !oscillator.pulseEnabled
        && !oscillator.waveMemoryEnabled)
        oscillator.sawEnabled = true;
    oscillator.waveMemoryIndex = std::clamp(
        oscillator.waveMemoryIndex, 0, static_cast<int>(kWaveMemoryFactoryCount) - 1);
    for (auto& step : oscillator.waveMemoryData)
        step = static_cast<std::uint8_t>(std::min<int>(31, step));
    oscillator.octave = clamp(oscillator.octave, -3.0, 3.0);
    oscillator.semitones = clamp(oscillator.semitones, -12.0, 12.0);
    oscillator.fineCents = clamp(oscillator.fineCents, -1200.0, 1200.0);
    oscillator.pulseWidth = clamp(oscillator.pulseWidth, 0.02, 0.98);
    oscillator.level = clamp(oscillator.level, 0.0, 1.0);
}
} // namespace

AnalogPatch normalizePatch(const AnalogPatch& source)
{
    auto patch = source;
    patch.filterMode = static_cast<FilterMode>(std::clamp(
        static_cast<int>(patch.filterMode), 0, 3));
    patch.transientAccent = clamp(patch.transientAccent, 0.0, 1.0);
    normalizeOscillator(patch.oscillatorA);
    normalizeOscillator(patch.oscillatorB);
    normalizeEnvelope(patch.filterEnvelope);
    normalizeEnvelope(patch.amplifierEnvelope);
    patch.filterCutoffHz = clamp(patch.filterCutoffHz, 20.0, 20000.0);
    patch.filterResonance = clamp(patch.filterResonance, 0.0, 1.0);
    patch.filterEnvelopeAmount = clamp(patch.filterEnvelopeAmount, -1.0, 1.0);
    patch.filterKeyboardTracking = clamp(patch.filterKeyboardTracking, 0.0, 1.0);
    patch.filterVelocityAmount = clamp(patch.filterVelocityAmount, 0.0, 1.0);
    patch.noiseLevel = clamp(patch.noiseLevel, 0.0, 1.0);
    patch.lfoRateHz = clamp(patch.lfoRateHz, 0.01, 30.0);
    patch.lfoInitialAmount = clamp(patch.lfoInitialAmount, 0.0, 1.0);
    patch.lfoWheelAmount = clamp(patch.lfoWheelAmount, 0.0, 1.0);
    patch.lfoDelaySeconds = clamp(patch.lfoDelaySeconds, 0.0, 10.0);
    patch.lfoFadeSeconds = clamp(patch.lfoFadeSeconds, 0.0, 10.0);
    patch.lfoWaveformMask = std::clamp(patch.lfoWaveformMask, 0, 31);
    patch.lfoPitchDepthASemitones = clamp(patch.lfoPitchDepthASemitones, 0.0, 12.0);
    patch.lfoPitchDepthBSemitones = clamp(patch.lfoPitchDepthBSemitones, 0.0, 12.0);
    patch.lfoFilterDepthOctaves = clamp(patch.lfoFilterDepthOctaves, 0.0, 8.0);
    patch.lfoPulseWidthDepthA = clamp(patch.lfoPulseWidthDepthA, 0.0, 0.48);
    patch.lfoPulseWidthDepthB = clamp(patch.lfoPulseWidthDepthB, 0.0, 0.48);
    patch.polyModOscillatorBToPitch = clamp(patch.polyModOscillatorBToPitch, 0.0, 1.0);
    patch.polyModFilterEnvelopeToPitch = clamp(patch.polyModFilterEnvelopeToPitch, 0.0, 1.0);
    patch.polyModOscillatorBToPulseWidthA = clamp(patch.polyModOscillatorBToPulseWidthA, 0.0, 1.0);
    patch.polyModFilterEnvelopeToPulseWidthA = clamp(patch.polyModFilterEnvelopeToPulseWidthA, 0.0, 1.0);
    patch.polyModOscillatorBToFilter = clamp(patch.polyModOscillatorBToFilter, 0.0, 1.0);
    patch.polyModFilterEnvelopeToFilter = clamp(patch.polyModFilterEnvelopeToFilter, 0.0, 1.0);
    patch.glideSeconds = clamp(patch.glideSeconds, 0.0, 5.0);
    patch.masterTuneCents = clamp(patch.masterTuneCents, -100.0, 100.0);
    patch.unisonDetuneCents = clamp(patch.unisonDetuneCents, 0.0, 100.0);
    patch.stereoSpread = clamp(patch.stereoSpread, 0.0, 1.0);
    patch.vintageAmount = clamp(patch.vintageAmount, 0.0, 1.0);
    patch.masterGain = clamp(patch.masterGain, 0.0, 1.0);
    return patch;
}

double midiNoteToFrequency(double midiNote)
{
    return 440.0 * std::pow(2.0, (midiNote - 69.0) / 12.0);
}

double polyModFrequencyMultiplier(double signal)
{
    const auto x = clamp(signal, -4.0, 4.0);

    // The central range is pitch-neutral for a bipolar waveform. The outer
    // range then opens progressively for deliberately extreme cross-modulation.
    if (std::abs(x) <= 1.0)
        return 2.0 / (1.0 + std::pow(2.0, -2.0 * x));

    if (x > 1.0)
        return 1.6 * std::pow(2.0, (x - 1.0) * std::log2(10.0) / 3.0);

    return 0.4 * std::pow(2.0, (x + 1.0) * std::log2(6.4) / 3.0);
}
} // namespace aureline
