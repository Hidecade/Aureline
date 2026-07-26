#import "AurelineMobileEngineBridge.h"

#include "Engine/AnalogEngine.h"
#include "Engine/FactoryPresets.h"
#include "Engine/PerformanceSequencer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

namespace
{
enum class CommandType { noteOn, noteOff, sustain, pitchBend, modWheel, panic };
struct Command { CommandType type {}; int note = 0; int velocity = 0; double value = 0.0; };
constexpr uint32_t kCommandCapacity = 1024;
constexpr std::size_t kScopeSize = 128;

enum Parameter : NSUInteger
{
    oscALevel, oscBLevel, oscBFine, pulseWidthA, pulseWidthB, noiseLevel,
    cutoff, resonance, filterEnvAmount, filterKeyTrack, filterVelocity,
    filterAttack, filterDecay, filterSustain, filterRelease,
    ampAttack, ampDecay, ampSustain, ampRelease,
    lfoRate, lfoAmount, lfoModRange, lfoDelay, lfoFade, lfoRetrigger, lfoWaveMask,
    lfoDestA, lfoDestB, lfoDestPWA, lfoDestPWB, lfoDestFilter,
    polyModFilterEnv, polyModOscB, polyDestPitch, polyDestPWA, polyDestFilter,
    spread, vintage, masterGain, transpose, glide, glideLegato,
    masterTune, unisonDetune, voiceMode, waveformMaskA, waveformMaskB,
    waveMemoryIndexA, waveMemoryIndexB, waveMemoryCharacterA, waveMemoryCharacterB,
    oscSync, oscAOctave, oscBOctave, oscBLowFrequency, oscBKeyTrack,
    pitchBendRange, arpEnabled, chordEnabled, arpHold, tempoBpm, arpRate,
    arpDirection, arpGate, scaleRoot, parameterCount
};

constexpr std::array<const char*, parameterCount> parameterNames {{
    "oscALevel", "oscBLevel", "oscBFine", "pulseWidthA", "pulseWidthB", "noiseLevel",
    "cutoff", "resonance", "filterEnvAmount", "filterKeyTrack", "filterVelocity",
    "filterAttack", "filterDecay", "filterSustain", "filterRelease",
    "ampAttack", "ampDecay", "ampSustain", "ampRelease",
    "lfoRate", "lfoAmount", "modRange", "lfoDelay", "lfoFade", "lfoRetrigger", "lfoWaveMask",
    "lfoDestA", "lfoDestB", "lfoDestPWA", "lfoDestPWB", "lfoDestFilter",
    "polyModFilterEnv", "polyModOscB", "polyDestPitch", "polyDestPWA", "polyDestFilter",
    "spread", "vintage", "masterGain", "transpose", "glide", "glideLegato",
    "masterTune", "unisonDetune", "voiceMode", "waveformMaskA", "waveformMaskB",
    "waveMemoryIndexA", "waveMemoryIndexB", "waveMemoryCharacterA", "waveMemoryCharacterB",
    "oscSync", "oscAOctave", "oscBOctave", "oscBLowFrequency", "oscBKeyTrack",
    "pitchBendRange", "arpEnabled", "chordEnabled", "arpHold", "tempoBpm",
    "arpRate", "arpDirection", "arpGate", "scaleRoot"
}};

Parameter parameterForName(NSString* name)
{
    for (NSUInteger index = 0; index < parameterNames.size(); ++index)
        if ([name isEqualToString:@(parameterNames[index])]) return static_cast<Parameter>(index);
    return parameterCount;
}

double clamp(double value, double low, double high) { return std::max(low, std::min(value, high)); }
}

@interface AurelineMobileEngineBridge ()
{
    std::unique_ptr<aureline::AnalogEngine> engine;
    aureline::PerformanceSequencer sequencer;
    std::array<std::atomic<double>, parameterCount> values;
    std::array<std::atomic<uint8_t>, aureline::kWaveMemorySize> waveMemoryStepsA;
    std::array<std::atomic<uint8_t>, aureline::kWaveMemorySize> waveMemoryStepsB;
    std::atomic<bool> waveMemoryUserA;
    std::atomic<bool> waveMemoryUserB;
    std::array<Command, kCommandCapacity> commands;
    std::atomic<uint32_t> writeIndex;
    std::atomic<uint32_t> readIndex;
    std::atomic<bool> commandOverflow;
    std::atomic<bool> patchDirty;
    std::array<std::atomic<float>, kScopeSize> scopeSamples;
    std::atomic<std::size_t> scopeWriteIndex;
    double preparedSampleRate;
}
@end

