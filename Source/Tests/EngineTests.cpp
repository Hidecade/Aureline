#include "DSP/Oscillator.h"
#include "Engine/AnalogEngine.h"

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
    const auto normalized = aureline::normalizePatch(patch);
    assert(normalized.oscillatorA.pulseWidth == 0.98);
    assert(normalized.filterCutoffHz == 20.0);
    assert(normalized.masterGain == 1.0);
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
    assert(engine.activeVoiceCount() == aureline::kVoiceCount);
    for (int sample = 0; sample < 48000; ++sample)
        assert(std::isfinite(engine.renderSample()));
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
} // namespace

int main()
{
    testPatchNormalization();
    testOscillatorIsFinite();
    testCombinedOscillatorWaveforms();
    testEightVoiceLimitAndAudio();
    testSustainAndPitchBend();
    testModulationRemainsFinite();
    testSyncAndPulseWidthModulation();
    testVoiceModesAndGlide();
    testStereoSpreadAndFilterExpression();
    testRapidParameterChangesRemainFinite();
    std::cout << "Aureline engine tests passed\n";
    return 0;
}
