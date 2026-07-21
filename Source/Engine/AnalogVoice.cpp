#include "Engine/AnalogVoice.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
namespace
{
double voiceVariation(int voiceIndex, unsigned int salt)
{
    auto state = static_cast<unsigned int>(voiceIndex + 1) * 0x9e3779b9U + salt;
    state ^= state >> 16U;
    state *= 0x7feb352dU;
    state ^= state >> 15U;
    return (static_cast<double>(state) / 4294967295.0) * 2.0 - 1.0;
}

EnvelopeParams variedEnvelope(const EnvelopeParams& source, double factor)
{
    auto envelope = source;
    envelope.attackSeconds *= factor;
    envelope.decaySeconds *= factor;
    envelope.releaseSeconds *= factor;
    return envelope;
}
} // namespace

void AnalogVoice::prepare(double sampleRate, int voiceIndex)
{
    currentSampleRate = std::max(1.0, sampleRate);
    oscillatorA.prepare(sampleRate);
    oscillatorB.prepare(sampleRate);
    filterEnvelope.prepare(sampleRate);
    amplifierEnvelope.prepare(sampleRate);
    filter.prepare(sampleRate);
    oscillatorALevelSmoother.prepare(sampleRate);
    oscillatorBLevelSmoother.prepare(sampleRate);
    pulseWidthASmoother.prepare(sampleRate);
    pulseWidthBSmoother.prepare(sampleRate);
    noiseLevelSmoother.prepare(sampleRate);
    cutoffSmoother.prepare(sampleRate, 0.02);
    resonanceSmoother.prepare(sampleRate, 0.02);
    voiceTuneCents = 0.0;
    voicePanPosition = (static_cast<double>(voiceIndex) / 7.0) * 2.0 - 1.0;
    oscillatorATuneVariation = voiceVariation(voiceIndex, 0x1234U);
    oscillatorBTuneVariation = voiceVariation(voiceIndex, 0x2345U);
    filterVariation = voiceVariation(voiceIndex, 0x3456U);
    envelopeVariation = voiceVariation(voiceIndex, 0x4567U);
    gainVariation = voiceVariation(voiceIndex, 0x5678U);
    vintagePanVariation = voiceVariation(voiceIndex, 0x6789U);
    driftRateHz = 0.035 + 0.055 * (voiceVariation(voiceIndex, 0x789aU) * 0.5 + 0.5);
    driftPhase = (voiceVariation(voiceIndex, 0x89abU) * 0.5 + 0.5) * 6.283185307179586;
    noise.reset(0x41c64e6dU + static_cast<unsigned int>(voiceIndex) * 0x9e3779b9U);
    reset();
}

void AnalogVoice::start(int midiNote, int velocity, std::uint64_t ageValue,
                        double startNote, double glideSeconds, double detuneCents,
                        double initialPhase)
{
    currentNote = std::clamp(midiNote, 0, 127);
    velocityGain = std::clamp(velocity, 1, 127) / 127.0;
    startAge = ageValue;
    unisonDetuneCents = detuneCents;
    configureGlide(startNote, static_cast<double>(currentNote), glideSeconds);
    releasing = false;
    noteSamples = 0;
    oscillatorA.reset(initialPhase);
    oscillatorB.reset(initialPhase + 0.37);
    filter.reset();
    filterEnvelope.noteOn();
    amplifierEnvelope.noteOn();
}

void AnalogVoice::retarget(int midiNote, double glideSeconds)
{
    currentNote = std::clamp(midiNote, 0, 127);
    configureGlide(currentPitchNote, static_cast<double>(currentNote), glideSeconds);
    releasing = false;
}

void AnalogVoice::configureGlide(double startNote, double targetNote, double glideSeconds)
{
    currentPitchNote = startNote;
    targetPitchNote = targetNote;
    const auto sampleCount = std::max(1.0, glideSeconds * currentSampleRate);
    glideStepPerSample = (targetPitchNote - currentPitchNote) / sampleCount;
    if (glideSeconds <= 0.0)
        currentPitchNote = targetPitchNote;
}

void AnalogVoice::release()
{
    releasing = true;
    filterEnvelope.noteOff();
    amplifierEnvelope.noteOff();
}

void AnalogVoice::reset()
{
    currentNote = -1;
    currentPitchNote = 60.0;
    targetPitchNote = 60.0;
    glideStepPerSample = 0.0;
    unisonDetuneCents = 0.0;
    velocityGain = 0.0;
    releasing = false;
    noteSamples = 0;
    filterEnvelope.reset();
    amplifierEnvelope.reset();
    filter.reset();
}

double AnalogVoice::oscillatorFrequency(const OscillatorParams& oscillator,
                                        double pitchBendSemitones,
                                        double vintageTuneCents) const
{
    if (oscillator.lowFrequencyMode)
        return 0.5 * std::pow(2.0, oscillator.octave + oscillator.semitones / 12.0
                                  + oscillator.fineCents / 1200.0);
    const auto trackedNote = oscillator.keyboardTracking ? currentPitchNote : 60.0;
    const auto note = trackedNote + oscillator.octave * 12.0
                    + oscillator.semitones
                    + (oscillator.fineCents + voiceTuneCents + unisonDetuneCents
                       + vintageTuneCents) / 100.0
                    + pitchBendSemitones;
    return midiNoteToFrequency(note);
}

