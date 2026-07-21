#include "Engine/AnalogEngine.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void AnalogEngine::prepare(double sampleRate)
{
    lfo.prepare(sampleRate);
    masterGainSmoother.prepare(sampleRate);
    for (int index = 0; index < kVoiceCount; ++index)
        voices[static_cast<std::size_t>(index)].prepare(sampleRate, index);
    voiceAge = 0;
    keyDownNotes.fill(false);
    sustainedNotes.fill(false);
    lastPlayedNote = -1;
}

void AnalogEngine::setPatch(const AnalogPatch& newPatch)
{
    patch = normalizePatch(newPatch);
}

AnalogVoice& AnalogEngine::selectVoice()
{
    const auto inactive = std::find_if(voices.begin(), voices.end(),
                                       [](const auto& voice) { return !voice.isActive(); });
    if (inactive != voices.end())
        return *inactive;

    const auto releasing = std::min_element(voices.begin(), voices.end(), [](const auto& a, const auto& b) {
        if (a.isReleasing() != b.isReleasing())
            return a.isReleasing();
        if (a.isReleasing())
            return a.level() < b.level();
        return a.age() < b.age();
    });
    return *releasing;
}

void AnalogEngine::noteOn(int midiNote, int velocity)
{
    const auto note = std::clamp(midiNote, 0, 127);
    if (patch.lfoRetrigger)
        lfo.reset();
    const bool anotherKeyHeld = std::any_of(keyDownNotes.begin(), keyDownNotes.end(),
                                            [] (bool down) { return down; });
    keyDownNotes[static_cast<std::size_t>(note)] = true;
    sustainedNotes[static_cast<std::size_t>(note)] = false;
    const auto startNote = lastPlayedNote >= 0 ? static_cast<double>(lastPlayedNote)
                                               : static_cast<double>(note);
    switch (patch.voiceMode)
    {
        case VoiceMode::poly:
            selectVoice().start(note, velocity, ++voiceAge,
                                static_cast<double>(note), 0.0);
            break;
        case VoiceMode::mono:
            if (voices[0].isActive() && !voices[0].isReleasing() && anotherKeyHeld)
                voices[0].retarget(note, patch.glideSeconds);
            else
            {
                const auto glideStart = patch.glideLegatoOnly
                                            ? static_cast<double>(note) : startNote;
                const auto glideTime = patch.glideLegatoOnly ? 0.0 : patch.glideSeconds;
                voices[0].start(note, velocity, ++voiceAge,
                                glideStart, glideTime);
            }
            break;
        case VoiceMode::unison:
        {
            const bool useGlide = !patch.glideLegatoOnly || anotherKeyHeld;
            const auto glideStart = useGlide ? startNote : static_cast<double>(note);
            const auto glideTime = useGlide ? patch.glideSeconds : 0.0;
            for (int index = 0; index < kVoiceCount; ++index)
            {
                const auto position = (static_cast<double>(index) / (kVoiceCount - 1)) - 0.5;
                const auto initialPhase = static_cast<double>(index) / kVoiceCount;
                voices[static_cast<std::size_t>(index)].start(
                    note, velocity, ++voiceAge, glideStart, glideTime,
                    position * patch.unisonDetuneCents, initialPhase);
            }
            break;
        }
    }
    lastPlayedNote = note;
}

void AnalogEngine::noteOff(int midiNote)
{
    if (midiNote < 0 || midiNote > 127)
        return;
    keyDownNotes[static_cast<std::size_t>(midiNote)] = false;
    if (sustainPedalDown)
    {
        sustainedNotes[static_cast<std::size_t>(midiNote)] = true;
        return;
    }
    for (auto& voice : voices)
        if (voice.isActive() && voice.note() == midiNote && !voice.isReleasing())
            voice.release();
}

void AnalogEngine::setSustainPedal(bool down)
{
    if (sustainPedalDown == down)
        return;
    sustainPedalDown = down;
    if (down)
        return;

    for (int note = 0; note < 128; ++note)
    {
        if (!sustainedNotes[static_cast<std::size_t>(note)]
            || keyDownNotes[static_cast<std::size_t>(note)])
            continue;
        sustainedNotes[static_cast<std::size_t>(note)] = false;
        for (auto& voice : voices)
            if (voice.isActive() && voice.note() == note && !voice.isReleasing())
                voice.release();
    }
}

void AnalogEngine::setPitchBend(double normalized)
{
    pitchBend = std::clamp(normalized, -1.0, 1.0);
}

void AnalogEngine::setPitchBendRange(double semitones)
{
    pitchBendRange = std::clamp(semitones, 0.0, 24.0);
}

void AnalogEngine::setModWheel(double normalized)
{
    modWheel = std::clamp(normalized, 0.0, 1.0);
}

void AnalogEngine::panic()
{
    for (auto& voice : voices)
        voice.reset();
    keyDownNotes.fill(false);
    sustainedNotes.fill(false);
    sustainPedalDown = false;
    lastPlayedNote = -1;
}

StereoSample AnalogEngine::renderStereoSample()
{
    constexpr double halfPi = 1.57079632679489661923;
    const auto lfoValue = lfo.render(patch.lfoRateHz, patch.lfoWaveformMask);
    StereoSample output;
    for (auto& voice : voices)
    {
        const auto sample = voice.render(patch, pitchBend * pitchBendRange, lfoValue, modWheel);
        const auto pan = std::clamp(voice.panPosition() * patch.stereoSpread
                                    + voice.vintagePanPosition() * patch.vintageAmount * 0.2,
                                    -1.0, 1.0);
        const auto angle = (pan + 1.0) * 0.5 * halfPi;
        output.left += sample * std::cos(angle);
        output.right += sample * std::sin(angle);
    }
    constexpr double unisonGain = 0.4;
    const auto modeGain = patch.voiceMode == VoiceMode::unison ? unisonGain : 1.0;
    const auto masterGain = masterGainSmoother.process(patch.masterGain) * modeGain;
    output.left *= masterGain;
    output.right *= masterGain;
    output.left = std::isfinite(output.left) ? std::clamp(output.left, -1.0, 1.0) : 0.0;
    output.right = std::isfinite(output.right) ? std::clamp(output.right, -1.0, 1.0) : 0.0;
    return output;
}

double AnalogEngine::renderSample()
{
    const auto stereo = renderStereoSample();
    return (stereo.left + stereo.right) * 0.5;
}

void AnalogEngine::renderBlock(float* left, float* right, int numSamples)
{
    if (left == nullptr || right == nullptr || numSamples <= 0)
        return;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto value = renderStereoSample();
        left[sample] = static_cast<float>(value.left);
        right[sample] = static_cast<float>(value.right);
    }
}

int AnalogEngine::activeVoiceCount() const
{
    return static_cast<int>(std::count_if(voices.begin(), voices.end(),
                                          [](const auto& voice) { return voice.isActive(); }));
}
} // namespace aureline