@implementation AurelineMobileEngineBridge

- (instancetype)init
{
    if ((self = [super init]))
    {
        engine = std::make_unique<aureline::AnalogEngine>();
        writeIndex.store(0); readIndex.store(0); commandOverflow.store(false); patchDirty.store(true);
        scopeWriteIndex.store(0);
        for (auto& sample : scopeSamples) sample.store(0.0f, std::memory_order_relaxed);
        preparedSampleRate = 44100.0;
        [self resetPatch];
    }
    return self;
}

- (void)prepareWithSampleRate:(double)sampleRate
{
    preparedSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine->prepare(preparedSampleRate);
    sequencer.prepare(preparedSampleRate);
    patchDirty.store(true, std::memory_order_release);
}

- (void)enqueue:(Command)command
{
    const auto write = writeIndex.load(std::memory_order_relaxed);
    const auto next = (write + 1) % kCommandCapacity;
    if (next == readIndex.load(std::memory_order_acquire))
    {
        commandOverflow.store(true, std::memory_order_release);
        return;
    }
    commands[write] = command;
    writeIndex.store(next, std::memory_order_release);
}

- (void)noteOn:(int)note velocity:(int)velocity { [self enqueue:{ CommandType::noteOn, note, velocity, 0.0 }]; }
- (void)noteOff:(int)note { [self enqueue:{ CommandType::noteOff, note, 0, 0.0 }]; }
- (void)setPitchBend:(double)value { [self enqueue:{ CommandType::pitchBend, 0, 0, clamp(value, -1.0, 1.0) }]; }
- (void)setModWheel:(double)value { [self enqueue:{ CommandType::modWheel, 0, 0, clamp(value, 0.0, 1.0) }]; }
- (void)setSustainPedal:(BOOL)down { [self enqueue:{ CommandType::sustain, 0, 0, down ? 1.0 : 0.0 }]; }
- (void)panic { [self enqueue:{ CommandType::panic, 0, 0, 0.0 }]; }
- (void)setPitchBendRange:(double)value { [self setParameter:@"pitchBendRange" value:value]; }
- (double)currentLFOValue { return engine->currentLfoValue(); }

- (void)resetPatch
{
    aureline::AnalogPatch patch;
    const std::array<double, parameterCount> defaults {{
        patch.oscillatorA.level, patch.oscillatorB.level, patch.oscillatorB.fineCents,
        patch.oscillatorA.pulseWidth, patch.oscillatorB.pulseWidth, patch.noiseLevel,
        patch.filterCutoffHz, patch.filterResonance, patch.filterEnvelopeAmount,
        patch.filterKeyboardTracking, patch.filterVelocityAmount,
        patch.filterEnvelope.attackSeconds, patch.filterEnvelope.decaySeconds,
        patch.filterEnvelope.sustainLevel, patch.filterEnvelope.releaseSeconds,
        patch.amplifierEnvelope.attackSeconds, patch.amplifierEnvelope.decaySeconds,
        patch.amplifierEnvelope.sustainLevel, patch.amplifierEnvelope.releaseSeconds,
        patch.lfoRateHz, patch.lfoInitialAmount, patch.lfoWheelAmount,
        patch.lfoDelaySeconds, patch.lfoFadeSeconds,
        patch.lfoRetrigger ? 1.0 : 0.0, static_cast<double>(patch.lfoWaveformMask),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        patch.stereoSpread, patch.vintageAmount, patch.masterGain, 0.0,
        patch.glideSeconds, patch.glideLegatoOnly ? 1.0 : 0.0, patch.masterTuneCents,
        patch.unisonDetuneCents, static_cast<double>(patch.voiceMode), 1, 1,
        0.0, 0.0, 0.0, 0.0,
        patch.oscillatorSync ? 1.0 : 0.0, patch.oscillatorA.octave, patch.oscillatorB.octave,
        patch.oscillatorB.lowFrequencyMode ? 1.0 : 0.0,
        patch.oscillatorB.keyboardTracking ? 1.0 : 0.0, 2.0,
        0.0, 0.0, 0.0, 120.0, 1.0, 0.0, 0.75, 0.0
    }};
    for (NSUInteger index = 0; index < defaults.size(); ++index)
        values[index].store(defaults[index], std::memory_order_relaxed);
    for (std::size_t index = 0; index < aureline::kWaveMemorySize; ++index)
    {
        waveMemoryStepsA[index].store(patch.oscillatorA.waveMemoryData[index], std::memory_order_relaxed);
        waveMemoryStepsB[index].store(patch.oscillatorB.waveMemoryData[index], std::memory_order_relaxed);
    }
    waveMemoryUserA.store(false, std::memory_order_relaxed);
    waveMemoryUserB.store(false, std::memory_order_relaxed);
    patchDirty.store(true, std::memory_order_release);
}

