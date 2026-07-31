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
        setResizable(false, false);
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
    interface.processPluginBlock(buffer, midi);
    midi.clear();
}

juce::AudioProcessorEditor* AurelinePluginProcessor::createEditor()
{
    return new AurelinePluginEditor(*this);
}

void AurelinePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = interface.captureMultiTimbralState().createXml())
        copyXmlToBinary(*xml, destination);
}

void AurelinePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        interface.restoreMultiTimbralState(
            juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AurelinePluginProcessor();
}
