#pragma once

#include "Engine/AnalogEngine.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

class AurelineMainComponent final : public juce::AudioAppComponent,
                                    private juce::MidiInputCallback,
                                    private juce::Slider::Listener,
                                    private juce::ComboBox::Listener,
                                    private juce::Button::Listener,
                                    private juce::Timer
{
public:
    explicit AurelineMainComponent(bool useStandaloneAudio = true);
    ~AurelineMainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;
    void releaseResources() override;
    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void playNote(int note, int velocity);
    void releaseNote(int note);
    bool isNoteHeld(int note) const;
    void queueMidiMessage(const juce::MidiMessage& message);
    juce::ValueTree capturePluginState() const;
    void restorePluginState(const juce::ValueTree& state);

private:
    class AurelineLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                              float, float, juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float,
                              float, float, juce::Slider::SliderStyle,
                              juce::Slider&) override;
        juce::Font getLabelFont(juce::Label&) override;
    };

    class Keyboard final : public juce::Component
    {
    public:
        explicit Keyboard(AurelineMainComponent& owner);
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseExit(const juce::MouseEvent&) override;

    private:
        int noteAt(juce::Point<int>) const;
        void setHeldNote(int note);
        AurelineMainComponent& owner;
        int heldNote = -1;
    };

    class PitchWheelSlider final : public juce::Slider
    {
    public:
        void mouseUp(const juce::MouseEvent& event) override
        {
            juce::Slider::mouseUp(event);
            setValue(0.0, juce::sendNotificationSync);
        }
    };

    class WaveformButton final : public juce::Button
    {
    public:
        explicit WaveformButton(aureline::Waveform waveform);
        explicit WaveformButton(aureline::LfoWaveform waveform);
        aureline::Waveform waveform() const { return buttonWaveform; }

    private:
        void paintButton(juce::Graphics&, bool highlighted, bool down) override;
        aureline::Waveform buttonWaveform { aureline::Waveform::saw };
        aureline::LfoWaveform buttonLfoWaveform { aureline::LfoWaveform::sawUp };
        bool isLfoButton = false;
    };

    class RockerButton final : public juce::Button
    {
    public:
        explicit RockerButton(const juce::String& label);

    private:
        void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    };

    struct Parameters
    {
        std::atomic<float> oscillatorALevel { 0.5f };
        std::atomic<float> oscillatorBLevel { 0.5f };
        std::atomic<float> oscillatorBFine { 7.0f };
        std::atomic<float> pulseWidthA { 0.5f };
        std::atomic<float> pulseWidthB { 0.5f };
        std::atomic<float> noiseLevel { 0.0f };
        std::atomic<float> cutoff { 8000.0f };
        std::atomic<float> resonance { 0.1f };
        std::atomic<float> filterEnvelope { 0.25f };
        std::atomic<float> filterKeyboardTracking { 0.0f };
        std::atomic<float> filterAttack { 0.01f };
        std::atomic<float> filterDecay { 0.3f };
        std::atomic<float> filterSustain { 0.4f };
        std::atomic<float> filterRelease { 0.5f };
        std::atomic<float> attack { 0.01f };
        std::atomic<float> decay { 0.25f };
        std::atomic<float> sustain { 0.75f };
        std::atomic<float> release { 0.4f };
        std::atomic<float> lfoRate { 5.0f };
        std::atomic<float> lfoAmount { 0.0f };
        std::atomic<float> lfoDelay { 0.0f };
        std::atomic<float> lfoFade { 0.0f };
        std::atomic<bool> lfoRetrigger { false };
        std::atomic<float> polyModFilterEnvelope { 0.0f };
        std::atomic<float> polyModOscillatorB { 0.0f };
        std::atomic<float> spread { 0.0f };
        std::atomic<float> vintage { 0.0f };
        std::atomic<float> tempoBpm { 120.0f };
        std::atomic<int> scaleRoot { 0 };
        std::atomic<float> master { 0.8f };
        std::atomic<float> transpose { 0.0f };
        std::atomic<float> pitchBendRange { 2.0f };
        std::atomic<float> glide { 0.0f };
        std::atomic<bool> glideLegatoOnly { false };
        std::atomic<float> masterTune { 0.0f };
        std::atomic<float> unisonDetune { 14.0f };
        std::atomic<float> filterVelocity { 0.0f };
        std::atomic<float> pitchBend { 0.0f };
        std::atomic<float> modWheel { 0.0f };
        std::atomic<int> voiceMode { 0 };
        std::atomic<int> waveformMaskA { 1 };
        std::atomic<int> waveformMaskB { 1 };
        std::atomic<int> lfoWaveformMask { 2 };
        std::atomic<bool> oscillatorSync { false };
        std::atomic<float> oscillatorAOctave { 0.0f };
        std::atomic<float> oscillatorBOctave { 0.0f };
        std::atomic<bool> oscillatorBLowFrequency { false };
        std::atomic<bool> oscillatorBKeyboardTracking { true };
        std::atomic<bool> polyModToFrequencyA { false };
        std::atomic<bool> polyModToPulseWidthA { false };
        std::atomic<bool> polyModToFilter { false };
        std::atomic<bool> arpEnabled { false };
        std::atomic<bool> chordEnabled { false };
        std::atomic<int> arpRate { 1 };
        std::atomic<int> arpDirection { 0 };
        std::atomic<float> arpGate { 0.75f };
        std::atomic<bool> arpHold { false };
        std::array<std::atomic<bool>, 5> lfoDestinations {};
    } parameters;

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void sliderValueChanged(juce::Slider*) override;
    void comboBoxChanged(juce::ComboBox*) override;
    void buttonClicked(juce::Button*) override;
    void configureKnob(juce::Slider&, juce::Label&, const juce::String&,
                       double min, double max, double initial, double skew = 1.0);
    void applyParameters();
    void resetToInitialVoice();
    void loadFactoryVoice(std::size_t index);
    void syncPcKeyboardNotes();
    void syncControlsFromParameters();
    void timerCallback() override;

    AurelineLookAndFeel lookAndFeel;
    juce::Image woodBackground;
    aureline::AnalogEngine engine;
    juce::MidiMessageCollector midiCollector;
    std::vector<juce::String> connectedMidiInputIds;
    bool ownsStandaloneAudio = true;
    std::array<std::atomic<bool>, 128> heldNotes {};
    std::array<bool, 128> pcKeyboardHeldNotes {};
    static constexpr std::size_t scopeSize = 2048;
    std::array<std::atomic<float>, scopeSize> scopeSamples {};
    std::atomic<std::size_t> scopeWriteIndex { 0 };
    std::array<bool, 128> arpHeldNotes {};
    std::array<bool, 128> arpInputHeldNotes {};
    std::atomic<bool> sequencerResetRequested { false };
    double currentSampleRate = 44100.0;
    int arpCurrentNote = -1;
    int arpLastNote = -1;
    int arpSamplesUntilStep = 0;
    int arpGateSamplesRemaining = 0;
    int arpVelocity = 100;
    bool arpMovingUp = true;
    std::uint32_t arpRandomState = 0x41555245U;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label statusLabel;
    juce::ComboBox presetBox;
    juce::TextButton initVoiceButton { "INIT VOICE" };
    juce::ComboBox voiceModeBox;
    RockerButton monoModeButton { "MONO" };
    RockerButton unisonModeButton { "UNISON" };
    RockerButton arpButton { "ARP" };
    RockerButton chordButton { "CORD" };
    RockerButton arpHoldButton { "HOLD" };
    RockerButton glideLegatoButton { "LEGATO" };
    RockerButton lfoRetriggerButton { "RETRIG" };
    std::array<WaveformButton, 3> waveformAButtons {
        WaveformButton(aureline::Waveform::saw),
        WaveformButton(aureline::Waveform::triangle),
        WaveformButton(aureline::Waveform::pulse)
    };
    std::array<WaveformButton, 3> waveformBButtons {
        WaveformButton(aureline::Waveform::saw),
        WaveformButton(aureline::Waveform::triangle),
        WaveformButton(aureline::Waveform::pulse)
    };
    std::array<WaveformButton, 5> lfoWaveformButtons {
        WaveformButton(aureline::LfoWaveform::sawUp),
        WaveformButton(aureline::LfoWaveform::triangle),
        WaveformButton(aureline::LfoWaveform::sawDown),
        WaveformButton(aureline::LfoWaveform::square),
        WaveformButton(aureline::LfoWaveform::sampleAndHold)
    };
    RockerButton syncButton { "SYNC" };
    RockerButton lowFrequencyButton { "LF" };
    RockerButton keyboardTrackingButton { "KB" };
    std::array<RockerButton, 3> polyModDestinationButtons {
        RockerButton("FREQ A"),
        RockerButton("PW A"),
        RockerButton("FILTER")
    };
    std::array<RockerButton, 5> lfoDestinationButtons {
        RockerButton("A FREQ"),
        RockerButton("B FREQ"),
        RockerButton("PW A"),
        RockerButton("PW B"),
        RockerButton("FILTER")
    };
    juce::Slider oscillatorARangeKnob;
    juce::Slider oscillatorBRangeKnob;
    juce::Label oscillatorARangeLabel;
    juce::Label oscillatorBRangeLabel;
    juce::Label oscillatorAShapeLabel;
    juce::Label oscillatorBShapeLabel;
    juce::Label lfoShapeLabel;

    std::array<juce::Slider, 23> knobs;
    std::array<juce::Label, 23> knobLabels;
    juce::Slider spreadKnob;
    juce::Label spreadLabel;
    juce::Slider vintageKnob;
    juce::Label vintageLabel;
    juce::Slider tempoKnob;
    juce::Label tempoLabel;
    juce::Slider scaleKnob;
    juce::Label scaleLabel;
    juce::Slider lfoDelayKnob;
    juce::Label lfoDelayLabel;
    juce::Slider lfoFadeKnob;
    juce::Label lfoFadeLabel;
    std::array<juce::Slider, 5> performanceKnobs;
    std::array<juce::Label, 5> performanceLabels;
    std::array<juce::Slider, 3> arpKnobs;
    std::array<juce::Label, 3> arpLabels;
    PitchWheelSlider pitchWheel;
    juce::Slider modWheel;
    juce::Slider transposeFader;
    juce::Label transposeLabel;
    juce::Label pitchLabel;
    juce::Label modLabel;
    Keyboard keyboard;
};