- (NSArray<NSString*>*)factoryPresetNames
{
    NSMutableArray<NSString*>* result = [NSMutableArray arrayWithCapacity:aureline::kFactoryPresetCount];
    for (const auto& preset : aureline::factoryPresets())
        [result addObject:@(preset.name)];
    return result;
}

- (void)loadFactoryPreset:(int)index
{
    if (index < 0 || index >= static_cast<int>(aureline::kFactoryPresetCount)) return;
    const auto& patch = aureline::factoryPreset(static_cast<std::size_t>(index)).patch;
    const int maskA = (patch.oscillatorA.sawEnabled ? 1 : 0) | (patch.oscillatorA.triangleEnabled ? 2 : 0)
        | (patch.oscillatorA.pulseEnabled ? 4 : 0) | (patch.oscillatorA.waveMemoryEnabled ? 8 : 0);
    const int maskB = (patch.oscillatorB.sawEnabled ? 1 : 0) | (patch.oscillatorB.triangleEnabled ? 2 : 0)
        | (patch.oscillatorB.pulseEnabled ? 4 : 0) | (patch.oscillatorB.waveMemoryEnabled ? 8 : 0);
    const auto polyFilterSource = std::max({ std::abs(patch.polyModFilterEnvelopeToPitch),
        std::abs(patch.polyModFilterEnvelopeToPulseWidthA),
        std::abs(patch.polyModFilterEnvelopeToFilter) });
    const auto polyOscillatorSource = std::max({ std::abs(patch.polyModOscillatorBToPitch),
        std::abs(patch.polyModOscillatorBToPulseWidthA),
        std::abs(patch.polyModOscillatorBToFilter) });
    const std::array<double, parameterCount> presetValues {{
        patch.oscillatorA.level, patch.oscillatorB.level, patch.oscillatorB.fineCents,
        patch.oscillatorA.pulseWidth, patch.oscillatorB.pulseWidth, patch.noiseLevel,
        patch.filterCutoffHz, patch.filterResonance, patch.filterEnvelopeAmount,
        patch.filterKeyboardTracking, patch.filterVelocityAmount,
        patch.filterEnvelope.attackSeconds, patch.filterEnvelope.decaySeconds,
        patch.filterEnvelope.sustainLevel, patch.filterEnvelope.releaseSeconds,
        patch.amplifierEnvelope.attackSeconds, patch.amplifierEnvelope.decaySeconds,
        patch.amplifierEnvelope.sustainLevel, patch.amplifierEnvelope.releaseSeconds,
        patch.lfoRateHz, patch.lfoInitialAmount, patch.lfoWheelAmount,
        patch.lfoDelaySeconds, patch.lfoFadeSeconds,
        patch.lfoRetrigger ? 1.0 : 0.0, static_cast<double>(patch.lfoWaveformMask),
        patch.lfoPitchDepthASemitones != 0.0 ? 1.0 : 0.0, patch.lfoPitchDepthBSemitones != 0.0 ? 1.0 : 0.0,
        patch.lfoPulseWidthDepthA != 0.0 ? 1.0 : 0.0, patch.lfoPulseWidthDepthB != 0.0 ? 1.0 : 0.0,
        patch.lfoFilterDepthOctaves != 0.0 ? 1.0 : 0.0, polyFilterSource,
        polyOscillatorSource, patch.polyModFilterEnvelopeToPitch != 0.0 || patch.polyModOscillatorBToPitch != 0.0 ? 1.0 : 0.0,
        patch.polyModFilterEnvelopeToPulseWidthA != 0.0 || patch.polyModOscillatorBToPulseWidthA != 0.0 ? 1.0 : 0.0,
        patch.polyModFilterEnvelopeToFilter != 0.0 || patch.polyModOscillatorBToFilter != 0.0 ? 1.0 : 0.0,
        patch.stereoSpread, patch.vintageAmount, patch.masterGain, patch.oscillatorA.semitones,
        patch.glideSeconds, patch.glideLegatoOnly ? 1.0 : 0.0, patch.masterTuneCents,
        patch.unisonDetuneCents, static_cast<double>(patch.voiceMode), static_cast<double>(maskA), static_cast<double>(maskB),
        static_cast<double>(patch.oscillatorA.waveMemoryIndex),
        static_cast<double>(patch.oscillatorB.waveMemoryIndex),
        static_cast<double>(patch.oscillatorA.waveMemoryCharacter),
        static_cast<double>(patch.oscillatorB.waveMemoryCharacter),
        patch.oscillatorSync ? 1.0 : 0.0, patch.oscillatorA.octave, patch.oscillatorB.octave,
        patch.oscillatorB.lowFrequencyMode ? 1.0 : 0.0, patch.oscillatorB.keyboardTracking ? 1.0 : 0.0, 2.0,
        0.0, 0.0, 0.0, 120.0, 1.0, 0.0, 0.75, 0.0
    }};
    for (NSUInteger parameter = 0; parameter < presetValues.size(); ++parameter)
        values[parameter].store(presetValues[parameter], std::memory_order_relaxed);
    for (std::size_t step = 0; step < aureline::kWaveMemorySize; ++step)
    {
        waveMemoryStepsA[step].store(patch.oscillatorA.waveMemoryData[step], std::memory_order_relaxed);
        waveMemoryStepsB[step].store(patch.oscillatorB.waveMemoryData[step], std::memory_order_relaxed);
    }
    waveMemoryUserA.store(false, std::memory_order_relaxed);
    waveMemoryUserB.store(false, std::memory_order_relaxed);
    patchDirty.store(true, std::memory_order_release);
}

