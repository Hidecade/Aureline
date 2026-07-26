#include "DSP/Oscillator.h"
#include "DSP/ProphetOtaFilter.h"
#include "Engine/AnalogEngine.h"
#include "Engine/FactoryPresets.h"
#include "Engine/PerformanceSequencer.h"
#include "Engine/RealtimeAudioRecorder.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testPatchNormalization()
{
    aureline::AnalogPatch patch;
    patch.oscillatorA.pulseWidth = 2.0;
    patch.filterCutoffHz = -100.0;
    patch.masterGain = 4.0;
    patch.polyModOscillatorBToPitch = -1.0;
    const auto normalized = aureline::normalizePatch(patch);
    assert(normalized.oscillatorA.pulseWidth == 0.98);
    assert(normalized.filterCutoffHz == 20.0);
    assert(normalized.masterGain == 1.0);
    assert(normalized.polyModOscillatorBToPitch == 0.0);
}

void testPolyModTransferFunctions()
{
    assert(std::abs(aureline::polyModOscillatorSource(1.0, 1.0) - 2.0) < 1.0e-12);
    assert(std::abs(aureline::polyModOscillatorSource(-1.0, 0.5) + 1.0) < 1.0e-12);
    assert(aureline::polyModFilterEnvelopeSource(1.0, 0.0) == 0.0);
    assert(std::abs(aureline::polyModFilterEnvelopeSource(1.0, 0.5) - 1.0)
           < 1.0e-12);
    assert(std::abs(aureline::polyModFilterEnvelopeSource(1.0, 1.0) - 4.0)
           < 1.0e-12);
    assert(std::abs(aureline::polyModFrequencyMultiplier(0.0) - 1.0) < 1.0e-12);
    assert(std::abs(aureline::polyModFrequencyMultiplier(1.0) - 1.6) < 1.0e-12);
    assert(std::abs(aureline::polyModFrequencyMultiplier(-1.0) - 0.4) < 1.0e-12);
    for (const auto signal : { 0.1, 0.25, 0.5, 0.75, 1.0 })
        assert(std::abs(aureline::polyModFrequencyMultiplier(signal)
                        + aureline::polyModFrequencyMultiplier(-signal)
                        - 2.0)
               < 1.0e-12);
    assert(std::abs(aureline::polyModFrequencyMultiplier(8.0) - 16.0) < 1.0e-12);
    assert(std::abs(aureline::polyModFrequencyMultiplier(-8.0) - 0.0625) < 1.0e-12);
    assert(aureline::polyModOscillatorPhaseOffset(1.0, 0.0) == 0.0);
    assert(std::abs(aureline::polyModOscillatorPhaseOffset(1.0, 1.0) - 0.65)
           < 1.0e-12);
    assert(std::abs(aureline::polyModOscillatorPhaseOffset(-1.0, 0.5) + 0.1625)
           < 1.0e-12);
    assert(std::abs(aureline::polyModPulseWidthOffset(1.0) + 0.25) < 1.0e-12);
}

void testOscillatorIsFinite()
{
    aureline::Oscillator oscillator;
    oscillator.prepare(48000.0);
    for (const auto waveform : { aureline::Waveform::saw,
                                 aureline::Waveform::pulse,
                                 aureline::Waveform::triangle })
        for (int sample = 0; sample < 48000; ++sample)
            assert(std::isfinite(oscillator.render(440.0, waveform, 0.5)));
}

void testCombinedOscillatorWaveforms()
{
    aureline::Oscillator oscillator;
    oscillator.prepare(48000.0);
    double peak = 0.0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto value = oscillator.render(440.0, true, true, true, 0.33);
        assert(std::isfinite(value));
        peak = std::max(peak, std::abs(value));
    }
    assert(peak > 0.1);
}

void testProphetOtaFilterResonance()
{
    constexpr double sampleRate = 48000.0;
    aureline::ProphetOtaFilter filter;
    filter.prepare(sampleRate);

    double normalPeak = 0.0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto impulse = sample == 0 ? 0.5 : 0.0;
        const auto value = filter.render(impulse, 1000.0, 0.65);
        assert(std::isfinite(value));
        normalPeak = std::max(normalPeak, std::abs(value));
    }
    assert(normalPeak < 1.0);

    filter.reset();
    double oscillationEnergy = 0.0;
    for (int sample = 0; sample < 96000; ++sample)
    {
        const auto value = filter.render(0.0, 1000.0, 1.0);
        assert(std::isfinite(value));
        if (sample >= 72000)
            oscillationEnergy += value * value;
    }
    const auto oscillationRms = std::sqrt(oscillationEnergy / 24000.0);
    assert(oscillationRms > 0.01);
    assert(oscillationRms < 1.0);
}

