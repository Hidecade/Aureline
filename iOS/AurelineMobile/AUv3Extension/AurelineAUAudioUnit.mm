#import "AurelineAUAudioUnit.h"
#import "AurelineMobileEngineBridge.h"

#include "Engine/FactoryPresets.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string_view>
#include <vector>

namespace
{
struct ParameterDefinition { const char* identifier; const char* name; AUValue min; AUValue max; AUValue initial; AudioUnitParameterUnit unit; };
constexpr ParameterDefinition definitions[] {
    { "factoryVoice", "Factory Voice", 0, 49, 0, kAudioUnitParameterUnit_Indexed },
    { "oscALevel", "Oscillator A Level", 0, 1, 0.5f, kAudioUnitParameterUnit_LinearGain },
    { "oscBLevel", "Oscillator B Level", 0, 1, 0.5f, kAudioUnitParameterUnit_LinearGain },
    { "oscBFine", "Oscillator B Detune", -100, 100, 7, kAudioUnitParameterUnit_Cents },
    { "pulseWidthA", "Pulse Width A", 0.02f, 0.98f, 0.5f, kAudioUnitParameterUnit_Generic },
    { "pulseWidthB", "Pulse Width B", 0.02f, 0.98f, 0.5f, kAudioUnitParameterUnit_Generic },
    { "noiseLevel", "Noise Level", 0, 1, 0, kAudioUnitParameterUnit_LinearGain },
    { "cutoff", "Filter Cutoff", 20, 20000, 8000, kAudioUnitParameterUnit_Hertz },
    { "resonance", "Resonance", 0, 1, 0.1f, kAudioUnitParameterUnit_Generic },
    { "filterEnvAmount", "Filter Envelope", -1, 1, 0.25f, kAudioUnitParameterUnit_Generic },
    { "filterKeyTrack", "Filter Keyboard Tracking", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "filterVelocity", "Filter Velocity", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "filterAttack", "Filter Attack", 0.001f, 5, 0.01f, kAudioUnitParameterUnit_Seconds },
    { "filterDecay", "Filter Decay", 0.001f, 5, 0.3f, kAudioUnitParameterUnit_Seconds },
    { "filterSustain", "Filter Sustain", 0, 1, 0.4f, kAudioUnitParameterUnit_Generic },
    { "filterRelease", "Filter Release", 0.001f, 8, 0.5f, kAudioUnitParameterUnit_Seconds },
    { "ampAttack", "Amp Attack", 0.001f, 5, 0.01f, kAudioUnitParameterUnit_Seconds },
    { "ampDecay", "Amp Decay", 0.001f, 5, 0.25f, kAudioUnitParameterUnit_Seconds },
    { "ampSustain", "Amp Sustain", 0, 1, 0.75f, kAudioUnitParameterUnit_Generic },
    { "ampRelease", "Amp Release", 0.001f, 8, 0.4f, kAudioUnitParameterUnit_Seconds },
    { "lfoRate", "LFO Rate", 0.01f, 30, 5, kAudioUnitParameterUnit_Hertz },
    { "lfoAmount", "LFO Initial Amount", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "lfoDelay", "LFO Delay", 0, 10, 0, kAudioUnitParameterUnit_Seconds },
    { "lfoFade", "LFO Fade", 0, 10, 0, kAudioUnitParameterUnit_Seconds },
    { "lfoRetrigger", "LFO Retrigger", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "lfoWaveMask", "LFO Waveform Mask", 0, 31, 2, kAudioUnitParameterUnit_Indexed },
    { "lfoDestA", "LFO to Oscillator A", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "lfoDestB", "LFO to Oscillator B", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "lfoDestPWA", "LFO to Pulse Width A", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "lfoDestPWB", "LFO to Pulse Width B", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "lfoDestFilter", "LFO to Filter", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "polyModFilterEnv", "Poly Mod Filter Envelope", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "polyModOscB", "Poly Mod Oscillator B", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "polyDestPitch", "Poly Mod to Oscillator A Frequency", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "polyDestPWA", "Poly Mod to Pulse Width A", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "polyDestFilter", "Poly Mod to Filter", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "spread", "Stereo Spread", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "vintage", "Vintage", 0, 1, 0, kAudioUnitParameterUnit_Generic },
    { "masterGain", "Master Gain", 0, 1, 0.25f, kAudioUnitParameterUnit_LinearGain },
    { "transpose", "Transpose", -24, 24, 0, kAudioUnitParameterUnit_RelativeSemiTones },
    { "glide", "Glide", 0, 5, 0, kAudioUnitParameterUnit_Seconds },
    { "glideLegato", "Glide Legato Only", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "masterTune", "Master Tune", -100, 100, 0, kAudioUnitParameterUnit_Cents },
    { "unisonDetune", "Unison Detune", 0, 100, 14, kAudioUnitParameterUnit_Cents },
    { "voiceMode", "Voice Mode", 0, 2, 0, kAudioUnitParameterUnit_Indexed },
    { "waveformMaskA", "Oscillator A Waveform Mask", 1, 15, 1, kAudioUnitParameterUnit_Indexed },
    { "waveformMaskB", "Oscillator B Waveform Mask", 1, 15, 1, kAudioUnitParameterUnit_Indexed },
    { "waveMemoryIndexA", "Oscillator A Wave Memory", 0, 15, 0, kAudioUnitParameterUnit_Indexed },
    { "waveMemoryIndexB", "Oscillator B Wave Memory", 0, 15, 0, kAudioUnitParameterUnit_Indexed },
    { "waveMemoryCharacterA", "Oscillator A Wave Character", 0, 2, 0, kAudioUnitParameterUnit_Indexed },
    { "waveMemoryCharacterB", "Oscillator B Wave Character", 0, 2, 0, kAudioUnitParameterUnit_Indexed },
    { "oscSync", "Oscillator Sync", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "oscAOctave", "Oscillator A Octave", -2, 2, 0, kAudioUnitParameterUnit_Indexed },
    { "oscBOctave", "Oscillator B Octave", -2, 2, 0, kAudioUnitParameterUnit_Indexed },
    { "oscBLowFrequency", "Oscillator B Low Frequency", 0, 1, 0, kAudioUnitParameterUnit_Boolean },
    { "oscBKeyTrack", "Oscillator B Keyboard Tracking", 0, 1, 1, kAudioUnitParameterUnit_Boolean },
    { "pitchBendRange", "Pitch Bend Range", 0, 24, 2, kAudioUnitParameterUnit_RelativeSemiTones }
    ,{ "arpEnabled", "Arpeggiator Enabled", 0, 1, 0, kAudioUnitParameterUnit_Boolean }
    ,{ "chordEnabled", "Chord Enabled", 0, 1, 0, kAudioUnitParameterUnit_Boolean }
    ,{ "arpHold", "Arpeggiator Hold", 0, 1, 0, kAudioUnitParameterUnit_Boolean }
    ,{ "tempoBpm", "Tempo", 40, 240, 120, kAudioUnitParameterUnit_BPM }
    ,{ "arpRate", "Arpeggiator Rate", 0, 2, 1, kAudioUnitParameterUnit_Indexed }
    ,{ "arpDirection", "Arpeggiator Direction", 0, 3, 0, kAudioUnitParameterUnit_Indexed }
    ,{ "arpGate", "Arpeggiator Gate", 0.1f, 0.95f, 0.75f, kAudioUnitParameterUnit_Generic }
    ,{ "scaleRoot", "Chord Scale Root", 0, 11, 0, kAudioUnitParameterUnit_Indexed }
};

NSUInteger parameterIndex(std::string_view identifier)
{
    for (NSUInteger index = 0; index < std::size(definitions); ++index)
        if (std::string_view(definitions[index].identifier) == identifier)
            return index;
    return 0;
}
}

