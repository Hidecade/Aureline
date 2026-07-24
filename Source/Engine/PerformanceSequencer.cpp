#include "Engine/PerformanceSequencer.h"

#include <algorithm>
#include <cmath>

namespace aureline
{
void PerformanceSequencer::prepare(double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    resetRequested = true;
}

void PerformanceSequencer::setSettings(const PerformanceSequencerSettings& value)
{
    const auto modeChanged = settings.arpeggiatorEnabled != value.arpeggiatorEnabled
        || settings.chordEnabled != value.chordEnabled || settings.holdEnabled != value.holdEnabled
        || settings.scaleRoot != value.scaleRoot;
    settings = value;
    settings.tempoBpm = std::clamp(settings.tempoBpm, 40.0, 240.0);
    settings.rate = std::clamp(settings.rate, 0, 2);
    settings.direction = std::clamp(settings.direction, 0, 3);
    settings.gate = std::clamp(settings.gate, 0.1, 0.95);
    settings.scaleRoot = std::clamp(settings.scaleRoot, 0, 11);
    resetRequested = resetRequested || modeChanged;
}

int PerformanceSequencer::chordNote(int root, int index) const
{
    if (!settings.chordEnabled) return std::clamp(root, 0, 127);
    constexpr std::array<int, 7> scale { 0, 2, 4, 5, 7, 9, 11 };
    const int relative = (root - settings.scaleRoot + 120) % 12;
    int degree = 0, nearest = 12;
    for (int candidate = 0; candidate < static_cast<int>(scale.size()); ++candidate) {
        const int distance = std::abs(scale[static_cast<std::size_t>(candidate)] - relative);
        if (distance < nearest) { nearest = distance; degree = candidate; }
    }
    const int chordDegree = degree + index * 2;
    return std::clamp(root - relative + scale[static_cast<std::size_t>(chordDegree % 7)] + chordDegree / 7 * 12, 0, 127);
}

void PerformanceSequencer::addRoot(int root)
{
    const int count = settings.chordEnabled ? 3 : 1;
    for (int index = 0; index < count; ++index) {
        const int note = chordNote(root, index);
        auto& rootCount = heldNotes[static_cast<std::size_t>(note)];
        if (rootCount < UINT8_MAX) ++rootCount;
        if (note <= 115) {
            auto& octaveCount = heldNotes[static_cast<std::size_t>(note + 12)];
            if (octaveCount < UINT8_MAX) ++octaveCount;
        }
    }
}

void PerformanceSequencer::removeRoot(int root)
{
    const int count = settings.chordEnabled ? 3 : 1;
    for (int index = 0; index < count; ++index) {
        const int note = chordNote(root, index);
        auto& rootCount = heldNotes[static_cast<std::size_t>(note)];
        if (rootCount > 0) --rootCount;
        if (note <= 115) {
            auto& octaveCount = heldNotes[static_cast<std::size_t>(note + 12)];
            if (octaveCount > 0) --octaveCount;
        }
    }
}

void PerformanceSequencer::noteOn(AnalogEngine& engine, int note, int noteVelocity)
{
    if (resetRequested) resetState(engine);
    note = std::clamp(note, 0, 127); velocity = std::clamp(noteVelocity, 1, 127);
    if (!settings.arpeggiatorEnabled) {
        const int count = settings.chordEnabled ? 3 : 1;
        for (int index = 0; index < count; ++index) engine.noteOn(chordNote(note, index), velocity);
        return;
    }
    if (settings.holdEnabled && std::none_of(inputNotes.begin(), inputNotes.end(), [](bool value) { return value; }))
        heldNotes.fill(false);
    inputNotes[static_cast<std::size_t>(note)] = true;
    addRoot(note);
}

void PerformanceSequencer::noteOff(AnalogEngine& engine, int note)
{
    if (resetRequested) resetState(engine);
    note = std::clamp(note, 0, 127);
    if (!settings.arpeggiatorEnabled) {
        const int count = settings.chordEnabled ? 3 : 1;
        for (int index = 0; index < count; ++index) engine.noteOff(chordNote(note, index));
        return;
    }
    inputNotes[static_cast<std::size_t>(note)] = false;
    if (!settings.holdEnabled) removeRoot(note);
}

void PerformanceSequencer::resetState(AnalogEngine& engine)
{
    engine.panic(); heldNotes.fill(false); currentNote = -1; lastNote = -1;
    samplesUntilStep = 0; gateSamplesRemaining = 0; movingUp = true;
    if (settings.arpeggiatorEnabled)
        for (int note = 0; note < 128; ++note) if (inputNotes[static_cast<std::size_t>(note)]) addRoot(note);
    resetRequested = false;
}

void PerformanceSequencer::panic(AnalogEngine& engine)
{
    engine.panic(); heldNotes.fill(false); inputNotes.fill(false); currentNote = -1; lastNote = -1;
    samplesUntilStep = 0; gateSamplesRemaining = 0;
}

int PerformanceSequencer::selectNextNote()
{
    std::array<int, 128> active {};
    int count = 0;
    for (int note = 0; note < 128; ++note) if (heldNotes[static_cast<std::size_t>(note)]) active[static_cast<std::size_t>(count++)] = note;
    if (count == 0) return -1;
    int index = -1;
    for (int candidate = 0; candidate < count; ++candidate) if (active[static_cast<std::size_t>(candidate)] == lastNote) index = candidate;
    if (settings.direction == 0) index = (index + 1 + count) % count;
    else if (settings.direction == 1) index = index < 0 ? count - 1 : (index - 1 + count) % count;
    else if (settings.direction == 2) {
        if (index < 0) { index = 0; movingUp = true; }
        else { if (index == count - 1) movingUp = false; else if (index == 0) movingUp = true; index = std::clamp(index + (movingUp ? 1 : -1), 0, count - 1); }
    } else { randomState = randomState * 1664525U + 1013904223U; index = static_cast<int>(randomState % static_cast<std::uint32_t>(count)); }
    return active[static_cast<std::size_t>(index)];
}

StereoSample PerformanceSequencer::renderStereoSample(AnalogEngine& engine)
{
    if (resetRequested) resetState(engine);
    if (!settings.arpeggiatorEnabled) return engine.renderStereoSample();
    constexpr std::array<double, 3> rateFactors { 30.0, 15.0, 7.5 };
    const int stepSamples = std::max(1, static_cast<int>(std::lround(sampleRate * rateFactors[static_cast<std::size_t>(settings.rate)] / settings.tempoBpm)));
    const int gateSamples = std::clamp(static_cast<int>(std::lround(stepSamples * settings.gate)), 1, stepSamples);
    if (currentNote >= 0 && !heldNotes[static_cast<std::size_t>(currentNote)]) { engine.noteOff(currentNote); currentNote = -1; samplesUntilStep = 0; }
    if (currentNote >= 0 && gateSamplesRemaining <= 0) { engine.noteOff(currentNote); currentNote = -1; }
    if (samplesUntilStep <= 0) {
        if (currentNote >= 0) engine.noteOff(currentNote);
        currentNote = selectNextNote(); lastNote = currentNote;
        if (currentNote >= 0) engine.noteOn(currentNote, velocity);
        samplesUntilStep = stepSamples; gateSamplesRemaining = gateSamples;
    }
    const auto output = engine.renderStereoSample();
    --samplesUntilStep; --gateSamplesRemaining;
    return output;
}
} // namespace aureline