void testRealtimeAudioRecorder()
{
    aureline::RealtimeAudioRecorder recorder;
    recorder.start(48000.0);
    constexpr std::array<float, 4> left { 0.1f, 0.2f, -0.3f, 0.4f };
    constexpr std::array<float, 4> right { -0.1f, -0.2f, 0.3f, -0.4f };
    recorder.push(left.data(), right.data(), static_cast<int>(left.size()));
    recorder.stop();
    assert(recorder.recordedFrameCount() == left.size());
    const auto samples = recorder.takeRecordedSamples();
    assert(samples.size() == left.size() * 2);
    for (std::size_t frame = 0; frame < left.size(); ++frame)
    {
        assert(samples[frame * 2] == left[frame]);
        assert(samples[frame * 2 + 1] == right[frame]);
    }
}

void testEightVoiceLimitAndAudio()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    for (int note = 60; note < 72; ++note)
        engine.noteOn(note, 100);
    assert(engine.activeVoiceCount() == aureline::kVoiceCount);

    double peak = 0.0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto value = engine.renderSample();
        assert(std::isfinite(value));
        peak = std::max(peak, std::abs(value));
    }
    assert(peak > 0.001);
    assert(peak <= 1.0);

    engine.panic();
    assert(engine.activeVoiceCount() == 0);
}

void testSustainAndPitchBend()
{
    aureline::AnalogEngine engine;
    engine.prepare(44100.0);
    engine.setPitchBendRange(12.0);
    engine.setPitchBend(0.5);
    engine.noteOn(60, 100);
    engine.setSustainPedal(true);
    engine.noteOff(60);
    for (int sample = 0; sample < 4410; ++sample)
        assert(std::isfinite(engine.renderSample()));
    assert(engine.activeVoiceCount() == 1);

    engine.setSustainPedal(false);
    for (int sample = 0; sample < 44100; ++sample)
        assert(std::isfinite(engine.renderSample()));
    assert(engine.activeVoiceCount() == 0);
}

void testModulationRemainsFinite()
{
    aureline::AnalogEngine engine;
    engine.prepare(96000.0);
    auto patch = engine.getPatch();
    patch.noiseLevel = 1.0;
    patch.lfoWaveformMask = 31;
    patch.lfoRateHz = 30.0;
    patch.lfoInitialAmount = 1.0;
    patch.lfoDelaySeconds = 0.1;
    patch.lfoFadeSeconds = 0.25;
    patch.lfoPitchDepthASemitones = 12.0;
    patch.lfoPitchDepthBSemitones = 12.0;
    patch.lfoFilterDepthOctaves = 8.0;
    patch.polyModOscillatorBToPitch = 1.0;
    patch.polyModFilterEnvelopeToPitch = 1.0;
    patch.polyModOscillatorBToPulseWidthA = 1.0;
    patch.polyModFilterEnvelopeToPulseWidthA = 1.0;
    patch.polyModOscillatorBToFilter = 1.0;
    patch.polyModFilterEnvelopeToFilter = 1.0;
    patch.vintageAmount = 1.0;
    patch.filterResonance = 1.0;
    engine.setPatch(patch);
    engine.setModWheel(1.0);
    engine.noteOn(96, 127);
    for (int sample = 0; sample < 96000; ++sample)
        assert(std::isfinite(engine.renderSample()));
}

void testOscillatorBPolyModIsIndependentOfMixerLevel()
{
    constexpr double sampleRate = 48000.0;
    aureline::AnalogEngine unmodulatedEngine;
    aureline::AnalogEngine modulatedEngine;
    unmodulatedEngine.prepare(sampleRate);
    modulatedEngine.prepare(sampleRate);

    auto patch = unmodulatedEngine.getPatch();
    patch.oscillatorA.level = 0.8;
    patch.oscillatorB.level = 0.0;
    patch.noiseLevel = 0.0;
    patch.filterCutoffHz = 12000.0;
    patch.filterResonance = 0.0;
    patch.polyModOscillatorBToPitch = 0.0;
    unmodulatedEngine.setPatch(patch);

    patch.polyModOscillatorBToPitch = 1.0;
    modulatedEngine.setPatch(patch);

    unmodulatedEngine.noteOn(60, 127);
    modulatedEngine.noteOn(60, 127);

    double accumulatedDifference = 0.0;
    for (int sample = 0; sample < 4096; ++sample)
    {
        const auto unmodulated = unmodulatedEngine.renderSample();
        const auto modulated = modulatedEngine.renderSample();
        assert(std::isfinite(unmodulated));
        assert(std::isfinite(modulated));
        accumulatedDifference += std::abs(unmodulated - modulated);
    }

    assert(accumulatedDifference > 0.1);
}