@interface AurelineAUAudioUnit ()
@property(nonatomic, strong) AUAudioUnitBus* outputBus;
@property(nonatomic, strong) AUAudioUnitBusArray* outputBusArray;
@property(nonatomic, strong) AUAudioUnitBusArray* inputBusArray;
@end

@implementation AurelineAUAudioUnit
{
    AurelineMobileEngineBridge* bridge;
    std::vector<float> left;
    std::vector<float> right;
    std::array<std::atomic<float>, std::size(definitions)> parameterValues;
    AUParameterObserverToken observerToken;
    AVAudioFrameCount frameCapacity;
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)description options:(AudioComponentInstantiationOptions)options error:(NSError**)error
{
    if ((self = [super initWithComponentDescription:description options:options error:error]))
    {
        AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
        NSError* busError = nil;
        _outputBus = [[AUAudioUnitBus alloc] initWithFormat:format error:&busError];
        if (_outputBus == nil) { if (error) *error = busError; return nil; }
        _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses:@[_outputBus]];
        _inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses:@[]];
        self.maximumFramesToRender = 4096;
        bridge = [AurelineMobileEngineBridge new];
        [self installParameterTree];
        frameCapacity = 0;
    }
    return self;
}

- (AUAudioUnitBusArray*)outputBusses { return self.outputBusArray; }
- (AUAudioUnitBusArray*)inputBusses { return self.inputBusArray; }