- (void)setParameter:(NSString*)identifier value:(double)value
{
    const auto setWaveMemoryStep = [&](NSString* prefix,
                                       std::array<std::atomic<uint8_t>, aureline::kWaveMemorySize>& steps,
                                       std::atomic<bool>& isUser)
    {
        if (![identifier hasPrefix:prefix]) return false;
        const NSInteger index = [[identifier substringFromIndex:prefix.length] integerValue];
        if (index < 0 || index >= static_cast<NSInteger>(aureline::kWaveMemorySize)) return true;
        steps[static_cast<std::size_t>(index)].store(
            static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(value)), 0, 31)),
            std::memory_order_relaxed);
        isUser.store(true, std::memory_order_relaxed);
        patchDirty.store(true, std::memory_order_release);
        return true;
    };
    if (setWaveMemoryStep(@"waveMemoryStepA", waveMemoryStepsA, waveMemoryUserA)
        || setWaveMemoryStep(@"waveMemoryStepB", waveMemoryStepsB, waveMemoryUserB)) return;
    if ([identifier isEqualToString:@"waveMemoryUserA"])
    {
        waveMemoryUserA.store(value >= 0.5, std::memory_order_relaxed);
        return;
    }
    if ([identifier isEqualToString:@"waveMemoryUserB"])
    {
        waveMemoryUserB.store(value >= 0.5, std::memory_order_relaxed);
        return;
    }
    const auto parameter = parameterForName(identifier);
    if (parameter == parameterCount || !std::isfinite(value)) return;
    values[parameter].store(value, std::memory_order_relaxed);
    if (parameter == waveMemoryIndexA || parameter == waveMemoryIndexB)
    {
        const auto factoryIndex = static_cast<std::size_t>(std::clamp(
            static_cast<int>(std::lround(value)), 0,
            static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1));
        auto& steps = parameter == waveMemoryIndexA ? waveMemoryStepsA : waveMemoryStepsB;
        auto& isUser = parameter == waveMemoryIndexA ? waveMemoryUserA : waveMemoryUserB;
        for (std::size_t step = 0; step < aureline::kWaveMemorySize; ++step)
            steps[step].store(aureline::waveMemoryFactoryBank()[factoryIndex][step],
                              std::memory_order_relaxed);
        isUser.store(false, std::memory_order_relaxed);
    }
    patchDirty.store(true, std::memory_order_release);
}