void testSyncAndPulseWidthModulation()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    auto patch = engine.getPatch();
    patch.oscillatorA.waveform = aureline::Waveform::pulse;
    patch.oscillatorA.pulseWidth = 0.02;
    patch.oscillatorB.waveform = aureline::Waveform::pulse;
    patch.oscillatorB.pulseWidth = 0.98;
    patch.oscillatorSync = true;
    patch.lfoWaveformMask = 4;
    patch.lfoRateHz = 30.0;
    patch.lfoPulseWidthDepthA = 0.48;
    patch.lfoPulseWidthDepthB = 0.48;
    engine.setPatch(patch);
    engine.setModWheel(1.0);
    engine.noteOn(84, 127);

    double peak = 0.0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto value = engine.renderSample();
        assert(std::isfinite(value));
        peak = std::max(peak, std::abs(value));
    }
    assert(peak > 0.001);
    assert(peak <= 1.0);
}

void testVoiceModesAndGlide()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    auto patch = engine.getPatch();

    patch.voiceMode = aureline::VoiceMode::mono;
    patch.glideSeconds = 0.25;
    patch.glideLegatoOnly = true;
    patch.lfoRetrigger = true;
    patch.masterTuneCents = 37.0;
    engine.setPatch(patch);
    engine.noteOn(48, 100);
    assert(engine.activeVoiceCount() == 1);
    engine.noteOn(72, 100);
    assert(engine.activeVoiceCount() == 1);
    for (int sample = 0; sample < 24000; ++sample)
        assert(std::isfinite(engine.renderSample()));
    engine.panic();

    patch.voiceMode = aureline::VoiceMode::unison;
    patch.unisonDetuneCents = 30.0;
    engine.setPatch(patch);
    engine.noteOn(60, 110);
    assert(engine.activeVoiceCount() == aureline::kUnisonVoiceCount);
    for (int sample = 0; sample < 48000; ++sample)
        assert(std::isfinite(engine.renderSample()));
}

void testMonoReturnsToLastHeldNote()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    auto patch = engine.getPatch();
    patch.voiceMode = aureline::VoiceMode::mono;
    patch.amplifierEnvelope.releaseSeconds = 0.01;
    engine.setPatch(patch);
    engine.noteOn(48, 100);
    engine.noteOn(72, 100);
    engine.noteOff(72);
    for (int sample = 0; sample < 9600; ++sample)
        assert(std::isfinite(engine.renderSample()));
    assert(engine.activeVoiceCount() == 1);
    engine.noteOff(48);
    for (int sample = 0; sample < 9600; ++sample)
        assert(std::isfinite(engine.renderSample()));
    assert(engine.activeVoiceCount() == 0);
}

void testStereoSpreadAndFilterExpression()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    auto patch = engine.getPatch();
    patch.voiceMode = aureline::VoiceMode::unison;
    patch.stereoSpread = 1.0;
    patch.filterKeyboardTracking = 1.0;
    patch.filterVelocityAmount = 1.0;
    patch.filterCutoffHz = 400.0;
    engine.setPatch(patch);
    engine.noteOn(84, 127);

    double stereoDifference = 0.0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto value = engine.renderStereoSample();
        assert(std::isfinite(value.left));
        assert(std::isfinite(value.right));
        stereoDifference += std::abs(value.left - value.right);
    }
    assert(stereoDifference > 0.01);
}

void testRapidParameterChangesRemainFinite()
{
    aureline::AnalogEngine engine;
    engine.prepare(48000.0);
    auto patch = engine.getPatch();
    engine.noteOn(60, 127);

    for (int sample = 0; sample < 48000; ++sample)
    {
        if (sample % 32 == 0)
        {
            const auto high = ((sample / 32) % 2) != 0;
            patch.oscillatorA.level = high ? 1.0 : 0.0;
            patch.oscillatorB.level = high ? 0.0 : 1.0;
            patch.oscillatorA.pulseWidth = high ? 0.98 : 0.02;
            patch.oscillatorB.pulseWidth = high ? 0.02 : 0.98;
            patch.noiseLevel = high ? 1.0 : 0.0;
            patch.filterCutoffHz = high ? 20000.0 : 20.0;
            patch.filterResonance = high ? 1.0 : 0.0;
            patch.masterGain = high ? 1.0 : 0.0;
            engine.setPatch(patch);
        }
        const auto value = engine.renderStereoSample();
        assert(std::isfinite(value.left));
        assert(std::isfinite(value.right));
        assert(std::abs(value.left) <= 1.0);
        assert(std::abs(value.right) <= 1.0);
    }
}