- (void)installParameterTree
{
    NSMutableArray<AUParameter*>* parameters = [NSMutableArray arrayWithCapacity:std::size(definitions)];
    NSMutableArray<NSString*>* factoryNames = [NSMutableArray arrayWithCapacity:aureline::kFactoryPresetCount];
    for (const auto& preset : aureline::factoryPresets())
        [factoryNames addObject:@(preset.name)];
    for (NSUInteger index = 0; index < std::size(definitions); ++index)
    {
        const auto& definition = definitions[index];
        NSArray<NSString*>* valueStrings = std::string_view(definition.identifier) == "factoryVoice"
            ? factoryNames : nil;
        AUParameter* parameter = [AUParameterTree createParameterWithIdentifier:@(definition.identifier)
            name:@(definition.name) address:index min:definition.min max:definition.max unit:definition.unit
            unitName:nil flags:kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable
            valueStrings:valueStrings dependentParameters:nil];
        parameter.value = definition.initial;
        parameterValues[index].store(definition.initial, std::memory_order_relaxed);
        [parameters addObject:parameter];
        if (std::string_view(definition.identifier) == "factoryVoice")
            [bridge loadFactoryPreset:0];
        else
            [bridge setParameter:@(definition.identifier) value:definition.initial];
    }
    [bridge loadFactoryPreset:0];
    [bridge setParameter:@"voiceMode" value:parameterValues[parameterIndex("voiceMode")].load()];
    [bridge setParameter:@"glide" value:parameterValues[parameterIndex("glide")].load()];
    [bridge setParameter:@"glideLegato" value:parameterValues[parameterIndex("glideLegato")].load()];
    self.parameterTree = [AUParameterTree createTreeWithChildren:parameters];
    __unsafe_unretained AurelineAUAudioUnit* unit = self;
    observerToken = [self.parameterTree tokenByAddingParameterObserver:^(AUParameterAddress address, AUValue value) {
        if (address < std::size(definitions)) {
            unit->parameterValues[address].store(value, std::memory_order_relaxed);
            if (std::string_view(definitions[address].identifier) == "factoryVoice") {
                [unit->bridge loadFactoryPreset:std::clamp(static_cast<int>(std::lround(value)),
                                                           0, static_cast<int>(aureline::kFactoryPresetCount - 1))];
                [unit->bridge setParameter:@"voiceMode"
                    value:unit->parameterValues[parameterIndex("voiceMode")].load()];
                [unit->bridge setParameter:@"glide"
                    value:unit->parameterValues[parameterIndex("glide")].load()];
                [unit->bridge setParameter:@"glideLegato"
                    value:unit->parameterValues[parameterIndex("glideLegato")].load()];
            } else
                [unit->bridge setParameter:@(definitions[address].identifier) value:value];
        }
    }];
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError**)error
{
    if (![super allocateRenderResourcesAndReturnError:error]) return NO;
    frameCapacity = std::max<AVAudioFrameCount>(1, self.maximumFramesToRender);
    left.assign(frameCapacity, 0); right.assign(frameCapacity, 0);
    [bridge prepareWithSampleRate:self.outputBus.format.sampleRate > 0 ? self.outputBus.format.sampleRate : 44100];
    return YES;
}

- (void)deallocateRenderResources
{
    [bridge panic]; left.clear(); right.clear(); frameCapacity = 0;
    [super deallocateRenderResources];
}

- (void)handleEvent:(const AURenderEvent*)event
{
    if (event->head.eventType != AURenderEventMIDI) return;
    const auto& midi = event->MIDI;
    if (midi.length == 0) return;
    const uint8_t status = midi.data[0] & 0xf0;
    const int a = midi.length > 1 ? midi.data[1] : 0;
    const int b = midi.length > 2 ? midi.data[2] : 0;
    if (status == 0x90 && b > 0) [bridge noteOn:a velocity:b];
    else if (status == 0x80 || (status == 0x90 && b == 0)) [bridge noteOff:a];
    else if (status == 0xb0 && a == 1) [bridge setModWheel:b / 127.0];
    else if (status == 0xb0 && a == 64) [bridge setSustainPedal:b >= 64];
    else if (status == 0xb0 && (a == 120 || a == 123)) [bridge panic];
    else if (status == 0xe0) [bridge setPitchBend:std::clamp(((a | (b << 7)) - 8192) / 8192.0, -1.0, 1.0)];
}

