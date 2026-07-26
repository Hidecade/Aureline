#include "Engine/FactoryPresets.h"

#include <algorithm>

namespace aureline
{
namespace
{
struct Definition
{
    const char* name;
    int waveA, waveB;
    double octaveA, octaveB;
    double levelA, levelB, fine, noise;
    double cutoff, resonance, filterEnvelope;
    double attack, decay, sustain, release;
    double lfoRate, lfoAmount, spread, vintage;
    int voiceMode;
    bool sync;
    double filterAttack = -1.0;
    double filterDecay = -1.0;
    double filterSustain = -1.0;
    double filterRelease = -1.0;
    double polyFilterEnvelope = 0.0;
    double polyOscillatorB = 0.0;
    bool polyToPitch = false;
    bool polyToPulseWidthA = false;
    bool polyToFilter = false;
    bool oscillatorBLowFrequency = false;
};

constexpr std::array<Definition, kFactoryPresetCount> definitions {{
    { "01 WARM BRASS",    1, 1,  0,  0, .72, .62,  5, .00, 4200, .18, .62, .04, .42, .78, .34, 5.2, .04, .18, .42, 0, false },
    { "02 POLY BRASS",    1, 5,  0,  0, .74, .62, 12, .00, 1650, .29, .92, .01, .55, .62, 1.10, 5.8, .03, .38, .66, 0, false, .005, .78, .16, 1.15 },
    { "03 SOFT STRINGS",  1, 1,  0,  0, .58, .56, 11, .00, 5200, .12, .32, .65, 1.20, .82, 1.80, 4.7, .12, .62, .68, 0, false },
    { "04 PWM STRINGS",   4, 4,  0,  0, .60, .58,  7, .00, 6100, .16, .28, .48, 1.00, .86, 1.55, 3.8, .22, .70, .62, 0, false },
    { "05 PROPHET PAD",   1, 2, -1,  0, .52, .48,  9, .00, 3900, .22, .45, 1.10, 1.65, .74, 2.30, 2.6, .08, .78, .76, 0, false },
    { "06 SLOW CHOIR",    2, 4,  0,  0, .48, .52,  4, .02, 2800, .28, .38, 1.70, 2.20, .80, 2.80, 1.4, .10, .72, .80, 0, false },
    { "07 ANALOG SWEEP",  1, 4, -1,  0, .55, .55, 13, .00, 1800, .40, .88, .70, 2.80, .55, 2.10, 0.7, .16, .68, .72, 0, false },
    { "08 DREAM PAD",     2, 1,  0,  1, .54, .30, -7, .01, 7200, .10, .22, 1.35, 1.80, .88, 3.20, 3.1, .07, .86, .58, 0, false },
    { "09 SOLID BASS",    1, 4, -1, -1, .78, .52,  3, .00, 1450, .22, .58, .01, .20, .72, .18, 4.5, .00, .05, .34, 1, false },
    { "10 SUB BASS",      2, 4, -1, -2, .64, .72,  0, .00,  780, .08, .22, .02, .28, .84, .24, 3.0, .00, .00, .22, 1, false },
    { "11 RESO BASS",     1, 1, -1, -1, .72, .44,  6, .00,  920, .68, .74, .01, .32, .58, .20, 5.4, .00, .04, .48, 1, false },
    { "12 SYNC BASS",     1, 4, -1,  0, .76, .38, 19, .00, 1700, .34, .64, .01, .18, .68, .16, 6.2, .03, .08, .46, 1, true  },
    { "13 ANALOG PIANO",  1, 4,  0,  1, .64, .34,  2, .00, 7200, .12, .46, .01, .48, .28, .42, 5.2, .00, .32, .38, 0, false },
    { "14 CLAV PULSE",    4, 4,  0,  1, .66, .36,  5, .00, 4300, .24, .72, .01, .16, .18, .12, 6.0, .00, .16, .42, 0, false },
    { "15 WOOD PLUCK",    2, 4,  0,  1, .62, .30, -3, .00, 3600, .18, .68, .01, .18, .08, .16, 4.2, .00, .22, .34, 0, false },
    { "16 DRAWBAR ORGAN", 4, 4,  0,  1, .62, .44,  0, .00, 8200, .06, .10, .01, .08, .96, .12, 6.4, .04, .40, .30, 0, false },
    { "17 CLASSIC LEAD",  1, 1,  0,  0, .72, .58,  7, .00, 5600, .24, .38, .01, .26, .76, .30, 5.1, .08, .16, .48, 1, false },
    { "18 SYNC LEAD",     1, 4,  0,  1, .72, .44, 28, .00, 4800, .30, .54, .01, .22, .70, .24, 6.0, .06, .14, .56, 1, true  },
    { "19 PWM LEAD",      4, 4,  0,  0, .68, .54,  5, .00, 6400, .18, .30, .02, .28, .80, .34, 4.0, .18, .18, .50, 1, false },
    { "20 SOFT SOLO",     2, 1,  0,  0, .64, .38, -4, .00, 3900, .12, .24, .08, .38, .74, .52, 5.6, .10, .12, .44, 1, false },
    { "21 POLY SWELL",    1, 2,  0,  0, .62, .42,  7, .00, 3300, .22, .46, .35, 1.10, .74, 1.50, 3.2, .06, .52, .62, 0, false,
      .08, 1.20, .42, 1.50, .08, .00, true, false, true, false },
    { "22 CROSSMOD LEAD", 1, 2,  0,  2, .68, .24, 11, .00, 5200, .18, .32, .01, .22, .76, .28, 5.1, .04, .18, .48, 1, false,
      .01, .28, .52, .24, .00, .34, true, false, false, false },
    { "23 ENV PWM PAD",   4, 1,  0,  0, .66, .32,  4, .00, 4700, .14, .34, .65, 1.35, .82, 2.10, 2.4, .05, .68, .70, 0, false,
      .45, 1.55, .62, 2.20, .58, .00, false, true, false, false },
    { "24 OSC PWM STR",   4, 2,  0,  1, .62, .22,  9, .00, 6100, .16, .26, .42, 1.10, .86, 1.70, 3.7, .08, .72, .62, 0, false,
      .20, 1.10, .72, 1.80, .00, .42, false, true, false, false },
    { "25 FILTER TALK",   1, 2, -1,  1, .58, .26, -7, .00, 1450, .62, .28, .02, .38, .68, .30, 4.2, .03, .26, .56, 1, false,
      .01, .44, .36, .28, .18, .30, false, false, true, false },

    { "26 SOFT DIGITAL",  8, 0,  0,  0, .78, .00,  0, .00, 6800, .10, .24, .04, .36, .76, .48, 4.6, .04, .34, .42, 0, false },
    { "27 HOLLOW KEYS",   8, 1,  0,  1, .68, .25, -7, .00, 5200, .18, .42, .01, .38, .24, .42, 5.1, .02, .28, .38, 0, false },
    { "28 BRIGHT FIFTH",  8, 8,  0,  1, .58, .34,  0, .00, 8400, .14, .36, .01, .30, .62, .38, 5.8, .04, .42, .44, 1, false },
    { "29 REED MOTION",   8, 4,  0,  0, .64, .28,  8, .00, 3600, .26, .58, .05, .46, .70, .55, 3.7, .07, .38, .52, 0, false },
    { "30 ORGAN WAVES",   8, 8, -1,  0, .66, .46,  0, .00, 7200, .08, .08, .01, .10, .94, .16, 6.2, .04, .48, .30, 0, false },
    { "31 BELL MATRIX",   8, 8,  0,  2, .60, .30, 11, .00, 9800, .30, .28, .01, .68, .05, 1.25, 7.0, .03, .46, .36, 0, false },
    { "32 METAL VOX",     8, 8,  0,  1, .62, .38,  7, .00, 6400, .38, .52, .02, .54, .54, .76, 4.4, .04, .55, .50, 1, false }
}};

struct WavePresetConfig
{
    int indexA;
    int indexB;
    WaveMemoryCharacter characterA;
    WaveMemoryCharacter characterB;
};

constexpr std::array<WavePresetConfig, 10> wavePresetConfigs {{
    { 0, 0, WaveMemoryCharacter::smooth,  WaveMemoryCharacter::smooth },
    { 1, 1, WaveMemoryCharacter::smooth,  WaveMemoryCharacter::fiveBit },
    { 2, 4, WaveMemoryCharacter::fiveBit, WaveMemoryCharacter::fiveBit },
    { 3, 3, WaveMemoryCharacter::smooth,  WaveMemoryCharacter::fourBit },
    { 4, 4, WaveMemoryCharacter::smooth,  WaveMemoryCharacter::smooth },
    { 5, 6, WaveMemoryCharacter::fiveBit, WaveMemoryCharacter::fourBit },
    { 6, 7, WaveMemoryCharacter::fourBit, WaveMemoryCharacter::smooth },
    { 7, 8, WaveMemoryCharacter::smooth,  WaveMemoryCharacter::smooth },
    { 10, 11, WaveMemoryCharacter::fourBit, WaveMemoryCharacter::fiveBit },
    { 13, 15, WaveMemoryCharacter::fiveBit, WaveMemoryCharacter::fiveBit }
}};

FactoryPreset makePreset(const Definition& source)
{
    AnalogPatch patch;
    patch.oscillatorA.sawEnabled = (source.waveA & 1) != 0;
    patch.oscillatorA.triangleEnabled = (source.waveA & 2) != 0;
    patch.oscillatorA.pulseEnabled = (source.waveA & 4) != 0;
    patch.oscillatorB.sawEnabled = (source.waveB & 1) != 0;
    patch.oscillatorB.triangleEnabled = (source.waveB & 2) != 0;
    patch.oscillatorB.pulseEnabled = (source.waveB & 4) != 0;
    patch.oscillatorA.octave = source.octaveA;
    patch.oscillatorB.octave = source.octaveB;
    patch.oscillatorA.level = source.levelA;
    patch.oscillatorB.level = source.levelB;
    patch.oscillatorB.fineCents = source.fine;
    patch.oscillatorB.lowFrequencyMode = source.oscillatorBLowFrequency;
    patch.noiseLevel = source.noise;
    patch.filterCutoffHz = source.cutoff;
    patch.filterResonance = source.resonance;
    patch.filterEnvelopeAmount = source.filterEnvelope;
    patch.filterKeyboardTracking = 0.35;
    patch.amplifierEnvelope = { source.attack, source.decay, source.sustain, source.release };
    patch.filterEnvelope.attackSeconds = source.filterAttack >= 0.0 ? source.filterAttack : std::min(source.attack, 1.5);
    patch.filterEnvelope.decaySeconds = source.filterDecay >= 0.0 ? source.filterDecay : std::max(0.08, source.decay * 0.8);
    patch.filterEnvelope.sustainLevel = source.filterSustain >= 0.0 ? source.filterSustain : std::clamp(source.sustain * 0.72, 0.0, 1.0);
    patch.filterEnvelope.releaseSeconds = source.filterRelease >= 0.0 ? source.filterRelease : source.release;
    patch.lfoRateHz = source.lfoRate;
    // INITIAL AMOUNT uses a squared response and reaches +/-12 semitones at
    // maximum. Factory voices should stay in a musical vibrato range; larger
    // sweeps remain available from the front-panel control.
    patch.lfoInitialAmount = std::clamp(source.lfoAmount, 0.0, 0.12);
    if (patch.lfoInitialAmount > 0.001) {
        patch.lfoPitchDepthASemitones = 12.0;
        patch.lfoPitchDepthBSemitones = 12.0;
    }
    patch.stereoSpread = source.spread;
    patch.vintageAmount = source.vintage;
    patch.voiceMode = static_cast<VoiceMode>(source.voiceMode);
    patch.oscillatorSync = source.sync;
    patch.polyModFilterEnvelopeToPitch =
        source.polyToPitch ? source.polyFilterEnvelope : 0.0;
    patch.polyModOscillatorBToPitch =
        source.polyToPitch ? source.polyOscillatorB : 0.0;
    patch.polyModFilterEnvelopeToPulseWidthA =
        source.polyToPulseWidthA ? source.polyFilterEnvelope : 0.0;
    patch.polyModOscillatorBToPulseWidthA =
        source.polyToPulseWidthA ? source.polyOscillatorB : 0.0;
    patch.polyModFilterEnvelopeToFilter =
        source.polyToFilter ? source.polyFilterEnvelope : 0.0;
    patch.polyModOscillatorBToFilter =
        source.polyToFilter ? source.polyOscillatorB : 0.0;
    return { source.name, normalizePatch(patch) };
}

void configureWaveMemoryPreset(FactoryPreset& preset, const WavePresetConfig& config)
{
    auto configure = [](OscillatorParams& oscillator, int index,
                        WaveMemoryCharacter character)
    {
        oscillator.waveMemoryEnabled = true;
        oscillator.waveMemoryIndex = index;
        oscillator.waveMemoryCharacter = character;
        oscillator.waveMemoryData =
            waveMemoryFactoryBank()[static_cast<std::size_t>(index)];
    };

    configure(preset.patch.oscillatorA, config.indexA, config.characterA);
    if (!preset.patch.oscillatorB.sawEnabled
        && !preset.patch.oscillatorB.triangleEnabled
        && !preset.patch.oscillatorB.pulseEnabled)
        configure(preset.patch.oscillatorB, config.indexB, config.characterB);
    preset.patch = normalizePatch(preset.patch);
}

void configureCategoryDetails(FactoryPreset& preset, std::size_t index)
{
    auto& patch = preset.patch;

    // Slots 36-39 demonstrate the four classic pulse duty ratios. Slot 40 is
    // the triangle bass counterpart.
    constexpr std::array<double, 4> pulseWidths { 0.125, 0.25, 0.5, 0.75 };
    if (index >= 35 && index <= 38)
    {
        patch.oscillatorA.pulseWidth = pulseWidths[index - 35];
        patch.oscillatorB.pulseWidth = pulseWidths[index - 35];
        patch.filterKeyboardTracking = 0.15;
        patch.filterEnvelope = { 0.002, 0.08, 0.0, 0.04 };
        patch.lfoInitialAmount = 0.0;
        patch.lfoPitchDepthASemitones = 0.0;
        patch.lfoPitchDepthBSemitones = 0.0;
        patch.stereoSpread = 0.0;
        patch.vintageAmount = 0.08;
    }
    else if (index == 39)
    {
        patch.oscillatorA.waveMemoryEnabled = true;
        patch.oscillatorA.waveMemoryCharacter = WaveMemoryCharacter::fourBit;
        patch.oscillatorA.waveMemoryData = {{
            31, 29, 27, 25, 23, 21, 19, 17,
            15, 13, 11, 9, 7, 5, 3, 1,
            1, 3, 5, 7, 9, 11, 13, 15,
            17, 19, 21, 23, 25, 27, 29, 31
        }};
        patch.filterKeyboardTracking = 0.15;
        patch.lfoInitialAmount = 0.0;
        patch.stereoSpread = 0.0;
        patch.vintageAmount = 0.06;
    }

    // The effects bank intentionally uses modulation depths that are much
    // stronger than the musical factory voices.
    switch (index)
    {
        case 40: // UFO DRONE
            patch.lfoWaveformMask = 2;
            patch.lfoInitialAmount = 0.52;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            patch.lfoPulseWidthDepthA = 0.22;
            patch.lfoPulseWidthDepthB = 0.22;
            patch.oscillatorA.pulseWidth = 0.35;
            patch.oscillatorB.pulseWidth = 0.65;
            break;
        case 41: // LASER FALL
            patch.lfoWaveformMask = 8;
            patch.lfoInitialAmount = 0.78;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            break;
        case 42: // LASER RISE
            patch.lfoWaveformMask = 1;
            patch.lfoInitialAmount = 0.78;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            break;
        case 43: // ALIEN SIREN
            patch.lfoWaveformMask = 2;
            patch.lfoInitialAmount = 0.58;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            patch.lfoFilterDepthOctaves = 1.4;
            break;
        case 44: // INVADER STEP
            patch.lfoWaveformMask = 4;
            patch.lfoInitialAmount = 0.42;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            break;
        case 47: // SPACE WIND
            patch.lfoWaveformMask = 16;
            patch.lfoInitialAmount = 0.48;
            patch.lfoFilterDepthOctaves = 2.0;
            break;
        case 48: // ROBOT BLEEPS
            patch.lfoWaveformMask = 16;
            patch.lfoInitialAmount = 0.64;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            break;
        case 49: // SPACE ALARM
            patch.lfoWaveformMask = 4;
            patch.lfoInitialAmount = 0.50;
            patch.lfoPitchDepthASemitones = 12.0;
            patch.lfoPitchDepthBSemitones = 12.0;
            patch.lfoFilterDepthOctaves = 1.2;
            break;
        default:
            break;
    }

    patch = normalizePatch(patch);
}
} // namespace

const std::array<FactoryPreset, kFactoryPresetCount>& factoryPresets()
{
    static const auto presets = [] {
        std::array<FactoryPreset, kFactoryPresetCount> result {};
        for (std::size_t index = 0; index < definitions.size(); ++index)
        {
            result[index] = makePreset(definitions[index]);
            if (index >= 25 && index < 35)
                configureWaveMemoryPreset(result[index],
                    wavePresetConfigs[index - 25]);
            configureCategoryDetails(result[index], index);
        }
        return result;
    }();
    return presets;
}

const FactoryPreset& factoryPreset(std::size_t index)
{
    return factoryPresets()[std::min(index, kFactoryPresetCount - 1)];
}
} // namespace aureline