- (double)parameterValue:(NSString*)identifier
{
    const auto parameter = parameterForName(identifier);
    return parameter == parameterCount ? 0.0 : values[parameter].load(std::memory_order_relaxed);
}

- (NSDictionary<NSString*, NSNumber*>*)patchSnapshot
{
    NSMutableDictionary* result = [NSMutableDictionary dictionaryWithCapacity:parameterCount + 66];
    for (NSUInteger index = 0; index < parameterNames.size(); ++index)
        result[@(parameterNames[index])] = @(values[index].load(std::memory_order_relaxed));
    result[@"waveMemoryUserA"] = @(waveMemoryUserA.load(std::memory_order_relaxed));
    result[@"waveMemoryUserB"] = @(waveMemoryUserB.load(std::memory_order_relaxed));
    for (std::size_t index = 0; index < aureline::kWaveMemorySize; ++index)
    {
        result[[NSString stringWithFormat:@"waveMemoryStepA%02zu", index]]
            = @(waveMemoryStepsA[index].load(std::memory_order_relaxed));
        result[[NSString stringWithFormat:@"waveMemoryStepB%02zu", index]]
            = @(waveMemoryStepsB[index].load(std::memory_order_relaxed));
    }
    return result;
}

- (void)applyPatchSnapshot:(NSDictionary<NSString*, NSNumber*>*)snapshot
{
    // Older voice/library files predate MOD RANGE. Give them the standard
    // musical wheel range instead of inheriting the previous voice.
    if (![snapshot[@"modRange"] isKindOfClass:NSNumber.class])
        [self setParameter:@"modRange" value:0.35];
    for (NSString* name in snapshot)
        if (![name hasPrefix:@"waveMemoryStep"] && ![name hasPrefix:@"waveMemoryUser"]
            && [snapshot[name] isKindOfClass:NSNumber.class])
            [self setParameter:name value:snapshot[name].doubleValue];
    for (NSString* name in snapshot)
        if ([name hasPrefix:@"waveMemoryStep"] && [snapshot[name] isKindOfClass:NSNumber.class])
            [self setParameter:name value:snapshot[name].doubleValue];
    for (NSString* name in snapshot)
        if ([name hasPrefix:@"waveMemoryUser"] && [snapshot[name] isKindOfClass:NSNumber.class])
            [self setParameter:name value:snapshot[name].doubleValue];
}