void testFactoryPresets()
{
    const auto& presets = aureline::factoryPresets();
    assert(presets.size() == aureline::kFactoryPresetCount);
    assert(presets.size() == 32);
    assert(std::abs(presets[0].patch.filterCutoffHz - 3600.0) < 0.0001);
    assert(std::abs(presets[10].patch.filterResonance - 0.19) < 0.0001);
    assert(std::abs(presets[25].patch.polyModOscillatorBToFilter - 0.26)
           < 0.0001);
    assert(std::abs(presets[26].patch.polyModOscillatorBToPitch - 0.46)
           < 0.0001);
    assert(std::abs(presets[26].patch.oscillatorB.level - 0.12) < 0.0001);
    assert(std::abs(presets[26].patch.filterCutoffHz - 1900.0) < 0.0001);
    assert(std::abs(presets[26].patch.masterGain - 0.90) < 0.0001);
    for (std::size_t index = 0; index < presets.size(); ++index)
    {
        const auto& preset = presets[index];
        assert(preset.name != nullptr && preset.name[0] != '\0');
        const auto& patch = preset.patch;
        const bool usesPolyMod = patch.polyModOscillatorBToPitch != 0.0
            || patch.polyModFilterEnvelopeToPitch != 0.0
            || patch.polyModOscillatorBToPulseWidthA != 0.0
            || patch.polyModFilterEnvelopeToPulseWidthA != 0.0
            || patch.polyModOscillatorBToFilter != 0.0
            || patch.polyModFilterEnvelopeToFilter != 0.0;
        const bool usesWaveMemory = patch.oscillatorA.waveMemoryEnabled
            || patch.oscillatorB.waveMemoryEnabled;
        const bool shouldUsePolyMod = index == 25 || index == 26 || index == 28
                                   || index == 30 || index == 31;
        assert(usesPolyMod == shouldUsePolyMod);
        assert(!usesWaveMemory);
        if (index >= 35 && index < 39)
        {
            constexpr std::array<double, 4> expectedWidths {
                0.125, 0.25, 0.5, 0.75
            };
            assert(std::abs(patch.oscillatorA.pulseWidth
                            - expectedWidths[index - 35]) < 0.0001);
        }
        if (index == 39)
        {
            constexpr aureline::WaveMemoryData expectedTriangle {{
                31, 29, 27, 25, 23, 21, 19, 17,
                15, 13, 11, 9, 7, 5, 3, 1,
                1, 3, 5, 7, 9, 11, 13, 15,
                17, 19, 21, 23, 25, 27, 29, 31
            }};
            assert(patch.oscillatorA.waveMemoryEnabled);
            assert(patch.oscillatorA.waveMemoryCharacter
                   == aureline::WaveMemoryCharacter::fourBit);
            assert(patch.oscillatorA.waveMemoryData == expectedTriangle);
            assert(std::abs(patch.filterCutoffHz - 5200.0) < 0.0001);
            assert(std::abs(patch.amplifierEnvelope.sustainLevel - 0.9)
                   < 0.0001);
        }
        aureline::AnalogEngine engine;
        engine.prepare(48000.0);
        engine.setPatch(preset.patch);
        engine.noteOn(60, 100);
        double peak = 0.0;
        for (int sample = 0; sample < 4096; ++sample)
        {
            const auto output = engine.renderStereoSample();
            assert(std::isfinite(output.left));
            assert(std::isfinite(output.right));
            peak = std::max({ peak, std::abs(output.left), std::abs(output.right) });
        }
        assert(peak > 0.0001);
        assert(peak <= 1.0);
    }
}

void testPerformanceSequencer()
{
    aureline::AnalogEngine engine;
    aureline::PerformanceSequencer sequencer;
    engine.prepare(48000.0);
    sequencer.prepare(48000.0);
    aureline::PerformanceSequencerSettings settings;
    settings.chordEnabled = true;
    sequencer.setSettings(settings);
    sequencer.noteOn(engine, 60, 100);
    assert(engine.activeVoiceCount() == 3);
    sequencer.noteOff(engine, 60);

    settings.arpeggiatorEnabled = true;
    settings.holdEnabled = true;
    settings.tempoBpm = 240.0;
    settings.rate = 2;
    sequencer.setSettings(settings);
    sequencer.noteOn(engine, 60, 110);
    sequencer.noteOff(engine, 60);
    double peak = 0.0;
    for (int sample = 0; sample < 24000; ++sample)
    {
        const auto output = sequencer.renderStereoSample(engine);
        assert(std::isfinite(output.left));
        assert(std::isfinite(output.right));
        peak = std::max(peak, std::abs(output.left));
    }
    assert(peak > 0.001);
    sequencer.panic(engine);
    assert(engine.activeVoiceCount() == 0);
}