double AnalogVoice::render(const AnalogPatch& patch, double pitchBendSemitones,
                           double lfoValue, double modWheel)
{
    if (!isActive())
        return 0.0;

    const auto elapsedSeconds = static_cast<double>(noteSamples++) / currentSampleRate;
    double automaticLfoAmount = 0.0;
    if (elapsedSeconds >= patch.lfoDelaySeconds)
    {
        const auto fadeElapsed = elapsedSeconds - patch.lfoDelaySeconds;
        const auto fadeProgress = patch.lfoFadeSeconds <= 0.0
            ? 1.0 : std::clamp(fadeElapsed / patch.lfoFadeSeconds, 0.0, 1.0);
        automaticLfoAmount = patch.lfoInitialAmount * fadeProgress;
    }
    const auto lfoModulationAmount = std::clamp(automaticLfoAmount + modWheel, 0.0, 1.0);

    driftPhase += 6.283185307179586 * driftRateHz / currentSampleRate;
    if (driftPhase >= 6.283185307179586)
        driftPhase -= 6.283185307179586;
    const auto driftCents = std::sin(driftPhase) * patch.vintageAmount * 1.5;
    const auto tuneA = patch.masterTuneCents
                     + patch.vintageAmount * oscillatorATuneVariation * 6.0 + driftCents;
    const auto tuneB = patch.masterTuneCents
                     + patch.vintageAmount * oscillatorBTuneVariation * 6.0 - driftCents * 0.73;
    const auto envelopeFactor = std::max(0.5, 1.0
        + patch.vintageAmount * envelopeVariation * 0.15);
    const auto filterEnvelopeParameters = variedEnvelope(patch.filterEnvelope, envelopeFactor);
    const auto amplifierEnvelopeParameters = variedEnvelope(patch.amplifierEnvelope,
                                                              2.0 - envelopeFactor);

    if (currentPitchNote != targetPitchNote)
    {
        const auto next = currentPitchNote + glideStepPerSample;
        if ((glideStepPerSample >= 0.0 && next >= targetPitchNote)
            || (glideStepPerSample < 0.0 && next <= targetPitchNote))
            currentPitchNote = targetPitchNote;
        else
            currentPitchNote = next;
    }

    const auto envelopeValue = filterEnvelope.render(filterEnvelopeParameters);
    const auto lfoPitchA = lfoValue * patch.lfoPitchDepthASemitones * lfoModulationAmount;
    const auto lfoPitchB = lfoValue * patch.lfoPitchDepthBSemitones * lfoModulationAmount;
    const auto pulseWidthB = pulseWidthBSmoother.process(patch.oscillatorB.pulseWidth)
                           + lfoValue * patch.lfoPulseWidthDepthB * lfoModulationAmount;
    const auto b = oscillatorB.render(oscillatorFrequency(patch.oscillatorB,
                                                          pitchBendSemitones + lfoPitchB, tuneB),
                                      patch.oscillatorB.sawEnabled,
                                      patch.oscillatorB.triangleEnabled,
                                      patch.oscillatorB.pulseEnabled,
                                      pulseWidthB)
                 * oscillatorBLevelSmoother.process(patch.oscillatorB.level);
    if (patch.oscillatorSync && oscillatorB.wrappedLastSample())
        oscillatorA.reset();
    const auto polyPitch = b * patch.polyModOscillatorBToPitch * 12.0
                         + envelopeValue * patch.polyModFilterEnvelopeToPitch * 12.0;
    const auto polyPulseWidthA = b * patch.polyModOscillatorBToPulseWidthA * 0.48
                               + envelopeValue
                                   * patch.polyModFilterEnvelopeToPulseWidthA * 0.48;
    const auto a = oscillatorA.render(oscillatorFrequency(patch.oscillatorA,
                                                          pitchBendSemitones + lfoPitchA + polyPitch, tuneA),
                                      patch.oscillatorA.sawEnabled,
                                      patch.oscillatorA.triangleEnabled,
                                      patch.oscillatorA.pulseEnabled,
                                      pulseWidthASmoother.process(patch.oscillatorA.pulseWidth)
                                          + lfoValue * patch.lfoPulseWidthDepthA * lfoModulationAmount
                                          + polyPulseWidthA)
                 * oscillatorALevelSmoother.process(patch.oscillatorA.level);
    const auto amp = amplifierEnvelope.render(amplifierEnvelopeParameters);
    const auto cutoffOctaves = patch.filterEnvelopeAmount * envelopeValue * 7.0
                             + lfoValue * patch.lfoFilterDepthOctaves * lfoModulationAmount
                             + b * patch.polyModOscillatorBToFilter * 4.0
                             + envelopeValue * patch.polyModFilterEnvelopeToFilter * 4.0
                             + ((currentPitchNote - 60.0) / 12.0) * patch.filterKeyboardTracking
                             + velocityGain * patch.filterVelocityAmount * 4.0;
    const auto cutoff = cutoffSmoother.process(patch.filterCutoffHz)
                      * std::pow(2.0, cutoffOctaves);
    const auto vintageCutoff = cutoff * std::pow(2.0,
        patch.vintageAmount * filterVariation * 0.2);
    const auto mixed = a + b + noise.render() * noiseLevelSmoother.process(patch.noiseLevel);
    return filter.render(std::tanh(mixed), vintageCutoff,
                         resonanceSmoother.process(patch.filterResonance))
         * amp * velocityGain * (1.0 + patch.vintageAmount * gainVariation * 0.08);
}
} // namespace aureline