- (void)renderFrames:(AVAudioFrameCount)count offset:(AVAudioFrameCount)offset
{
    [bridge renderLeft:left.data() + offset right:right.data() + offset frames:static_cast<int>(count)];
}

- (void)copyOutput:(AudioBufferList*)output frames:(AVAudioFrameCount)frames
{
    if (output->mNumberBuffers >= 2) {
        if (output->mBuffers[0].mData) std::copy_n(left.data(), frames, static_cast<float*>(output->mBuffers[0].mData));
        if (output->mBuffers[1].mData) std::copy_n(right.data(), frames, static_cast<float*>(output->mBuffers[1].mData));
    } else if (output->mNumberBuffers == 1 && output->mBuffers[0].mData) {
        auto* data = static_cast<float*>(output->mBuffers[0].mData);
        for (AVAudioFrameCount i = 0; i < frames; ++i) { data[i * 2] = left[i]; data[i * 2 + 1] = right[i]; }
    }
}

- (AUInternalRenderBlock)internalRenderBlock
{
    __unsafe_unretained AurelineAUAudioUnit* unit = self;
    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags*, const AudioTimeStamp* timestamp, AVAudioFrameCount count,
        NSInteger, AudioBufferList* output, const AURenderEvent* events, AURenderPullInputBlock) {
        if (!output || count == 0) return noErr;
        if (count > unit->frameCapacity) return kAudioUnitErr_TooManyFramesToProcess;
        const AUEventSampleTime start = timestamp ? timestamp->mSampleTime : 0;
        AVAudioFrameCount rendered = 0;
        for (auto* event = events; event; event = event->head.next) {
            const auto frame = static_cast<AVAudioFrameCount>(std::clamp<AUEventSampleTime>(event->head.eventSampleTime - start, 0, count));
            if (frame > rendered) { [unit renderFrames:frame - rendered offset:rendered]; rendered = frame; }
            [unit handleEvent:event];
        }
        if (rendered < count) [unit renderFrames:count - rendered offset:rendered];
        [unit copyOutput:output frames:count];
        return noErr;
    };
}

- (NSDictionary<NSString*, id>*)fullState
{
    NSMutableDictionary* state = [NSMutableDictionary dictionaryWithObject:@2 forKey:@"version"];
    NSMutableDictionary* parameters =
        [NSMutableDictionary dictionaryWithCapacity:std::size(definitions)];
    for (NSUInteger index = 0; index < std::size(definitions); ++index)
        parameters[@(definitions[index].identifier)] =
            @(parameterValues[index].load(std::memory_order_relaxed));
    state[@"parameters"] = parameters;
    state[@"patch"] = [bridge patchSnapshot];
    return state;
}
- (NSDictionary<NSString*, id>*)fullStateForDocument { return self.fullState; }
- (void)setFullStateForDocument:(NSDictionary<NSString*, id>*)state { self.fullState = state; }
- (void)setFullState:(NSDictionary<NSString*, id>*)state
{
    if (![state isKindOfClass:NSDictionary.class]) return;
    NSDictionary* savedParameters = [state[@"parameters"] isKindOfClass:NSDictionary.class]
        ? state[@"parameters"] : state; // Version 1 compatibility.

    // Restore the factory slot first because loading it replaces the complete patch.
    NSNumber* factoryVoice = savedParameters[@"factoryVoice"];
    if ([factoryVoice isKindOfClass:NSNumber.class]) {
        AUParameter* parameter = [self.parameterTree parameterWithAddress:parameterIndex("factoryVoice")];
        parameter.value = std::clamp<AUValue>(factoryVoice.floatValue,
                                               parameter.minValue, parameter.maxValue);
    }

    for (NSUInteger index = 0; index < std::size(definitions); ++index) {
        if (std::string_view(definitions[index].identifier) == "factoryVoice") continue;
        NSNumber* number = savedParameters[@(definitions[index].identifier)];
        if (![number isKindOfClass:NSNumber.class]) continue;
        AUParameter* parameter = [self.parameterTree parameterWithAddress:index];
        parameter.value = std::clamp<AUValue>(number.floatValue,
                                               parameter.minValue, parameter.maxValue);
    }

    NSDictionary* patch = state[@"patch"];
    if ([patch isKindOfClass:NSDictionary.class])
        [bridge applyPatchSnapshot:patch];
}

@end