- (void)applyPatch
{
    aureline::AnalogPatch patch;
    auto v = [self](Parameter p) { return self->values[p].load(std::memory_order_relaxed); };
    patch.oscillatorA.level = v(oscALevel); patch.oscillatorB.level = v(oscBLevel);
    patch.oscillatorB.fineCents = v(oscBFine); patch.oscillatorA.pulseWidth = v(pulseWidthA);
    patch.oscillatorB.pulseWidth = v(pulseWidthB); patch.noiseLevel = v(noiseLevel);
    patch.filterCutoffHz = v(cutoff); patch.filterResonance = v(resonance);
    patch.filterEnvelopeAmount = v(filterEnvAmount); patch.filterKeyboardTracking = v(filterKeyTrack);
    patch.filterVelocityAmount = v(filterVelocity);
    patch.filterEnvelope = { v(filterAttack), v(filterDecay), v(filterSustain), v(filterRelease) };
    patch.amplifierEnvelope = { v(ampAttack), v(ampDecay), v(ampSustain), v(ampRelease) };
    patch.lfoRateHz = v(lfoRate); patch.lfoInitialAmount = v(lfoAmount);
    patch.lfoWheelAmount = v(lfoModRange);
    patch.lfoDelaySeconds = v(lfoDelay); patch.lfoFadeSeconds = v(lfoFade);
    patch.lfoRetrigger = v(lfoRetrigger) >= 0.5; patch.lfoWaveformMask = static_cast<int>(std::lround(v(lfoWaveMask)));
    // Destination depths are fixed maxima. lfoInitialAmount/mod wheel is applied
    // once in AnalogVoice, avoiding the previous amount-squared response.
    patch.lfoPitchDepthASemitones = v(lfoDestA) >= 0.5 ? 12.0 : 0.0;
    patch.lfoPitchDepthBSemitones = v(lfoDestB) >= 0.5 ? 12.0 : 0.0;
    patch.lfoPulseWidthDepthA = v(lfoDestPWA) >= 0.5 ? 0.25 : 0.0;
    patch.lfoPulseWidthDepthB = v(lfoDestPWB) >= 0.5 ? 0.25 : 0.0;
    patch.lfoFilterDepthOctaves = v(lfoDestFilter) >= 0.5 ? 2.0 : 0.0;
    const auto filterSource = v(polyModFilterEnv), oscSource = v(polyModOscB);
    patch.polyModFilterEnvelopeToPitch = v(polyDestPitch) >= 0.5 ? filterSource : 0.0;
    patch.polyModOscillatorBToPitch = v(polyDestPitch) >= 0.5 ? oscSource : 0.0;
    patch.polyModFilterEnvelopeToPulseWidthA = v(polyDestPWA) >= 0.5 ? filterSource : 0.0;
    patch.polyModOscillatorBToPulseWidthA = v(polyDestPWA) >= 0.5 ? oscSource : 0.0;
    patch.polyModFilterEnvelopeToFilter = v(polyDestFilter) >= 0.5 ? filterSource : 0.0;
    patch.polyModOscillatorBToFilter = v(polyDestFilter) >= 0.5 ? oscSource : 0.0;
    patch.stereoSpread = v(spread); patch.vintageAmount = v(vintage); patch.masterGain = v(masterGain);
    patch.oscillatorA.semitones = v(transpose); patch.oscillatorB.semitones = v(transpose);
    patch.glideSeconds = v(glide); patch.glideLegatoOnly = v(glideLegato) >= 0.5;
    patch.masterTuneCents = v(masterTune); patch.unisonDetuneCents = v(unisonDetune);
    patch.voiceMode = static_cast<aureline::VoiceMode>(std::clamp(static_cast<int>(std::lround(v(voiceMode))), 0, 2));
    const int maskA = static_cast<int>(std::lround(v(waveformMaskA)));
    const int maskB = static_cast<int>(std::lround(v(waveformMaskB)));
    patch.oscillatorA.sawEnabled = (maskA & 1) != 0; patch.oscillatorA.triangleEnabled = (maskA & 2) != 0; patch.oscillatorA.pulseEnabled = (maskA & 4) != 0;
    patch.oscillatorB.sawEnabled = (maskB & 1) != 0; patch.oscillatorB.triangleEnabled = (maskB & 2) != 0; patch.oscillatorB.pulseEnabled = (maskB & 4) != 0;
    patch.oscillatorA.waveMemoryEnabled = (maskA & 8) != 0;
    patch.oscillatorB.waveMemoryEnabled = (maskB & 8) != 0;
    patch.oscillatorA.waveMemoryIndex = std::clamp(static_cast<int>(std::lround(v(waveMemoryIndexA))), 0,
                                                   static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1);
    patch.oscillatorB.waveMemoryIndex = std::clamp(static_cast<int>(std::lround(v(waveMemoryIndexB))), 0,
                                                   static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1);
    patch.oscillatorA.waveMemoryCharacter = static_cast<aureline::WaveMemoryCharacter>(
        std::clamp(static_cast<int>(std::lround(v(waveMemoryCharacterA))), 0, 2));
    patch.oscillatorB.waveMemoryCharacter = static_cast<aureline::WaveMemoryCharacter>(
        std::clamp(static_cast<int>(std::lround(v(waveMemoryCharacterB))), 0, 2));
    for (std::size_t index = 0; index < aureline::kWaveMemorySize; ++index)
    {
        patch.oscillatorA.waveMemoryData[index] = waveMemoryStepsA[index].load(std::memory_order_relaxed);
        patch.oscillatorB.waveMemoryData[index] = waveMemoryStepsB[index].load(std::memory_order_relaxed);
    }
    patch.oscillatorSync = v(oscSync) >= 0.5; patch.oscillatorA.octave = v(oscAOctave); patch.oscillatorB.octave = v(oscBOctave);
    patch.oscillatorB.lowFrequencyMode = v(oscBLowFrequency) >= 0.5; patch.oscillatorB.keyboardTracking = v(oscBKeyTrack) >= 0.5;
    engine->setPatch(aureline::normalizePatch(patch));
    engine->setPitchBendRange(clamp(v(pitchBendRange), 0.0, 24.0));
    aureline::PerformanceSequencerSettings sequencerSettings;
    sequencerSettings.arpeggiatorEnabled = v(arpEnabled) >= 0.5;
    sequencerSettings.chordEnabled = v(chordEnabled) >= 0.5;
    sequencerSettings.holdEnabled = v(arpHold) >= 0.5;
    sequencerSettings.tempoBpm = v(tempoBpm);
    sequencerSettings.rate = static_cast<int>(std::lround(v(arpRate)));
    sequencerSettings.direction = static_cast<int>(std::lround(v(arpDirection)));
    sequencerSettings.gate = v(arpGate);
    sequencerSettings.scaleRoot = static_cast<int>(std::lround(v(scaleRoot)));
    sequencer.setSettings(sequencerSettings);
}

