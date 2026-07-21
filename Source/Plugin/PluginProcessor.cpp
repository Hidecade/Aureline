#include "Plugin/PluginProcessor.h"

namespace
{
class AurelinePluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AurelinePluginEditor(AurelinePluginProcessor& ownerProcessor)
        : juce::AudioProcessorEditor(ownerProcessor),
          interface(ownerProcessor.interfaceComponent())
    {
        addAndMakeVisible(interface);
        setResizable(true, true);
        setResizeLimits(820, 535, 1536, 1002);
        setSize(1024, 668);
    }

    void resized() override
    {
        interface.setBounds(getLocalBounds());
    }

private:
    AurelineMainComponent& interface;
};
}

AurelinePluginProcessor::AurelinePluginProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void AurelinePluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    interface.prepareToPlay(samplesPerBlock, sampleRate);
}

void AurelinePluginProcessor::releaseResources()
{
    interface.releaseResources();
}

bool AurelinePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AurelinePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (const auto metadata : midi)
    {
        auto message = metadata.getMessage();
        message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
        interface.queueMidiMessage(message);
    }
    juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
    interface.getNextAudioBlock(info);
    midi.clear();
}

juce::AudioProcessorEditor* AurelinePluginProcessor::createEditor()
{
    return new AurelinePluginEditor(*this);
}

void AurelinePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = interface.capturePluginState().createXml())
        copyXmlToBinary(*xml, destination);
}

void AurelinePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        interface.restorePluginState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AurelinePluginProcessor();
}