void testPatchChangeStartsFromNewParameters()
{
    constexpr double sampleRate = 48000.0;
    aureline::AnalogEngine changedEngine;
    aureline::AnalogEngine freshEngine;
    changedEngine.prepare(sampleRate);
    freshEngine.prepare(sampleRate);

    auto oldPatch = changedEngine.getPatch();
    oldPatch.oscillatorA.level = 0.02;
    oldPatch.oscillatorB.level = 0.95;
    oldPatch.oscillatorA.pulseWidth = 0.12;
    oldPatch.noiseLevel = 0.7;
    oldPatch.filterCutoffHz = 120.0;
    oldPatch.filterResonance = 0.9;
    oldPatch.masterGain = 0.1;
    changedEngine.setPatch(oldPatch);
    changedEngine.noteOn(48, 127);
    for (int sample = 0; sample < 4096; ++sample)
        changedEngine.renderStereoSample();

    auto newPatch = freshEngine.getPatch();
    newPatch.oscillatorA.level = 0.82;
    newPatch.oscillatorB.level = 0.0;
    newPatch.oscillatorA.pulseWidth = 0.68;
    newPatch.noiseLevel = 0.0;
    newPatch.filterCutoffHz = 9200.0;
    newPatch.filterResonance = 0.12;
    newPatch.masterGain = 0.78;

    changedEngine.setPatch(newPatch);
    changedEngine.panic();
    freshEngine.setPatch(newPatch);
    freshEngine.panic();
    changedEngine.noteOn(60, 127);
    freshEngine.noteOn(60, 127);

    for (int sample = 0; sample < 512; ++sample)
    {
        const auto changed = changedEngine.renderStereoSample();
        const auto fresh = freshEngine.renderStereoSample();
        assert(std::abs(changed.left - fresh.left) < 1.0e-12);
        assert(std::abs(changed.right - fresh.right) < 1.0e-12);
    }
}

void testWaveMemoryOscillator()
{
    assert(aureline::waveMemoryFactoryBank().size() == 16);
    for (const auto& waveform : aureline::waveMemoryFactoryBank())
        for (const auto step : waveform)
            assert(step <= 31);

    for (const auto character : { aureline::WaveMemoryCharacter::fiveBit,
                                  aureline::WaveMemoryCharacter::fourBit,
                                  aureline::WaveMemoryCharacter::smooth })
    {
        aureline::AnalogEngine engine;
        engine.prepare(48000.0);
        auto patch = engine.getPatch();
        patch.oscillatorA.sawEnabled = false;
        patch.oscillatorA.triangleEnabled = false;
        patch.oscillatorA.pulseEnabled = false;
        patch.oscillatorA.waveMemoryEnabled = true;
        patch.oscillatorA.waveMemoryIndex = 12;
        patch.oscillatorA.waveMemoryData = aureline::waveMemoryFactoryBank()[12];
        patch.oscillatorA.waveMemoryCharacter = character;
        patch.oscillatorB.level = 0.0;
        engine.setPatch(patch);
        engine.noteOn(60, 127);
        double peak = 0.0;
        for (int sample = 0; sample < 4096; ++sample)
        {
            const auto output = engine.renderStereoSample();
            assert(std::isfinite(output.left));
            assert(std::isfinite(output.right));
            peak = std::max(peak, std::abs(output.left));
        }
        assert(peak > 0.001);
    }
}

} // namespace

int main()
{
    testPatchNormalization();
    testPolyModTransferFunctions();
    testOscillatorIsFinite();
    testCombinedOscillatorWaveforms();
    testProphetOtaFilterResonance();
    testRealtimeAudioRecorder();
    testEightVoiceLimitAndAudio();
    testSustainAndPitchBend();
    testModulationRemainsFinite();
    testOscillatorBPolyModIsIndependentOfMixerLevel();
    testSyncAndPulseWidthModulation();
    testVoiceModesAndGlide();
    testMonoReturnsToLastHeldNote();
    testStereoSpreadAndFilterExpression();
    testRapidParameterChangesRemainFinite();
    testFactoryPresets();
    testPerformanceSequencer();
    testPatchChangeStartsFromNewParameters();
    testWaveMemoryOscillator();
    std::cout << "Aureline engine tests passed\n";
    return 0;
}