- (void)drainCommands
{
    if (commandOverflow.exchange(false, std::memory_order_acq_rel)) sequencer.panic(*engine);
    auto read = readIndex.load(std::memory_order_relaxed);
    const auto write = writeIndex.load(std::memory_order_acquire);
    while (read != write)
    {
        const auto command = commands[read];
        switch (command.type)
        {
            case CommandType::noteOn: sequencer.noteOn(*engine, std::clamp(command.note, 0, 127), std::clamp(command.velocity, 1, 127)); break;
            case CommandType::noteOff: sequencer.noteOff(*engine, std::clamp(command.note, 0, 127)); break;
            case CommandType::sustain: engine->setSustainPedal(command.value >= 0.5); break;
            case CommandType::pitchBend: engine->setPitchBend(command.value); break;
            case CommandType::modWheel: engine->setModWheel(command.value); break;
            case CommandType::panic: sequencer.panic(*engine); break;
        }
        read = (read + 1) % kCommandCapacity;
    }
    readIndex.store(read, std::memory_order_release);
}

- (void)renderLeft:(float*)left right:(float*)right frames:(int)frames
{
    if (frames <= 0) return;
    if (patchDirty.exchange(false, std::memory_order_acq_rel)) [self applyPatch];
    [self drainCommands];
    for (int frame = 0; frame < frames; ++frame)
    {
        const auto output = sequencer.renderStereoSample(*engine);
        left[frame] = static_cast<float>(output.left);
        right[frame] = static_cast<float>(output.right);
        const auto index = scopeWriteIndex.load(std::memory_order_relaxed);
        scopeSamples[index].store(left[frame], std::memory_order_relaxed);
        scopeWriteIndex.store((index + 1) % kScopeSize, std::memory_order_release);
    }
}

- (NSData*)scopeSnapshotData
{
    std::array<float, kScopeSize> snapshot {};
    const auto start = scopeWriteIndex.load(std::memory_order_acquire);
    for (std::size_t index = 0; index < kScopeSize; ++index)
        snapshot[index] = scopeSamples[(start + index) % kScopeSize].load(std::memory_order_relaxed);
    return [NSData dataWithBytes:snapshot.data() length:sizeof(snapshot)];
}

- (void)renderToAudioBufferList:(AudioBufferList*)buffers frames:(int)frames
{
    if (buffers == nullptr || buffers->mNumberBuffers == 0) return;
    if (buffers->mNumberBuffers >= 2)
    {
        auto* left = static_cast<float*>(buffers->mBuffers[0].mData);
        auto* right = static_cast<float*>(buffers->mBuffers[1].mData);
        if (left && right) [self renderLeft:left right:right frames:frames];
        return;
    }
    auto* interleaved = static_cast<float*>(buffers->mBuffers[0].mData);
    if (!interleaved) return;
    for (int frame = 0; frame < frames; ++frame)
    {
        float left = 0.0f, right = 0.0f;
        [self renderLeft:&left right:&right frames:1];
        interleaved[frame * 2] = left; interleaved[frame * 2 + 1] = right;
    }
}

@end
