#include "App/MainComponent.h"
#include "BinaryData.h"
#include "Engine/FactoryPresets.h"

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

#include <algorithm>
#include <cmath>
#include <tuple>

namespace
{
constexpr int firstKeyboardNote = 48;
constexpr int keyboardNoteCount = 37;
constexpr int voiceSlotsPerBank = 32;
constexpr int libraryFormatVersion = 2;
constexpr std::array<const char*, 4> voiceBankNames {{
    "ANALOG 1", "ANALOG 2", "RETRO", "8-BIT"
}};

int voiceMenuItemId(int bank, int slot)
{
    return juce::jlimit(0, 3, bank) * voiceSlotsPerBank
        + juce::jlimit(0, voiceSlotsPerBank - 1, slot) + 1;
}
constexpr int pcKeyboardTranspose = 12;
constexpr float waveformDisplayGain = 2.0f;
// Modern vintage palette: graphite chassis, warm control surfaces,
// aged brass secondary type, ivory labels, and restrained burnt-orange accents.
constexpr juce::uint32 themeBackground = 0xff0b0d0c;
constexpr juce::uint32 themePanel = 0xff171816;
constexpr juce::uint32 themePanelLight = 0xff302d28;
constexpr juce::uint32 themeLineGray = 0xff626865;
constexpr juce::uint32 themeAmber = 0xffe9782d;
constexpr juce::uint32 themeGold = 0xffb9a06f;
constexpr juce::uint32 themeText = 0xffe7e2d7;

juce::String audioDeviceStatus(juce::AudioDeviceManager& manager)
{
    if (auto* device = manager.getCurrentAudioDevice())
        return "Audio: " + device->getName();

    return "Audio: off";
}

juce::String midiDeviceStatus(juce::AudioDeviceManager& manager)
{
    const auto devices = juce::MidiInput::getAvailableDevices();
    juce::StringArray enabledNames;
    for (const auto& device : devices)
        if (manager.isMidiInputDeviceEnabled(device.identifier))
            enabledNames.add(device.name);

    if (enabledNames.isEmpty())
        return devices.isEmpty() ? "MIDI: no input" : "MIDI: off";
    if (enabledNames.size() == devices.size())
        return "MIDI: all inputs";
    if (enabledNames.size() == 1)
        return "MIDI: " + enabledNames[0];

    return "MIDI: " + enabledNames.joinIntoString(", ");
}

bool containsNonAscii(const juce::String& text)
{
    for (const auto character : text)
        if (static_cast<juce::uint32>(character) > 0x7f)
            return true;

    return false;
}

juce::File aurelineDocumentsDirectory()
{
    auto directory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("Aureline");
    directory.createDirectory();
    return directory;
}

juce::File aurelineApplicationSupportDirectory()
{
    auto directory = juce::File::getSpecialLocation(
                         juce::File::userApplicationDataDirectory)
                         .getChildFile("Aureline");
    directory.createDirectory();
    return directory;
}

juce::File lastSelectedVoiceFile()
{
    const auto current = aurelineApplicationSupportDirectory().getChildFile(
        ".last-selected-voice");
    const auto legacy = aurelineDocumentsDirectory().getChildFile(
        ".last-selected-voice");
    if (!current.existsAsFile() && legacy.existsAsFile())
        legacy.moveFileTo(current);
    return current;
}

juce::File legacyStoredVoiceFile(std::size_t slot)
{
    return aurelineDocumentsDirectory().getChildFile(
        "slot-" + juce::String(static_cast<int>(slot) + 1).paddedLeft('0', 2)
        + ".aurelinevoice");
}

juce::File activeLibraryFile(int bank)
{
    return aurelineApplicationSupportDirectory().getChildFile(
        "bank-" + juce::String(juce::jlimit(0, 3, bank) + 1)
        + ".aurelinelibrary.xml");
}

juce::File legacyActiveLibraryFile()
{
    return aurelineApplicationSupportDirectory().getChildFile(
        "active-library.aurelinelibrary.xml");
}

juce::File lastSelectedBankFile()
{
    return aurelineApplicationSupportDirectory().getChildFile(
        ".last-selected-bank");
}

void installBuiltInVoiceLibrary(const juce::File& file,
                                const char* data,
                                const int dataSize)
{
    if (data != nullptr && dataSize > 0)
        file.replaceWithData(data, static_cast<std::size_t>(dataSize));
}

juce::ValueTree readVoiceLibrary(const juce::File& file)
{
    const auto xml = juce::XmlDocument::parse(file);
    const auto library = xml != nullptr ? juce::ValueTree::fromXml(*xml)
                                        : juce::ValueTree {};
    if (!library.isValid()
        || library.getType().toString() != "AurelineLibrary"
        || library.getProperty("format").toString()
               != "com.hidecade.aureline.library"
        || static_cast<int>(library.getProperty("version"))
               != libraryFormatVersion
        || library.getNumChildren() != voiceSlotsPerBank)
        return {};
    for (int index = 0; index < library.getNumChildren(); ++index)
        if (static_cast<int>(
                library.getChild(index).getProperty("slot", -1)) != index)
            return {};
    return library;
}

juce::String voiceNameWithoutSlotPrefix(juce::String name)
{
    name = name.trim();
    if (name.length() >= 3
        && juce::CharacterFunctions::isDigit(name[0])
        && juce::CharacterFunctions::isDigit(name[1])
        && juce::CharacterFunctions::isWhitespace(name[2]))
        name = name.substring(3).trimStart();
    return name.substring(0, juce::jmin(16, name.length()));
}

juce::String slotVoiceDisplayName(std::size_t index, const juce::String& voiceName)
{
    return juce::String(static_cast<int>(index) + 1).paddedLeft('0', 2)
        + " " + voiceNameWithoutSlotPrefix(voiceName);
}

bool isReservedLibraryFilename(const juce::File& file)
{
    return file.getFileName().equalsIgnoreCase("Analog.aurelinelibrary.xml")
        || file.getFileName().equalsIgnoreCase("Analog2.aurelinelibrary.xml")
        || file.getFileName().equalsIgnoreCase("Retro.aurelinelibrary.xml")
        || file.getFileName().equalsIgnoreCase(
            "8-Bit.aurelinelibrary.xml");
}

juce::String jsonVoiceKeyForStateKey(const juce::String& key)
{
    if (key == "oscillatorALevel") return "oscALevel";
    if (key == "oscillatorBLevel") return "oscBLevel";
    if (key == "oscillatorBFine") return "oscBFine";
    if (key == "filterEnvelope") return "filterEnvAmount";
    if (key == "filterKeyboardTracking") return "filterKeyTrack";
    if (key == "attack") return "ampAttack";
    if (key == "decay") return "ampDecay";
    if (key == "sustain") return "ampSustain";
    if (key == "release") return "ampRelease";
    if (key == "lfoWaveformMask") return "lfoWaveMask";
    if (key == "polyModFilterEnvelope") return "polyModFilterEnv";
    if (key == "polyModOscillatorB") return "polyModOscB";
    if (key == "polyModToFrequencyA") return "polyDestPitch";
    if (key == "polyModToPulseWidthA") return "polyDestPWA";
    if (key == "polyModToFilter") return "polyDestFilter";
    if (key == "master") return "masterGain";
    if (key == "glideLegatoOnly") return "glideLegato";
    if (key == "oscillatorSync") return "oscSync";
    if (key == "oscillatorAOctave") return "oscAOctave";
    if (key == "oscillatorBOctave") return "oscBOctave";
    if (key == "oscillatorBLowFrequency") return "oscBLowFrequency";
    if (key == "oscillatorBKeyboardTracking") return "oscBKeyTrack";
    if (key == "lfoDestination0") return "lfoDestA";
    if (key == "lfoDestination1") return "lfoDestB";
    if (key == "lfoDestination2") return "lfoDestPWA";
    if (key == "lfoDestination3") return "lfoDestPWB";
    if (key == "lfoDestination4") return "lfoDestFilter";
    return key;
}

juce::String stateKeyForJsonVoiceKey(const juce::String& key)
{
    if (key == "oscALevel") return "oscillatorALevel";
    if (key == "oscBLevel") return "oscillatorBLevel";
    if (key == "oscBFine") return "oscillatorBFine";
    if (key == "filterEnvAmount") return "filterEnvelope";
    if (key == "filterKeyTrack") return "filterKeyboardTracking";
    if (key == "ampAttack") return "attack";
    if (key == "ampDecay") return "decay";
    if (key == "ampSustain") return "sustain";
    if (key == "ampRelease") return "release";
    if (key == "lfoWaveMask") return "lfoWaveformMask";
    if (key == "polyModFilterEnv") return "polyModFilterEnvelope";
    if (key == "polyModOscB") return "polyModOscillatorB";
    if (key == "polyDestPitch") return "polyModToFrequencyA";
    if (key == "polyDestPWA") return "polyModToPulseWidthA";
    if (key == "polyDestFilter") return "polyModToFilter";
    if (key == "masterGain") return "master";
    if (key == "glideLegato") return "glideLegatoOnly";
    if (key == "oscSync") return "oscillatorSync";
    if (key == "oscAOctave") return "oscillatorAOctave";
    if (key == "oscBOctave") return "oscillatorBOctave";
    if (key == "oscBLowFrequency") return "oscillatorBLowFrequency";
    if (key == "oscBKeyTrack") return "oscillatorBKeyboardTracking";
    if (key == "lfoDestA") return "lfoDestination0";
    if (key == "lfoDestB") return "lfoDestination1";
    if (key == "lfoDestPWA") return "lfoDestination2";
    if (key == "lfoDestPWB") return "lfoDestination3";
    if (key == "lfoDestFilter") return "lfoDestination4";
    return key;
}

juce::var makeVoiceFileJson(const juce::ValueTree& state, const juce::String& name)
{
    auto patch = std::make_unique<juce::DynamicObject>();
    for (int index = 0; index < state.getNumProperties(); ++index)
    {
        const auto key = state.getPropertyName(index).toString();
        if (key != "version" && key != "voiceName" && key != "slot")
            patch->setProperty(jsonVoiceKeyForStateKey(key),
                               static_cast<double>(
                                   state.getProperty(state.getPropertyName(index))));
    }

    auto performance = std::make_unique<juce::DynamicObject>();
    performance->setProperty("transpose", state.getProperty("transpose", 0.0));
    performance->setProperty("pitchBendRange",
                             state.getProperty("pitchBendRange", 2.0));

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty("format", "com.hidecade.aureline.voice");
    root->setProperty("version", 1);
    root->setProperty("name", name);
    root->setProperty("author", "");
    root->setProperty("category", "User");
    root->setProperty("patch", juce::var(patch.release()));
    root->setProperty("performance", juce::var(performance.release()));
    return juce::var(root.release());
}

juce::ValueTree stateFromVoiceFileJson(const juce::var& json)
{
    const auto* root = json.getDynamicObject();
    if (root == nullptr
        || root->getProperty("format").toString() != "com.hidecade.aureline.voice"
        || static_cast<int>(root->getProperty("version")) != 1)
        return {};

    const auto* patch = root->getProperty("patch").getDynamicObject();
    if (patch == nullptr)
        return {};

    juce::ValueTree state("AurelineState");
    state.setProperty("version", 2, nullptr);
    const auto& properties = patch->getProperties();
    for (int index = 0; index < properties.size(); ++index)
        state.setProperty(stateKeyForJsonVoiceKey(properties.getName(index).toString()),
                          properties.getValueAt(index), nullptr);
    state.setProperty("voiceName", root->getProperty("name"), nullptr);
    return state;
}

#if 0 // Factory definitions moved to Engine/FactoryPresets.cpp.
struct FactoryVoice
{
    const char* name;
    int waveA, waveB;
    float octaveA, octaveB;
    float levelA, levelB, fine, noise;
    float cutoff, resonance, filterEnvelope;
    float attack, decay, sustain, release;
    float lfoRate, lfoAmount, spread, vintage;
    int voiceMode;
    bool sync;
    float filterAttack = -1.0f;
    float filterDecay = -1.0f;
    float filterSustain = -1.0f;
    float filterRelease = -1.0f;
};

constexpr std::array<FactoryVoice, 32> factoryVoices {{
    { "01 WARM BRASS",    1, 1,  0,  0, .72f, .62f,  5, .00f, 4200, .18f, .62f, .04f, .42f, .78f, .34f, 5.2f, .04f, .18f, .42f, 0, false },
    { "02 POLY BRASS",    1, 5,  0,  0, .74f, .62f, 12, .00f, 1650, .29f, .92f, .01f, .55f, .62f, 1.10f, 5.8f, .03f, .38f, .66f, 0, false,
      .005f, .78f, .16f, 1.15f },
    { "03 SOFT STRINGS",  1, 1,  0,  0, .58f, .56f, 11, .00f, 5200, .12f, .32f, .65f, 1.20f, .82f, 1.80f, 4.7f, .12f, .62f, .68f, 0, false },
    { "04 PWM STRINGS",   4, 4,  0,  0, .60f, .58f,  7, .00f, 6100, .16f, .28f, .48f, 1.00f, .86f, 1.55f, 3.8f, .22f, .70f, .62f, 0, false },
    { "05 PROPHET PAD",   1, 2, -1,  0, .52f, .48f,  9, .00f, 3900, .22f, .45f, 1.10f, 1.65f, .74f, 2.30f, 2.6f, .08f, .78f, .76f, 0, false },
    { "06 SLOW CHOIR",    2, 4,  0,  0, .48f, .52f,  4, .02f, 2800, .28f, .38f, 1.70f, 2.20f, .80f, 2.80f, 1.4f, .10f, .72f, .80f, 0, false },
    { "07 ANALOG SWEEP",  1, 4, -1,  0, .55f, .55f, 13, .00f, 1800, .40f, .88f, .70f, 2.80f, .55f, 2.10f, 0.7f, .16f, .68f, .72f, 0, false },
    { "08 DREAM PAD",     2, 1,  0,  1, .54f, .30f, -7, .01f, 7200, .10f, .22f, 1.35f, 1.80f, .88f, 3.20f, 3.1f, .07f, .86f, .58f, 0, false },

    { "09 SOLID BASS",    1, 4, -1, -1, .78f, .52f,  3, .00f, 1450, .22f, .58f, .01f, .20f, .72f, .18f, 4.5f, .00f, .05f, .34f, 1, false },
    { "10 SUB BASS",      2, 4, -1, -2, .64f, .72f,  0, .00f,  780, .08f, .22f, .02f, .28f, .84f, .24f, 3.0f, .00f, .00f, .22f, 1, false },
    { "11 RESO BASS",     1, 1, -1, -1, .72f, .44f,  6, .00f,  920, .68f, .74f, .01f, .32f, .58f, .20f, 5.4f, .00f, .04f, .48f, 1, false },
    { "12 SYNC BASS",     1, 4, -1,  0, .76f, .38f, 19, .00f, 1700, .34f, .64f, .01f, .18f, .68f, .16f, 6.2f, .03f, .08f, .46f, 1, true  },
    { "13 FUNK BASS",     1, 2, -1,  0, .72f, .32f, -5, .00f, 2100, .26f, .82f, .01f, .12f, .46f, .12f, 5.0f, .00f, .05f, .38f, 1, false },
    { "14 PULSE BASS",    4, 4, -1, -1, .72f, .48f,  9, .00f, 1250, .18f, .52f, .01f, .24f, .76f, .22f, 4.1f, .08f, .06f, .52f, 1, false },
    { "15 UNISON BASS",   1, 1, -1, -1, .70f, .58f,  8, .00f, 1850, .20f, .46f, .01f, .22f, .78f, .20f, 4.8f, .02f, .12f, .64f, 2, false },
    { "16 ACID LINE",     1, 4, -1,  0, .68f, .40f, 12, .00f,  680, .82f, .94f, .01f, .16f, .34f, .10f, 6.8f, .00f, .03f, .32f, 1, false },

    { "17 CLASSIC LEAD",  1, 1,  0,  0, .72f, .58f,  7, .00f, 5600, .24f, .38f, .01f, .26f, .76f, .30f, 5.1f, .08f, .16f, .48f, 1, false },
    { "18 SYNC LEAD",     1, 4,  0,  1, .72f, .44f, 28, .00f, 4800, .30f, .54f, .01f, .22f, .70f, .24f, 6.0f, .06f, .14f, .56f, 1, true  },
    { "19 PWM LEAD",      4, 4,  0,  0, .68f, .54f,  5, .00f, 6400, .18f, .30f, .02f, .28f, .80f, .34f, 4.0f, .18f, .18f, .50f, 1, false },
    { "20 SOFT SOLO",     2, 1,  0,  0, .64f, .38f, -4, .00f, 3900, .12f, .24f, .08f, .38f, .74f, .52f, 5.6f, .10f, .12f, .44f, 1, false },
    { "21 FIFTH LEAD",    1, 1,  0,  1, .66f, .50f,  0, .00f, 6200, .20f, .42f, .01f, .24f, .72f, .28f, 5.3f, .07f, .20f, .52f, 1, false },
    { "22 GLIDE SOLO",    1, 2,  0,  0, .70f, .42f,  6, .00f, 5100, .16f, .36f, .03f, .32f, .78f, .40f, 4.6f, .12f, .10f, .58f, 1, false },
    { "23 NOISE LEAD",    1, 4,  0,  0, .62f, .42f, 11, .18f, 3600, .38f, .58f, .01f, .20f, .62f, .22f, 7.4f, .08f, .15f, .62f, 1, false },
    { "24 VINTAGE SOLO",  1, 1,  0,  0, .70f, .56f, -8, .01f, 4700, .22f, .44f, .02f, .30f, .74f, .36f, 5.0f, .09f, .14f, .92f, 1, false },

    { "25 ANALOG PIANO",  1, 4,  0,  1, .64f, .34f,  2, .00f, 7200, .12f, .46f, .01f, .48f, .28f, .42f, 5.2f, .00f, .32f, .38f, 0, false },
    { "26 CLAV PULSE",    4, 4,  0,  1, .66f, .36f,  5, .00f, 4300, .24f, .72f, .01f, .16f, .18f, .12f, 6.0f, .00f, .16f, .42f, 0, false },
    { "27 WOOD PLUCK",    2, 4,  0,  1, .62f, .30f, -3, .00f, 3600, .18f, .68f, .01f, .18f, .08f, .16f, 4.2f, .00f, .22f, .34f, 0, false },
    { "28 ANALOG MALLET", 2, 1,  0,  1, .58f, .28f,  7, .00f, 5800, .34f, .52f, .01f, .34f, .12f, .38f, 5.8f, .02f, .28f, .46f, 0, false },
    { "29 DRAWBAR ORGAN", 4, 4,  0,  1, .62f, .44f,  0, .00f, 8200, .06f, .10f, .01f, .08f, .96f, .12f, 6.4f, .06f, .40f, .30f, 0, false },
    { "30 ANALOG BELL",   2, 2,  0,  2, .52f, .24f, 13, .00f, 9800, .42f, .36f, .01f, .72f, .06f, 1.40f, 7.8f, .04f, .54f, .44f, 0, false },
    { "31 WIND MACHINE",  2, 1, -1,  1, .24f, .18f, 17, .62f, 2400, .46f, .28f, 1.30f, 2.20f, .72f, 2.70f, 0.4f, .26f, .84f, .74f, 0, false },
    { "32 NOISE SWEEP",   1, 4, -1,  1, .18f, .14f, -9, .78f,  740, .76f, .92f, .80f, 3.10f, .40f, 3.50f, 0.2f, .18f, .76f, .82f, 0, false }
}};
#endif

const auto& factoryVoices = aureline::factoryPresets();

std::array<std::uint8_t, 5> lcdGlyph(juce::juce_wchar character)
{
    switch (character)
    {
        case '0': return { 0x3e, 0x51, 0x49, 0x45, 0x3e };
        case '1': return { 0x00, 0x42, 0x7f, 0x40, 0x00 };
        case '2': return { 0x42, 0x61, 0x51, 0x49, 0x46 };
        case '3': return { 0x21, 0x41, 0x45, 0x4b, 0x31 };
        case '4': return { 0x18, 0x14, 0x12, 0x7f, 0x10 };
        case '5': return { 0x27, 0x45, 0x45, 0x45, 0x39 };
        case '6': return { 0x3c, 0x4a, 0x49, 0x49, 0x30 };
        case '7': return { 0x01, 0x71, 0x09, 0x05, 0x03 };
        case '8': return { 0x36, 0x49, 0x49, 0x49, 0x36 };
        case '9': return { 0x06, 0x49, 0x49, 0x29, 0x1e };
        case 'A': return { 0x7e, 0x11, 0x11, 0x11, 0x7e };
        case 'B': return { 0x7f, 0x49, 0x49, 0x49, 0x36 };
        case 'C': return { 0x3e, 0x41, 0x41, 0x41, 0x22 };
        case 'D': return { 0x7f, 0x41, 0x41, 0x22, 0x1c };
        case 'E': return { 0x7f, 0x49, 0x49, 0x49, 0x41 };
        case 'F': return { 0x7f, 0x09, 0x09, 0x09, 0x01 };
        case 'G': return { 0x3e, 0x41, 0x49, 0x49, 0x7a };
        case 'H': return { 0x7f, 0x08, 0x08, 0x08, 0x7f };
        case 'I': return { 0x00, 0x41, 0x7f, 0x41, 0x00 };
        case 'J': return { 0x20, 0x40, 0x41, 0x3f, 0x01 };
        case 'K': return { 0x7f, 0x08, 0x14, 0x22, 0x41 };
        case 'L': return { 0x7f, 0x40, 0x40, 0x40, 0x40 };
        case 'M': return { 0x7f, 0x02, 0x0c, 0x02, 0x7f };
        case 'N': return { 0x7f, 0x04, 0x08, 0x10, 0x7f };
        case 'O': return { 0x3e, 0x41, 0x41, 0x41, 0x3e };
        case 'P': return { 0x7f, 0x09, 0x09, 0x09, 0x06 };
        case 'Q': return { 0x3e, 0x41, 0x51, 0x21, 0x5e };
        case 'R': return { 0x7f, 0x09, 0x19, 0x29, 0x46 };
        case 'S': return { 0x46, 0x49, 0x49, 0x49, 0x31 };
        case 'T': return { 0x01, 0x01, 0x7f, 0x01, 0x01 };
        case 'U': return { 0x3f, 0x40, 0x40, 0x40, 0x3f };
        case 'V': return { 0x1f, 0x20, 0x40, 0x20, 0x1f };
        case 'W': return { 0x3f, 0x40, 0x38, 0x40, 0x3f };
        case 'X': return { 0x63, 0x14, 0x08, 0x14, 0x63 };
        case 'Y': return { 0x07, 0x08, 0x70, 0x08, 0x07 };
        case 'Z': return { 0x61, 0x51, 0x49, 0x45, 0x43 };
        case '-': return { 0x08, 0x08, 0x08, 0x08, 0x08 };
        default: return { 0x00, 0x00, 0x00, 0x00, 0x00 };
    }
}

struct PcKeyNote
{
    int keyCode;
    int note;
};

constexpr std::array<PcKeyNote, 41> pcKeyboardMap {{
    { 'z', 36 }, { 'x', 38 }, { 'c', 40 }, { 'v', 41 }, { 'b', 43 }, { 'n', 45 }, { 'm', 47 },
    { ',', 48 }, { '.', 50 }, { '/', 52 }, { '\\', 53 },
    { 's', 37 }, { 'd', 39 }, { 'g', 42 }, { 'h', 44 }, { 'j', 46 }, { 'l', 49 }, { ';', 51 }, { ':', 51 },
    { 'q', 48 }, { 'w', 50 }, { 'e', 52 }, { 'r', 53 }, { 't', 55 }, { 'y', 57 }, { 'u', 59 },
    { 'i', 60 }, { 'o', 62 }, { 'p', 64 }, { '@', 65 }, { '[', 67 }, { ']', 67 },
    { '2', 49 }, { '3', 51 }, { '5', 54 }, { '6', 56 }, { '7', 58 }, { '9', 61 }, { '0', 63 },
    { '-', 66 }, { '^', 66 }
}};

bool isPcKeyCurrentlyDown(int keyCode)
{
    if (keyCode >= 'a' && keyCode <= 'z')
        return juce::KeyPress::isKeyCurrentlyDown(keyCode)
            || juce::KeyPress::isKeyCurrentlyDown(keyCode - 'a' + 'A');
    return juce::KeyPress::isKeyCurrentlyDown(keyCode);
}

bool isBlackKey(int note)
{
    const int pitch = ((note % 12) + 12) % 12;
    return pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
}

int whiteIndexForNote(int note)
{
    int result = 0;
    for (int current = firstKeyboardNote; current < note; ++current)
        if (!isBlackKey(current))
            ++result;
    return result;
}
}

void AurelineMainComponent::AurelineLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float position,
    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                                static_cast<float>(y),
                                                static_cast<float>(width),
                                                static_cast<float>(height)).reduced(2.0f);
    const auto knob = bounds.withSizeKeepingCentre(juce::jmin(bounds.getWidth(), bounds.getHeight()),
                                                    juce::jmin(bounds.getWidth(), bounds.getHeight()));
    const auto radius = knob.getWidth() * 0.46f;
    const auto centre = knob.getCentre();
    const auto angle = startAngle + position * (endAngle - startAngle);
    const auto arcRange = endAngle - startAngle;

    g.setColour(juce::Colours::black.withAlpha(0.34f));
    g.fillEllipse(knob.reduced(knob.getWidth() * 0.18f).translated(0.0f, 2.0f));

    const bool tempoNumbered = slider.getName() == "tempoNumberedKnob";
    const bool numbered = slider.getName() == "numberedKnob" || tempoNumbered;
    const auto interval = slider.getInterval();
    const int discreteValueCount = interval > 0.0
        ? juce::roundToInt((slider.getMaximum() - slider.getMinimum()) / interval) + 1
        : 0;
    const bool discrete = !numbered && discreteValueCount >= 2 && discreteValueCount <= 12;
    const int tickCount = numbered ? 11 : discrete ? discreteValueCount : 23;
    const auto inactiveTick = juce::Colour(themeLineGray).withAlpha(0.45f);
    const auto activeTick = juce::Colour(0xffc7cac9);
    const auto tickOuter = radius - 1.0f;
    const auto activeIndex = static_cast<int>(std::round(position * static_cast<float>(tickCount - 1)));
    for (int tick = 0; tick < tickCount; ++tick)
    {
        const auto tickPosition = static_cast<float>(tick) / static_cast<float>(tickCount - 1);
        const auto tickAngle = startAngle + tickPosition * arcRange;
        const bool major = numbered || discrete || tick == 0 || tick == (tickCount - 1) / 2
                         || tick == tickCount - 1;
        const bool selectedTick = discrete ? tick == activeIndex : tick <= activeIndex;
        g.setColour(selectedTick ? activeTick
                                 : discrete ? juce::Colour(0xff686d6f) : inactiveTick);
        if (numbered)
        {
            const auto markStart = centre.getPointOnCircumference(tickOuter - 5.5f, tickAngle);
            const auto markEnd = centre.getPointOnCircumference(tickOuter - 2.5f, tickAngle);
            g.drawLine({ markStart, markEnd }, tick <= activeIndex ? 1.5f : 1.0f);
            const auto numberCentre = centre.getPointOnCircumference(tickOuter + 3.0f, tickAngle);
            const auto numberBounds = juce::Rectangle<float>(tempoNumbered ? 18.0f : 12.0f, 9.0f)
                                          .withCentre(numberCentre);
            g.setFont(juce::FontOptions(tempoNumbered ? 5.5f : 7.5f, juce::Font::bold));
            const auto tickText = tempoNumbered ? juce::String(40 + tick * 20)
                                                : juce::String(tick);
            g.drawText(tickText, numberBounds, juce::Justification::centred, false);
        }
        else if (major)
        {
            if (discrete)
            {
                const auto markStart = centre.getPointOnCircumference(tickOuter - 5.0f, tickAngle);
                const auto markEnd = centre.getPointOnCircumference(tickOuter - 1.8f, tickAngle);
                g.drawLine({ markStart, markEnd }, 1.6f);
                const auto value = slider.getMinimum() + interval * static_cast<double>(tick);
                const auto valueCentre = centre.getPointOnCircumference(tickOuter + 1.5f, tickAngle);
                const auto valueBounds = juce::Rectangle<float>(24.0f, 8.0f)
                                             .withCentre(valueCentre);
                g.setFont(juce::FontOptions(discreteValueCount > 8 ? 7.5f : 8.5f,
                                            juce::Font::bold));
                g.drawText(slider.getTextFromValue(value), valueBounds,
                           juce::Justification::centred, false);
            }
            else
            {
                const auto tickStart = tickOuter - 3.0f;
                const auto start = centre.getPointOnCircumference(tickStart, tickAngle);
                const auto end = centre.getPointOnCircumference(tickStart + 3.9f, tickAngle);
                g.drawLine({ start, end }, 1.8f);
            }
        }
        else
        {
            const auto dot = centre.getPointOnCircumference(tickOuter - 1.7f, tickAngle);
            g.fillEllipse(juce::Rectangle<float>(2.0f, 2.0f).withCentre(dot));
        }
    }

    const auto outer = knob.reduced(knob.getWidth() * 0.27f);
    g.setGradientFill({ juce::Colour(0xff4a4033), outer.getX(), outer.getY(),
                        juce::Colour(0xff090806), outer.getRight(), outer.getBottom(), false });
    g.fillEllipse(outer);
    g.setColour(juce::Colour(0xff050505));
    g.drawEllipse(outer, 2.0f);

    const auto inner = outer.reduced(outer.getWidth() * 0.16f);
    g.setGradientFill({ juce::Colour(0xff29251f), inner.getX(), inner.getY(),
                        juce::Colour(0xff0d0b09), inner.getRight(), inner.getBottom(), false });
    g.fillEllipse(inner);
    g.setColour(juce::Colour(0xff000000).withAlpha(0.75f));
    g.drawEllipse(inner, 1.25f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.65f, -radius * 0.54f, 3.3f, radius * 0.43f, 1.2f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.setGradientFill({ juce::Colour(themeText), centre.x, centre.y - radius * 0.54f,
                        juce::Colour(themeLineGray), centre.x, centre.y, false });
    g.fillPath(pointer);
}

void AurelineMainComponent::AurelineLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPosition,
    float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
        return;
    if (slider.getName() == "waveMemoryStep")
    {
        const auto bounds = juce::Rectangle<float>(
            static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height)).reduced(1.5f, 1.0f);
        g.setColour(juce::Colour(themeBackground));
        g.fillRect(bounds);
        const auto barTop = juce::jlimit(bounds.getY(), bounds.getBottom(), sliderPosition);
        const auto bar = juce::Rectangle<float>(
            bounds.getX(), barTop, bounds.getWidth(),
            juce::jmax(2.0f, bounds.getBottom() - barTop));
        g.setGradientFill({ juce::Colour(themeAmber), bar.getX(), bar.getY(),
                            juce::Colour(themeAmber).darker(0.35f),
                            bar.getX(), bar.getBottom(), false });
        g.fillRoundedRectangle(bar, 1.0f);
        return;
    }
    if (slider.getName() == "volumeFader")
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                    static_cast<float>(width), static_cast<float>(height)).reduced(2.0f);
        g.setGradientFill({ juce::Colour(0xff211f1a), bounds.getX(), bounds.getY(),
                            juce::Colour(0xff0a0907), bounds.getX(), bounds.getBottom(), false });
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colour(0xff050504));
        g.drawRoundedRectangle(bounds, 2.0f, 2.0f);
        g.setColour(juce::Colour(0xff4a463c));
        g.drawRoundedRectangle(bounds.reduced(2.0f), 1.0f, 1.0f);

        const auto track = juce::Rectangle<float>(bounds.getX() + 15.0f, bounds.getY() + 23.0f,
                                                   8.0f, bounds.getHeight() - 46.0f);
        g.setColour(juce::Colour(0xff030303));
        g.fillRoundedRectangle(track, 3.0f);
        g.setColour(juce::Colour(0xff171714));
        g.drawRoundedRectangle(track, 3.0f, 1.0f);

        g.setColour(juce::Colour(0xff77776f));
        for (int mark = 0; mark <= 8; ++mark)
        {
            const float markY = juce::jmap(static_cast<float>(mark), 0.0f, 8.0f,
                                            track.getY(), track.getBottom());
            g.drawLine(track.getRight() + 8.0f, markY,
                       track.getRight() + (mark % 4 == 0 ? 22.0f : 18.0f), markY, 1.3f);
        }

        g.setColour(juce::Colour(0xffeeeeea));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText("MAX", juce::Rectangle<float>(bounds.getRight() - 28.0f, bounds.getY() + 3.0f,
                                                  26.0f, 14.0f), juce::Justification::centredRight);
        g.drawText("MIN", juce::Rectangle<float>(bounds.getRight() - 28.0f, bounds.getBottom() - 17.0f,
                                                  26.0f, 14.0f), juce::Justification::centredRight);

        const auto thumb = juce::Rectangle<float>(bounds.getX() + 4.0f, sliderPosition - 5.0f,
                                                   31.0f, 10.0f);
        g.setGradientFill({ juce::Colour(0xffd7dfde), thumb.getX(), thumb.getY(),
                            juce::Colour(0xff657071), thumb.getX(), thumb.getBottom(), false });
        g.fillRoundedRectangle(thumb, 1.5f);
        g.setColour(juce::Colour(0xff222626));
        g.drawRoundedRectangle(thumb, 1.5f, 1.2f);
        g.setColour(juce::Colour(0xffe9efed).withAlpha(0.55f));
        g.drawLine(thumb.getX() + 2.0f, thumb.getCentreY(), thumb.getRight() - 2.0f,
                   thumb.getCentreY(), 1.0f);
        return;
    }
    if (slider.getName() == "wheelFader")
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                    static_cast<float>(width), static_cast<float>(height)).reduced(2.0f);
        const auto slot = bounds.withSizeKeepingCentre(juce::jmin(bounds.getWidth(), 30.0f), bounds.getHeight());
        g.setColour(juce::Colours::black.withAlpha(0.48f));
        g.fillRoundedRectangle(slot.translated(0.0f, 2.0f), 3.0f);
        g.setGradientFill({ juce::Colour(0xff171a1c), slot.getX(), slot.getY(),
                            juce::Colour(0xff020303), slot.getX(), slot.getBottom(), false });
        g.fillRoundedRectangle(slot, 3.0f);
        g.setColour(juce::Colour(0xff020202));
        g.drawRoundedRectangle(slot, 3.0f, 1.8f);
        const auto wheel = slot.reduced(5.0f, 2.0f);
        g.setGradientFill({ juce::Colour(0xff1d2022), wheel.getCentreX(), wheel.getY(),
                            juce::Colour(0xff030404), wheel.getCentreX(), wheel.getBottom(), false });
        g.fillRoundedRectangle(wheel, 2.0f);
        const auto range = slider.getMaximum() - slider.getMinimum();
        const auto normalized = range > 0.0
            ? static_cast<float>((slider.getValue() - slider.getMinimum()) / range) : 0.0f;
        const float ribSpacing = 3.25f;
        const float phase = std::fmod(normalized * 42.0f, ribSpacing);
        for (int rib = -2; rib < static_cast<int>(wheel.getHeight() / ribSpacing) + 4; ++rib)
        {
            const float ribY = wheel.getY() + 2.0f + phase + static_cast<float>(rib) * ribSpacing;
            if (ribY < wheel.getY() + 2.0f || ribY > wheel.getBottom() - 2.0f)
                continue;
            const float curve = 1.0f - std::abs((ribY - wheel.getCentreY()) / (wheel.getHeight() * 0.5f));
            const float inset = 3.0f + (1.0f - curve) * 2.2f;
            g.setColour(juce::Colour(0xff727779).withAlpha(0.16f + curve * 0.28f));
            g.drawLine(wheel.getX() + inset, ribY, wheel.getRight() - inset, ribY, 1.0f);
            g.setColour(juce::Colours::black.withAlpha(0.62f));
            g.drawLine(wheel.getX() + inset, ribY + 1.0f, wheel.getRight() - inset, ribY + 1.0f, 1.0f);
        }
        const auto indicatorY = juce::jmap(normalized, wheel.getBottom() - 3.0f, wheel.getY() + 3.0f);
        g.setColour(juce::Colour(themeAmber));
        g.fillRoundedRectangle(wheel.getX() + 2.0f, indicatorY - 1.5f, wheel.getWidth() - 4.0f, 3.0f, 1.0f);
        return;
    }
    const auto track = juce::Rectangle<float>(static_cast<float>(x + width / 2 - 5),
                                               static_cast<float>(y + 4), 10.0f,
                                               static_cast<float>(height - 8));
    g.setColour(juce::Colour(0xff080705));
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(juce::Colour(0xff4b3a21));
    g.drawRoundedRectangle(track, 3.0f, 1.0f);
    g.setColour(juce::Colour(themeAmber));
    g.fillRoundedRectangle(juce::Rectangle<float>(track.getX() - 4.0f,
                                                   sliderPosition - 5.0f,
                                                   track.getWidth() + 8.0f, 10.0f), 2.0f);
}

void AurelineMainComponent::AurelineLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonWidth, int buttonHeight, juce::ComboBox& box)
{
    if (box.getName() != "voiceSelector"
        && box.getName() != "waveMemorySelector")
    {
        juce::LookAndFeel_V4::drawComboBox(g, width, height, isButtonDown,
                                           buttonX, buttonY, buttonWidth, buttonHeight, box);
        return;
    }

    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f,
                                                static_cast<float>(width - 1),
                                                static_cast<float>(height - 1));
    g.setColour(juce::Colour(themeBackground));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(isButtonDown ? juce::Colour(themeAmber)
                             : juce::Colour(themeLineGray).withAlpha(0.75f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    juce::Path arrow;
    const auto centre = juce::Point<float>(static_cast<float>(width - 13),
                                           static_cast<float>(height) * 0.5f);
    arrow.startNewSubPath(centre.x - 3.0f, centre.y - 1.5f);
    arrow.lineTo(centre.x, centre.y + 2.0f);
    arrow.lineTo(centre.x + 3.0f, centre.y - 1.5f);
    g.setColour(juce::Colour(themeAmber));
    g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
}

void AurelineMainComponent::AurelineLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
    bool highlighted, bool down)
{
    if (button.getName() != "voiceActionButton" && button.getName() != "voiceStepButton")
    {
        juce::LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour,
                                                   highlighted, down);
        return;
    }

    const bool step = button.getName() == "voiceStepButton";
    const bool store = button.getButtonText() == "STORE";
    const bool recording = button.getButtonText() == "STOP";
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto top = store || recording ? juce::Colour(themeAmber)
                           : step ? juce::Colour(0xff3a3731) : juce::Colour(themePanelLight);
    const auto bottom = store || recording ? juce::Colour(0xff6f2412)
                              : step ? juce::Colour(0xff171614) : juce::Colour(0xff17110d);
    g.setGradientFill({ down ? top.brighter(0.12f) : top, bounds.getX(), bounds.getY(),
                        down ? bottom.brighter(0.08f) : bottom,
                        bounds.getX(), bounds.getBottom(), false });
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    if (highlighted && !down)
    {
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 2.0f, 1.0f);
    }
}

void AurelineMainComponent::AurelineLookAndFeel::drawButtonText(
    juce::Graphics& g, juce::TextButton& button, bool highlighted, bool down)
{
    if (button.getName() != "voiceActionButton" && button.getName() != "voiceStepButton")
    {
        juce::LookAndFeel_V4::drawButtonText(g, button, highlighted, down);
        return;
    }

    const bool step = button.getName() == "voiceStepButton";
    const bool store = button.getButtonText() == "STORE";
    const bool recording = button.getButtonText() == "STOP";
    const bool editWave = button.getButtonText() == "EDIT WAVE";
    const bool saveAll = button.getButtonText() == "SAVE BANK";
    const bool headerAction = saveAll || button.getButtonText() == "WAV"
                              || recording;
    g.setFont(juce::FontOptions(editWave ? 11.0f : headerAction ? 11.0f
                                                       : step ? 14.0f : 9.0f,
                                juce::Font::bold));
    g.setColour(store && !down ? juce::Colours::black
                               : recording ? juce::Colours::white
                               : down ? juce::Colours::white
                                      : step ? juce::Colour(themeText)
                                             : juce::Colour(themeAmber));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(3, 1),
                     juce::Justification::centred, 1);
}

juce::Font AurelineMainComponent::AurelineLookAndFeel::getLabelFont(juce::Label& label)
{
    if (const auto* slider = dynamic_cast<const juce::Slider*>(label.getParentComponent());
        slider != nullptr && slider->getName() == "performanceKnob")
        return juce::Font(juce::FontOptions(10.0f));

    return juce::LookAndFeel_V4::getLabelFont(label);
}

AurelineMainComponent::Keyboard::Keyboard(AurelineMainComponent& ownerIn) : owner(ownerIn)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void AurelineMainComponent::Keyboard::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(0xff050403));
    const auto area = bounds.reduced(7.0f, 6.0f);
    const int whiteCount = whiteIndexForNote(firstKeyboardNote + keyboardNoteCount);
    const float whiteWidth = area.getWidth() / static_cast<float>(whiteCount);
    const float blackWidth = whiteWidth * 0.58f;
    const float blackHeight = area.getHeight() * 0.62f;

    for (int note = firstKeyboardNote; note < firstKeyboardNote + keyboardNoteCount; ++note)
    {
        if (isBlackKey(note))
            continue;
        const auto key = juce::Rectangle<float>(area.getX() + whiteWidth * static_cast<float>(whiteIndexForNote(note)) + 0.4f,
                                                 area.getY(), whiteWidth - 0.8f, area.getHeight());
        const bool down = note == heldNote || owner.isNoteHeld(note);
        g.setGradientFill({ down ? juce::Colour(0xffffd47a) : juce::Colour(0xfff5efe2),
                            key.getX(), key.getY(),
                            down ? juce::Colour(0xffd9992d) : juce::Colour(0xffcfc4ae),
                            key.getX(), key.getBottom(), false });
        g.fillRoundedRectangle(key, 1.5f);
        g.setColour(juce::Colour(0xff584a35));
        g.drawRoundedRectangle(key, 1.5f, 1.0f);
    }

    for (int note = firstKeyboardNote; note < firstKeyboardNote + keyboardNoteCount; ++note)
    {
        if (!isBlackKey(note))
            continue;
        const auto key = juce::Rectangle<float>(area.getX() + whiteWidth * static_cast<float>(whiteIndexForNote(note)) - blackWidth * 0.5f,
                                                 area.getY(), blackWidth, blackHeight);
        const bool down = note == heldNote || owner.isNoteHeld(note);
        g.setGradientFill({ down ? juce::Colour(0xff78551d) : juce::Colour(0xff252018),
                            key.getX(), key.getY(),
                            down ? juce::Colour(themeAmber) : juce::Colour(0xff050403),
                            key.getX(), key.getBottom(), false });
        g.fillRoundedRectangle(key, 1.5f);
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(key, 1.5f, 1.0f);
    }
}

int AurelineMainComponent::Keyboard::noteAt(juce::Point<int> position) const
{
    const auto area = getLocalBounds().toFloat().reduced(7.0f, 6.0f);
    const int whiteCount = whiteIndexForNote(firstKeyboardNote + keyboardNoteCount);
    const float whiteWidth = area.getWidth() / static_cast<float>(whiteCount);
    const float blackWidth = whiteWidth * 0.58f;
    const float blackHeight = area.getHeight() * 0.62f;
    const auto point = position.toFloat();
    if (point.y < area.getY() + blackHeight)
        for (int note = firstKeyboardNote; note < firstKeyboardNote + keyboardNoteCount; ++note)
            if (isBlackKey(note))
            {
                const auto key = juce::Rectangle<float>(area.getX() + whiteWidth * static_cast<float>(whiteIndexForNote(note)) - blackWidth * 0.5f,
                                                         area.getY(), blackWidth, blackHeight);
                if (key.contains(point))
                    return note;
            }
    const int wanted = juce::jlimit(0, whiteCount - 1,
        static_cast<int>((point.x - area.getX()) / whiteWidth));
    int white = 0;
    for (int note = firstKeyboardNote; note < firstKeyboardNote + keyboardNoteCount; ++note)
        if (!isBlackKey(note) && white++ == wanted)
            return note;
    return -1;
}

void AurelineMainComponent::Keyboard::setHeldNote(int note)
{
    if (heldNote == note)
        return;
    if (heldNote >= 0)
        owner.releaseNote(heldNote);
    heldNote = note;
    if (heldNote >= 0)
        owner.playNote(heldNote, 104);
    repaint();
}

void AurelineMainComponent::Keyboard::mouseDown(const juce::MouseEvent& e)
{
    owner.grabKeyboardFocus();
    setHeldNote(noteAt(e.getPosition()));
}
void AurelineMainComponent::Keyboard::mouseDrag(const juce::MouseEvent& e) { setHeldNote(noteAt(e.getPosition())); }
void AurelineMainComponent::Keyboard::mouseUp(const juce::MouseEvent&) { setHeldNote(-1); }
void AurelineMainComponent::Keyboard::mouseExit(const juce::MouseEvent&) { setHeldNote(-1); }

AurelineMainComponent::WaveformButton::WaveformButton(aureline::Waveform waveform)
    : juce::Button("waveform"), buttonWaveform(waveform)
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

AurelineMainComponent::WaveformButton::WaveformButton(aureline::LfoWaveform waveform)
    : juce::Button("lfo waveform"), buttonLfoWaveform(waveform), isLfoButton(true)
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

AurelineMainComponent::RockerButton::RockerButton(const juce::String& label)
    : juce::Button(label)
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void AurelineMainComponent::RockerButton::paintButton(juce::Graphics& g,
                                                       bool highlighted, bool down)
{
    auto area = getLocalBounds().toFloat();
    const auto labelArea = area.removeFromTop(16.0f);
    const auto housingHeight = juce::jmin(50.0f, area.getHeight());
    const auto housing = juce::Rectangle<float>(26.0f, housingHeight)
        .withCentre({ area.getCentreX(), area.getY() + housingHeight * 0.5f });
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(housing.translated(0.0f, 2.0f), 2.0f);
    g.setGradientFill({ highlighted || down ? juce::Colour(0xff42474a) : juce::Colour(0xff303538),
                        housing.getX(), housing.getY(), juce::Colour(0xff080a0b),
                        housing.getX(), housing.getBottom(), false });
    g.fillRoundedRectangle(housing, 2.0f);
    g.setColour(juce::Colour(0xff020303));
    g.drawRoundedRectangle(housing, 2.0f, 1.5f);
    g.setColour(juce::Colour(0xffa7adaf).withAlpha(0.32f));
    g.drawLine(housing.getX() + 2.0f, housing.getY() + 1.5f,
               housing.getRight() - 2.0f, housing.getY() + 1.5f, 0.9f);
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawLine(housing.getX() + 2.0f, housing.getBottom() - 1.5f,
               housing.getRight() - 2.0f, housing.getBottom() - 1.5f, 1.2f);

    const auto upper = housing.withHeight(housing.getHeight() * 0.42f).reduced(3.0f);
    const auto led = juce::Rectangle<float>(8.0f, 8.0f)
        .withCentre({ upper.getCentreX(), upper.getCentreY() - 1.0f });
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(led.expanded(1.5f));
    g.setColour(getToggleState() ? juce::Colour(0xffff321c) : juce::Colour(0xff35100c));
    g.fillEllipse(led);
    if (getToggleState())
    {
        g.setColour(juce::Colour(0xffffb08a).withAlpha(0.72f));
        g.fillEllipse(led.reduced(2.0f).translated(-0.8f, -0.8f));
    }

    const auto rocker = housing.withTrimmedTop(housing.getHeight() * 0.40f).reduced(3.0f, 2.0f);
    g.setGradientFill({ getToggleState() ? juce::Colour(0xff62686b) : juce::Colour(0xff4b5053),
                        rocker.getX(), rocker.getY(), juce::Colour(0xff111416),
                        rocker.getX(), rocker.getBottom(), false });
    g.fillRoundedRectangle(rocker, 1.5f);
    g.setGradientFill({ juce::Colour(0xff5b6062), rocker.getX(), rocker.getY(),
                        juce::Colour(0xff24282a), rocker.getRight(), rocker.getBottom(), false });
    g.drawRoundedRectangle(rocker, 1.5f, 0.8f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine(rocker.getX() + 1.5f, rocker.getY() + 1.5f,
               rocker.getRight() - 1.5f, rocker.getY() + 1.5f, 0.8f);
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.drawLine(rocker.getX() + 1.5f, rocker.getBottom() - 1.5f,
               rocker.getRight() - 1.5f, rocker.getBottom() - 1.5f, 1.0f);

    g.setColour(juce::Colour(0xffc7c9c8));
    const float labelSize = getButtonText().length() > 5 ? 8.0f : 10.0f;
    g.setFont(juce::FontOptions(labelSize, juce::Font::bold));
    g.drawText(getButtonText(), labelArea, juce::Justification::centred, false);
}

void AurelineMainComponent::WaveformButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto icon = bounds.removeFromTop(19.0f).reduced(7.0f, 1.0f).withTrimmedBottom(3.0f);
    juce::Path path;
    const bool sawUp = isLfoButton ? buttonLfoWaveform == aureline::LfoWaveform::sawUp
                                   : buttonWaveform == aureline::Waveform::saw;
    const bool triangle = isLfoButton ? buttonLfoWaveform == aureline::LfoWaveform::triangle
                                      : buttonWaveform == aureline::Waveform::triangle;
    const bool sawDown = isLfoButton && buttonLfoWaveform == aureline::LfoWaveform::sawDown;
    const bool sampleAndHold = isLfoButton
        && buttonLfoWaveform == aureline::LfoWaveform::sampleAndHold;
    const bool waveMemory = !isLfoButton
        && buttonWaveform == aureline::Waveform::waveMemory;
    if (waveMemory)
    {
        constexpr std::array<float, 9> steps { 0.72f, 0.25f, 0.48f, 0.08f, 0.38f,
                                               0.82f, 0.56f, 0.18f, 0.64f };
        const auto stepWidth = icon.getWidth() / static_cast<float>(steps.size());
        path.startNewSubPath(icon.getX(), icon.getY() + steps[0] * icon.getHeight());
        for (std::size_t index = 0; index < steps.size(); ++index)
        {
            const auto x = icon.getX() + static_cast<float>(index) * stepWidth;
            const auto y = icon.getY() + steps[index] * icon.getHeight();
            if (index > 0)
                path.lineTo(x, y);
            path.lineTo(x + stepWidth, y);
        }
    }
    else if (sawUp)
    {
        path.startNewSubPath(icon.getX(), icon.getBottom());
        path.lineTo(icon.getRight(), icon.getY());
        path.lineTo(icon.getRight(), icon.getBottom());
    }
    else if (triangle)
    {
        path.startNewSubPath(icon.getX(), icon.getBottom());
        path.lineTo(icon.getCentreX(), icon.getY());
        path.lineTo(icon.getRight(), icon.getBottom());
    }
    else if (sawDown)
    {
        path.startNewSubPath(icon.getX(), icon.getY());
        path.lineTo(icon.getRight(), icon.getBottom());
        path.lineTo(icon.getRight(), icon.getY());
    }
    else if (sampleAndHold)
    {
        const auto third = icon.getWidth() / 3.0f;
        path.startNewSubPath(icon.getX(), icon.getCentreY());
        path.lineTo(icon.getX() + third, icon.getCentreY());
        path.lineTo(icon.getX() + third, icon.getY());
        path.lineTo(icon.getX() + third * 2.0f, icon.getY());
        path.lineTo(icon.getX() + third * 2.0f, icon.getBottom());
        path.lineTo(icon.getRight(), icon.getBottom());
    }
    else
    {
        path.startNewSubPath(icon.getX(), icon.getBottom());
        path.lineTo(icon.getX(), icon.getY());
        path.lineTo(icon.getCentreX(), icon.getY());
        path.lineTo(icon.getCentreX(), icon.getBottom());
        path.lineTo(icon.getRight(), icon.getBottom());
    }
    g.setColour(getToggleState() ? juce::Colour(0xffeef1f0) : juce::Colour(0xff777c7e));
    g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    bounds.removeFromTop(3.0f);
    const auto housing = bounds.withSizeKeepingCentre(26.0f, bounds.getHeight() - 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(housing.translated(0.0f, 2.0f), 2.0f);
    g.setGradientFill({ highlighted || down ? juce::Colour(0xff42474a) : juce::Colour(0xff303538),
                        housing.getX(), housing.getY(), juce::Colour(0xff080a0b),
                        housing.getX(), housing.getBottom(), false });
    g.fillRoundedRectangle(housing, 2.0f);
    g.setColour(juce::Colour(0xff020303));
    g.drawRoundedRectangle(housing, 2.0f, 1.3f);
    g.setColour(juce::Colour(0xffa7adaf).withAlpha(0.32f));
    g.drawLine(housing.getX() + 2.0f, housing.getY() + 1.5f,
               housing.getRight() - 2.0f, housing.getY() + 1.5f, 0.9f);
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawLine(housing.getX() + 2.0f, housing.getBottom() - 1.5f,
               housing.getRight() - 2.0f, housing.getBottom() - 1.5f, 1.2f);

    const auto led = juce::Rectangle<float>(8.0f, 8.0f)
        .withCentre({ housing.getCentreX(), housing.getY() + 8.0f });
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(led.expanded(1.5f));
    g.setColour(getToggleState() ? juce::Colour(0xffff321c) : juce::Colour(0xff35100c));
    g.fillEllipse(led);
    if (getToggleState())
    {
        g.setColour(juce::Colour(0xffffb08a).withAlpha(0.72f));
        g.fillEllipse(led.reduced(2.0f).translated(-0.8f, -0.8f));
    }

    const auto rocker = housing.withTrimmedTop(15.0f).reduced(3.0f, 2.0f);
    g.setGradientFill({ getToggleState() ? juce::Colour(0xff62686b) : juce::Colour(0xff4b5053),
                        rocker.getX(), rocker.getY(), juce::Colour(0xff111416),
                        rocker.getX(), rocker.getBottom(), false });
    g.fillRoundedRectangle(rocker, 1.5f);
    g.setGradientFill({ juce::Colour(0xff5b6062), rocker.getX(), rocker.getY(),
                        juce::Colour(0xff24282a), rocker.getRight(), rocker.getBottom(), false });
    g.drawRoundedRectangle(rocker, 1.5f, 0.8f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine(rocker.getX() + 1.5f, rocker.getY() + 1.5f,
               rocker.getRight() - 1.5f, rocker.getY() + 1.5f, 0.8f);
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.drawLine(rocker.getX() + 1.5f, rocker.getBottom() - 1.5f,
               rocker.getRight() - 1.5f, rocker.getBottom() - 1.5f, 1.0f);
}

class AurelineMainComponent::WaveMemoryEditorWindow final : public juce::Component
{
    class Editor final : public juce::Component,
                         private juce::Slider::Listener,
                         private juce::ComboBox::Listener,
                         private juce::Button::Listener,
                         private juce::KeyListener
    {
    public:
        using juce::Component::keyPressed;

        Editor(AurelineMainComponent& ownerIn, std::size_t oscillatorIn)
            : owner(ownerIn), oscillator(oscillatorIn)
        {
            setWantsKeyboardFocus(true);
            setMouseClickGrabsKeyboardFocus(false);
            memoryBox.setName("voiceSelector");
            for (std::size_t index = 0; index < aureline::kWaveMemoryFactoryCount; ++index)
                memoryBox.addItem(aureline::waveMemoryFactoryNames()[index],
                                  static_cast<int>(index) + 1);
            memoryBox.setSelectedId((oscillator == 0
                ? owner.parameters.waveMemoryIndexA.load()
                : owner.parameters.waveMemoryIndexB.load()) + 1,
                juce::dontSendNotification);
            memoryBox.addListener(this);
            addAndMakeVisible(memoryBox);

            for (const auto* name : { "5-BIT", "4-BIT", "SMOOTH" })
                characterBox.addItem(name, characterBox.getNumItems() + 1);
            characterBox.setName("voiceSelector");
            characterBox.setSelectedId((oscillator == 0
                ? owner.parameters.waveMemoryCharacterA.load()
                : owner.parameters.waveMemoryCharacterB.load()) + 1,
                juce::dontSendNotification);
            characterBox.addListener(this);
            addAndMakeVisible(characterBox);

            for (std::size_t index = 0; index < steps.size(); ++index)
            {
                auto& slider = steps[index];
                slider.setSliderStyle(juce::Slider::LinearVertical);
                slider.setName("waveMemoryStep");
                slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                slider.setRange(0.0, 31.0, 1.0);
                slider.setValue(owner.userWaveMemory[oscillator][index],
                                juce::dontSendNotification);
                slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff080b0a));
                slider.setColour(juce::Slider::trackColourId, juce::Colour(themeAmber));
                slider.setInterceptsMouseClicks(false, false);
                slider.addListener(this);
                addAndMakeVisible(slider);
            }

            stepInput.setSliderStyle(juce::Slider::LinearBar);
            stepInput.setRange(1.0, 32.0, 1.0);
            stepInput.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 46, 24);
            stepInput.setValue(1.0, juce::dontSendNotification);
            stepInput.addListener(this);
            stepInput.setWantsKeyboardFocus(true);
            addAndMakeVisible(stepInput);

            valueInput.setSliderStyle(juce::Slider::LinearBar);
            valueInput.setRange(0.0, 31.0, 1.0);
            valueInput.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 24);
            valueInput.setValue(owner.userWaveMemory[oscillator][0],
                                juce::dontSendNotification);
            valueInput.addListener(this);
            valueInput.setWantsKeyboardFocus(true);
            valueInput.addKeyListener(this);
            addAndMakeVisible(valueInput);

            for (auto* button : { &auditionButton, &copyButton, &pasteButton,
                                  &initButton, &loadWaveButton, &saveWaveButton,
                                  &stepPreviousButton, &stepNextButton,
                                  &closeButton })
            {
                button->setName("voiceStepButton");
                button->addListener(this);
                addAndMakeVisible(*button);
            }
            pasteButton.setEnabled(owner.hasCopiedWaveMemory);
            setSize(760, 390);
        }

        ~Editor() override
        {
            valueInput.removeKeyListener(this);
            stopAudition();
        }

        void prepareToClose() { stopAudition(); }

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (juce::Rectangle<float>(18.0f, 52.0f, 566.0f, 292.0f)
                    .contains(event.position))
                grabKeyboardFocus();
            lastDrawnStep = -1;
            lastDrawnValue = -1;
            drawWaveAt(event.position);
        }

        void mouseDrag(const juce::MouseEvent& event) override
        {
            drawWaveAt(event.position);
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            lastDrawnStep = -1;
            lastDrawnValue = -1;
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff0c100f));
            g.setColour(juce::Colour(themeAmber));
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText("WAVE MEMORY " + juce::String(oscillator == 0 ? "A" : "B"),
                       18, 12, 420, 28, juce::Justification::centredLeft);
            g.setColour(juce::Colour(0xffaeb2b0));
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText("32 STEPS", 18, getHeight() - 30, 420, 18,
                       juce::Justification::centredLeft);
            g.setColour(juce::Colour(0xff6d7170));
            g.drawRoundedRectangle(juce::Rectangle<float>(16.0f, 48.0f, 570.0f, 300.0f),
                                   5.0f, 1.0f);
            const auto selectedX = 18.0f
                + static_cast<float>(selectedStep) * (566.0f / 32.0f);
            const auto selectedWidth = 566.0f / 32.0f;
            g.setColour(juce::Colour(themeAmber).withAlpha(0.14f));
            g.fillRect(selectedX, 52.0f, selectedWidth, 292.0f);
            g.setColour(juce::Colour(themeAmber).withAlpha(0.88f));
            g.drawRect(juce::Rectangle<float>(
                selectedX, 52.0f, selectedWidth, 292.0f), 1.2f);
            for (int line = 1; line < 4; ++line)
            {
                const auto y = 48.0f + 300.0f * static_cast<float>(line) / 4.0f;
                g.setColour(juce::Colours::white.withAlpha(line == 2 ? 0.16f : 0.07f));
                g.drawHorizontalLine(juce::roundToInt(y), 17.0f, 585.0f);
            }
            g.setColour(juce::Colour(0xffaeb2b0));
            g.drawText("MEMORY", 610, 48, 130, 18, juce::Justification::centred);
            g.drawText("CHARACTER", 610, 104, 130, 18, juce::Justification::centred);
            g.drawText("STEP", 610, 154, 30, 18, juce::Justification::centred);
            g.drawText("VALUE", 696, 154, 44, 18, juce::Justification::centred);
        }

        void resized() override
        {
            constexpr int canvasX = 18;
            constexpr int canvasY = 52;
            constexpr int canvasWidth = 566;
            constexpr int canvasHeight = 292;
            const auto stepWidth = static_cast<float>(canvasWidth)
                                 / static_cast<float>(steps.size());
            for (std::size_t index = 0; index < steps.size(); ++index)
                steps[index].setBounds(juce::roundToInt(
                                           canvasX + static_cast<float>(index) * stepWidth),
                                       canvasY, juce::jmax(3, juce::roundToInt(stepWidth)),
                                       canvasHeight);
            memoryBox.setBounds(610, 68, 130, 28);
            characterBox.setBounds(610, 124, 130, 28);
            stepInput.setBounds(610, 172, 30, 28);
            stepPreviousButton.setBounds(644, 172, 20, 28);
            stepNextButton.setBounds(668, 172, 20, 28);
            valueInput.setBounds(696, 172, 44, 28);
            auditionButton.setBounds(610, 207, 130, 30);
            copyButton.setBounds(610, 243, 62, 28);
            pasteButton.setBounds(678, 243, 62, 28);
            initButton.setBounds(610, 277, 130, 28);
            loadWaveButton.setBounds(610, 311, 62, 28);
            saveWaveButton.setBounds(678, 311, 62, 28);
            closeButton.setBounds(610, 345, 130, 28);
        }

    private:
        void sliderValueChanged(juce::Slider* slider) override
        {
            if (refreshingNumericInputs)
                return;
            if (slider == &stepInput)
            {
                selectedStep = juce::jlimit(
                    0, 31, juce::roundToInt(stepInput.getValue()) - 1);
                refreshNumericInputs();
                repaint();
                return;
            }
            if (slider == &valueInput)
            {
                const auto value = juce::jlimit(
                    0, 31, juce::roundToInt(valueInput.getValue()));
                owner.userWaveMemory[oscillator][static_cast<std::size_t>(selectedStep)]
                    = static_cast<std::uint8_t>(value);
                steps[static_cast<std::size_t>(selectedStep)].setValue(
                    value, juce::dontSendNotification);
                owner.userWaveMemoryActive[oscillator] = true;
                owner.applyParameters();
                owner.repaint();
                repaint();
                return;
            }
            for (std::size_t index = 0; index < steps.size(); ++index)
                if (slider == &steps[index])
                {
                    owner.userWaveMemory[oscillator][index] = static_cast<std::uint8_t>(
                        juce::jlimit(0, 31, juce::roundToInt(slider->getValue())));
                    owner.userWaveMemoryActive[oscillator] = true;
                    owner.applyParameters();
                    owner.repaint();
                    return;
                }
        }

        void comboBoxChanged(juce::ComboBox* box) override
        {
            if (box == &memoryBox)
            {
                const auto selected = juce::jlimit(0,
                    static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1,
                    memoryBox.getSelectedId() - 1);
                owner.userWaveMemory[oscillator] =
                    aureline::waveMemoryFactoryBank()[static_cast<std::size_t>(selected)];
                owner.userWaveMemoryActive[oscillator] = false;
                (oscillator == 0 ? owner.parameters.waveMemoryIndexA
                                 : owner.parameters.waveMemoryIndexB).store(selected);
                owner.waveMemoryBoxes[oscillator].setSelectedId(selected + 1,
                                                               juce::dontSendNotification);
                reloadSteps();
            }
            else if (box == &characterBox)
            {
                const auto value = juce::jlimit(0, 2, characterBox.getSelectedId() - 1);
                (oscillator == 0 ? owner.parameters.waveMemoryCharacterA
                                 : owner.parameters.waveMemoryCharacterB).store(value);
                owner.waveCharacterBoxes[oscillator].setSelectedId(value + 1,
                                                                  juce::dontSendNotification);
            }
            owner.applyParameters();
            owner.repaint();
        }

        void buttonClicked(juce::Button* button) override
        {
            if (button == &copyButton)
            {
                owner.copiedWaveMemory = owner.userWaveMemory[oscillator];
                owner.hasCopiedWaveMemory = true;
                pasteButton.setEnabled(true);
            }
            else if (button == &pasteButton && owner.hasCopiedWaveMemory)
            {
                owner.userWaveMemory[oscillator] = owner.copiedWaveMemory;
                owner.userWaveMemoryActive[oscillator] = true;
                reloadSteps();
                owner.applyParameters();
            }
            else if (button == &initButton)
            {
                owner.userWaveMemory[oscillator] = aureline::waveMemoryFactoryBank()[0];
                owner.userWaveMemoryActive[oscillator] = false;
                memoryBox.setSelectedId(1, juce::dontSendNotification);
                (oscillator == 0 ? owner.parameters.waveMemoryIndexA
                                 : owner.parameters.waveMemoryIndexB).store(0);
                owner.waveMemoryBoxes[oscillator].setSelectedId(1, juce::dontSendNotification);
                reloadSteps();
                owner.applyParameters();
            }
            else if (button == &auditionButton)
            {
                auditioning ? stopAudition() : startAudition();
            }
            else if (button == &saveWaveButton)
            {
                saveWave();
            }
            else if (button == &loadWaveButton)
            {
                loadWave();
            }
            else if (button == &stepPreviousButton)
            {
                selectedStep = juce::jmax(0, selectedStep - 1);
                refreshNumericInputs();
            }
            else if (button == &stepNextButton)
            {
                selectedStep = juce::jmin(31, selectedStep + 1);
                refreshNumericInputs();
            }
            else if (button == &closeButton)
            {
                stopAudition();
                if (auto* window = findParentComponentOfClass<WaveMemoryEditorWindow>())
                    window->closeEditor();
            }
            owner.repaint();
            repaint();
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::leftKey
                || key == juce::KeyPress::rightKey)
            {
                selectedStep = juce::jlimit(
                    0, 31, selectedStep
                        + (key == juce::KeyPress::rightKey ? 1 : -1));
                refreshNumericInputs();
                repaint();
                return true;
            }
            if (key == juce::KeyPress::upKey
                || key == juce::KeyPress::downKey)
            {
                valueInput.setValue(
                    juce::jlimit(
                        0, 31, juce::roundToInt(valueInput.getValue())
                                   + (key == juce::KeyPress::upKey ? 1 : -1)),
                    juce::sendNotificationSync);
                return true;
            }
            return false;
        }

        bool keyPressed(const juce::KeyPress& key,
                        juce::Component*) override
        {
            if (!valueInput.hasKeyboardFocus(true)
                || (key != juce::KeyPress::leftKey
                    && key != juce::KeyPress::rightKey))
                return false;

            selectedStep = juce::jlimit(
                0, 31, selectedStep
                    + (key == juce::KeyPress::rightKey ? 1 : -1));
            refreshNumericInputs();
            repaint();
            return true;
        }

        void reloadSteps()
        {
            for (std::size_t index = 0; index < steps.size(); ++index)
                steps[index].setValue(owner.userWaveMemory[oscillator][index],
                                      juce::dontSendNotification);
            refreshNumericInputs();
            repaint();
        }

        void refreshNumericInputs()
        {
            const juce::ScopedValueSetter<bool> guard(
                refreshingNumericInputs, true);
            stepInput.setValue(selectedStep + 1, juce::dontSendNotification);
            valueInput.setValue(
                owner.userWaveMemory[oscillator][static_cast<std::size_t>(
                    selectedStep)],
                juce::dontSendNotification);
        }

        void drawWaveAt(juce::Point<float> position)
        {
            constexpr float canvasX = 18.0f;
            constexpr float canvasY = 52.0f;
            constexpr float canvasWidth = 566.0f;
            constexpr float canvasHeight = 292.0f;
            if (position.x < canvasX || position.x > canvasX + canvasWidth
                || position.y < canvasY || position.y > canvasY + canvasHeight)
                return;

            const auto index = juce::jlimit(0, 31, static_cast<int>(
                (position.x - canvasX) / canvasWidth * 32.0f));
            auto value = juce::jlimit(0, 31, juce::roundToInt(
                (canvasY + canvasHeight - position.y) / canvasHeight * 31.0f));
            const auto character = oscillator == 0
                ? owner.parameters.waveMemoryCharacterA.load()
                : owner.parameters.waveMemoryCharacterB.load();
            if (character == 1)
            {
                const auto fourBit = juce::roundToInt(
                    static_cast<double>(value) / 31.0 * 15.0);
                value = juce::roundToInt(
                    static_cast<double>(fourBit) * 31.0 / 15.0);
            }

            const auto setValue = [this](int stepIndex, int stepValue)
            {
                const auto clamped = juce::jlimit(0, 31, stepValue);
                owner.userWaveMemory[oscillator][static_cast<std::size_t>(stepIndex)]
                    = static_cast<std::uint8_t>(clamped);
                steps[static_cast<std::size_t>(stepIndex)].setValue(
                    clamped, juce::dontSendNotification);
            };

            if (lastDrawnStep >= 0 && lastDrawnStep != index)
            {
                const auto distance = std::abs(index - lastDrawnStep);
                for (int offset = 0; offset <= distance; ++offset)
                {
                    const auto stepIndex = lastDrawnStep
                        + (index > lastDrawnStep ? offset : -offset);
                    const auto amount = static_cast<double>(offset) / distance;
                    setValue(stepIndex, juce::roundToInt(
                        lastDrawnValue + (value - lastDrawnValue) * amount));
                }
            }
            else
                setValue(index, value);

            lastDrawnStep = index;
            lastDrawnValue = value;
            selectedStep = index;
            refreshNumericInputs();
            owner.userWaveMemoryActive[oscillator] = true;
            owner.applyParameters();
            owner.repaint();
            repaint();
        }

        void saveWave()
        {
            waveFileChooser = std::make_unique<juce::FileChooser>(
                "Save Aureline wave",
                aurelineDocumentsDirectory().getChildFile(
                    oscillator == 0 ? "Wave A.aurelinewave"
                                    : "Wave B.aurelinewave"),
                "*.aurelinewave");
            waveFileChooser->launchAsync(
                juce::FileBrowserComponent::saveMode
                    | juce::FileBrowserComponent::canSelectFiles
                    | juce::FileBrowserComponent::warnAboutOverwriting,
                [safe = juce::Component::SafePointer<Editor>(this)](
                    const juce::FileChooser& chooser)
                {
                    if (safe == nullptr)
                        return;
                    auto file = chooser.getResult();
                    if (file == juce::File {})
                        return;
                    if (file.getFileExtension() != ".aurelinewave")
                        file = file.withFileExtension(".aurelinewave");
                    juce::XmlElement wave("AURELINE_WAVE");
                    wave.setAttribute("version", 1);
                    wave.setAttribute("character",
                        safe->oscillator == 0
                            ? safe->owner.parameters.waveMemoryCharacterA.load()
                            : safe->owner.parameters.waveMemoryCharacterB.load());
                    juce::StringArray values;
                    for (const auto value :
                         safe->owner.userWaveMemory[safe->oscillator])
                        values.add(juce::String(static_cast<int>(value)));
                    wave.setAttribute("steps", values.joinIntoString(","));
                    file.replaceWithText(wave.toString());
                });
        }

        void loadWave()
        {
            waveFileChooser = std::make_unique<juce::FileChooser>(
                "Load Aureline wave", aurelineDocumentsDirectory(),
                "*.aurelinewave");
            waveFileChooser->launchAsync(
                juce::FileBrowserComponent::openMode
                    | juce::FileBrowserComponent::canSelectFiles,
                [safe = juce::Component::SafePointer<Editor>(this)](
                    const juce::FileChooser& chooser)
                {
                    if (safe == nullptr)
                        return;
                    const auto file = chooser.getResult();
                    if (file == juce::File {})
                        return;
                    const auto wave = juce::XmlDocument::parse(file);
                    if (wave == nullptr || !wave->hasTagName("AURELINE_WAVE"))
                        return;
                    juce::StringArray values;
                    values.addTokens(wave->getStringAttribute("steps"), ",", "");
                    if (values.size() != 32)
                        return;
                    for (int index = 0; index < 32; ++index)
                        safe->owner.userWaveMemory[safe->oscillator][
                            static_cast<std::size_t>(index)]
                            = static_cast<std::uint8_t>(juce::jlimit(
                                0, 31, values[index].getIntValue()));
                    const auto character = juce::jlimit(
                        0, 2, wave->getIntAttribute("character", 0));
                    (safe->oscillator == 0
                         ? safe->owner.parameters.waveMemoryCharacterA
                         : safe->owner.parameters.waveMemoryCharacterB).store(
                             character);
                    safe->characterBox.setSelectedId(
                        character + 1, juce::dontSendNotification);
                    safe->owner.waveCharacterBoxes[safe->oscillator].setSelectedId(
                        character + 1, juce::dontSendNotification);
                    safe->owner.userWaveMemoryActive[safe->oscillator] = true;
                    safe->reloadSteps();
                    safe->owner.applyParameters();
                    safe->owner.repaint();
                });
        }

        void startAudition()
        {
            restoreMaskA = owner.parameters.waveformMaskA.load();
            restoreMaskB = owner.parameters.waveformMaskB.load();
            restoreLevelA = owner.parameters.oscillatorALevel.load();
            restoreLevelB = owner.parameters.oscillatorBLevel.load();
            restoreNoise = owner.parameters.noiseLevel.load();
            owner.parameters.waveformMaskA.store(oscillator == 0 ? 8 : restoreMaskA);
            owner.parameters.waveformMaskB.store(oscillator == 1 ? 8 : restoreMaskB);
            owner.parameters.oscillatorALevel.store(oscillator == 0 ? 1.0f : 0.0f);
            owner.parameters.oscillatorBLevel.store(oscillator == 1 ? 1.0f : 0.0f);
            owner.parameters.noiseLevel.store(0.0f);
            owner.applyParameters();
            owner.playNote(60, 100);
            auditioning = true;
            auditionButton.setButtonText("STOP C4");
        }

        void stopAudition()
        {
            if (!auditioning)
                return;
            owner.heldNotes[60].store(false);
            owner.midiCollector.addMessageToQueue(
                juce::MidiMessage::allSoundOff(1));
            owner.parameters.waveformMaskA.store(restoreMaskA);
            owner.parameters.waveformMaskB.store(restoreMaskB);
            owner.parameters.oscillatorALevel.store(restoreLevelA);
            owner.parameters.oscillatorBLevel.store(restoreLevelB);
            owner.parameters.noiseLevel.store(restoreNoise);
            owner.applyParameters();
            auditioning = false;
            auditionButton.setButtonText("AUDITION C4");
        }

        AurelineMainComponent& owner;
        const std::size_t oscillator;
        std::array<juce::Slider, aureline::kWaveMemorySize> steps;
        juce::Slider stepInput;
        juce::Slider valueInput;
        juce::ComboBox memoryBox;
        juce::ComboBox characterBox;
        juce::TextButton auditionButton { "AUDITION C4" };
        juce::TextButton copyButton { "COPY" };
        juce::TextButton pasteButton { "PASTE" };
        juce::TextButton initButton { "INIT" };
        juce::TextButton loadWaveButton { "LOAD" };
        juce::TextButton saveWaveButton { "SAVE" };
        juce::TextButton stepPreviousButton { "<" };
        juce::TextButton stepNextButton { ">" };
        juce::TextButton closeButton { "CLOSE" };
        std::unique_ptr<juce::FileChooser> waveFileChooser;
        bool auditioning = false;
        bool refreshingNumericInputs = false;
        int selectedStep = 0;
        int lastDrawnStep = -1;
        int lastDrawnValue = -1;
        int restoreMaskA = 0, restoreMaskB = 0;
        float restoreLevelA = 0.0f, restoreLevelB = 0.0f, restoreNoise = 0.0f;
    };

public:
    WaveMemoryEditorWindow(AurelineMainComponent& owner, std::size_t oscillator)
    {
        editor = std::make_unique<Editor>(owner, oscillator);
        addAndMakeVisible(*editor);
        setInterceptsMouseClicks(true, true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.72f));
    }

    void resized() override
    {
        editor->setBounds(getLocalBounds().withSizeKeepingCentre(
            juce::jmin(760, getWidth() - 32),
            juce::jmin(390, getHeight() - 32)));
    }

    void closeEditor()
    {
        editor->prepareToClose();
        setVisible(false);
    }

private:
    std::unique_ptr<Editor> editor;
};

#if 0 // Removed prototype step sequencer UI.
class AurelineMainComponent::StepSequencerEditor final : public juce::Component,
                                                         private juce::Timer
{
public:
    explicit StepSequencerEditor(AurelineMainComponent& ownerIn) : owner(ownerIn)
    {
        lengthBox.setName("voiceSelector");
        for (const auto length : { 16, 32, 48, 64 })
            lengthBox.addItem(juce::String(length) + " STEPS", length);
        lengthBox.setSelectedId(owner.stepPattern.length, juce::dontSendNotification);
        lengthBox.onChange = [this]
        {
            owner.stepPattern.length = lengthBox.getSelectedId();
            page = juce::jmin(page, owner.stepPattern.length / 16 - 1);
            apply();
        };
        addAndMakeVisible(lengthBox);

        resolutionBox.setName("voiceSelector");
        for (const auto* text : { "1/8", "1/16", "1/32" })
            resolutionBox.addItem(text, resolutionBox.getNumItems() + 1);
        resolutionBox.setSelectedId(owner.stepPattern.resolution + 1,
                                    juce::dontSendNotification);
        resolutionBox.onChange = [this]
        {
            owner.stepPattern.resolution = resolutionBox.getSelectedId() - 1;
            apply();
        };
        addAndMakeVisible(resolutionBox);

        modeBox.setName("voiceSelector");
        for (const auto* text : { "NOTE", "REST", "TIE" })
            modeBox.addItem(text, modeBox.getNumItems() + 1);
        modeBox.onChange = [this]
        {
            step().mode = static_cast<aureline::StepMode>(modeBox.getSelectedId() - 1);
            apply();
        };
        addAndMakeVisible(modeBox);

        configureEditorSlider(pitchSlider, -24, 24);
        configureEditorSlider(velocitySlider, 1, 127);
        configureEditorSlider(gateSlider, 10, 100);
        pitchSlider.onValueChange = [this] { step().pitch = juce::roundToInt(pitchSlider.getValue()); apply(); };
        velocitySlider.onValueChange = [this] { step().velocity = juce::roundToInt(velocitySlider.getValue()); apply(); };
        gateSlider.onValueChange = [this] { step().gate = juce::roundToInt(gateSlider.getValue()); apply(); };

        for (std::size_t index = 0; index < stepButtons.size(); ++index)
        {
            stepButtons[index].setName("voiceStepButton");
            stepButtons[index].onClick = [this, index]
            {
                selected = page * 16 + static_cast<int>(index);
                refreshControls();
            };
            addAndMakeVisible(stepButtons[index]);
        }
        for (auto* button : { &previousPageButton, &nextPageButton, &playButton,
                              &clearButton, &initButton, &closeButton })
        {
            button->setName("voiceStepButton");
            addAndMakeVisible(*button);
        }
        previousPageButton.onClick = [this] { page = juce::jmax(0, page - 1); refreshControls(); };
        nextPageButton.onClick = [this]
        {
            page = juce::jmin(owner.stepPattern.length / 16 - 1, page + 1);
            refreshControls();
        };
        playButton.onClick = [this]
        {
            owner.stepPattern.enabled = !owner.stepPattern.enabled;
            owner.sequencerButton.setToggleState(owner.stepPattern.enabled,
                                                 juce::dontSendNotification);
            if (owner.stepPattern.enabled)
            {
                owner.parameters.arpEnabled.store(false);
                owner.arpButton.setToggleState(false, juce::dontSendNotification);
            }
            else
                owner.stepSequencer.stop(owner.engine);
            apply();
        };
        clearButton.onClick = [this]
        {
            for (auto& item : owner.stepPattern.steps)
                item.mode = aureline::StepMode::rest;
            apply();
        };
        initButton.onClick = [this]
        {
            const auto enabled = owner.stepPattern.enabled;
            owner.stepPattern = {};
            owner.stepPattern.enabled = enabled;
            page = 0;
            selected = 0;
            lengthBox.setSelectedId(16, juce::dontSendNotification);
            resolutionBox.setSelectedId(2, juce::dontSendNotification);
            apply();
        };
        closeButton.onClick = [this] { setVisible(false); };
        refreshControls();
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.74f));
        const auto panel = panelBounds().toFloat();
        g.setColour(juce::Colour(0xff101413));
        g.fillRoundedRectangle(panel, 7.0f);
        g.setColour(juce::Colour(0xff765d28));
        g.drawRoundedRectangle(panel, 7.0f, 1.5f);
        g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
        g.setColour(juce::Colour(themeAmber));
        g.drawText("STEP SEQUENCER", panel.withHeight(42.0f).reduced(18.0f, 0.0f),
                   juce::Justification::centredLeft);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xffaeb2b0));
        g.drawText("PITCH", labelBounds(0), juce::Justification::centred);
        g.drawText("VELOCITY", labelBounds(1), juce::Justification::centred);
        g.drawText("GATE", labelBounds(2), juce::Justification::centred);
        const auto firstStep = page * 16 + 1;
        g.drawText(juce::String(firstStep).paddedLeft('0', 2) + "-"
                       + juce::String(firstStep + 15).paddedLeft('0', 2)
                       + " / " + juce::String(owner.stepPattern.length),
                   panelBounds().getX() + 18, panelBounds().getY() + 47,
                   150, 16, juce::Justification::centredLeft);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        for (std::size_t index = 0; index < stepButtons.size(); ++index)
        {
            const auto absolute = page * 16 + static_cast<int>(index) + 1;
            g.setColour(juce::Colour(themeAmber));
            g.drawText(juce::String(absolute).paddedLeft('0', 2),
                       stepButtons[index].getBounds().reduced(4, 3).withHeight(12),
                       juce::Justification::topLeft);
        }
        const auto current = owner.stepSequencer.currentStep();
        if (current >= page * 16 && current < page * 16 + 16)
        {
            const auto index = current - page * 16;
            g.setColour(juce::Colour(themeAmber).withAlpha(0.42f));
            g.drawRoundedRectangle(stepButtons[static_cast<std::size_t>(index)]
                                       .getBounds().toFloat().expanded(2.0f),
                                   4.0f, 2.0f);
        }
    }

    void resized() override
    {
        const auto panel = panelBounds();
        lengthBox.setBounds(panel.getX() + 220, panel.getY() + 10, 120, 28);
        resolutionBox.setBounds(panel.getX() + 350, panel.getY() + 10, 90, 28);
        playButton.setBounds(panel.getRight() - 184, panel.getY() + 10, 78, 28);
        closeButton.setBounds(panel.getRight() - 98, panel.getY() + 10, 80, 28);
        const int gridX = panel.getX() + 18;
        const int gridY = panel.getY() + 70;
        const int gridWidth = panel.getWidth() - 36;
        const int cellWidth = gridWidth / 16;
        for (std::size_t index = 0; index < stepButtons.size(); ++index)
            stepButtons[index].setBounds(gridX + static_cast<int>(index) * cellWidth,
                                         gridY, cellWidth - 3, 82);
        previousPageButton.setBounds(panel.getX() + 18, gridY + 94, 42, 28);
        nextPageButton.setBounds(panel.getX() + 66, gridY + 94, 42, 28);
        modeBox.setBounds(panel.getX() + 130, gridY + 94, 100, 28);
        pitchSlider.setBounds(panel.getX() + 270, gridY + 92, 120, 46);
        velocitySlider.setBounds(panel.getX() + 420, gridY + 92, 120, 46);
        gateSlider.setBounds(panel.getX() + 570, gridY + 92, 120, 46);
        clearButton.setBounds(panel.getRight() - 180, gridY + 96, 74, 28);
        initButton.setBounds(panel.getRight() - 98, gridY + 96, 80, 28);
    }

private:
    void configureEditorSlider(juce::Slider& slider, double minimum, double maximum)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setRange(minimum, maximum, 1.0);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
        addAndMakeVisible(slider);
    }

    juce::Rectangle<int> panelBounds() const
    {
        return getLocalBounds().withSizeKeepingCentre(
            juce::jmin(920, getWidth() - 30), juce::jmin(310, getHeight() - 30));
    }

    juce::Rectangle<int> labelBounds(int index) const
    {
        const auto panel = panelBounds();
        return { panel.getX() + 270 + index * 150, panel.getY() + 156, 120, 16 };
    }

    aureline::StepData& step()
    {
        return owner.stepPattern.steps[static_cast<std::size_t>(
            juce::jlimit(0, 63, selected))];
    }

    void apply()
    {
        owner.stepSequencer.setPattern(owner.stepPattern);
        owner.stepSequencer.setTempo(owner.parameters.tempoBpm.load());
        refreshControls();
        owner.repaint();
    }

    void refreshControls()
    {
        selected = juce::jlimit(page * 16, page * 16 + 15, selected);
        for (std::size_t index = 0; index < stepButtons.size(); ++index)
        {
            const auto absolute = page * 16 + static_cast<int>(index);
            const auto& item = owner.stepPattern.steps[static_cast<std::size_t>(absolute)];
            const auto mode = item.mode == aureline::StepMode::rest ? "R"
                            : item.mode == aureline::StepMode::tie ? "T"
                            : (item.pitch >= 0 ? "+" : "") + juce::String(item.pitch);
            stepButtons[index].setButtonText(mode);
            stepButtons[index].setToggleState(absolute == selected,
                                              juce::dontSendNotification);
        }
        const juce::ScopedValueSetter<bool> guard(refreshing, true);
        pitchSlider.setValue(step().pitch, juce::dontSendNotification);
        velocitySlider.setValue(step().velocity, juce::dontSendNotification);
        gateSlider.setValue(step().gate, juce::dontSendNotification);
        modeBox.setSelectedId(static_cast<int>(step().mode) + 1,
                              juce::dontSendNotification);
        previousPageButton.setEnabled(page > 0);
        nextPageButton.setEnabled(page + 1 < owner.stepPattern.length / 16);
        playButton.setButtonText(owner.stepPattern.enabled ? "STOP" : "PLAY");
        repaint();
    }

    void timerCallback() override { repaint(); }

    AurelineMainComponent& owner;
    std::array<juce::TextButton, 16> stepButtons;
    juce::ComboBox lengthBox, resolutionBox, modeBox;
    juce::Slider pitchSlider, velocitySlider, gateSlider;
    juce::TextButton previousPageButton { "<" }, nextPageButton { ">" };
    juce::TextButton playButton { "PLAY" }, clearButton { "CLEAR" };
    juce::TextButton initButton { "INIT" }, closeButton { "CLOSE" };
    int page = 0;
    int selected = 0;
    bool refreshing = false;
};
#endif

AurelineMainComponent::AurelineMainComponent(bool useStandaloneAudio)
    : ownsStandaloneAudio(useStandaloneAudio), keyboard(*this)
{
    woodBackground = juce::ImageFileFormat::loadFrom(
        BinaryData::aurelinewoodbackground_png,
        BinaryData::aurelinewoodbackground_pngSize);
    setLookAndFeel(&lookAndFeel);
    lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(themePanel));
    lookAndFeel.setColour(juce::PopupMenu::textColourId, juce::Colour(themeText));
    lookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(themeAmber));
    lookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    titleLabel.setText("AURELINE", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(themeAmber));
    titleLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(titleLabel);
    subtitleLabel.setText("8-VOICE ANALOG MODELING SYNTHESIZER", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(themeGold));
    subtitleLabel.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(subtitleLabel);
    statusLabel.setText("Audio: starting  |  MIDI: starting", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(themeGold));
    statusLabel.setJustificationType(juce::Justification::bottomRight);
    addAndMakeVisible(statusLabel);

    presetBox.setName("voiceSelector");
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(themeAmber));
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(themeBackground));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(themeLineGray));
    presetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(themeAmber));
    presetBox.addListener(this);
    addAndMakeVisible(presetBox);
    for (auto* button : { &previousVoiceButton, &nextVoiceButton })
    {
        button->setName("voiceStepButton");
        button->addListener(this);
        addAndMakeVisible(*button);
    }
    for (auto* button : { &loadVoiceButton, &saveVoiceButton, &copyVoiceButton,
                          &pasteVoiceButton, &initVoiceButton, &storeVoiceButton })
    {
        button->addListener(this);
        button->setName("voiceActionButton");
        addAndMakeVisible(*button);
    }
    saveLibraryButton.addListener(this);
    saveLibraryButton.setName("voiceActionButton");
    addAndMakeVisible(saveLibraryButton);
    wavRecordButton.addListener(this);
    wavRecordButton.setName("voiceActionButton");
    wavRecordButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(themeAmber));
    addAndMakeVisible(wavRecordButton);
    pasteVoiceButton.setEnabled(false);
    voiceModeBox.addItem("POLY", 1);
    voiceModeBox.addItem("MONO", 2);
    voiceModeBox.addItem("UNISON", 3);
    voiceModeBox.setSelectedId(1);
    voiceModeBox.addListener(this);
    monoModeButton.setClickingTogglesState(true);
    monoModeButton.addListener(this);
    addAndMakeVisible(monoModeButton);
    unisonModeButton.setClickingTogglesState(true);
    unisonModeButton.addListener(this);
    addAndMakeVisible(unisonModeButton);
    glideLegatoButton.setClickingTogglesState(true);
    glideLegatoButton.addListener(this);
    addAndMakeVisible(glideLegatoButton);
    lfoRetriggerButton.setClickingTogglesState(true);
    lfoRetriggerButton.addListener(this);
    addAndMakeVisible(lfoRetriggerButton);
    for (auto* button : { &arpButton, &chordButton })
    {
        button->setClickingTogglesState(true);
        button->addListener(this);
        addAndMakeVisible(*button);
    }
    arpHoldButton.setClickingTogglesState(true);
    arpHoldButton.addListener(this);
    addAndMakeVisible(arpHoldButton);
    for (auto& button : waveformAButtons)
    {
        button.addListener(this);
        addAndMakeVisible(button);
    }
    for (auto& button : waveformBButtons)
    {
        button.addListener(this);
        addAndMakeVisible(button);
    }
    waveformAButtons[0].setToggleState(true, juce::dontSendNotification);
    waveformBButtons[0].setToggleState(true, juce::dontSendNotification);
    for (std::size_t oscillator = 0; oscillator < waveMemoryBoxes.size(); ++oscillator)
    {
        auto& memory = waveMemoryBoxes[oscillator];
        for (std::size_t index = 0; index < aureline::kWaveMemoryFactoryCount; ++index)
            memory.addItem(aureline::waveMemoryFactoryNames()[index],
                           static_cast<int>(index) + 1);
        memory.setSelectedId(1, juce::dontSendNotification);
        memory.addListener(this);
        memory.setName("waveMemorySelector");
        memory.setColour(juce::ComboBox::textColourId, juce::Colour(themeAmber));
        memory.setColour(juce::ComboBox::backgroundColourId, juce::Colour(themeBackground));
        memory.setColour(juce::ComboBox::outlineColourId, juce::Colour(themeLineGray));
        addAndMakeVisible(memory);

        auto& character = waveCharacterBoxes[oscillator];
        character.addItem("5-BIT", 1);
        character.addItem("4-BIT", 2);
        character.addItem("SMOOTH", 3);
        character.setSelectedId(1, juce::dontSendNotification);
        character.addListener(this);
        character.setName("waveMemorySelector");
        character.setColour(juce::ComboBox::textColourId, juce::Colour(themeAmber));
        character.setColour(juce::ComboBox::backgroundColourId, juce::Colour(themeBackground));
        character.setColour(juce::ComboBox::outlineColourId, juce::Colour(themeLineGray));
        addAndMakeVisible(character);

        auto& waveButton = waveMemoryButtons[oscillator];
        waveButton.setName("voiceStepButton");
        waveButton.addListener(this);
        addAndMakeVisible(waveButton);
    }
    for (auto& button : lfoWaveformButtons)
    {
        button.addListener(this);
        addAndMakeVisible(button);
    }
    lfoWaveformButtons[1].setToggleState(true, juce::dontSendNotification);
    for (auto* rangeKnob : { &oscillatorARangeKnob, &oscillatorBRangeKnob })
    {
        rangeKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        rangeKnob->setRange(-2.0, 2.0, 1.0);
        rangeKnob->setValue(0.0);
        rangeKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 14);
        rangeKnob->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdedfdd));
        rangeKnob->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0906));
        rangeKnob->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff45433e));
        rangeKnob->textFromValueFunction = [](double value)
        {
            constexpr std::array<const char*, 5> labels { "32'", "16'", "8'", "4'", "2'" };
            return juce::String(labels[static_cast<std::size_t>(juce::jlimit(0, 4,
                static_cast<int>(std::lround(value)) + 2))]);
        };
        rangeKnob->updateText();
        rangeKnob->addListener(this);
        addAndMakeVisible(*rangeKnob);
    }
    for (auto* label : { &oscillatorAShapeLabel, &oscillatorBShapeLabel, &lfoShapeLabel,
                         &oscillatorARangeLabel, &oscillatorBRangeLabel })
    {
        label->setFont(juce::FontOptions(10.5f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colour(themeText));
        addAndMakeVisible(*label);
    }
    oscillatorAShapeLabel.setText("- SHAPE -", juce::dontSendNotification);
    oscillatorBShapeLabel.setText("- SHAPE -", juce::dontSendNotification);
    lfoShapeLabel.setText("- SHAPE -", juce::dontSendNotification);
    oscillatorARangeLabel.setText("RANGE", juce::dontSendNotification);
    oscillatorBRangeLabel.setText("RANGE", juce::dontSendNotification);
    syncButton.setClickingTogglesState(true);
    syncButton.addListener(this);
    addAndMakeVisible(syncButton);
    for (auto* button : { &lowFrequencyButton, &keyboardTrackingButton })
    {
        button->setClickingTogglesState(true);
        button->addListener(this);
        addAndMakeVisible(*button);
    }
    keyboardTrackingButton.setToggleState(true, juce::dontSendNotification);
    for (auto& button : polyModDestinationButtons)
    {
        button.setClickingTogglesState(true);
        button.addListener(this);
        addAndMakeVisible(button);
    }
    for (auto& button : lfoDestinationButtons)
    {
        button.setClickingTogglesState(true);
        button.addListener(this);
        addAndMakeVisible(button);
    }

    const std::array<const char*, 23> names { "VCO A", "VCO B", "DETUNE", "PW A", "PW B",
        "NOISE", "CUTOFF", "RESONANCE", "FILTER ENV", "KEY TRACK", "ATTACK", "DECAY",
        "SUSTAIN", "RELEASE", "LFO RATE", "MOD AMT", "FILTER ENV", "OSC B", "VOLUME",
        "ATTACK", "DECAY", "SUSTAIN", "RELEASE" };
    const std::array<double, 23> mins { 0, 0, -100, 0.02, 0.02, 0, 20, 0, -1, 0,
        0.001, 0.001, 0, 0.001, 0.01, 0, 0, 0, 0, 0.001, 0.001, 0, 0.001 };
    const std::array<double, 23> maxs { 1, 1, 100, 0.98, 0.98, 1, 20000, 1, 1, 1,
        5, 5, 1, 8, 30, 1, 1, 1, 1, 5, 5, 1, 8 };
    const std::array<double, 23> values { 0.5, 0.5, 7, 0.5, 0.5, 0, 8000, 0.1, 0.25, 0,
        0.01, 0.25, 0.75, 0.4, 5, 0, 0, 0, 0.8, 0.01, 0.3, 0.4, 0.5 };
    for (std::size_t i = 0; i < knobs.size(); ++i)
        configureKnob(knobs[i], knobLabels[i], names[i], mins[i], maxs[i], values[i],
                      i == 6 ? 0.25 : 1.0);
    knobs[14].setRange(0.01, 30.0, 0.01);
    knobs[14].setSkewFactorFromMidPoint(2.0);
    knobs[14].setNumDecimalPlacesToDisplay(2);
    knobs[14].textFromValueFunction = [](double value)
    {
        if (value < 1.0)
            return juce::String(value, 2);
        if (value < 10.0)
            return juce::String(value, 1);
        return juce::String(value, 0);
    };
    knobs[14].valueFromTextFunction = [](const juce::String& text)
    {
        return juce::jlimit(0.01, 30.0, text.getDoubleValue());
    };
    knobs[14].updateText();
    knobLabels[18].setFont(juce::FontOptions(11.5f, juce::Font::bold));
    configureKnob(spreadKnob, spreadLabel, "SPREAD", 0.0, 1.0, 0.0);
    configureKnob(vintageKnob, vintageLabel, "VINTAGE", 0.0, 1.0, 0.0);
    configureKnob(tempoKnob, tempoLabel, "TEMPO", 40.0, 240.0, 120.0);
    configureKnob(scaleKnob, scaleLabel, "SCALE", 0.0, 11.0, 0.0);
    scaleKnob.setRange(0.0, 11.0, 1.0);
    scaleKnob.textFromValueFunction = [](double value)
    {
        constexpr std::array<const char*, 12> noteNames {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        return juce::String(noteNames[static_cast<std::size_t>(juce::jlimit(
            0, 11, static_cast<int>(std::lround(value))))]);
    };
    scaleKnob.updateText();
    configureKnob(lfoDelayKnob, lfoDelayLabel, "LFO DELAY", 0.0, 10.0, 0.0, 0.5);
    configureKnob(lfoFadeKnob, lfoFadeLabel, "LFO FADE", 0.0, 10.0, 0.0, 0.5);
    configureKnob(modRangeKnob, modRangeLabel, "MOD RANGE", 0.0, 1.0, 0.35);
    const std::array<const char*, 5> performanceNames {
        "RANGE", "UNI DETUNE", "MASTER", "GLIDE", "VELOCITY"
    };
    const std::array<double, 5> performanceMin { 0.0, 0.0, -100.0, 0.0, 0.0 };
    const std::array<double, 5> performanceMax { 24.0, 100.0, 100.0, 5.0, 1.0 };
    const std::array<double, 5> performanceInitial { 2.0, 14.0, 0.0, 0.0, 0.0 };
    for (std::size_t i = 0; i < performanceKnobs.size(); ++i)
    {
        configureKnob(performanceKnobs[i], performanceLabels[i], performanceNames[i],
                      performanceMin[i], performanceMax[i], performanceInitial[i],
                      i == 3 ? 0.5 : 1.0);
        performanceKnobs[i].setName("performanceKnob");
        performanceLabels[i].setFont(juce::FontOptions(i == 0 || i == 3 ? 9.0f : 8.0f));
        performanceKnobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    }
    performanceLabels[4].setFont(juce::FontOptions(10.5f));
    performanceKnobs[4].setName({});
    performanceKnobs[4].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
    configureKnob(arpKnobs[0], arpLabels[0], "ARP RATE", 0.0, 2.0, 1.0);
    configureKnob(arpKnobs[1], arpLabels[1], "DIRECTION", 0.0, 3.0, 0.0);
    configureKnob(arpKnobs[2], arpLabels[2], "GATE", 0.1, 0.95, 0.75);
    arpKnobs[0].setRange(0.0, 2.0, 1.0);
    arpKnobs[0].textFromValueFunction = [](double value)
    {
        constexpr std::array<const char*, 3> rates { "1/8", "1/16", "1/32" };
        return juce::String(rates[static_cast<std::size_t>(juce::jlimit(
            0, 2, static_cast<int>(std::lround(value))))]);
    };
    arpKnobs[1].setRange(0.0, 3.0, 1.0);
    arpKnobs[1].textFromValueFunction = [](double value)
    {
        constexpr std::array<const char*, 4> directions { "UP", "DOWN", "U/D", "RND" };
        return juce::String(directions[static_cast<std::size_t>(juce::jlimit(
            0, 3, static_cast<int>(std::lround(value))))]);
    };
    for (auto& knob : arpKnobs)
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    arpKnobs[0].updateText();
    arpKnobs[1].updateText();

    const auto useZeroToTenDisplay = [](juce::Slider& slider,
                                        bool showOneDecimal = false)
    {
        slider.setNumDecimalPlacesToDisplay(showOneDecimal ? 1 : 0);
        slider.textFromValueFunction = [&slider, showOneDecimal](double value)
        {
            const auto position = slider.valueToProportionOfLength(value);
            return showOneDecimal ? juce::String(position * 10.0, 1)
                                  : juce::String(juce::roundToInt(position * 10.0));
        };
        slider.valueFromTextFunction = [&slider](const juce::String& text)
        {
            const auto position = juce::jlimit(0.0, 1.0, text.getDoubleValue() / 10.0);
            return slider.proportionOfLengthToValue(position);
        };
        slider.updateText();
    };
    constexpr std::array<std::size_t, 20> zeroToTenKnobIndices {
        0, 1, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 15, 16, 17,
        19, 20, 21, 22
    };
    for (const auto index : zeroToTenKnobIndices)
        useZeroToTenDisplay(knobs[index], index == 15);
    for (auto* slider : { &spreadKnob, &vintageKnob, &lfoDelayKnob, &lfoFadeKnob,
                          &modRangeKnob,
                          &performanceKnobs[3], &performanceKnobs[4], &arpKnobs[2] })
        useZeroToTenDisplay(*slider);

    pitchWheel.setSliderStyle(juce::Slider::LinearVertical);
    pitchWheel.setName("wheelFader");
    pitchWheel.setRange(-1.0, 1.0, 0.001);
    pitchWheel.setValue(0.0);
    pitchWheel.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchWheel.addListener(this);
    addAndMakeVisible(pitchWheel);
    modWheel.setSliderStyle(juce::Slider::LinearVertical);
    modWheel.setName("wheelFader");
    modWheel.setRange(0.0, 1.0, 0.001);
    modWheel.setValue(0.0);
    modWheel.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    modWheel.addListener(this);
    addAndMakeVisible(modWheel);
    transposeFader.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    transposeFader.setRange(-24.0, 24.0, 1.0);
    transposeFader.setValue(0.0);
    transposeFader.setNumDecimalPlacesToDisplay(0);
    transposeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    transposeFader.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdedfdd));
    transposeFader.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0906));
    transposeFader.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff45433e));
    transposeFader.addListener(this);
    addAndMakeVisible(transposeFader);
    for (auto* label : { &pitchLabel, &modLabel, &transposeLabel })
    {
        label->setColour(juce::Label::textColourId, juce::Colour(themeText));
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    pitchLabel.setText("PITCH", juce::dontSendNotification);
    pitchLabel.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    modLabel.setText("MOD", juce::dontSendNotification);
    modLabel.setFont(juce::FontOptions(9.5f, juce::Font::plain));
    transposeLabel.setText("TRANSPOSE", juce::dontSendNotification);
    transposeLabel.setFont(juce::FontOptions(8.0f));
    addAndMakeVisible(keyboard);

    for (auto& note : heldNotes)
        note.store(false);
    pcKeyboardHeldNotes.fill(false);
    for (auto& sample : scopeSamples)
        sample.store(0.0f);
    initialiseVoiceBanks();
    if (lastSelectedBankFile().existsAsFile())
        selectedVoiceBank = juce::jlimit(
            0, 3, lastSelectedBankFile().loadFileAsString().trim().getIntValue());
    refreshVoiceBankNames();
    if (readVoiceLibrary(activeLibraryFile(selectedVoiceBank)).getNumChildren()
        == static_cast<int>(factoryVoices.size()))
    {
        for (std::size_t index = 0; index < factoryVoices.size(); ++index)
        {
            legacyStoredVoiceFile(index).deleteFile();
            aurelineDocumentsDirectory()
                .getChildFile(juce::String(factoryVoices[index].name)
                              + ".aurelinevoice")
                .deleteFile();
        }
    }
    auto startupVoiceIndex = 0;
    const auto selectionFile = lastSelectedVoiceFile();
    if (selectionFile.existsAsFile())
        startupVoiceIndex = selectionFile.loadFileAsString().trim().getIntValue();
    startupVoiceIndex = juce::jlimit(
        0, static_cast<int>(factoryVoices.size()) - 1, startupVoiceIndex);
    loadFactoryVoice(static_cast<std::size_t>(startupVoiceIndex));
    installBuiltInVoiceLibrary(
        aurelineDocumentsDirectory().getChildFile(
            "Analog.aurelinelibrary.xml"),
        BinaryData::Analog_aurelinelibrary_xml,
        BinaryData::Analog_aurelinelibrary_xmlSize);
    installBuiltInVoiceLibrary(
        aurelineDocumentsDirectory().getChildFile(
            "Analog2.aurelinelibrary.xml"),
        BinaryData::Analog2_aurelinelibrary_xml,
        BinaryData::Analog2_aurelinelibrary_xmlSize);
    installBuiltInVoiceLibrary(
        aurelineDocumentsDirectory().getChildFile(
            "Retro.aurelinelibrary.xml"),
        BinaryData::Retro_aurelinelibrary_xml,
        BinaryData::Retro_aurelinelibrary_xmlSize);
    installBuiltInVoiceLibrary(
        aurelineDocumentsDirectory().getChildFile(
            "8-Bit.aurelinelibrary.xml"),
        BinaryData::_8Bit_aurelinelibrary_xml,
        BinaryData::_8Bit_aurelinelibrary_xmlSize);
    startTimerHz(30);
    setSize(1024, 668);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    if (ownsStandaloneAudio)
    {
        setAudioChannels(0, 2);
        for (const auto& input : juce::MidiInput::getAvailableDevices())
        {
            deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
            deviceManager.addMidiInputDeviceCallback(input.identifier, this);
            connectedMidiInputIds.push_back(input.identifier);
        }
    }
    refreshDeviceStatus();
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<AurelineMainComponent>(this)]
    {
        if (safe != nullptr)
            safe->grabKeyboardFocus();
    });
}

AurelineMainComponent::~AurelineMainComponent()
{
    stopTimer();
    for (const auto& identifier : connectedMidiInputIds)
        deviceManager.removeMidiInputDeviceCallback(identifier, this);
    if (ownsStandaloneAudio)
        shutdownAudio();
    setLookAndFeel(nullptr);
}

void AurelineMainComponent::configureKnob(juce::Slider& slider, juce::Label& label,
    const juce::String& name, double min, double max, double initial, double skew)
{
    label.setText(name, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour(themeText));
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(10.5f));
    addAndMakeVisible(label);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRange(min, max, (max - min) / 1000.0);
    slider.setValue(initial);
    slider.setSkewFactor(skew);
    const bool normalizedValue = min >= 0.0 && max <= 1.0;
    slider.setNumDecimalPlacesToDisplay(normalizedValue ? 2 : 0);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdedfdd));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0906));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff45433e));
    slider.addListener(this);
    addAndMakeVisible(slider);
    slider.updateText();
}

void AurelineMainComponent::prepareToPlay(int, double sampleRate)
{
    currentSampleRate = sampleRate;
    engine.prepare(sampleRate);
    performanceSequencer.prepare(sampleRate);
    midiCollector.reset(sampleRate);
}

void AurelineMainComponent::applyParameters()
{
    auto patch = engine.getPatch();
    patch.oscillatorA.level = parameters.oscillatorALevel.load();
    patch.oscillatorB.level = parameters.oscillatorBLevel.load();
    const auto waveformMaskA = parameters.waveformMaskA.load();
    const auto waveformMaskB = parameters.waveformMaskB.load();
    patch.oscillatorA.sawEnabled = (waveformMaskA & 1) != 0;
    patch.oscillatorA.triangleEnabled = (waveformMaskA & 2) != 0;
    patch.oscillatorA.pulseEnabled = (waveformMaskA & 4) != 0;
    patch.oscillatorA.waveMemoryEnabled = (waveformMaskA & 8) != 0;
    patch.oscillatorA.waveMemoryIndex = parameters.waveMemoryIndexA.load();
    patch.oscillatorA.waveMemoryCharacter = static_cast<aureline::WaveMemoryCharacter>(
        juce::jlimit(0, 2, parameters.waveMemoryCharacterA.load()));
    patch.oscillatorA.waveMemoryData = userWaveMemoryActive[0]
        ? userWaveMemory[0]
        : aureline::waveMemoryFactoryBank()[static_cast<std::size_t>(juce::jlimit(
            0, static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1,
            patch.oscillatorA.waveMemoryIndex))];
    patch.oscillatorB.sawEnabled = (waveformMaskB & 1) != 0;
    patch.oscillatorB.triangleEnabled = (waveformMaskB & 2) != 0;
    patch.oscillatorB.pulseEnabled = (waveformMaskB & 4) != 0;
    patch.oscillatorB.waveMemoryEnabled = (waveformMaskB & 8) != 0;
    patch.oscillatorB.waveMemoryIndex = parameters.waveMemoryIndexB.load();
    patch.oscillatorB.waveMemoryCharacter = static_cast<aureline::WaveMemoryCharacter>(
        juce::jlimit(0, 2, parameters.waveMemoryCharacterB.load()));
    patch.oscillatorB.waveMemoryData = userWaveMemoryActive[1]
        ? userWaveMemory[1]
        : aureline::waveMemoryFactoryBank()[static_cast<std::size_t>(juce::jlimit(
            0, static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1,
            patch.oscillatorB.waveMemoryIndex))];
    patch.oscillatorA.octave = parameters.oscillatorAOctave.load();
    patch.oscillatorB.octave = parameters.oscillatorBOctave.load();
    patch.oscillatorB.lowFrequencyMode = parameters.oscillatorBLowFrequency.load();
    patch.oscillatorB.keyboardTracking = parameters.oscillatorBKeyboardTracking.load();
    patch.oscillatorSync = parameters.oscillatorSync.load();
    patch.oscillatorB.fineCents = parameters.oscillatorBFine.load();
    patch.oscillatorA.pulseWidth = parameters.pulseWidthA.load();
    patch.oscillatorB.pulseWidth = parameters.pulseWidthB.load();
    patch.oscillatorA.semitones = parameters.transpose.load();
    patch.oscillatorB.semitones = parameters.transpose.load();
    patch.noiseLevel = parameters.noiseLevel.load();
    patch.filterCutoffHz = parameters.cutoff.load();
    patch.filterResonance = parameters.resonance.load();
    patch.filterEnvelopeAmount = parameters.filterEnvelope.load();
    patch.filterKeyboardTracking = parameters.filterKeyboardTracking.load();
    patch.filterVelocityAmount = parameters.filterVelocity.load();
    patch.filterEnvelope.attackSeconds = parameters.filterAttack.load();
    patch.filterEnvelope.decaySeconds = parameters.filterDecay.load();
    patch.filterEnvelope.sustainLevel = parameters.filterSustain.load();
    patch.filterEnvelope.releaseSeconds = parameters.filterRelease.load();
    patch.amplifierEnvelope.attackSeconds = parameters.attack.load();
    patch.amplifierEnvelope.decaySeconds = parameters.decay.load();
    patch.amplifierEnvelope.sustainLevel = parameters.sustain.load();
    patch.amplifierEnvelope.releaseSeconds = parameters.release.load();
    patch.lfoRateHz = parameters.lfoRate.load();
    const auto lfoAmount = parameters.lfoAmount.load();
    patch.lfoWaveformMask = parameters.lfoWaveformMask.load();
    patch.lfoInitialAmount = lfoAmount;
    patch.lfoWheelAmount = parameters.modRange.load();
    patch.lfoDelaySeconds = parameters.lfoDelay.load();
    patch.lfoFadeSeconds = parameters.lfoFade.load();
    patch.lfoRetrigger = parameters.lfoRetrigger.load();
    // Keep the full knob/wheel range musical: pitch remains vibrato rather than
    // an octave trill, while PW and filter still reach clearly audible depths.
    patch.lfoPitchDepthASemitones = parameters.lfoDestinations[0].load() ? 12.0 : 0.0;
    patch.lfoPitchDepthBSemitones = parameters.lfoDestinations[1].load() ? 12.0 : 0.0;
    patch.lfoPulseWidthDepthA = parameters.lfoDestinations[2].load() ? 0.25 : 0.0;
    patch.lfoPulseWidthDepthB = parameters.lfoDestinations[3].load() ? 0.25 : 0.0;
    patch.lfoFilterDepthOctaves = parameters.lfoDestinations[4].load() ? 2.0 : 0.0;
    const auto polyModOscillatorB = parameters.polyModOscillatorB.load();
    const auto polyModFilterEnvelope = parameters.polyModFilterEnvelope.load();
    const bool polyModToFrequencyA = parameters.polyModToFrequencyA.load();
    const bool polyModToPulseWidthA = parameters.polyModToPulseWidthA.load();
    const bool polyModToFilter = parameters.polyModToFilter.load();
    patch.polyModOscillatorBToPitch = polyModToFrequencyA ? polyModOscillatorB : 0.0;
    patch.polyModFilterEnvelopeToPitch = polyModToFrequencyA ? polyModFilterEnvelope : 0.0;
    patch.polyModOscillatorBToPulseWidthA = polyModToPulseWidthA ? polyModOscillatorB : 0.0;
    patch.polyModFilterEnvelopeToPulseWidthA = polyModToPulseWidthA ? polyModFilterEnvelope : 0.0;
    patch.polyModOscillatorBToFilter = polyModToFilter ? polyModOscillatorB : 0.0;
    patch.polyModFilterEnvelopeToFilter = polyModToFilter ? polyModFilterEnvelope : 0.0;
    patch.stereoSpread = parameters.spread.load();
    patch.vintageAmount = parameters.vintage.load();
    patch.glideSeconds = parameters.glide.load();
    patch.glideLegatoOnly = parameters.glideLegatoOnly.load();
    patch.masterTuneCents = parameters.masterTune.load();
    patch.unisonDetuneCents = parameters.unisonDetune.load();
    patch.masterGain = parameters.master.load();
    patch.voiceMode = static_cast<aureline::VoiceMode>(parameters.voiceMode.load());
    engine.setPatch(patch);
    engine.setPitchBend(parameters.pitchBend.load());
    engine.setPitchBendRange(parameters.pitchBendRange.load());
    engine.setModWheel(parameters.modWheel.load());
}

#if 0 // Replaced by the shared JUCE-independent PerformanceSequencer implementation below.
void AurelineMainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    applyParameters();
    const bool arpEnabled = parameters.arpEnabled.load();
    const bool chordEnabled = parameters.chordEnabled.load();
    const int chordCount = chordEnabled ? 3 : 1;
    const auto chordNote = [this, chordEnabled](int root, int index)
    {
        if (!chordEnabled)
            return juce::jlimit(0, 127, root);
        constexpr std::array<int, 7> majorScale { 0, 2, 4, 5, 7, 9, 11 };
        const int scaleRoot = parameters.scaleRoot.load();
        const int relativePitch = (root - scaleRoot + 120) % 12;
        int degree = 0;
        int nearestDistance = 12;
        for (int candidate = 0; candidate < static_cast<int>(majorScale.size()); ++candidate)
        {
            const int distance = std::abs(majorScale[static_cast<std::size_t>(candidate)]
                                          - relativePitch);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                degree = candidate;
            }
        }
        const int chordDegree = degree + index * 2;
        const int octave = chordDegree / static_cast<int>(majorScale.size());
        const int pitch = majorScale[static_cast<std::size_t>(
            chordDegree % static_cast<int>(majorScale.size()))];
        const int scaleOctaveBase = root - relativePitch;
        return juce::jlimit(0, 127, scaleOctaveBase + pitch + octave * 12);
    };
    if (sequencerResetRequested.exchange(false))
    {
        engine.panic();
        arpHeldNotes.fill(false);
        arpInputHeldNotes.fill(false);
        arpCurrentNote = -1;
        arpLastNote = -1;
        arpSamplesUntilStep = 0;
        arpGateSamplesRemaining = 0;
        arpMovingUp = true;
        if (arpEnabled)
            for (int root = 0; root < 128; ++root)
                if (heldNotes[static_cast<std::size_t>(root)].load())
                {
                    arpInputHeldNotes[static_cast<std::size_t>(root)] = true;
                    for (int index = 0; index < chordCount; ++index)
                    {
                        const int note = chordNote(root, index);
                        arpHeldNotes[static_cast<std::size_t>(note)] = true;
                        if (note <= 115)
                            arpHeldNotes[static_cast<std::size_t>(note + 12)] = true;
                    }
                }
    }
    juce::MidiBuffer midi;
    midiCollector.removeNextBlockOfMessages(midi, info.numSamples);
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            const int root = message.getNoteNumber();
            arpVelocity = static_cast<int>(message.getVelocity());
            if (arpEnabled && parameters.arpHold.load()
                && std::none_of(arpInputHeldNotes.begin(), arpInputHeldNotes.end(),
                                [](bool held) { return held; }))
                arpHeldNotes.fill(false);
            arpInputHeldNotes[static_cast<std::size_t>(root)] = true;
            for (int index = 0; index < chordCount; ++index)
            {
                const int note = chordNote(root, index);
                if (arpEnabled)
                {
                    arpHeldNotes[static_cast<std::size_t>(note)] = true;
                    if (note <= 115)
                        arpHeldNotes[static_cast<std::size_t>(note + 12)] = true;
                }
                else
                    engine.noteOn(note, arpVelocity);
            }
        }
        else if (message.isNoteOff())
        {
            const int root = message.getNoteNumber();
            arpInputHeldNotes[static_cast<std::size_t>(root)] = false;
            for (int index = 0; index < chordCount; ++index)
            {
                const int note = chordNote(root, index);
                if (arpEnabled)
                {
                    if (!parameters.arpHold.load())
                    {
                        arpHeldNotes[static_cast<std::size_t>(note)] = false;
                        if (note <= 115)
                            arpHeldNotes[static_cast<std::size_t>(note + 12)] = false;
                    }
                }
                else
                    engine.noteOff(note);
            }
        }
        else if (message.isPitchWheel())
            engine.setPitchBend((message.getPitchWheelValue() - 8192) / 8192.0);
        else if (message.isController() && message.getControllerNumber() == 1)
            engine.setModWheel(message.getControllerValue() / 127.0);
        else if (message.isController() && message.getControllerNumber() == 64)
            engine.setSustainPedal(message.getControllerValue() >= 64);
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            engine.panic();
            arpHeldNotes.fill(false);
            arpInputHeldNotes.fill(false);
            arpCurrentNote = -1;
            arpLastNote = -1;
        }
    }
    auto* left = info.buffer->getWritePointer(0, info.startSample);
    auto* right = info.buffer->getNumChannels() > 1
        ? info.buffer->getWritePointer(1, info.startSample) : left;
    if (arpEnabled)
    {
        const auto tempoBpm = juce::jlimit(40.0f, 240.0f, parameters.tempoBpm.load());
        constexpr std::array<double, 3> rateFactors { 30.0, 15.0, 7.5 };
        const int rateIndex = juce::jlimit(0, 2, parameters.arpRate.load());
        const int stepSamples = juce::jmax(1, juce::roundToInt(
            currentSampleRate * rateFactors[static_cast<std::size_t>(rateIndex)]
            / static_cast<double>(tempoBpm)));
        const int gateSamples = juce::jlimit(1, stepSamples, juce::roundToInt(
            static_cast<float>(stepSamples)
            * juce::jlimit(0.1f, 0.95f, parameters.arpGate.load())));
        for (int sample = 0; sample < info.numSamples; ++sample)
        {
            if (arpCurrentNote >= 0
                && !arpHeldNotes[static_cast<std::size_t>(arpCurrentNote)])
            {
                engine.noteOff(arpCurrentNote);
                arpCurrentNote = -1;
                arpSamplesUntilStep = 0;
            }
            if (arpCurrentNote >= 0 && arpGateSamplesRemaining <= 0)
            {
                engine.noteOff(arpCurrentNote);
                arpCurrentNote = -1;
            }
            if (arpSamplesUntilStep <= 0)
            {
                if (arpCurrentNote >= 0)
                    engine.noteOff(arpCurrentNote);
                std::array<int, 128> activeNotes {};
                int activeCount = 0;
                for (int note = 0; note < 128; ++note)
                    if (arpHeldNotes[static_cast<std::size_t>(note)])
                        activeNotes[static_cast<std::size_t>(activeCount++)] = note;
                int nextNote = -1;
                if (activeCount > 0)
                {
                    const int direction = juce::jlimit(0, 3, parameters.arpDirection.load());
                    int currentIndex = -1;
                    for (int index = 0; index < activeCount; ++index)
                        if (activeNotes[static_cast<std::size_t>(index)] == arpLastNote)
                            currentIndex = index;
                    if (direction == 0)
                        currentIndex = (currentIndex + 1 + activeCount) % activeCount;
                    else if (direction == 1)
                        currentIndex = currentIndex < 0 ? activeCount - 1
                                                       : (currentIndex - 1 + activeCount) % activeCount;
                    else if (direction == 2)
                    {
                        if (currentIndex < 0)
                        {
                            currentIndex = 0;
                            arpMovingUp = true;
                        }
                        else
                        {
                            if (currentIndex == activeCount - 1)
                                arpMovingUp = false;
                            else if (currentIndex == 0)
                                arpMovingUp = true;
                            currentIndex += arpMovingUp ? 1 : -1;
                            currentIndex = juce::jlimit(0, activeCount - 1, currentIndex);
                        }
                    }
                    else
                    {
                        arpRandomState = arpRandomState * 1664525U + 1013904223U;
                        currentIndex = static_cast<int>(arpRandomState
                            % static_cast<std::uint32_t>(activeCount));
                    }
                    nextNote = activeNotes[static_cast<std::size_t>(currentIndex)];
                }
                arpCurrentNote = nextNote;
                arpLastNote = nextNote;
                if (nextNote >= 0)
                    engine.noteOn(nextNote, arpVelocity);
                arpSamplesUntilStep = stepSamples;
                arpGateSamplesRemaining = gateSamples;
            }
            const auto value = engine.renderStereoSample();
            left[sample] = static_cast<float>(value.left);
            right[sample] = static_cast<float>(value.right);
            --arpSamplesUntilStep;
            --arpGateSamplesRemaining;
        }
    }
    else
        engine.renderBlock(left, right, info.numSamples);
    auto writeIndex = scopeWriteIndex.load(std::memory_order_relaxed);
    for (int sample = 0; sample < info.numSamples; ++sample)
    {
        scopeSamples[writeIndex].store(left[sample], std::memory_order_relaxed);
        writeIndex = (writeIndex + 1) % scopeSize;
    }
    scopeWriteIndex.store(writeIndex, std::memory_order_release);
}
#endif

void AurelineMainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    juce::MidiBuffer midi;
    midiCollector.removeNextBlockOfMessages(midi, info.numSamples);
    renderAudioBlock(info, midi);
}

void AurelineMainComponent::processPluginBlock(juce::AudioBuffer<float>& buffer,
                                                const juce::MidiBuffer& midi)
{
    auto combinedMidi = midi;
    midiCollector.removeNextBlockOfMessages(combinedMidi, buffer.getNumSamples());
    juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
    renderAudioBlock(info, combinedMidi);
}

void AurelineMainComponent::renderAudioBlock(
    const juce::AudioSourceChannelInfo& info, const juce::MidiBuffer& midi)
{
    info.clearActiveBufferRegion();
    applyParameters();
    aureline::PerformanceSequencerSettings settings;
    settings.arpeggiatorEnabled = parameters.arpEnabled.load();
    settings.chordEnabled = parameters.chordEnabled.load();
    settings.holdEnabled = parameters.arpHold.load();
    settings.tempoBpm = parameters.tempoBpm.load();
    settings.rate = parameters.arpRate.load();
    settings.direction = parameters.arpDirection.load();
    settings.gate = parameters.arpGate.load();
    settings.scaleRoot = parameters.scaleRoot.load();
    performanceSequencer.setSettings(settings);
    if (sequencerResetRequested.exchange(false))
    {
        performanceSequencer.panic(engine);
        for (int note = 0; note < 128; ++note)
            if (heldNotes[static_cast<std::size_t>(note)].load())
                performanceSequencer.noteOn(engine, note, 100);
    }

    auto handleMessage = [this](const juce::MidiMessage& message)
    {
        if (message.isNoteOn())
        {
            waveformPitchNote.store(message.getNoteNumber(), std::memory_order_relaxed);
            heldNotes[static_cast<std::size_t>(message.getNoteNumber())].store(true);
            performanceSequencer.noteOn(engine, message.getNoteNumber(),
                                        static_cast<int>(message.getVelocity()));
        }
        else if (message.isNoteOff())
        {
            heldNotes[static_cast<std::size_t>(message.getNoteNumber())].store(false);
            performanceSequencer.noteOff(engine, message.getNoteNumber());
        }
        else if (message.isPitchWheel())
        {
            const auto value = juce::jlimit(
                -1.0f, 1.0f,
                static_cast<float>(message.getPitchWheelValue() - 8192) / 8192.0f);
            parameters.pitchBend.store(value);
            engine.setPitchBend(value);
        }
        else if (message.isController() && message.getControllerNumber() == 1)
        {
            const auto value = static_cast<float>(message.getControllerValue()) / 127.0f;
            parameters.modWheel.store(value);
            engine.setModWheel(value);
        }
        else if (message.isController() && message.getControllerNumber() == 64)
            engine.setSustainPedal(message.getControllerValue() >= 64);
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            performanceSequencer.panic(engine);
        }
    };

    auto* left = info.buffer->getWritePointer(0, info.startSample);
    auto* right = info.buffer->getNumChannels() > 1
        ? info.buffer->getWritePointer(1, info.startSample) : left;
    auto iterator = midi.cbegin();
    const auto end = midi.cend();
    for (int sample = 0; sample < info.numSamples; ++sample)
    {
        while (iterator != end && (*iterator).samplePosition <= sample)
        {
            handleMessage((*iterator).getMessage());
            ++iterator;
        }
        const auto output = performanceSequencer.renderStereoSample(engine);
        left[sample] = static_cast<float>(output.left);
        right[sample] = static_cast<float>(output.right);
    }

    auto writeIndex = scopeWriteIndex.load(std::memory_order_relaxed);
    for (int sample = 0; sample < info.numSamples; ++sample)
    {
        scopeSamples[writeIndex].store(left[sample], std::memory_order_relaxed);
        writeIndex = (writeIndex + 1) % scopeSize;
    }
    scopeWriteIndex.store(writeIndex, std::memory_order_release);
    if (wavRecording.load(std::memory_order_relaxed))
        wavRecorder.push(left, right, info.numSamples);
}

void AurelineMainComponent::releaseResources() { engine.panic(); }

void AurelineMainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message);
    if (message.isNoteOn())
        waveformPitchNote.store(message.getNoteNumber(), std::memory_order_relaxed);
    if (message.isPitchWheel())
    {
        const auto value = juce::jlimit(
            -1.0f, 1.0f,
            static_cast<float>(message.getPitchWheelValue() - 8192) / 8192.0f);
        parameters.pitchBend.store(value);
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this), value]
            {
                if (safe != nullptr)
                {
                    safe->pitchWheel.setValue(value, juce::dontSendNotification);
                    safe->pitchWheel.repaint();
                }
            });
    }
    else if (message.isController() && message.getControllerNumber() == 1)
    {
        const auto value = static_cast<float>(message.getControllerValue()) / 127.0f;
        parameters.modWheel.store(value);
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this), value]
            {
                if (safe != nullptr)
                {
                    safe->modWheel.setValue(value, juce::dontSendNotification);
                    safe->modWheel.repaint();
                }
            });
    }
    if (message.isNoteOnOrOff())
    {
        heldNotes[static_cast<std::size_t>(message.getNoteNumber())].store(message.isNoteOn());
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<AurelineMainComponent>(this)]
        {
            if (safe != nullptr)
                safe->keyboard.repaint();
        });
    }
}

void AurelineMainComponent::playNote(int note, int velocity)
{
    waveformPitchNote.store(note, std::memory_order_relaxed);
    heldNotes[static_cast<std::size_t>(note)].store(true);
    midiCollector.addMessageToQueue(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(velocity)));
}

void AurelineMainComponent::releaseNote(int note)
{
    heldNotes[static_cast<std::size_t>(note)].store(false);
    midiCollector.addMessageToQueue(juce::MidiMessage::noteOff(1, note));
}

bool AurelineMainComponent::isNoteHeld(int note) const
{
    return note >= 0 && note < 128 && heldNotes[static_cast<std::size_t>(note)].load();
}

void AurelineMainComponent::queueMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        waveformPitchNote.store(message.getNoteNumber(), std::memory_order_relaxed);
    if (message.isPitchWheel())
    {
        const auto value = juce::jlimit(
            -1.0f, 1.0f,
            static_cast<float>(message.getPitchWheelValue() - 8192) / 8192.0f);
        parameters.pitchBend.store(value);
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this), value]
            {
                if (safe != nullptr)
                {
                    safe->pitchWheel.setValue(value, juce::dontSendNotification);
                    safe->pitchWheel.repaint();
                }
            });
    }
    else if (message.isController() && message.getControllerNumber() == 1)
    {
        const auto value = static_cast<float>(message.getControllerValue()) / 127.0f;
        parameters.modWheel.store(value);
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this), value]
            {
                if (safe != nullptr)
                {
                    safe->modWheel.setValue(value, juce::dontSendNotification);
                    safe->modWheel.repaint();
                }
            });
    }
    midiCollector.addMessageToQueue(message);
}

juce::ValueTree AurelineMainComponent::capturePluginState() const
{
    juce::ValueTree state("AurelineState");
    state.setProperty("version", 2, nullptr);
#define SAVE_FLOAT(name) state.setProperty(#name, parameters.name.load(), nullptr)
#define SAVE_INT(name) state.setProperty(#name, parameters.name.load(), nullptr)
#define SAVE_BOOL(name) state.setProperty(#name, parameters.name.load(), nullptr)
    SAVE_FLOAT(oscillatorALevel); SAVE_FLOAT(oscillatorBLevel); SAVE_FLOAT(oscillatorBFine);
    SAVE_FLOAT(pulseWidthA); SAVE_FLOAT(pulseWidthB); SAVE_FLOAT(noiseLevel);
    SAVE_FLOAT(cutoff); SAVE_FLOAT(resonance); SAVE_FLOAT(filterEnvelope);
    SAVE_FLOAT(filterKeyboardTracking); SAVE_FLOAT(filterAttack); SAVE_FLOAT(filterDecay);
    SAVE_FLOAT(filterSustain); SAVE_FLOAT(filterRelease); SAVE_FLOAT(attack);
    SAVE_FLOAT(decay); SAVE_FLOAT(sustain); SAVE_FLOAT(release); SAVE_FLOAT(lfoRate);
    SAVE_FLOAT(lfoAmount); SAVE_FLOAT(modRange); SAVE_FLOAT(lfoDelay); SAVE_FLOAT(lfoFade);
    SAVE_FLOAT(polyModFilterEnvelope); SAVE_FLOAT(polyModOscillatorB); SAVE_FLOAT(spread);
    SAVE_FLOAT(vintage); SAVE_FLOAT(tempoBpm); SAVE_FLOAT(master); SAVE_FLOAT(transpose);
    SAVE_FLOAT(pitchBendRange); SAVE_FLOAT(glide); SAVE_FLOAT(masterTune);
    SAVE_FLOAT(unisonDetune); SAVE_FLOAT(filterVelocity);
    SAVE_FLOAT(oscillatorAOctave); SAVE_FLOAT(oscillatorBOctave); SAVE_FLOAT(arpGate);
    SAVE_INT(scaleRoot); SAVE_INT(voiceMode); SAVE_INT(waveformMaskA);
    SAVE_INT(waveformMaskB); SAVE_INT(lfoWaveformMask); SAVE_INT(arpRate);
    SAVE_INT(arpDirection); SAVE_INT(waveMemoryIndexA); SAVE_INT(waveMemoryIndexB);
    SAVE_INT(waveMemoryCharacterA); SAVE_INT(waveMemoryCharacterB);
    SAVE_BOOL(lfoRetrigger); SAVE_BOOL(glideLegatoOnly); SAVE_BOOL(oscillatorSync);
    SAVE_BOOL(oscillatorBLowFrequency); SAVE_BOOL(oscillatorBKeyboardTracking);
    SAVE_BOOL(polyModToFrequencyA); SAVE_BOOL(polyModToPulseWidthA);
    SAVE_BOOL(polyModToFilter); SAVE_BOOL(arpEnabled); SAVE_BOOL(chordEnabled);
    SAVE_BOOL(arpHold);
    for (std::size_t index = 0; index < parameters.lfoDestinations.size(); ++index)
        state.setProperty("lfoDestination" + juce::String(index),
                          parameters.lfoDestinations[index].load(), nullptr);
    for (std::size_t oscillator = 0; oscillator < userWaveMemory.size(); ++oscillator)
    {
        const auto suffix = oscillator == 0 ? "A" : "B";
        state.setProperty("waveMemoryUser" + juce::String(suffix),
                          userWaveMemoryActive[oscillator], nullptr);
        for (std::size_t step = 0; step < aureline::kWaveMemorySize; ++step)
            state.setProperty("waveMemoryStep" + juce::String(suffix)
                                  + juce::String(static_cast<int>(step)).paddedLeft('0', 2),
                              static_cast<int>(userWaveMemory[oscillator][step]), nullptr);
    }
#undef SAVE_FLOAT
#undef SAVE_INT
#undef SAVE_BOOL
    return state;
}

void AurelineMainComponent::restorePluginState(const juce::ValueTree& state)
{
    if (!state.isValid() || !state.hasType("AurelineState"))
        return;
    const auto restoreFloat = [&state](const char* name, std::atomic<float>& target)
    {
        if (state.hasProperty(name)) target.store(static_cast<float>(state[name]));
    };
    const auto restoreInt = [&state](const char* name, std::atomic<int>& target)
    {
        if (state.hasProperty(name)) target.store(static_cast<int>(state[name]));
    };
    const auto restoreBool = [&state](const char* name, std::atomic<bool>& target)
    {
        if (state.hasProperty(name)) target.store(static_cast<bool>(state[name]));
    };
#define LOAD_FLOAT(name) restoreFloat(#name, parameters.name)
#define LOAD_INT(name) restoreInt(#name, parameters.name)
#define LOAD_BOOL(name) restoreBool(#name, parameters.name)
    LOAD_FLOAT(oscillatorALevel); LOAD_FLOAT(oscillatorBLevel); LOAD_FLOAT(oscillatorBFine);
    LOAD_FLOAT(pulseWidthA); LOAD_FLOAT(pulseWidthB); LOAD_FLOAT(noiseLevel);
    LOAD_FLOAT(cutoff); LOAD_FLOAT(resonance); LOAD_FLOAT(filterEnvelope);
    LOAD_FLOAT(filterKeyboardTracking); LOAD_FLOAT(filterAttack); LOAD_FLOAT(filterDecay);
    LOAD_FLOAT(filterSustain); LOAD_FLOAT(filterRelease); LOAD_FLOAT(attack);
    LOAD_FLOAT(decay); LOAD_FLOAT(sustain); LOAD_FLOAT(release); LOAD_FLOAT(lfoRate);
    LOAD_FLOAT(lfoAmount);
    if (state.hasProperty("modRange"))
        LOAD_FLOAT(modRange);
    else
        parameters.modRange.store(0.35f);
    LOAD_FLOAT(lfoDelay); LOAD_FLOAT(lfoFade);
    LOAD_FLOAT(polyModFilterEnvelope); LOAD_FLOAT(polyModOscillatorB); LOAD_FLOAT(spread);
    LOAD_FLOAT(vintage); LOAD_FLOAT(tempoBpm); LOAD_FLOAT(master); LOAD_FLOAT(transpose);
    LOAD_FLOAT(pitchBendRange); LOAD_FLOAT(glide); LOAD_FLOAT(masterTune);
    LOAD_FLOAT(unisonDetune); LOAD_FLOAT(filterVelocity);
    LOAD_FLOAT(oscillatorAOctave); LOAD_FLOAT(oscillatorBOctave); LOAD_FLOAT(arpGate);
    LOAD_INT(scaleRoot); LOAD_INT(voiceMode); LOAD_INT(waveformMaskA);
    LOAD_INT(waveformMaskB); LOAD_INT(lfoWaveformMask); LOAD_INT(arpRate);
    LOAD_INT(arpDirection); LOAD_INT(waveMemoryIndexA); LOAD_INT(waveMemoryIndexB);
    LOAD_INT(waveMemoryCharacterA); LOAD_INT(waveMemoryCharacterB);
    LOAD_BOOL(lfoRetrigger); LOAD_BOOL(glideLegatoOnly); LOAD_BOOL(oscillatorSync);
    LOAD_BOOL(oscillatorBLowFrequency); LOAD_BOOL(oscillatorBKeyboardTracking);
    LOAD_BOOL(polyModToFrequencyA); LOAD_BOOL(polyModToPulseWidthA);
    LOAD_BOOL(polyModToFilter); LOAD_BOOL(arpEnabled); LOAD_BOOL(chordEnabled);
    LOAD_BOOL(arpHold);
#undef LOAD_FLOAT
#undef LOAD_INT
#undef LOAD_BOOL
    for (std::size_t index = 0; index < parameters.lfoDestinations.size(); ++index)
    {
        const auto name = "lfoDestination" + juce::String(index);
        if (state.hasProperty(name))
            parameters.lfoDestinations[index].store(static_cast<bool>(state[name]));
    }
    for (std::size_t oscillator = 0; oscillator < userWaveMemory.size(); ++oscillator)
    {
        const auto suffix = oscillator == 0 ? "A" : "B";
        const auto activeName = "waveMemoryUser" + juce::String(suffix);
        userWaveMemoryActive[oscillator] = state.hasProperty(activeName)
            && static_cast<bool>(state[activeName]);
        for (std::size_t step = 0; step < aureline::kWaveMemorySize; ++step)
        {
            const auto name = "waveMemoryStep" + juce::String(suffix)
                            + juce::String(static_cast<int>(step)).paddedLeft('0', 2);
            if (state.hasProperty(name))
                userWaveMemory[oscillator][step] = static_cast<std::uint8_t>(
                    juce::jlimit(0, 31, static_cast<int>(state[name])));
        }
    }
    sequencerResetRequested.store(true);
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        syncControlsFromParameters();
    else
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<AurelineMainComponent>(this)]
        {
            if (safe != nullptr) safe->syncControlsFromParameters();
        });
}

void AurelineMainComponent::syncControlsFromParameters()
{
    const std::array<float, 23> values {
        parameters.oscillatorALevel.load(), parameters.oscillatorBLevel.load(),
        parameters.oscillatorBFine.load(), parameters.pulseWidthA.load(),
        parameters.pulseWidthB.load(), parameters.noiseLevel.load(), parameters.cutoff.load(),
        parameters.resonance.load(), parameters.filterEnvelope.load(),
        parameters.filterKeyboardTracking.load(), parameters.attack.load(),
        parameters.decay.load(), parameters.sustain.load(), parameters.release.load(),
        parameters.lfoRate.load(), parameters.lfoAmount.load(),
        parameters.polyModFilterEnvelope.load(), parameters.polyModOscillatorB.load(),
        parameters.master.load(), parameters.filterAttack.load(), parameters.filterDecay.load(),
        parameters.filterSustain.load(), parameters.filterRelease.load()
    };
    for (std::size_t index = 0; index < knobs.size(); ++index)
        knobs[index].setValue(values[index], juce::dontSendNotification);
    oscillatorARangeKnob.setValue(parameters.oscillatorAOctave.load(), juce::dontSendNotification);
    oscillatorBRangeKnob.setValue(parameters.oscillatorBOctave.load(), juce::dontSendNotification);
    spreadKnob.setValue(parameters.spread.load(), juce::dontSendNotification);
    vintageKnob.setValue(parameters.vintage.load(), juce::dontSendNotification);
    tempoKnob.setValue(parameters.tempoBpm.load(), juce::dontSendNotification);
    scaleKnob.setValue(parameters.scaleRoot.load(), juce::dontSendNotification);
    lfoDelayKnob.setValue(parameters.lfoDelay.load(), juce::dontSendNotification);
    lfoFadeKnob.setValue(parameters.lfoFade.load(), juce::dontSendNotification);
    modRangeKnob.setValue(parameters.modRange.load(), juce::dontSendNotification);
    const std::array<float, 5> performanceValues {
        parameters.pitchBendRange.load(), parameters.unisonDetune.load(),
        parameters.masterTune.load(), parameters.glide.load(), parameters.filterVelocity.load()
    };
    for (std::size_t index = 0; index < performanceKnobs.size(); ++index)
        performanceKnobs[index].setValue(performanceValues[index], juce::dontSendNotification);
    transposeFader.setValue(parameters.transpose.load(), juce::dontSendNotification);
    const int mode = parameters.voiceMode.load();
    monoModeButton.setToggleState(mode == 1, juce::dontSendNotification);
    unisonModeButton.setToggleState(mode == 2, juce::dontSendNotification);
    voiceModeBox.setSelectedId(mode + 1, juce::dontSendNotification);
    glideLegatoButton.setToggleState(parameters.glideLegatoOnly.load(), juce::dontSendNotification);
    lfoRetriggerButton.setToggleState(parameters.lfoRetrigger.load(), juce::dontSendNotification);
    syncButton.setToggleState(parameters.oscillatorSync.load(), juce::dontSendNotification);
    lowFrequencyButton.setToggleState(parameters.oscillatorBLowFrequency.load(), juce::dontSendNotification);
    keyboardTrackingButton.setToggleState(parameters.oscillatorBKeyboardTracking.load(), juce::dontSendNotification);
    arpButton.setToggleState(parameters.arpEnabled.load(), juce::dontSendNotification);
    chordButton.setToggleState(parameters.chordEnabled.load(), juce::dontSendNotification);
    arpHoldButton.setToggleState(parameters.arpHold.load(), juce::dontSendNotification);
    arpKnobs[0].setValue(parameters.arpRate.load(), juce::dontSendNotification);
    arpKnobs[1].setValue(parameters.arpDirection.load(), juce::dontSendNotification);
    arpKnobs[2].setValue(parameters.arpGate.load(), juce::dontSendNotification);
    const int maskA = parameters.waveformMaskA.load();
    const int maskB = parameters.waveformMaskB.load();
    const int lfoMask = parameters.lfoWaveformMask.load();
    for (std::size_t index = 0; index < waveformAButtons.size(); ++index)
    {
        waveformAButtons[index].setToggleState((maskA & (1 << index)) != 0,
                                                juce::dontSendNotification);
        waveformBButtons[index].setToggleState((maskB & (1 << index)) != 0,
                                                juce::dontSendNotification);
    }
    waveMemoryBoxes[0].setSelectedId(parameters.waveMemoryIndexA.load() + 1,
                                     juce::dontSendNotification);
    waveMemoryBoxes[1].setSelectedId(parameters.waveMemoryIndexB.load() + 1,
                                     juce::dontSendNotification);
    waveCharacterBoxes[0].setSelectedId(parameters.waveMemoryCharacterA.load() + 1,
                                        juce::dontSendNotification);
    waveCharacterBoxes[1].setSelectedId(parameters.waveMemoryCharacterB.load() + 1,
                                        juce::dontSendNotification);
    constexpr std::array<int, 5> lfoBits { 1, 2, 8, 4, 16 };
    for (std::size_t index = 0; index < lfoWaveformButtons.size(); ++index)
        lfoWaveformButtons[index].setToggleState((lfoMask & lfoBits[index]) != 0,
                                                  juce::dontSendNotification);
    polyModDestinationButtons[0].setToggleState(parameters.polyModToFrequencyA.load(), juce::dontSendNotification);
    polyModDestinationButtons[1].setToggleState(parameters.polyModToPulseWidthA.load(), juce::dontSendNotification);
    polyModDestinationButtons[2].setToggleState(parameters.polyModToFilter.load(), juce::dontSendNotification);
    for (std::size_t index = 0; index < lfoDestinationButtons.size(); ++index)
        lfoDestinationButtons[index].setToggleState(parameters.lfoDestinations[index].load(),
                                                     juce::dontSendNotification);
    presetBox.setText("RESTORED VOICE", juce::dontSendNotification);
    repaint();
}

void AurelineMainComponent::sliderValueChanged(juce::Slider* slider)
{
    const auto value = static_cast<float>(slider->getValue());
    if (slider == &pitchWheel) parameters.pitchBend.store(value);
    else if (slider == &modWheel) parameters.modWheel.store(value);
    else if (slider == &spreadKnob) parameters.spread.store(value);
    else if (slider == &vintageKnob) parameters.vintage.store(value);
    else if (slider == &tempoKnob) parameters.tempoBpm.store(value);
    else if (slider == &scaleKnob) parameters.scaleRoot.store(
        juce::jlimit(0, 11, static_cast<int>(std::lround(value))));
    else if (slider == &arpKnobs[0]) parameters.arpRate.store(
        juce::jlimit(0, 2, static_cast<int>(std::lround(value))));
    else if (slider == &arpKnobs[1]) parameters.arpDirection.store(
        juce::jlimit(0, 3, static_cast<int>(std::lround(value))));
    else if (slider == &arpKnobs[2]) parameters.arpGate.store(value);
    else if (slider == &lfoDelayKnob) parameters.lfoDelay.store(value);
    else if (slider == &lfoFadeKnob) parameters.lfoFade.store(value);
    else if (slider == &modRangeKnob) parameters.modRange.store(value);
    else if (slider == &transposeFader) parameters.transpose.store(value);
    else if (slider == &oscillatorARangeKnob) parameters.oscillatorAOctave.store(value);
    else if (slider == &oscillatorBRangeKnob) parameters.oscillatorBOctave.store(value);
    else
    {
        for (std::size_t index = 0; index < performanceKnobs.size(); ++index)
        {
            if (slider != &performanceKnobs[index])
                continue;
            switch (index)
            {
                case 0: parameters.pitchBendRange.store(value); break;
                case 1: parameters.unisonDetune.store(value); break;
                case 2: parameters.masterTune.store(value); break;
                case 3: parameters.glide.store(value); break;
                case 4: parameters.filterVelocity.store(value); break;
                default: break;
            }
            return;
        }
        const auto index = static_cast<std::size_t>(slider - knobs.data());
        const bool userIsAdjusting = slider->isMouseButtonDown();
        if (userIsAdjusting && index >= 19 && index <= 22)
        {
            const auto now = juce::Time::getMillisecondCounterHiRes();
            if (envelopePreviewKind != 1
                || now - envelopePreviewChangedMs >= 2200.0)
                envelopePreviewStartedMs = now;
            envelopePreviewKind = 1;
            envelopePreviewChangedMs = now;
        }
        else if (userIsAdjusting && index >= 10 && index <= 13)
        {
            const auto now = juce::Time::getMillisecondCounterHiRes();
            if (envelopePreviewKind != 2
                || now - envelopePreviewChangedMs >= 2200.0)
                envelopePreviewStartedMs = now;
            envelopePreviewKind = 2;
            envelopePreviewChangedMs = now;
        }
        switch (index)
        {
            case 0: parameters.oscillatorALevel.store(value); break;
            case 1: parameters.oscillatorBLevel.store(value); break;
            case 2: parameters.oscillatorBFine.store(value); break;
            case 3: parameters.pulseWidthA.store(value); break;
            case 4: parameters.pulseWidthB.store(value); break;
            case 5: parameters.noiseLevel.store(value); break;
            case 6: parameters.cutoff.store(value); break;
            case 7: parameters.resonance.store(value); break;
            case 8: parameters.filterEnvelope.store(value); break;
            case 9: parameters.filterKeyboardTracking.store(value); break;
            case 10: parameters.attack.store(value); break;
            case 11: parameters.decay.store(value); break;
            case 12: parameters.sustain.store(value); break;
            case 13: parameters.release.store(value); break;
            case 14: parameters.lfoRate.store(value); break;
            case 15: parameters.lfoAmount.store(value); break;
            case 16: parameters.polyModFilterEnvelope.store(value); break;
            case 17: parameters.polyModOscillatorB.store(value); break;
            case 18: parameters.master.store(value); break;
            case 19: parameters.filterAttack.store(value); break;
            case 20: parameters.filterDecay.store(value); break;
            case 21: parameters.filterSustain.store(value); break;
            case 22: parameters.filterRelease.store(value); break;
            default: break;
        }
    }
}

void AurelineMainComponent::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &presetBox)
    {
        const int encoded = presetBox.getSelectedId() - 1;
        const int selectedBank = encoded / voiceSlotsPerBank;
        const int selectedIndex = encoded % voiceSlotsPerBank;
        selectedVoiceBank = juce::jlimit(0, 3, selectedBank);
        lastSelectedBankFile().replaceWithText(juce::String(selectedVoiceBank));
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(factoryVoices.size()))
            loadFactoryVoice(static_cast<std::size_t>(selectedIndex));
        else
            selectedFactoryVoiceIndex = -1;
    }
    else if (box == &voiceModeBox)
    {
        parameters.voiceMode.store(juce::jlimit(0, 2, voiceModeBox.getSelectedId() - 1));
        repaint();
    }
    else
    {
        for (std::size_t index = 0; index < waveMemoryBoxes.size(); ++index)
        {
            if (box == &waveMemoryBoxes[index])
            {
                const auto selected = juce::jlimit(0,
                    static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1,
                    waveMemoryBoxes[index].getSelectedId() - 1);
                userWaveMemory[index] =
                    aureline::waveMemoryFactoryBank()[static_cast<std::size_t>(selected)];
                userWaveMemoryActive[index] = false;
                (index == 0 ? parameters.waveMemoryIndexA
                            : parameters.waveMemoryIndexB).store(selected);
                return;
            }
            if (box == &waveCharacterBoxes[index])
            {
                const auto selected = juce::jlimit(0, 2,
                    waveCharacterBoxes[index].getSelectedId() - 1);
                (index == 0 ? parameters.waveMemoryCharacterA
                            : parameters.waveMemoryCharacterB).store(selected);
                return;
            }
        }
    }
}

void AurelineMainComponent::saveVoiceToFile(const juce::File& requestedFile)
{
    auto file = requestedFile.hasFileExtension(".aurelinevoice")
        ? requestedFile : requestedFile.withFileExtension(".aurelinevoice");
    auto state = capturePluginState();
    const auto json = makeVoiceFileJson(state, currentVoiceName);
    if (file.replaceWithText(juce::JSON::toString(json, true)))
        statusLabel.setText("Voice saved: " + file.getFileName(), juce::dontSendNotification);
    else
        statusLabel.setText("Voice save failed", juce::dontSendNotification);
}

void AurelineMainComponent::promptAndSaveVoice()
{
    auto* dialog = new juce::AlertWindow(
        "Save voice", "Enter a voice name (up to 16 characters).",
        juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("voiceName", currentVoiceName.substring(0, 16),
                          "Voice name");
    if (auto* editor = dialog->getTextEditor("voiceName"))
    {
        editor->setInputRestrictions(16);
        editor->selectAll();
    }
    dialog->addButton("Cancel", 0,
                      juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->addButton("Save", 1,
                      juce::KeyPress(juce::KeyPress::returnKey));
    dialog->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this),
             dialogSafe = juce::Component::SafePointer<juce::AlertWindow>(dialog)]
            (int result)
            {
                if (result != 1 || safe == nullptr || dialogSafe == nullptr)
                    return;
                auto voiceName = voiceNameWithoutSlotPrefix(
                    dialogSafe->getTextEditorContents("voiceName"));
                if (voiceName.isEmpty())
                    voiceName = safe->currentVoiceName;
                safe->currentVoiceName = voiceName;
                auto fileName = voiceName
                    .retainCharacters(
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_ ")
                    .trim();
                if (fileName.isEmpty())
                    fileName = "AurelineVoice";
                safe->voiceFileChooser = std::make_unique<juce::FileChooser>(
                    "Save Aureline voice",
                    aurelineDocumentsDirectory().getChildFile(
                        fileName + ".aurelinevoice"),
                    "*.aurelinevoice");
                safe->voiceFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles,
                    [safe](const juce::FileChooser& chooser)
                    {
                        if (safe != nullptr && chooser.getResult() != juce::File {})
                            safe->saveVoiceToFile(chooser.getResult());
                    });
                safe->repaint();
            }),
        true);
}

void AurelineMainComponent::saveVoiceLibraryToFile(const juce::File& requestedFile)
{
    const auto file = requestedFile.getFullPathName().endsWithIgnoreCase(
                          ".aurelinelibrary.xml")
        ? requestedFile
        : juce::File(requestedFile.getFullPathName() + ".aurelinelibrary.xml");
    if (isReservedLibraryFilename(file))
    {
        statusLabel.setText(
            file.getFileName() + " is reserved and cannot be overwritten",
            juce::dontSendNotification);
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Reserved built-in library",
            file.getFileName() + " is a built-in voice library and cannot be "
            "overwritten. Please choose a different file name.");
        return;
    }
    writeVoiceLibraryToFile(file, false, true);
}

void AurelineMainComponent::writeVoiceLibraryToFile(
    const juce::File& file, bool factoryOnly, bool showStatus)
{
    const auto currentState = capturePluginState().createCopy();
    const auto currentIndex = selectedFactoryVoiceIndex;
    const auto currentSelectedId = presetBox.getSelectedId();
    const auto currentName = currentVoiceName;
    const juce::ScopedValueSetter<bool> persistenceGuard(
        suppressLastVoicePersistence, true);

    juce::ValueTree library("AurelineLibrary");
    library.setProperty("format", "com.hidecade.aureline.library", nullptr);
    library.setProperty("version", libraryFormatVersion, nullptr);
    library.setProperty("voiceCount", static_cast<int>(factoryVoices.size()), nullptr);
    for (std::size_t index = 0; index < factoryVoices.size(); ++index)
    {
        loadFactoryVoice(index, !factoryOnly);
        auto voice = capturePluginState().createCopy();
        voice.setProperty("slot", static_cast<int>(index), nullptr);
        voice.setProperty("voiceName", currentVoiceName, nullptr);
        library.addChild(voice, -1, nullptr);
    }

    restorePluginState(currentState);
    selectedFactoryVoiceIndex = currentIndex;
    presetBox.setSelectedId(currentSelectedId, juce::dontSendNotification);
    currentVoiceName = currentName;

    const auto xml = library.createXml();
    const bool saved = xml != nullptr && file.replaceWithText(xml->toString());
    if (showStatus)
        statusLabel.setText(saved ? "Bank " + juce::String(selectedVoiceBank + 1)
                                      + " saved: " + file.getFileName()
                                  : "Voice library save failed",
                            juce::dontSendNotification);
}

void AurelineMainComponent::writeRetroGameLibraryToFile(const juce::File& file)
{
    static constexpr std::array<const char*, voiceSlotsPerBank> names {{
        "8BIT HERO", "8BIT QUEST", "PIXEL PLUCK", "BLOCKY BASS",
        "TRIANGLE CAVE", "COIN SPARK", "BOSS WARNING", "CHIP FANFARE",
        "DUNGEON STEP", "TINY DRUM", "CASTLE LEAD", "POWER UP",
        "SECRET DOOR", "NIGHT STAGE", "FINAL CASTLE",
        "PSG RACER", "PSG SKYLINE", "TONE CHANNEL", "NOISE RIDER",
        "ARCADE START", "BLUE DASH", "RING PULSE", "GRID RUNNER",
        "SPACE PORT", "PSG VICTORY", "WAVE HERO", "CRYSTAL CHANNEL",
        "NEON BASS", "LASER HARBOR", "ORBIT LEAD", "LASER ZAP",
        "SHIP EXPLODE"
    }};

    const auto currentState = capturePluginState().createCopy();
    const auto currentIndex = selectedFactoryVoiceIndex;
    const auto currentSelectedId = presetBox.getSelectedId();
    const auto savedVoiceName = currentVoiceName;
    const juce::ScopedValueSetter<bool> persistenceGuard(
        suppressLastVoicePersistence, true);

    juce::ValueTree library("AurelineLibrary");
    library.setProperty("format", "com.hidecade.aureline.library", nullptr);
    library.setProperty("version", libraryFormatVersion, nullptr);
    library.setProperty("voiceCount", voiceSlotsPerBank, nullptr);

    for (std::size_t index = 0; index < names.size(); ++index)
    {
        loadFactoryVoice(index, false);
        auto voice = capturePluginState().createCopy();
        voice.setProperty("slot", static_cast<int>(index), nullptr);
        voice.setProperty("voiceName", names[index], nullptr);
        voice.setProperty("vintage", 0.12 + (index % 4) * 0.04, nullptr);
        voice.setProperty("spread", index % 5 == 0 ? 0.22 : 0.0, nullptr);
        voice.setProperty("noiseLevel", 0.0, nullptr);
        voice.setProperty("oscillatorALevel", 0.78, nullptr);
        voice.setProperty("oscillatorBLevel", index % 3 == 0 ? 0.22 : 0.0, nullptr);
        voice.setProperty("attack", index % 10 == 4 ? 0.08 : 0.005, nullptr);
        voice.setProperty("decay", 0.10 + (index % 5) * 0.07, nullptr);
        voice.setProperty("sustain", index % 4 == 0 ? 0.72 : 0.28, nullptr);
        voice.setProperty("release", 0.04 + (index % 4) * 0.06, nullptr);
        voice.setProperty("filterAttack", 0.002, nullptr);
        voice.setProperty("filterDecay", 0.12, nullptr);
        voice.setProperty("filterSustain", 0.25, nullptr);
        voice.setProperty("filterRelease", 0.08, nullptr);
        voice.setProperty("voiceMode", index % 6 == 0 ? 0 : 1, nullptr);
        voice.setProperty("lfoAmount", 0.0, nullptr);
        voice.setProperty("lfoDelay", 0.0, nullptr);
        voice.setProperty("lfoFade", 0.0, nullptr);
        for (int destination = 0; destination < 5; ++destination)
            voice.setProperty("lfoDestination" + juce::String(destination),
                              false, nullptr);

        if (index < 15) // Pulse/triangle/noise character of early 8-bit consoles.
        {
            const bool triangle = index == 4 || index == 8 || index == 13;
            voice.setProperty("waveformMaskA", triangle ? 2 : 4, nullptr);
            voice.setProperty("pulseWidthA",
                              std::array<double, 4> { 0.125, 0.25, 0.5, 0.75 }[index % 4],
                              nullptr);
            voice.setProperty("oscillatorAOctave", index % 5 == 3 ? -1.0 : 0.0,
                              nullptr);
            voice.setProperty("cutoff", triangle ? 5200.0 : 9000.0, nullptr);
            if (index == 5 || index == 9 || index == 12)
                voice.setProperty("noiseLevel", 0.20 + (index % 3) * 0.12, nullptr);
        }
        else if (index < 25) // Three-channel square PSG plus noise.
        {
            voice.setProperty("waveformMaskA", 4, nullptr);
            voice.setProperty("waveformMaskB", 4, nullptr);
            voice.setProperty("pulseWidthA", 0.5, nullptr);
            voice.setProperty("pulseWidthB", index % 2 == 0 ? 0.25 : 0.5, nullptr);
            voice.setProperty("oscillatorBFine", 0.0, nullptr);
            voice.setProperty("oscillatorBOctave", index % 3 == 0 ? 1.0 : 0.0, nullptr);
            voice.setProperty("noiseLevel", index == 18 || index == 23 ? 0.32 : 0.0,
                              nullptr);
            voice.setProperty("cutoff", 7600.0, nullptr);
        }
        else if (index < 35) // Multi-channel 5-bit wavetable style.
        {
            voice.setProperty("waveformMaskA", 8, nullptr);
            voice.setProperty("waveformMaskB", index % 2 == 0 ? 8 : 0, nullptr);
            voice.setProperty("waveMemoryUserA", true, nullptr);
            voice.setProperty("waveMemoryUserB", true, nullptr);
            voice.setProperty("waveMemoryCharacterA", 0, nullptr);
            voice.setProperty("waveMemoryCharacterB", 0, nullptr);
            for (int step = 0; step < 32; ++step)
            {
                const auto phase = juce::MathConstants<double>::twoPi * step / 32.0;
                const auto harmonic = 0.68 * std::sin(phase)
                    + (0.16 + (index % 4) * 0.05) * std::sin(phase * (2 + index % 3))
                    + 0.10 * std::sin(phase * 5.0);
                const auto value = juce::jlimit(
                    0, 31, juce::roundToInt(15.5 + harmonic * 14.0));
                voice.setProperty("waveMemoryStepA"
                                      + juce::String(step).paddedLeft('0', 2),
                                  value, nullptr);
                voice.setProperty("waveMemoryStepB"
                                      + juce::String(step).paddedLeft('0', 2),
                                  juce::jlimit(0, 31, 31 - value), nullptr);
            }
            voice.setProperty("cutoff", 11000.0, nullptr);
        }
        else if (index < 45) // Compact arcade wavetable voices.
        {
            voice.setProperty("waveformMaskA", 8, nullptr);
            voice.setProperty("waveformMaskB", 0, nullptr);
            voice.setProperty("waveMemoryUserA", true, nullptr);
            voice.setProperty("waveMemoryCharacterA", 1, nullptr);
            for (int step = 0; step < 32; ++step)
            {
                const auto phase = juce::MathConstants<double>::twoPi * step / 32.0;
                double sample = std::sin(phase);
                if (index == 35) sample += 0.48 * std::sin(phase * 3.0);
                if (index == 36) sample += 0.38 * std::sin(phase * 5.0);
                if (index == 37) sample = std::sin(phase) > 0.0 ? 0.75 : -0.55;
                if (index == 39 || index == 41) sample += 0.45 * std::sin(phase * 2.0);
                const auto value = juce::jlimit(
                    0, 31, juce::roundToInt(15.5 + sample * 11.0));
                voice.setProperty("waveMemoryStepA"
                                      + juce::String(step).paddedLeft('0', 2),
                                  value, nullptr);
            }
            if (index == 43)
            {
                voice.setProperty("lfoRate", 6.5, nullptr);
                voice.setProperty("lfoAmount", 0.16, nullptr);
                voice.setProperty("lfoDestination0", true, nullptr);
            }
            voice.setProperty("cutoff", 8800.0, nullptr);
        }
        else // Short noise, laser and falling-pitch effects.
        {
            voice.setProperty("waveformMaskA", index == 46 || index == 47 ? 0 : 4,
                              nullptr);
            voice.setProperty("noiseLevel", index == 46 ? 0.65
                                      : index == 47 ? 0.92 : 0.28, nullptr);
            voice.setProperty("sustain", 0.0, nullptr);
            voice.setProperty("decay", index == 47 ? 0.85 : 0.18, nullptr);
            voice.setProperty("release", 0.05, nullptr);
            voice.setProperty("filterEnvelope", index % 2 == 0 ? 0.9 : -0.75,
                              nullptr);
            voice.setProperty("filterDecay", 0.35, nullptr);
            voice.setProperty("lfoRate",
                              8.0 + static_cast<double>(index - 45) * 2.0,
                              nullptr);
            voice.setProperty("lfoAmount", index == 47 ? 0.22 : 0.16, nullptr);
            voice.setProperty("lfoDestination0", true, nullptr);
        }

        // Period-authentic, restrained pitch modulation for sustained leads,
        // fanfares and wavetable/arcade tones. With the engine's squared curve,
        // 0.06..0.12 corresponds to roughly +/-0.04..0.17 semitones.
        static constexpr std::array<std::size_t, 21> melodicLfoVoices {{
            0, 1, 6, 7, 10, 13, 14,
            15, 16, 19, 20, 24,
            25, 27, 29, 30, 32, 34,
            36, 40, 44
        }};
        if (std::find(melodicLfoVoices.begin(), melodicLfoVoices.end(), index)
            != melodicLfoVoices.end())
        {
            const auto amount = 0.06 + static_cast<double>(index % 4) * 0.02;
            voice.setProperty("lfoRate", 4.5 + static_cast<double>(index % 5) * 0.7,
                              nullptr);
            voice.setProperty("lfoAmount", amount, nullptr);
            voice.setProperty("lfoDelay", index % 3 == 0 ? 0.16 : 0.0, nullptr);
            voice.setProperty("lfoFade", index % 3 == 0 ? 0.28 : 0.0, nullptr);
            voice.setProperty("lfoDestination0", true, nullptr);
            if (static_cast<double>(voice.getProperty("oscillatorBLevel", 0.0)) > 0.01)
                voice.setProperty("lfoDestination1", true, nullptr);
        }
        library.addChild(voice, -1, nullptr);
    }

    restorePluginState(currentState);
    selectedFactoryVoiceIndex = currentIndex;
    presetBox.setSelectedId(currentSelectedId, juce::dontSendNotification);
    currentVoiceName = savedVoiceName;
    if (const auto xml = library.createXml(); xml != nullptr)
        file.replaceWithText(xml->toString());
}

void AurelineMainComponent::initialiseVoiceBanks()
{
    const auto legacy = legacyActiveLibraryFile();
    const auto bank1 = activeLibraryFile(0);
    if (!bank1.existsAsFile() && legacy.existsAsFile())
        legacy.copyFileTo(bank1);
    if (readVoiceLibrary(bank1).getNumChildren()
        != static_cast<int>(factoryVoices.size()))
        writeVoiceLibraryToFile(bank1, true, false);

    const auto bank2 = activeLibraryFile(1);
    const auto bank3 = activeLibraryFile(2);
    const auto bank4 = activeLibraryFile(3);
    const auto presetLibraryMarker = aurelineApplicationSupportDirectory()
        .getChildFile(".analog-presets-64-v4");
    if (!presetLibraryMarker.existsAsFile())
    {
        const bool installed =
            bank1.replaceWithData(BinaryData::Analog_aurelinelibrary_xml,
                                  BinaryData::Analog_aurelinelibrary_xmlSize)
            && bank2.replaceWithData(BinaryData::Analog2_aurelinelibrary_xml,
                                     BinaryData::Analog2_aurelinelibrary_xmlSize);
        if (installed)
            presetLibraryMarker.replaceWithText("1");
    }
    const auto orderMarker = aurelineApplicationSupportDirectory()
        .getChildFile(".library-order-analog1-analog2-retro-8bit");

    // Preserve the editable Retro and 8-Bit banks while inserting Analog 2.
    // The discarded fourth bank was the generated INIT bank.
    if (!orderMarker.existsAsFile()
        && readVoiceLibrary(bank2).getNumChildren() == voiceSlotsPerBank
        && readVoiceLibrary(bank3).getNumChildren() == voiceSlotsPerBank)
    {
        const auto bank2Xml = bank2.loadFileAsString();
        const auto bank3Xml = bank3.loadFileAsString();
        const auto bank4Xml = bank4.loadFileAsString();
        const auto bank2Name = readVoiceLibrary(bank2).getProperty("name").toString();
        const bool bank2WasEightBit = bank2Name.equalsIgnoreCase("8-Bit");
        const auto& retroXml = bank2WasEightBit ? bank3Xml : bank2Xml;
        const auto& eightBitXml = bank2WasEightBit ? bank2Xml : bank3Xml;

        const bool migrated =
            bank2.replaceWithData(BinaryData::Analog2_aurelinelibrary_xml,
                                  BinaryData::Analog2_aurelinelibrary_xmlSize)
            && bank3.replaceWithText(retroXml)
            && bank4.replaceWithText(eightBitXml);
        if (migrated)
            orderMarker.replaceWithText("3");
        else
        {
            bank2.replaceWithText(bank2Xml);
            bank3.replaceWithText(bank3Xml);
            if (bank4Xml.isNotEmpty())
                bank4.replaceWithText(bank4Xml);
        }
    }

    const std::array<std::tuple<int, const char*, int>, 3> bundled {{
        { 1, BinaryData::Analog2_aurelinelibrary_xml,
          BinaryData::Analog2_aurelinelibrary_xmlSize },
        { 2, BinaryData::Retro_aurelinelibrary_xml,
          BinaryData::Retro_aurelinelibrary_xmlSize },
        { 3, BinaryData::_8Bit_aurelinelibrary_xml,
          BinaryData::_8Bit_aurelinelibrary_xmlSize }
    }};
    for (const auto& [bank, data, size] : bundled)
    {
        const auto file = activeLibraryFile(bank);
        if (readVoiceLibrary(file).getNumChildren()
            != static_cast<int>(factoryVoices.size()))
            file.replaceWithData(data, static_cast<std::size_t>(size));
    }
    if (!orderMarker.existsAsFile())
        orderMarker.replaceWithText("3");
}

void AurelineMainComponent::refreshVoiceBankNames()
{
    auto* root = presetBox.getRootMenu();
    if (root == nullptr)
        return;
    root->clear();
    for (int bank = 0; bank < 4; ++bank)
    {
        juce::PopupMenu voices;
        const auto library = readVoiceLibrary(activeLibraryFile(bank));
        for (std::size_t index = 0; index < factoryVoices.size(); ++index)
        {
            auto name = juce::String(factoryVoices[index].name);
            if (library.getNumChildren() == static_cast<int>(factoryVoices.size()))
                name = library.getChild(static_cast<int>(index))
                           .getProperty("voiceName", name).toString();
            voices.addItem(voiceMenuItemId(bank, static_cast<int>(index)),
                           slotVoiceDisplayName(index, name));
        }
        root->addSubMenu("BANK " + juce::String(bank + 1) + "  "
                             + voiceBankNames[static_cast<std::size_t>(bank)],
                         voices);
    }
    if (selectedFactoryVoiceIndex >= 0)
        presetBox.setSelectedId(
            voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
            juce::dontSendNotification);
}

void AurelineMainComponent::confirmAndLoadVoiceLibrary(const juce::File& file)
{
    auto* dialog = new juce::AlertWindow(
        "LOAD LIBRARY TO BANK",
        "Choose the destination bank. Its 32 voices will be overwritten.",
        juce::MessageBoxIconType::WarningIcon,
        this);
    dialog->addButton("BANK 1", 1);
    dialog->addButton("BANK 2", 2);
    dialog->addButton("BANK 3", 3);
    dialog->addButton("BANK 4", 4);
    dialog->addButton("CANCEL", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this), file]
            (int result)
            {
                if (safe != nullptr && result >= 1 && result <= 4)
                    safe->loadVoiceLibraryFromFile(file, result - 1);
            }),
        true);
}

void AurelineMainComponent::loadVoiceLibraryFromFile(const juce::File& file,
                                                      int bank)
{
    const auto xml = juce::XmlDocument::parse(file);
    const auto library = xml != nullptr ? juce::ValueTree::fromXml(*xml)
                                        : juce::ValueTree {};
    if (!library.isValid()
        || library.getType().toString() != "AurelineLibrary"
        || library.getProperty("format").toString() != "com.hidecade.aureline.library"
        || static_cast<int>(library.getProperty("version"))
               != libraryFormatVersion
        || library.getNumChildren() != static_cast<int>(factoryVoices.size()))
    {
        statusLabel.setText("Voice library load failed: invalid file",
                            juce::dontSendNotification);
        return;
    }

    for (int index = 0; index < library.getNumChildren(); ++index)
    {
        const auto voice = library.getChild(index);
        if (!voice.isValid() || static_cast<int>(voice.getProperty("slot", -1)) != index)
        {
            statusLabel.setText("Voice library load failed: invalid slot data",
                                juce::dontSendNotification);
            return;
        }
    }

    std::vector<juce::String> loadedVoiceNames;
    loadedVoiceNames.reserve(static_cast<std::size_t>(library.getNumChildren()));
    for (int index = 0; index < library.getNumChildren(); ++index)
    {
        const auto slotName = juce::String(factoryVoices[
            static_cast<std::size_t>(index)].name);
        auto voiceName = library.getChild(index).getProperty("voiceName").toString().trim();
        if (voiceName.isEmpty())
            voiceName = voiceNameWithoutSlotPrefix(slotName);
        voiceName = voiceNameWithoutSlotPrefix(voiceName);
        library.getChild(index).setProperty("voiceName", voiceName, nullptr);
        loadedVoiceNames.push_back(voiceName);
    }

    const auto normalizedXml = library.createXml();
    if (normalizedXml == nullptr
        || !activeLibraryFile(bank).replaceWithText(normalizedXml->toString()))
    {
        statusLabel.setText("Voice library load failed; previous library restored",
                            juce::dontSendNotification);
        return;
    }
    for (std::size_t index = 0; index < factoryVoices.size(); ++index)
    {
        legacyStoredVoiceFile(index).deleteFile();
        aurelineDocumentsDirectory()
            .getChildFile(juce::String(factoryVoices[index].name)
                          + ".aurelinevoice")
            .deleteFile();
    }

    selectedVoiceBank = juce::jlimit(0, 3, bank);
    lastSelectedBankFile().replaceWithText(juce::String(selectedVoiceBank));
    refreshVoiceBankNames();

    if (selectedFactoryVoiceIndex >= 0
        && selectedFactoryVoiceIndex < static_cast<int>(factoryVoices.size()))
        loadFactoryVoice(static_cast<std::size_t>(selectedFactoryVoiceIndex));
    statusLabel.setText("Bank " + juce::String(selectedVoiceBank + 1)
                            + " replaced from: " + file.getFileName(),
                        juce::dontSendNotification);
}

void AurelineMainComponent::loadVoiceFromFile(const juce::File& file)
{
    envelopePreviewKind = 0;
    const auto json = juce::JSON::parse(file.loadFileAsString());
    const auto state = stateFromVoiceFileJson(json);
    if (!state.isValid())
    {
        statusLabel.setText("Voice load failed", juce::dontSendNotification);
        return;
    }
    restorePluginState(state);
    auto selectedVoiceName = state.getProperty("voiceName").toString().trim();
    if (selectedVoiceName.isEmpty())
        selectedVoiceName = file.getFileNameWithoutExtension();
    selectedVoiceName = selectedVoiceName
        .retainCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_ ")
        .trim();
    selectedVoiceName = voiceNameWithoutSlotPrefix(selectedVoiceName);
    if (selectedVoiceName.isEmpty())
        selectedVoiceName = "LOADED VOICE";
    if (selectedFactoryVoiceIndex >= 0
        && selectedFactoryVoiceIndex < static_cast<int>(factoryVoices.size()))
    {
        presetBox.changeItemText(
            voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                                 slotVoiceDisplayName(
                                     static_cast<std::size_t>(selectedFactoryVoiceIndex),
                                     selectedVoiceName));
        presetBox.setSelectedId(
            voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                                juce::dontSendNotification);
    }
    currentVoiceName = selectedVoiceName;
    statusLabel.setText("Voice loaded temporarily into: " + selectedVoiceName,
                        juce::dontSendNotification);
}

void AurelineMainComponent::storeCurrentVoice()
{
    if (selectedFactoryVoiceIndex < 0
        || selectedFactoryVoiceIndex >= static_cast<int>(factoryVoices.size()))
    {
        statusLabel.setText("Select a numbered voice before STORE",
                            juce::dontSendNotification);
        return;
    }
    const auto slotName = juce::String(factoryVoices[
        static_cast<std::size_t>(selectedFactoryVoiceIndex)].name);
    auto voiceName = currentVoiceName.trim();
    if (voiceName.isEmpty())
        voiceName = slotName;
    currentVoiceName = voiceName;
    auto library = readVoiceLibrary(activeLibraryFile(selectedVoiceBank));
    if (library.getNumChildren() != static_cast<int>(factoryVoices.size()))
    {
        statusLabel.setText("Voice store failed: active library is invalid",
                            juce::dontSendNotification);
        return;
    }
    auto voice = capturePluginState().createCopy();
    voice.setProperty("slot", selectedFactoryVoiceIndex, nullptr);
    voice.setProperty("voiceName", voiceName, nullptr);
    library.removeChild(selectedFactoryVoiceIndex, nullptr);
    library.addChild(voice, selectedFactoryVoiceIndex, nullptr);
    const auto xml = library.createXml();
    if (xml == nullptr
        || !activeLibraryFile(selectedVoiceBank).replaceWithText(xml->toString()))
    {
        statusLabel.setText("Voice store failed", juce::dontSendNotification);
        return;
    }
    presetBox.changeItemText(
        voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
        slotVoiceDisplayName(static_cast<std::size_t>(selectedFactoryVoiceIndex),
                             voiceName));
    presetBox.setSelectedId(
        voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                            juce::dontSendNotification);
    statusLabel.setText("Voice stored in slot "
                            + juce::String(selectedFactoryVoiceIndex + 1)
                            + ": " + voiceName,
                        juce::dontSendNotification);
}

void AurelineMainComponent::openWaveMemoryEditor(std::size_t oscillator)
{
    if (oscillator >= waveMemoryButtons.size())
        return;
    const auto selected = oscillator == 0
        ? parameters.waveMemoryIndexA.load() : parameters.waveMemoryIndexB.load();
    if (!userWaveMemoryActive[oscillator])
        userWaveMemory[oscillator] = aureline::waveMemoryFactoryBank()[
            static_cast<std::size_t>(juce::jlimit(
                0, static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1, selected))];
    auto& mask = oscillator == 0 ? parameters.waveformMaskA : parameters.waveformMaskB;
    mask.store(mask.load() | 8);
    waveformAButtons[3].setToggleState(
        (parameters.waveformMaskA.load() & 8) != 0, juce::dontSendNotification);
    waveformBButtons[3].setToggleState(
        (parameters.waveformMaskB.load() & 8) != 0, juce::dontSendNotification);
    applyParameters();
    waveMemoryEditorWindow =
        std::make_unique<WaveMemoryEditorWindow>(*this, oscillator);
    addAndMakeVisible(*waveMemoryEditorWindow);
    waveMemoryEditorWindow->setBounds(getLocalBounds());
    waveMemoryEditorWindow->toFront(true);
}

void AurelineMainComponent::loadFactoryVoice(std::size_t index,
                                              bool useStoredOverride)
{
    if (index >= factoryVoices.size())
        return;

    selectedFactoryVoiceIndex = static_cast<int>(index);
    if (!suppressLastVoicePersistence)
        lastSelectedVoiceFile().replaceWithText(juce::String(index));
    presetBox.changeItemText(
        voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
                             slotVoiceDisplayName(index, factoryVoices[index].name));
    currentVoiceName = voiceNameWithoutSlotPrefix(factoryVoices[index].name);
    presetBox.setSelectedId(
        voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
                            juce::dontSendNotification);
    const auto activeLibrary =
        readVoiceLibrary(activeLibraryFile(selectedVoiceBank));
    const auto activeVoice = activeLibrary.getNumChildren()
            == static_cast<int>(factoryVoices.size())
        ? activeLibrary.getChild(static_cast<int>(index))
        : juce::ValueTree {};
    auto overrideFile = legacyStoredVoiceFile(index);
    const auto legacyOverrideFile = aurelineDocumentsDirectory().getChildFile(
        juce::String(factoryVoices[index].name) + ".aurelinevoice");
    if (useStoredOverride && !overrideFile.existsAsFile()
        && legacyOverrideFile.existsAsFile())
    {
        if (!legacyOverrideFile.moveFileTo(overrideFile))
            overrideFile = legacyOverrideFile;
    }
    if (useStoredOverride && activeVoice.isValid()
        && static_cast<int>(activeVoice.getProperty("slot", -1))
               == static_cast<int>(index))
    {
        restorePluginState(activeVoice);
        currentVoiceName = voiceNameWithoutSlotPrefix(
            activeVoice.getProperty("voiceName").toString());
        presetBox.changeItemText(
            voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
            slotVoiceDisplayName(index, currentVoiceName));
        presetBox.setSelectedId(
            voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
                                juce::dontSendNotification);
        return;
    }
    if (useStoredOverride && overrideFile.existsAsFile())
    {
        loadVoiceFromFile(overrideFile);
        presetBox.setSelectedId(
            voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
                                juce::dontSendNotification);
        return;
    }

    envelopePreviewKind = 0;
    const auto& voice = factoryVoices[index];
    const auto& patch = voice.patch;
    resetToInitialVoice();

    const auto setWaveform = [](auto& buttons, int mask)
    {
        for (std::size_t buttonIndex = 0; buttonIndex < buttons.size(); ++buttonIndex)
            buttons[buttonIndex].setToggleState((mask & (1 << static_cast<int>(buttonIndex))) != 0,
                                                juce::dontSendNotification);
    };
    const int waveA = (patch.oscillatorA.sawEnabled ? 1 : 0)
        | (patch.oscillatorA.triangleEnabled ? 2 : 0) | (patch.oscillatorA.pulseEnabled ? 4 : 0)
        | (patch.oscillatorA.waveMemoryEnabled ? 8 : 0);
    const int waveB = (patch.oscillatorB.sawEnabled ? 1 : 0)
        | (patch.oscillatorB.triangleEnabled ? 2 : 0) | (patch.oscillatorB.pulseEnabled ? 4 : 0)
        | (patch.oscillatorB.waveMemoryEnabled ? 8 : 0);
    setWaveform(waveformAButtons, waveA);
    setWaveform(waveformBButtons, waveB);
    parameters.waveformMaskA.store(waveA);
    parameters.waveformMaskB.store(waveB);
    parameters.waveMemoryIndexA.store(patch.oscillatorA.waveMemoryIndex);
    parameters.waveMemoryIndexB.store(patch.oscillatorB.waveMemoryIndex);
    userWaveMemory[0] = patch.oscillatorA.waveMemoryData;
    userWaveMemory[1] = patch.oscillatorB.waveMemoryData;
    userWaveMemoryActive = { index == 39, false };
    parameters.waveMemoryCharacterA.store(static_cast<int>(patch.oscillatorA.waveMemoryCharacter));
    parameters.waveMemoryCharacterB.store(static_cast<int>(patch.oscillatorB.waveMemoryCharacter));
    waveMemoryBoxes[0].setSelectedId(patch.oscillatorA.waveMemoryIndex + 1,
                                     juce::dontSendNotification);
    waveMemoryBoxes[1].setSelectedId(patch.oscillatorB.waveMemoryIndex + 1,
                                     juce::dontSendNotification);
    waveCharacterBoxes[0].setSelectedId(static_cast<int>(patch.oscillatorA.waveMemoryCharacter) + 1,
                                        juce::dontSendNotification);
    waveCharacterBoxes[1].setSelectedId(static_cast<int>(patch.oscillatorB.waveMemoryCharacter) + 1,
                                        juce::dontSendNotification);

    oscillatorARangeKnob.setValue(patch.oscillatorA.octave, juce::sendNotificationSync);
    oscillatorBRangeKnob.setValue(patch.oscillatorB.octave, juce::sendNotificationSync);
    knobs[0].setValue(patch.oscillatorA.level, juce::sendNotificationSync);
    knobs[1].setValue(patch.oscillatorB.level, juce::sendNotificationSync);
    knobs[2].setValue(patch.oscillatorB.fineCents, juce::sendNotificationSync);
    knobs[5].setValue(patch.noiseLevel, juce::sendNotificationSync);
    knobs[6].setValue(patch.filterCutoffHz, juce::sendNotificationSync);
    knobs[7].setValue(patch.filterResonance, juce::sendNotificationSync);
    knobs[8].setValue(patch.filterEnvelopeAmount, juce::sendNotificationSync);
    knobs[9].setValue(patch.filterKeyboardTracking, juce::sendNotificationSync);

    knobs[10].setValue(patch.amplifierEnvelope.attackSeconds, juce::sendNotificationSync);
    knobs[11].setValue(patch.amplifierEnvelope.decaySeconds, juce::sendNotificationSync);
    knobs[12].setValue(patch.amplifierEnvelope.sustainLevel, juce::sendNotificationSync);
    knobs[13].setValue(patch.amplifierEnvelope.releaseSeconds, juce::sendNotificationSync);
    knobs[19].setValue(patch.filterEnvelope.attackSeconds, juce::sendNotificationSync);
    knobs[20].setValue(patch.filterEnvelope.decaySeconds, juce::sendNotificationSync);
    knobs[21].setValue(patch.filterEnvelope.sustainLevel, juce::sendNotificationSync);
    knobs[22].setValue(patch.filterEnvelope.releaseSeconds, juce::sendNotificationSync);

    knobs[14].setValue(patch.lfoRateHz, juce::sendNotificationSync);
    knobs[15].setValue(patch.lfoInitialAmount, juce::sendNotificationSync);
    const auto polyFilterSource = std::max({ std::abs(patch.polyModFilterEnvelopeToPitch),
        std::abs(patch.polyModFilterEnvelopeToPulseWidthA),
        std::abs(patch.polyModFilterEnvelopeToFilter) });
    const auto polyOscillatorSource = std::max({ std::abs(patch.polyModOscillatorBToPitch),
        std::abs(patch.polyModOscillatorBToPulseWidthA),
        std::abs(patch.polyModOscillatorBToFilter) });
    knobs[16].setValue(polyFilterSource, juce::sendNotificationSync);
    knobs[17].setValue(polyOscillatorSource, juce::sendNotificationSync);
    spreadKnob.setValue(patch.stereoSpread, juce::sendNotificationSync);
    vintageKnob.setValue(patch.vintageAmount, juce::sendNotificationSync);

    const std::array<bool, 5> lfoDestinations {
        patch.lfoPitchDepthASemitones != 0.0, patch.lfoPitchDepthBSemitones != 0.0,
        patch.lfoPulseWidthDepthA != 0.0, patch.lfoPulseWidthDepthB != 0.0,
        patch.lfoFilterDepthOctaves != 0.0
    };
    for (std::size_t destination = 0; destination < lfoDestinationButtons.size(); ++destination)
    {
        const bool enabled = lfoDestinations[destination];
        lfoDestinationButtons[destination].setToggleState(enabled, juce::dontSendNotification);
        parameters.lfoDestinations[destination].store(enabled);
    }

    const std::array<bool, 3> polyDestinations {
        patch.polyModFilterEnvelopeToPitch != 0.0 || patch.polyModOscillatorBToPitch != 0.0,
        patch.polyModFilterEnvelopeToPulseWidthA != 0.0 || patch.polyModOscillatorBToPulseWidthA != 0.0,
        patch.polyModFilterEnvelopeToFilter != 0.0 || patch.polyModOscillatorBToFilter != 0.0
    };
    parameters.polyModFilterEnvelope.store(static_cast<float>(polyFilterSource));
    parameters.polyModOscillatorB.store(static_cast<float>(polyOscillatorSource));
    parameters.polyModToFrequencyA.store(polyDestinations[0]);
    parameters.polyModToPulseWidthA.store(polyDestinations[1]);
    parameters.polyModToFilter.store(polyDestinations[2]);
    for (std::size_t destination = 0; destination < polyModDestinationButtons.size(); ++destination)
        polyModDestinationButtons[destination].setToggleState(polyDestinations[destination],
                                                               juce::dontSendNotification);

    syncButton.setToggleState(patch.oscillatorSync, juce::dontSendNotification);
    parameters.oscillatorSync.store(patch.oscillatorSync);
    lowFrequencyButton.setToggleState(patch.oscillatorB.lowFrequencyMode,
                                      juce::dontSendNotification);
    parameters.oscillatorBLowFrequency.store(patch.oscillatorB.lowFrequencyMode);
    const auto voiceMode = static_cast<int>(patch.voiceMode);
    monoModeButton.setToggleState(voiceMode == 1, juce::dontSendNotification);
    unisonModeButton.setToggleState(voiceMode == 2, juce::dontSendNotification);
    parameters.voiceMode.store(voiceMode);
    voiceModeBox.setSelectedId(voiceMode + 1, juce::dontSendNotification);

    presetBox.setSelectedId(
        voiceMenuItemId(selectedVoiceBank, static_cast<int>(index)),
        juce::dontSendNotification);
    applyParameters();
    repaint();
}

void AurelineMainComponent::resetToInitialVoice()
{
    envelopePreviewKind = 0;
    userWaveMemory = { aureline::waveMemoryFactoryBank()[0],
                       aureline::waveMemoryFactoryBank()[0] };
    userWaveMemoryActive = { false, false };
    constexpr std::array<double, 23> initialKnobValues {
        0.5, 0.5, 7.0, 0.5, 0.5, 0.0, 8000.0, 0.1, 0.25, 0.0,
        0.01, 0.25, 0.75, 0.4, 5.0, 0.0, 0.0, 0.0, 0.8,
        0.01, 0.3, 0.4, 0.5
    };
    for (std::size_t index = 0; index < knobs.size(); ++index)
        knobs[index].setValue(initialKnobValues[index], juce::sendNotificationSync);

    oscillatorARangeKnob.setValue(0.0, juce::sendNotificationSync);
    oscillatorBRangeKnob.setValue(0.0, juce::sendNotificationSync);
    spreadKnob.setValue(0.0, juce::sendNotificationSync);
    vintageKnob.setValue(0.0, juce::sendNotificationSync);
    tempoKnob.setValue(120.0, juce::sendNotificationSync);
    scaleKnob.setValue(0.0, juce::sendNotificationSync);
    lfoDelayKnob.setValue(0.0, juce::sendNotificationSync);
    lfoFadeKnob.setValue(0.0, juce::sendNotificationSync);
    modRangeKnob.setValue(0.35, juce::sendNotificationSync);
    constexpr std::array<double, 5> initialPerformanceValues { 2.0, 14.0, 0.0, 0.0, 0.0 };
    for (std::size_t index = 0; index < performanceKnobs.size(); ++index)
        performanceKnobs[index].setValue(initialPerformanceValues[index],
                                         juce::sendNotificationSync);
    transposeFader.setValue(0.0, juce::sendNotificationSync);
    pitchWheel.setValue(0.0, juce::sendNotificationSync);
    modWheel.setValue(0.0, juce::sendNotificationSync);

    for (std::size_t index = 0; index < waveformAButtons.size(); ++index)
    {
        waveformAButtons[index].setToggleState(index == 0, juce::dontSendNotification);
        waveformBButtons[index].setToggleState(index == 0, juce::dontSendNotification);
    }
    for (std::size_t index = 0; index < lfoWaveformButtons.size(); ++index)
        lfoWaveformButtons[index].setToggleState(index == 1, juce::dontSendNotification);
    parameters.waveformMaskA.store(1);
    parameters.waveformMaskB.store(1);
    parameters.waveMemoryIndexA.store(0);
    parameters.waveMemoryIndexB.store(0);
    parameters.waveMemoryCharacterA.store(0);
    parameters.waveMemoryCharacterB.store(0);
    for (std::size_t index = 0; index < waveMemoryBoxes.size(); ++index)
    {
        waveMemoryBoxes[index].setSelectedId(1, juce::dontSendNotification);
        waveCharacterBoxes[index].setSelectedId(1, juce::dontSendNotification);
    }
    parameters.lfoWaveformMask.store(2);

    monoModeButton.setToggleState(false, juce::dontSendNotification);
    unisonModeButton.setToggleState(false, juce::dontSendNotification);
    arpButton.setToggleState(false, juce::dontSendNotification);
    chordButton.setToggleState(false, juce::dontSendNotification);
    arpHoldButton.setToggleState(false, juce::dontSendNotification);
    glideLegatoButton.setToggleState(false, juce::dontSendNotification);
    lfoRetriggerButton.setToggleState(false, juce::dontSendNotification);
    syncButton.setToggleState(false, juce::dontSendNotification);
    lowFrequencyButton.setToggleState(false, juce::dontSendNotification);
    keyboardTrackingButton.setToggleState(true, juce::dontSendNotification);
    parameters.voiceMode.store(0);
    parameters.arpEnabled.store(false);
    parameters.chordEnabled.store(false);
    parameters.arpHold.store(false);
    parameters.glideLegatoOnly.store(false);
    parameters.lfoRetrigger.store(false);
    parameters.arpRate.store(1);
    parameters.arpDirection.store(0);
    parameters.arpGate.store(0.75f);
    arpKnobs[0].setValue(1.0, juce::sendNotificationSync);
    arpKnobs[1].setValue(0.0, juce::sendNotificationSync);
    arpKnobs[2].setValue(0.75, juce::sendNotificationSync);
    sequencerResetRequested.store(true);
    parameters.oscillatorSync.store(false);
    parameters.oscillatorBLowFrequency.store(false);
    parameters.oscillatorBKeyboardTracking.store(true);

    for (auto& button : polyModDestinationButtons)
        button.setToggleState(false, juce::dontSendNotification);
    parameters.polyModToFrequencyA.store(false);
    parameters.polyModToPulseWidthA.store(false);
    parameters.polyModToFilter.store(false);
    for (std::size_t index = 0; index < lfoDestinationButtons.size(); ++index)
    {
        lfoDestinationButtons[index].setToggleState(false, juce::dontSendNotification);
        parameters.lfoDestinations[index].store(false);
    }

    applyParameters();
    repaint();
}

void AurelineMainComponent::startWavRecording()
{
    if (wavRecording.load(std::memory_order_relaxed))
        return;
    wavRecorder.start(currentSampleRate);
    wavRecording.store(true, std::memory_order_relaxed);
    wavRecordButton.setButtonText("STOP");
    wavRecordButton.setToggleState(true, juce::dontSendNotification);
    statusLabel.setText("WAV recording...", juce::dontSendNotification);
}

void AurelineMainComponent::stopWavRecordingAndChooseFile()
{
    if (!wavRecording.exchange(false, std::memory_order_relaxed))
        return;
    wavRecorder.stop();
    wavRecordButton.setButtonText("WAV");
    wavRecordButton.setToggleState(false, juce::dontSendNotification);

    if (wavRecorder.recordedFrameCount() == 0)
    {
        statusLabel.setText("WAV recording is empty", juce::dontSendNotification);
        return;
    }

    const auto musicDirectory = juce::File::getSpecialLocation(
        juce::File::userMusicDirectory);
    wavFileChooser = std::make_unique<juce::FileChooser>(
        "Save WAV recording", musicDirectory.getChildFile("Aureline.wav"),
        "*.wav");
    wavFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe = juce::Component::SafePointer<AurelineMainComponent>(this)](
            const juce::FileChooser& chooser)
        {
            if (safe == nullptr)
                return;
            auto file = chooser.getResult();
            if (file == juce::File {})
            {
                safe->statusLabel.setText("WAV save cancelled",
                                          juce::dontSendNotification);
                return;
            }
            if (!file.hasFileExtension("wav"))
                file = file.withFileExtension("wav");
            safe->writeWavRecordingToFile(file);
        });
}

void AurelineMainComponent::writeWavRecordingToFile(const juce::File& file)
{
    const auto sampleRate = wavRecorder.sampleRate();
    const auto droppedFrames = wavRecorder.droppedFrameCount();
    auto interleaved = wavRecorder.takeRecordedSamples();
    const auto frameCount = static_cast<int>(interleaved.size() / 2);
    if (frameCount <= 0)
    {
        statusLabel.setText("WAV recording is empty", juce::dontSendNotification);
        return;
    }

    juce::AudioBuffer<float> audio(2, frameCount);
    auto* left = audio.getWritePointer(0);
    auto* right = audio.getWritePointer(1);
    for (int frame = 0; frame < frameCount; ++frame)
    {
        left[frame] = interleaved[static_cast<std::size_t>(frame) * 2];
        right[frame] = interleaved[static_cast<std::size_t>(frame) * 2 + 1];
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::OutputStream> output(
        file.createOutputStream().release());
    if (output == nullptr)
    {
        statusLabel.setText("WAV save failed", juce::dontSendNotification);
        return;
    }
    auto writer = wavFormat.createWriterFor(
        output, juce::AudioFormatWriterOptions {}
                    .withSampleRate(sampleRate)
                    .withNumChannels(2)
                    .withBitsPerSample(24));
    if (writer == nullptr
        || !writer->writeFromAudioSampleBuffer(audio, 0, frameCount))
    {
        statusLabel.setText("WAV write failed", juce::dontSendNotification);
        return;
    }

    statusLabel.setText(
        "WAV saved: " + file.getFileName()
            + (droppedFrames > 0
                   ? "  |  dropped " + juce::String(droppedFrames) + " frames"
                   : juce::String {}),
        juce::dontSendNotification);
}

void AurelineMainComponent::buttonClicked(juce::Button* button)
{
    for (std::size_t index = 0; index < waveMemoryButtons.size(); ++index)
        if (button == &waveMemoryButtons[index])
        {
            openWaveMemoryEditor(index);
            return;
        }

    if (button == &wavRecordButton)
    {
        if (wavRecording.load(std::memory_order_relaxed))
            stopWavRecordingAndChooseFile();
        else
            startWavRecording();
    }
    else if (button == &previousVoiceButton || button == &nextVoiceButton)
    {
        const int itemCount = static_cast<int>(factoryVoices.size());
        const int direction = button == &previousVoiceButton ? -1 : 1;
        const int current = juce::jmax(0, selectedFactoryVoiceIndex);
        loadFactoryVoice(static_cast<std::size_t>(
            (current + direction + itemCount) % itemCount));
    }
    else if (button == &loadVoiceButton)
    {
        // macOS native file dialogs only filter reliably by the final suffix.
        // Allow XML here, then identify the compound library extension below.
        voiceFileChooser = std::make_unique<juce::FileChooser>(
            "Load Aureline voice or library",
            aurelineDocumentsDirectory(), "*.aurelinevoice;*.xml");
        voiceFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this)](const juce::FileChooser& chooser)
            {
                if (safe == nullptr || chooser.getResult() == juce::File {})
                    return;
                const auto file = chooser.getResult();
                if (file.getFullPathName().endsWithIgnoreCase(".aurelinelibrary.xml"))
                    safe->confirmAndLoadVoiceLibrary(file);
                else
                    safe->loadVoiceFromFile(file);
            });
    }
    else if (button == &saveVoiceButton)
    {
        promptAndSaveVoice();
    }
    else if (button == &copyVoiceButton)
    {
        copiedVoiceState = capturePluginState().createCopy();
        copiedVoiceName = currentVoiceName;
        pasteVoiceButton.setEnabled(true);
        statusLabel.setText("Voice copied: " + copiedVoiceName, juce::dontSendNotification);
    }
    else if (button == &pasteVoiceButton && copiedVoiceState.isValid())
    {
        restorePluginState(copiedVoiceState);
        const auto destinationName = voiceNameWithoutSlotPrefix(copiedVoiceName);
        currentVoiceName = destinationName;
        if (selectedFactoryVoiceIndex >= 0
            && selectedFactoryVoiceIndex < static_cast<int>(factoryVoices.size()))
        {
            presetBox.changeItemText(
                voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                slotVoiceDisplayName(
                    static_cast<std::size_t>(selectedFactoryVoiceIndex),
                    destinationName));
            presetBox.setSelectedId(
                voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                                    juce::dontSendNotification);
        }
        statusLabel.setText("Voice pasted temporarily into: " + destinationName,
                            juce::dontSendNotification);
    }
    else if (button == &storeVoiceButton)
        storeCurrentVoice();
    else if (button == &saveLibraryButton)
    {
        voiceFileChooser = std::make_unique<juce::FileChooser>(
            "Save current Aureline bank",
            aurelineDocumentsDirectory().getChildFile(
                "Aureline Bank " + juce::String(selectedVoiceBank + 1)
                    + ".aurelinelibrary.xml"),
            "*.xml");
        voiceFileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode
                | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<AurelineMainComponent>(this)](
                const juce::FileChooser& chooser)
            {
                if (safe != nullptr && chooser.getResult() != juce::File {})
                    safe->saveVoiceLibraryToFile(chooser.getResult());
            });
    }
    else if (button == &initVoiceButton)
    {
        resetToInitialVoice();
        currentVoiceName = "INIT ANALOG";
        if (selectedFactoryVoiceIndex >= 0
            && selectedFactoryVoiceIndex < static_cast<int>(factoryVoices.size()))
        {
            const auto slot = static_cast<std::size_t>(selectedFactoryVoiceIndex);
            presetBox.changeItemText(
                voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                                     slotVoiceDisplayName(slot, currentVoiceName));
            presetBox.setSelectedId(
                voiceMenuItemId(selectedVoiceBank, selectedFactoryVoiceIndex),
                                    juce::dontSendNotification);
            const auto destinationName = currentVoiceName;
            statusLabel.setText("Voice initialized temporarily in: " + destinationName,
                                juce::dontSendNotification);
        }
        else
        {
            statusLabel.setText("Voice initialized", juce::dontSendNotification);
        }
    }
    else if (button == &monoModeButton)
    {
        if (monoModeButton.getToggleState())
            unisonModeButton.setToggleState(false, juce::dontSendNotification);
        parameters.voiceMode.store(monoModeButton.getToggleState() ? 1
                                   : unisonModeButton.getToggleState() ? 2 : 0);
        voiceModeBox.setSelectedId(parameters.voiceMode.load() + 1,
                                   juce::dontSendNotification);
        repaint();
    }
    else if (button == &unisonModeButton)
    {
        if (unisonModeButton.getToggleState())
            monoModeButton.setToggleState(false, juce::dontSendNotification);
        parameters.voiceMode.store(unisonModeButton.getToggleState() ? 2
                                   : monoModeButton.getToggleState() ? 1 : 0);
        voiceModeBox.setSelectedId(parameters.voiceMode.load() + 1,
                                   juce::dontSendNotification);
        repaint();
    }
    else if (button == &arpButton)
    {
        parameters.arpEnabled.store(arpButton.getToggleState());
        sequencerResetRequested.store(true);
        repaint();
    }
    else if (button == &chordButton)
    {
        parameters.chordEnabled.store(chordButton.getToggleState());
        sequencerResetRequested.store(true);
        repaint();
    }
    else if (button == &arpHoldButton)
    {
        parameters.arpHold.store(arpHoldButton.getToggleState());
        if (!arpHoldButton.getToggleState())
            sequencerResetRequested.store(true);
        repaint();
    }
    else if (button == &glideLegatoButton)
        parameters.glideLegatoOnly.store(glideLegatoButton.getToggleState());
    else if (button == &lfoRetriggerButton)
        parameters.lfoRetrigger.store(lfoRetriggerButton.getToggleState());
    else if (button == &syncButton)
        parameters.oscillatorSync.store(syncButton.getToggleState());
    else if (button == &lowFrequencyButton)
        parameters.oscillatorBLowFrequency.store(lowFrequencyButton.getToggleState());
    else if (button == &keyboardTrackingButton)
        parameters.oscillatorBKeyboardTracking.store(keyboardTrackingButton.getToggleState());
    else
    {
        for (std::size_t index = 0; index < polyModDestinationButtons.size(); ++index)
            if (button == &polyModDestinationButtons[index])
            {
                const bool enabled = polyModDestinationButtons[index].getToggleState();
                if (index == 0) parameters.polyModToFrequencyA.store(enabled);
                if (index == 1) parameters.polyModToPulseWidthA.store(enabled);
                if (index == 2) parameters.polyModToFilter.store(enabled);
                repaint();
            }
    }
    for (const auto& waveformButton : waveformAButtons)
        if (button == &waveformButton)
        {
            int mask = 0;
            for (std::size_t index = 0; index < waveformAButtons.size(); ++index)
                if (waveformAButtons[index].getToggleState())
                    mask |= 1 << static_cast<int>(index);
            if (mask == 0)
            {
                button->setToggleState(true, juce::dontSendNotification);
                mask = waveformButton.waveform() == aureline::Waveform::saw ? 1
                    : waveformButton.waveform() == aureline::Waveform::triangle ? 2
                    : waveformButton.waveform() == aureline::Waveform::pulse ? 4 : 8;
            }
            parameters.waveformMaskA.store(mask);
        }
    for (const auto& waveformButton : waveformBButtons)
        if (button == &waveformButton)
        {
            int mask = 0;
            for (std::size_t index = 0; index < waveformBButtons.size(); ++index)
                if (waveformBButtons[index].getToggleState())
                    mask |= 1 << static_cast<int>(index);
            if (mask == 0)
            {
                button->setToggleState(true, juce::dontSendNotification);
                mask = waveformButton.waveform() == aureline::Waveform::saw ? 1
                    : waveformButton.waveform() == aureline::Waveform::triangle ? 2
                    : waveformButton.waveform() == aureline::Waveform::pulse ? 4 : 8;
            }
            parameters.waveformMaskB.store(mask);
        }
    for (std::size_t index = 0; index < lfoWaveformButtons.size(); ++index)
        if (button == &lfoWaveformButtons[index])
        {
            int mask = 0;
            for (std::size_t buttonIndex = 0; buttonIndex < lfoWaveformButtons.size(); ++buttonIndex)
                if (lfoWaveformButtons[buttonIndex].getToggleState())
                    mask |= 1 << static_cast<int>(buttonIndex);
            parameters.lfoWaveformMask.store(mask);
            repaint();
        }
    for (std::size_t index = 0; index < lfoDestinationButtons.size(); ++index)
        if (button == &lfoDestinationButtons[index])
            parameters.lfoDestinations[index].store(
                lfoDestinationButtons[index].getToggleState());
}

void AurelineMainComponent::timerCallback()
{
    refreshDeviceStatus();
    pitchWheel.setValue(parameters.pitchBend.load(), juce::dontSendNotification);
    modWheel.setValue(parameters.modWheel.load(), juce::dontSendNotification);
    syncPcKeyboardNotes();
    repaint();
}

void AurelineMainComponent::refreshDeviceStatus()
{
    juce::String audio;
    juce::String midi;

    if (ownsStandaloneAudio)
    {
        audio = audioDeviceStatus(deviceManager);
        midi = midiDeviceStatus(deviceManager);
    }
    else
    {
       #if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
        {
            audio = audioDeviceStatus(holder->deviceManager);
            midi = midiDeviceStatus(holder->deviceManager);
        }
        else
       #endif
        {
            audio = "Audio: host";
            midi = "MIDI: host";
        }
    }

    const auto newStatus = audio + "  |  " + midi;
    if (newStatus != lastDeviceStatus)
    {
        lastDeviceStatus = newStatus;
        statusLabel.setFont(juce::FontOptions(
            containsNonAscii(newStatus) ? 12.5f : 15.0f,
            juce::Font::bold));
        statusLabel.setText(newStatus, juce::dontSendNotification);
    }
}

void AurelineMainComponent::syncPcKeyboardNotes()
{
    std::array<bool, 128> shouldHold {};
    bool inputAllowed = hasKeyboardFocus(true);
    if (auto* peer = getPeer())
        inputAllowed = inputAllowed && peer->isFocused();

    if (inputAllowed)
    {
        for (const auto& mapping : pcKeyboardMap)
        {
            const int note = mapping.note + pcKeyboardTranspose;
            if (juce::isPositiveAndBelow(note, static_cast<int>(shouldHold.size()))
                && isPcKeyCurrentlyDown(mapping.keyCode))
                shouldHold[static_cast<std::size_t>(note)] = true;
        }
    }

    bool changed = false;
    for (int note = 0; note < static_cast<int>(pcKeyboardHeldNotes.size()); ++note)
    {
        const auto index = static_cast<std::size_t>(note);
        if (shouldHold[index] == pcKeyboardHeldNotes[index])
            continue;
        pcKeyboardHeldNotes[index] = shouldHold[index];
        changed = true;
        if (shouldHold[index])
            playNote(note, 108);
        else
            releaseNote(note);
    }
    if (changed)
        keyboard.repaint();
}

void AurelineMainComponent::paint(juce::Graphics& g)
{
    const auto drawPanel = [&g](juce::Rectangle<float> panel, float radius)
    {
        panel = panel.reduced(0.5f);
        g.setColour(juce::Colours::black.withAlpha(0.46f));
        g.fillRoundedRectangle(panel.translated(0.0f, 2.0f), radius);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2923),
                                               panel.getX(),
                                               panel.getY(),
                                               juce::Colour(0xff12120f),
                                               panel.getX(),
                                               panel.getBottom(),
                                               false));
        g.fillRoundedRectangle(panel, radius);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawLine(panel.getX() + radius, panel.getY() + 1.0f,
                   panel.getRight() - radius, panel.getY() + 1.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.42f));
        g.drawLine(panel.getX() + radius, panel.getBottom() - 1.0f,
                   panel.getRight() - radius, panel.getBottom() - 1.0f, 1.0f);
        g.setColour(juce::Colour(0xff070706));
        g.drawRoundedRectangle(panel, radius, 1.4f);
        g.setColour(juce::Colour(0xff554f40).withAlpha(0.40f));
        g.drawRoundedRectangle(panel.reduced(2.0f),
                               juce::jmax(1.0f, radius - 1.0f), 1.0f);
    };
    g.fillAll(juce::Colour(themeBackground));
    const auto woodenChassis = getLocalBounds().reduced(7).toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.46f));
    g.fillRoundedRectangle(woodenChassis.translated(0.0f, 2.0f), 6.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2923),
                                           woodenChassis.getX(),
                                           woodenChassis.getY(),
                                           juce::Colour(0xff12120f),
                                           woodenChassis.getX(),
                                           woodenChassis.getBottom(),
                                           false));
    g.fillRoundedRectangle(woodenChassis, 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine(woodenChassis.getX() + 6.0f, woodenChassis.getY() + 1.0f,
               woodenChassis.getRight() - 6.0f, woodenChassis.getY() + 1.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.drawLine(woodenChassis.getX() + 6.0f, woodenChassis.getBottom() - 1.0f,
               woodenChassis.getRight() - 6.0f, woodenChassis.getBottom() - 1.0f, 1.0f);
    g.setColour(juce::Colour(0xff070706));
    g.drawRoundedRectangle(woodenChassis, 6.0f, 1.4f);
    g.setColour(juce::Colour(0xff554f40).withAlpha(0.40f));
    g.drawRoundedRectangle(woodenChassis.reduced(2.0f), 5.0f, 1.0f);

    auto headerBounds = getLocalBounds().reduced(20).removeFromTop(34);
    auto headerArea = headerBounds.removeFromLeft(200)
                          .withTrimmedBottom(2).toFloat();
    const juce::Font logoFont(juce::FontOptions(25.0f, juce::Font::bold));
    g.setFont(logoFont);
    g.setColour(juce::Colour(themeAmber));
    g.drawText("AURELINE", headerArea.removeFromLeft(118.0f),
               juce::Justification::bottomLeft);
    headerArea.removeFromLeft(8.0f);
    auto version = juce::String(JUCE_APPLICATION_VERSION_STRING);
    if (version.endsWith(".0"))
        version = version.dropLastCharacters(2);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.setColour(juce::Colour(themeGold));
    g.drawText("v" + version, headerArea,
               juce::Justification::bottomLeft);

    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(34);
    area.removeFromTop(4);
    constexpr int unifiedRowHeight = 104;
    constexpr int unifiedRowGap = 8;
    constexpr int controlRowsInsets = 10;
    const auto display = area.removeFromTop(unifiedRowHeight);
    area.removeFromTop(unifiedRowGap);
    const auto top = area.removeFromTop(
        unifiedRowHeight * 3 + unifiedRowGap * 2 + controlRowsInsets);
    area.removeFromTop(4);
    drawPanel(area.withBottom(area.getBottom() + 2).toFloat(), 2.0f);

    auto displayContent = display.toFloat().reduced(4.0f, 0.0f)
                              .withTrimmedTop(6.0f)
                              .withHeight(static_cast<float>(unifiedRowHeight));
    const auto drawDisplayFrame = [&g](juce::Rectangle<float> bounds, const juce::String& title)
    {
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), 4.0f);
        g.setGradientFill({ juce::Colour(0xff30251d), bounds.getX(), bounds.getY(),
                            juce::Colour(0xff090706), bounds.getX(), bounds.getBottom(), false });
        g.fillRoundedRectangle(bounds, 4.0f);
        const auto screen = bounds.reduced(2.0f);
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff17100a), screen.getCentreX(), screen.getCentreY(),
            juce::Colour(0xff030201), screen.getRight(), screen.getBottom(), true));
        g.fillRoundedRectangle(screen, 3.0f);
        {
            juce::Graphics::ScopedSaveState screenClip(g);
            g.reduceClipRegion(screen.getSmallestIntegerContainer());
            g.setColour(juce::Colour(0xffffa05a).withAlpha(0.035f));
            for (float y = screen.getY() + 1.0f; y < screen.getBottom(); y += 3.0f)
                g.drawHorizontalLine(juce::roundToInt(y), screen.getX(), screen.getRight());
            g.setGradientFill({ juce::Colours::white.withAlpha(0.055f),
                                screen.getX(), screen.getY(),
                                juce::Colours::transparentWhite,
                                screen.getX(), screen.getCentreY(), false });
            g.fillRoundedRectangle(screen.withHeight(screen.getHeight() * 0.48f),
                                   3.0f);
        }
        g.setColour(juce::Colour(0xff080403));
        g.drawRoundedRectangle(bounds, 4.0f, 1.4f);
        g.setColour(juce::Colour(0xff80604a).withAlpha(0.65f));
        g.drawRoundedRectangle(screen, 3.0f, 0.8f);
        g.setColour(juce::Colour(themeText));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(title, bounds.removeFromTop(17.0f).reduced(7.0f, 0.0f),
                   juce::Justification::centredLeft, false);
    };
    const auto drawControlFrame = [&g](juce::Rectangle<float> bounds, const juce::String& title)
    {
        const auto captionFont = juce::Font(juce::FontOptions(11.5f, juce::Font::bold));
        juce::GlyphArrangement captionGlyphs;
        captionGlyphs.addLineOfText(captionFont, title, 0.0f, 0.0f);
        const auto captionWidth = std::ceil(captionGlyphs.getBoundingBox(
            0, captionGlyphs.getNumGlyphs(), true).getWidth()) + 8.0f;
        const auto caption = juce::Rectangle<float>(bounds.getX() + 10.0f,
                                                     bounds.getY() - 6.0f,
                                                     captionWidth, 12.0f);
        {
            juce::Graphics::ScopedSaveState savedState(g);
            g.excludeClipRegion(caption.getSmallestIntegerContainer());
            g.setColour(juce::Colour(themeLineGray));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 0.75f);
        }
        g.setColour(juce::Colour(themeText));
        g.setFont(captionFont);
        g.drawText(title, caption.reduced(4.0f, 0.0f),
                   juce::Justification::centredLeft, false);
    };

    auto lcdColumn = displayContent.removeFromLeft(280.0f);
    auto lcd = lcdColumn.removeFromTop(52.0f);
    displayContent.removeFromLeft(8.0f);
    auto arpeggioDisplay = displayContent.removeFromRight(250.0f);
    displayContent.removeFromRight(8.0f);
    auto mixerDisplay = displayContent.removeFromRight(228.0f);
    displayContent.removeFromRight(8.0f);
    auto scope = displayContent;
    auto filterEnvelopeDisplay = scope;
    auto amplifierDisplay = filterEnvelopeDisplay;
    auto waveformPanelArea = scope;
    constexpr float waveformRowGap = 4.0f;
    auto waveformCombinedPanel = waveformPanelArea.removeFromTop(
        juce::jmax(1.0f,
                   (waveformPanelArea.getHeight() - waveformRowGap) * 0.5f));
    waveformPanelArea.removeFromTop(waveformRowGap);
    auto waveformAPanel = waveformPanelArea.removeFromLeft(
        juce::jmax(1.0f, waveformPanelArea.getWidth() * 0.5f - 2.0f));
    waveformPanelArea.removeFromLeft(4.0f);
    auto waveformBPanel = waveformPanelArea;
    g.setGradientFill({ juce::Colour(0xff28170d), lcd.getX(), lcd.getY(),
                        juce::Colour(0xff0b0704), lcd.getX(), lcd.getBottom(), false });
    g.fillRoundedRectangle(lcd, 4.0f);
    g.setColour(juce::Colour(0xff594235));
    g.drawRoundedRectangle(lcd, 4.0f, 1.0f);
    drawDisplayFrame(waveformCombinedPanel, {});
    drawDisplayFrame(waveformAPanel, {});
    drawDisplayFrame(waveformBPanel, {});
    drawControlFrame(mixerDisplay, "MIXER");
    drawControlFrame(arpeggioDisplay, "ARPEGGIO");

    const auto writeIndex = scopeWriteIndex.load(std::memory_order_acquire);
    constexpr std::size_t levelWindow = 128;
    double meanSquare = 0.0;
    for (std::size_t index = 0; index < levelWindow; ++index)
    {
        const auto sampleIndex = (writeIndex + scopeSize - levelWindow + index) % scopeSize;
        const auto sample = scopeSamples[sampleIndex].load(std::memory_order_relaxed);
        if (std::isfinite(sample))
            meanSquare += static_cast<double>(sample) * static_cast<double>(sample);
    }
    auto measuredLevel = static_cast<float>(
        std::sqrt(meanSquare / static_cast<double>(levelWindow)) * 5.0);
    if (!std::isfinite(measuredLevel))
        measuredLevel = 0.0f;
    measuredLevel = juce::jlimit(0.0f, 1.0f, measuredLevel);
    if (!std::isfinite(waveformOutputLevel))
        waveformOutputLevel = 0.0f;
    const auto levelResponse = measuredLevel > waveformOutputLevel ? 0.68f : 0.22f;
    waveformOutputLevel += (measuredLevel - waveformOutputLevel) * levelResponse;
    if (measuredLevel < 0.0005f && waveformOutputLevel < 0.002f)
        waveformOutputLevel = 0.0f;

    constexpr std::size_t shapeSize = 256;
    std::array<std::array<float, shapeSize>, 2> oscillatorShapes {};
    std::array<float, shapeSize> combinedShape {};
    const auto renderWaveMemory = [this](double phase, int memoryIndex, int character,
                                         bool oscillatorA)
    {
        const auto oscillator = static_cast<std::size_t>(oscillatorA ? 0 : 1);
        const auto& data = userWaveMemoryActive[oscillator]
            ? userWaveMemory[oscillator]
            : aureline::waveMemoryFactoryBank()[static_cast<std::size_t>(
                juce::jlimit(0, static_cast<int>(aureline::kWaveMemoryFactoryCount) - 1,
                             memoryIndex))];
        const auto position = phase * static_cast<double>(aureline::kWaveMemorySize);
        const auto index = static_cast<std::size_t>(std::floor(position))
            % aureline::kWaveMemorySize;
        const auto quantize = [character](std::uint8_t step)
        {
            if (character == 1)
                return static_cast<double>(std::lround(static_cast<double>(step) / 31.0 * 15.0))
                    / 15.0 * 2.0 - 1.0;
            return static_cast<double>(step) / 31.0 * 2.0 - 1.0;
        };
        const auto current = quantize(data[index]);
        if (character != 2)
            return current;
        const auto next = quantize(data[(index + 1) % aureline::kWaveMemorySize]);
        const auto fraction = position - std::floor(position);
        return current + (next - current) * fraction;
    };
    const auto addOscillator = [&](bool oscillatorA)
    {
        auto& shape = oscillatorShapes[oscillatorA ? 0 : 1];
        const auto mask = oscillatorA ? parameters.waveformMaskA.load()
                                      : parameters.waveformMaskB.load();
        const auto rawLevel = oscillatorA ? parameters.oscillatorALevel.load()
                                          : parameters.oscillatorBLevel.load();
        const auto level = std::isfinite(rawLevel)
            ? juce::jlimit(0.0f, 1.0f, rawLevel) : 0.0f;
        const auto rawPulseWidth = oscillatorA ? parameters.pulseWidthA.load()
                                               : parameters.pulseWidthB.load();
        const auto pulseWidth = std::isfinite(rawPulseWidth)
            ? juce::jlimit(0.01f, 0.99f, rawPulseWidth) : 0.5f;
        const auto memoryIndex = oscillatorA ? parameters.waveMemoryIndexA.load()
                                             : parameters.waveMemoryIndexB.load();
        const auto character = oscillatorA ? parameters.waveMemoryCharacterA.load()
                                           : parameters.waveMemoryCharacterB.load();
        const auto enabledCount = ((mask & 1) != 0) + ((mask & 2) != 0)
            + ((mask & 4) != 0) + ((mask & 8) != 0);
        if (enabledCount == 0 || level <= 0.0f)
            return;
        for (std::size_t index = 0; index < shapeSize; ++index)
        {
            const auto phase = static_cast<double>(index) / shapeSize;
            double sample = 0.0;
            if ((mask & 1) != 0) sample += phase * 2.0 - 1.0;
            if ((mask & 2) != 0) sample += 1.0 - 4.0 * std::abs(phase - 0.5);
            if ((mask & 4) != 0) sample += phase < pulseWidth ? 1.0 : -1.0;
            if ((mask & 8) != 0)
                sample += renderWaveMemory(phase, memoryIndex, character, oscillatorA);
            shape[index] += static_cast<float>(
                static_cast<double>(level) * sample / std::sqrt(enabledCount));
        }
    };
    addOscillator(true);
    addOscillator(false);
    for (auto& shape : oscillatorShapes)
    {
        for (auto& sample : shape)
        {
            if (!std::isfinite(sample))
                sample = 0.0f;
            sample = juce::jlimit(-1.0f, 1.0f,
                                  sample * waveformOutputLevel);
        }
    }
    const auto rawNoiseLevel = parameters.noiseLevel.load();
    const auto noiseLevel = std::isfinite(rawNoiseLevel)
        ? juce::jlimit(0.0f, 1.0f, rawNoiseLevel) : 0.0f;
    const auto oscillatorAIsAudible = parameters.waveformMaskA.load() != 0
        && parameters.oscillatorALevel.load() > 0.0f;
    const auto oscillatorBIsAudible = parameters.waveformMaskB.load() != 0
        && parameters.oscillatorBLevel.load() > 0.0f;
    const bool combinedUsesOscillatorA = oscillatorAIsAudible || !oscillatorBIsAudible;
    const auto rawAnchorOctave = combinedUsesOscillatorA
        ? parameters.oscillatorAOctave.load() : parameters.oscillatorBOctave.load();
    const auto rawAnchorFine = combinedUsesOscillatorA
        ? 0.0f : parameters.oscillatorBFine.load();
    const auto anchorOctave = std::isfinite(rawAnchorOctave) ? rawAnchorOctave : 0.0f;
    const auto anchorFine = std::isfinite(rawAnchorFine) ? rawAnchorFine : 0.0f;
    const auto anchorRatio = std::pow(
        2.0, static_cast<double>(anchorOctave)
            + static_cast<double>(anchorFine) / 1200.0);
    for (std::size_t index = 0; index < shapeSize; ++index)
    {
        const auto phase = static_cast<double>(index) / shapeSize;
        const auto rawOctaveA = parameters.oscillatorAOctave.load();
        const auto rawOctaveB = parameters.oscillatorBOctave.load();
        const auto rawFineB = parameters.oscillatorBFine.load();
        const auto octaveA = std::isfinite(rawOctaveA) ? rawOctaveA : 0.0f;
        const auto octaveB = std::isfinite(rawOctaveB) ? rawOctaveB : 0.0f;
        const auto fineB = std::isfinite(rawFineB) ? rawFineB : 0.0f;
        const auto ratioA = std::pow(2.0, static_cast<double>(octaveA));
        const auto ratioB = std::pow(
            2.0, static_cast<double>(octaveB)
                + static_cast<double>(fineB) / 1200.0);
        const auto safeAnchorRatio = std::max(anchorRatio, 0.000001);
        const auto indexA = static_cast<std::size_t>(
            std::fmod(phase * ratioA / safeAnchorRatio, 1.0) * shapeSize) % shapeSize;
        const auto indexB = static_cast<std::size_t>(
            std::fmod(phase * ratioB / safeAnchorRatio, 1.0) * shapeSize) % shapeSize;
        // Stable pseudo-noise keeps the display stationary while representing
        // the digital noise source in the mixed waveform.
        auto state = static_cast<std::uint32_t>(index + 1u) * 747796405u + 2891336453u;
        state = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        state = (state >> 22u) ^ state;
        const auto noise = static_cast<float>(state & 0xffffu) / 32767.5f - 1.0f;
        combinedShape[index] = juce::jlimit(
            -1.0f, 1.0f,
            oscillatorShapes[0][indexA] + oscillatorShapes[1][indexB]
                + noise * noiseLevel * waveformOutputLevel);
    }

    // FINAL MIX follows the iPhone scope: display a trigger-aligned window of
    // the actual engine output, so filter, envelopes, Poly Mod and dynamics
    // are represented rather than reconstructing only the oscillator mix.
    constexpr std::size_t finalMixWindowSize = 512;
    constexpr std::size_t finalMixPreTrigger = 48;
    std::array<float, scopeSize> scopeSnapshot {};
    double scopeMean = 0.0;
    for (std::size_t index = 0; index < scopeSize; ++index)
    {
        const auto sample = scopeSamples[(writeIndex + index) % scopeSize]
                                .load(std::memory_order_relaxed);
        scopeSnapshot[index] = std::isfinite(sample) ? sample : 0.0f;
        scopeMean += scopeSnapshot[index];
    }
    scopeMean /= static_cast<double>(scopeSize);

    std::vector<std::size_t> crossings;
    const auto lastCrossing = scopeSize - (finalMixWindowSize - finalMixPreTrigger);
    for (std::size_t index = 1; index < lastCrossing; ++index)
        if (scopeSnapshot[index - 1] <= scopeMean
            && scopeSnapshot[index] > scopeMean)
            crossings.push_back(index);

    const auto finalMixRawBend =
        parameters.pitchBend.load() * parameters.pitchBendRange.load();
    const auto finalMixNote = static_cast<double>(
        waveformPitchNote.load(std::memory_order_relaxed))
        + parameters.transpose.load()
        + finalMixRawBend
        + parameters.masterTune.load() / 100.0;
    const auto finalMixFrequency = 440.0 * std::pow(
        2.0, (finalMixNote - 69.0) / 12.0);
    const auto expectedPeriod = juce::jmax(
        2.0, currentSampleRate / juce::jmax(1.0, finalMixFrequency));

    std::size_t bestCrossing = finalMixPreTrigger;
    double bestScore = std::numeric_limits<double>::max();
    bool foundPeriod = false;
    for (std::size_t first = 0; first + 1 < crossings.size(); ++first)
    {
        for (std::size_t second = first + 1; second < crossings.size(); ++second)
        {
            const auto interval = static_cast<double>(
                crossings[second] - crossings[first]);
            if (interval > expectedPeriod * 1.5)
                break;
            const auto periodError =
                std::abs(interval - expectedPeriod) / expectedPeriod;
            const auto slope = static_cast<double>(
                scopeSnapshot[crossings[first]]
                - scopeSnapshot[crossings[first] - 1]);
            const auto score = periodError - juce::jmin(0.02, slope * 0.02);
            if (score < bestScore)
            {
                bestScore = score;
                bestCrossing = crossings[first];
                foundPeriod = true;
            }
        }
    }
    if (!foundPeriod && !crossings.empty())
    {
        bestCrossing = *std::max_element(
            crossings.begin(), crossings.end(),
            [&scopeSnapshot](const auto left, const auto right)
            {
                return scopeSnapshot[left] - scopeSnapshot[left - 1]
                    < scopeSnapshot[right] - scopeSnapshot[right - 1];
            });
    }
    const auto finalMixStart = juce::jlimit<std::size_t>(
        0, scopeSize - finalMixWindowSize,
        bestCrossing > finalMixPreTrigger
            ? bestCrossing - finalMixPreTrigger : 0);
    for (std::size_t index = 0; index < shapeSize; ++index)
    {
        const auto sourceIndex = finalMixStart
            + index * finalMixWindowSize / shapeSize;
        combinedShape[index] = juce::jlimit(
            -1.0f, 1.0f, scopeSnapshot[sourceIndex]);
    }

    const auto rawBend = parameters.pitchBend.load() * parameters.pitchBendRange.load();
    const auto bend = std::isfinite(rawBend) ? juce::jlimit(-48.0f, 48.0f, rawBend) : 0.0f;
    const auto rawLfoAmount = parameters.lfoAmount.load()
        + parameters.modWheel.load() * parameters.modRange.load();
    const auto lfoAmount = std::isfinite(rawLfoAmount)
        ? juce::jlimit(0.0f, 1.0f, rawLfoAmount) : 0.0f;
    const auto rawLfoValue = engine.currentLfoValue();
    const auto lfoValue = std::isfinite(rawLfoValue)
        ? juce::jlimit(-1.0, 1.0, rawLfoValue) : 0.0;
    const bool isSilent = waveformOutputLevel <= 0.001f;
    auto combinedArea = waveformCombinedPanel.reduced(4.0f, 3.0f);
    auto oscillatorAArea = waveformAPanel.reduced(4.0f, 3.0f);
    auto oscillatorBArea = waveformBPanel.reduced(4.0f, 3.0f);
    const auto drawOscillatorWave = [&](const std::array<float, shapeSize>& shape,
                                        juce::Rectangle<float> waveArea,
                                        bool oscillatorA,
                                        bool includeOscillatorTuning,
                                        bool finalMix)
    {
        waveArea = waveArea.reduced(0.0f, 1.0f);
        const auto lfoDepth = lfoValue
            * static_cast<double>(lfoAmount * lfoAmount) * 12.0;
        double vibratoSemitones = 0.0;
        if (includeOscillatorTuning)
        {
            const auto destination = oscillatorA ? 0 : 1;
            if (parameters.lfoDestinations[
                    static_cast<std::size_t>(destination)].load())
                vibratoSemitones = lfoDepth;
        }
        else
        {
            const auto levelA = juce::jmax(0.0f, parameters.oscillatorALevel.load());
            const auto levelB = juce::jmax(0.0f, parameters.oscillatorBLevel.load());
            const auto totalLevel = levelA + levelB;
            if (totalLevel > 0.0f)
            {
                const auto modulatedLevel =
                    (parameters.lfoDestinations[0].load() ? levelA : 0.0f)
                    + (parameters.lfoDestinations[1].load() ? levelB : 0.0f);
                vibratoSemitones = lfoDepth
                    * static_cast<double>(modulatedLevel / totalLevel);
            }
        }
        const auto rawNote = static_cast<double>(
            waveformPitchNote.load(std::memory_order_relaxed))
            + static_cast<double>(bend) + vibratoSemitones;
        const auto note = std::isfinite(rawNote)
            ? juce::jlimit(0.0, 127.0, rawNote) : 60.0;
        const auto rawOctave = oscillatorA ? parameters.oscillatorAOctave.load()
                                           : parameters.oscillatorBOctave.load();
        const auto octave = includeOscillatorTuning && std::isfinite(rawOctave)
            ? juce::jlimit(-5.0, 5.0, static_cast<double>(rawOctave)) : 0.0;
        const auto rawFine = oscillatorA ? 0.0f : parameters.oscillatorBFine.load();
        const auto fine = includeOscillatorTuning && std::isfinite(rawFine)
            ? juce::jlimit(-1200.0, 1200.0, static_cast<double>(rawFine)) / 1200.0
            : 0.0;
        const auto rawCycles = finalMix ? 1.0 : 2.0 * std::pow(
            2.0, (note - 60.0) / 12.0 + octave + fine);
        const auto cycles = std::isfinite(rawCycles)
            ? juce::jlimit(0.5, 8.0, rawCycles) : 2.0;
        const auto startPhase = 0.5 - cycles * 0.5;
        const auto width = juce::jmax(1, static_cast<int>(waveArea.getWidth()));
        const auto centreHalf = juce::jmin(0.5, 0.5 / cycles);
        const auto centreStart = static_cast<int>((0.5 - centreHalf) * width);
        const auto centreEnd = static_cast<int>((0.5 + centreHalf) * width);
        const auto centreEndWithWrap = juce::jmin(width, centreEnd + 1);
        juce::Path leftWave, centreWave, rightWave;
        bool leftStarted = false, centreStarted = false, rightStarted = false;
        juce::Point<float> previousPoint;
        bool hasPreviousPoint = false;
        for (int pixel = 0; pixel <= width; ++pixel)
        {
            auto phase = std::fmod(
                startPhase + static_cast<double>(pixel) / width * cycles, 1.0);
            if (phase < 0.0)
                phase += 1.0;
            const auto sampleIndex = juce::jlimit<std::size_t>(
                0, shapeSize - 1, static_cast<std::size_t>(phase * shapeSize));
            const auto sample = std::isfinite(shape[sampleIndex])
                ? juce::jlimit(-1.0f, 1.0f,
                               shape[sampleIndex] * waveformDisplayGain)
                : 0.0f;
            const auto point = juce::Point<float>(
                waveArea.getX() + static_cast<float>(pixel),
                waveArea.getCentreY() - sample * waveArea.getHeight() * 0.47f);
            auto& path = pixel < centreStart ? leftWave
                       : pixel <= centreEndWithWrap ? centreWave : rightWave;
            auto& started = pixel < centreStart ? leftStarted
                          : pixel <= centreEndWithWrap ? centreStarted : rightStarted;
            if (!started)
            {
                path.startNewSubPath(hasPreviousPoint ? previousPoint : point);
                started = true;
                if (hasPreviousPoint)
                {
                    if (std::abs(point.y - previousPoint.y)
                        > waveArea.getHeight() * 0.18f)
                        path.lineTo(point.x, previousPoint.y);
                    path.lineTo(point);
                }
            }
            else
            {
                if (std::abs(point.y - previousPoint.y)
                    > waveArea.getHeight() * 0.18f)
                    path.lineTo(point.x, previousPoint.y);
                path.lineTo(point);
            }
            previousPoint = point;
            hasPreviousPoint = true;
        }
        if (isSilent)
        {
            const auto y = waveArea.getCentreY();
            g.setColour(juce::Colour(0xffff7a28).withAlpha(0.10f));
            g.drawLine(waveArea.getX(), y, waveArea.getRight(), y, 4.0f);
            g.setColour(juce::Colour(0xffffb06a).withAlpha(0.38f));
            g.drawLine(waveArea.getX(), y,
                       waveArea.getX() + static_cast<float>(centreStart), y, 1.0f);
            g.drawLine(waveArea.getX() + static_cast<float>(centreEnd), y,
                       waveArea.getRight(), y, 1.0f);
            g.setColour(juce::Colour(0xffffa14f).withAlpha(0.22f));
            g.drawLine(waveArea.getX() + static_cast<float>(centreStart), y,
                       waveArea.getX() + static_cast<float>(centreEnd), y, 4.0f);
            g.setColour(juce::Colour(0xffffbd72));
            g.drawLine(waveArea.getX() + static_cast<float>(centreStart), y,
                       waveArea.getX() + static_cast<float>(centreEnd), y, 1.3f);
        }
        else
        {
            g.setColour(juce::Colour(0xffff7a28).withAlpha(0.09f));
            g.strokePath(leftWave, juce::PathStrokeType(4.0f));
            g.strokePath(centreWave, juce::PathStrokeType(5.0f));
            g.strokePath(rightWave, juce::PathStrokeType(4.0f));
            g.setColour(juce::Colour(0xffffb06a).withAlpha(0.48f));
            g.strokePath(leftWave, juce::PathStrokeType(1.0f));
            g.strokePath(rightWave, juce::PathStrokeType(1.0f));
            g.setColour(juce::Colour(0xffffbd72));
            g.strokePath(centreWave, juce::PathStrokeType(1.3f));
        }
    };
    drawOscillatorWave(combinedShape, combinedArea,
                       combinedUsesOscillatorA, true, true);
    drawOscillatorWave(oscillatorShapes[0], oscillatorAArea, true, true, false);
    drawOscillatorWave(oscillatorShapes[1], oscillatorBArea, false, true, false);
    const auto waveformLabelFont = juce::Font(
        juce::FontOptions(9.0f, juce::Font::bold));
    const auto drawWaveformLabel = [&g, &waveformLabelFont](
        juce::Rectangle<float> panel, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(waveformLabelFont, text, 0.0f, 0.0f);
        const auto textWidth = glyphs.getBoundingBox(
            0, glyphs.getNumGlyphs(), true).getWidth();
        const auto label = juce::Rectangle<float>(
            panel.getX() + 10.0f, panel.getY() - 7.0f,
            textWidth + 8.0f, 14.0f);
        const auto borderMask = juce::Rectangle<float>(
            label.getX(), panel.getY() - 1.5f, label.getWidth(), 3.0f);
        g.setColour(juce::Colour(0xff17100a));
        g.fillRect(borderMask);
        g.setColour(juce::Colour(themeText));
        g.setFont(waveformLabelFont);
        g.drawText(text, label.reduced(4.0f, 0.0f),
                   juce::Justification::centredLeft, false);
    };
    drawWaveformLabel(waveformCombinedPanel, "FINAL MIX");
    drawWaveformLabel(waveformAPanel, "OSC A");
    drawWaveformLabel(waveformBPanel, "OSC B");

    const auto innerLcd = lcd.reduced(6.0f, 3.0f);
    constexpr int columnsPerChar = 5;
    constexpr int rowsPerChar = 8;
    constexpr int characterCount = 16;
    const float pitchFromWidth = innerLcd.getWidth()
        / static_cast<float>(characterCount * (columnsPerChar + 1) - 1);
    const float pitchFromHeight = innerLcd.getHeight() / 18.0f;
    const float pitch = juce::jmin(pitchFromWidth, pitchFromHeight);
    const float dot = juce::jmax(1.4f, pitch * 0.68f);
    const float matrixWidth = pitch * static_cast<float>(characterCount * 6 - 1);
    const float matrixHeight = pitch * 17.0f;
    const float startX = innerLcd.getCentreX() - matrixWidth * 0.5f;
    const float startY = innerLcd.getCentreY() - matrixHeight * 0.5f;
    const auto offDot = juce::Colour(0xff603119).withAlpha(0.46f);
    const auto onDot = juce::Colour(0xffffa04a);
    const auto voiceMode = parameters.voiceMode.load();
    const juce::String modeText = voiceMode == 1 ? "MONO" : voiceMode == 2 ? "UNI" : "POLY";
    constexpr std::array<const char*, 4> lcdBankNames {{
        "ANLG1", "ANLG2", "RETRO", "8-BIT"
    }};
    const auto bankText = "B" + juce::String(selectedVoiceBank + 1) + " "
        + lcdBankNames[static_cast<std::size_t>(
            juce::jlimit(0, 3, selectedVoiceBank))];
    const auto playLine = (modeText + " " + bankText)
                              .substring(0, characterCount).paddedRight(' ', characterCount);
    auto voiceLine = currentVoiceName.toUpperCase();
    if (voiceLine.isEmpty())
        voiceLine = "INIT ANALOG";
    voiceLine = voiceLine.substring(0, characterCount).paddedRight(' ', characterCount);
    const std::array<juce::String, 2> lcdLines { playLine, voiceLine };
    for (int line = 0; line < 2; ++line)
        for (int character = 0; character < characterCount; ++character)
        {
            const auto glyph = lcdGlyph(lcdLines[static_cast<std::size_t>(line)][character]);
            const float xBase = startX + static_cast<float>(character) * pitch * 6.0f;
            const float yBase = startY + static_cast<float>(line) * pitch * 9.0f;
            for (int column = 0; column < columnsPerChar; ++column)
                for (int row = 0; row < rowsPerChar; ++row)
                {
                    const bool enabled = (glyph[static_cast<std::size_t>(column)] & (1 << row)) != 0;
                    g.setColour(enabled ? onDot : offDot);
                    g.fillRoundedRectangle(xBase + static_cast<float>(column) * pitch,
                                           yBase + static_cast<float>(row) * pitch,
                                           dot, dot, dot * 0.22f);
                }
        }
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(lcd.reduced(5.0f).withHeight(lcd.getHeight() * 0.24f), 3.0f);

    auto topInside = top.reduced(4).withTrimmedTop(2);
    topInside.removeFromLeft(70);
    topInside.removeFromLeft(2);
    auto patchArea = topInside.removeFromLeft(244).reduced(8, 0);

    const auto drawOscillatorGroup = [&g](juce::Rectangle<float> group, const juce::String& name)
    {
        const auto captionFont = juce::Font(juce::FontOptions(11.5f, juce::Font::bold));
        juce::GlyphArrangement captionGlyphs;
        captionGlyphs.addLineOfText(captionFont, name, 0.0f, 0.0f);
        const auto captionWidth = std::ceil(captionGlyphs.getBoundingBox(
            0, captionGlyphs.getNumGlyphs(), true).getWidth()) + 8.0f;
        const auto caption = juce::Rectangle<float>(group.getX() + 10.0f,
                                                     group.getY() - 6.0f,
                                                     captionWidth, 12.0f);
        {
            juce::Graphics::ScopedSaveState savedState(g);
            g.excludeClipRegion(caption.getSmallestIntegerContainer());
            g.setColour(juce::Colour(themeLineGray));
            g.drawRoundedRectangle(group.reduced(0.5f), 4.0f, 0.75f);
        }
        g.setColour(juce::Colour(themeText));
        g.setFont(captionFont);
        g.drawText(name, caption.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
    };
    auto leftGroups = patchArea.toFloat();
    const auto rowHeight = std::floor((leftGroups.getHeight() - 16.0f) / 3.0f);
    const auto polyModGroup = leftGroups.removeFromTop(rowHeight);
    leftGroups.removeFromTop(8.0f);
    const auto performanceGroup = leftGroups.removeFromTop(rowHeight);
    leftGroups.removeFromTop(8.0f);
    const auto amplifierGroup = leftGroups.removeFromTop(rowHeight);
    drawOscillatorGroup(polyModGroup, "POLY MOD");
    drawOscillatorGroup(performanceGroup, "PERFORMANCE");
    drawOscillatorGroup(amplifierGroup, "LFO MOD");

    const auto drawEnvelopeGraph = [&g](juce::Rectangle<float> displayArea,
                                         float attack, float decay,
                                         float sustain, float release)
    {
        auto graph = displayArea.reduced(9.0f, 20.0f).withTrimmedTop(2.0f);
        const float total = juce::jmax(0.1f, attack + decay + release);
        const float attackNormalized = juce::jlimit(0.0f, 1.0f, attack / 5.0f);
        const float attackWidth = 1.5f + graph.getWidth() * 0.30f * attackNormalized;
        const float decayWidth = graph.getWidth() * (0.16f + 0.16f * decay / total);
        const float releaseWidth = graph.getWidth() * (0.16f + 0.16f * release / total);
        const float sustainWidth = juce::jmax(12.0f, graph.getWidth()
            - attackWidth - decayWidth - releaseWidth);
        const float baseY = graph.getBottom() - 2.0f;
        const float peakY = graph.getY() + 2.0f;
        const float sustainY = juce::jmap(juce::jlimit(0.0f, 1.0f, sustain), baseY, peakY);
        juce::Path envelope;
        envelope.startNewSubPath(graph.getX(), baseY);
        envelope.lineTo(graph.getX() + attackWidth, peakY);
        envelope.lineTo(graph.getX() + attackWidth + decayWidth, sustainY);
        envelope.lineTo(graph.getX() + attackWidth + decayWidth + sustainWidth, sustainY);
        envelope.lineTo(graph.getRight(), baseY);
        g.setColour(juce::Colour(0xffff7a28).withAlpha(0.14f));
        g.strokePath(envelope, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved));
        g.setColour(juce::Colour(0xffff9a42));
        g.strokePath(envelope, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    };
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    const auto previewAgeMs = nowMs - envelopePreviewChangedMs;
    const auto previewVisibleAgeMs = nowMs - envelopePreviewStartedMs;
    if (envelopePreviewKind != 0 && previewAgeMs < 2200.0)
    {
        constexpr double fadeInMs = 180.0;
        constexpr double fadeOutStartMs = 1600.0;
        constexpr double fadeOutMs = 600.0;
        const auto fadeInAlpha = juce::jlimit(0.0, 1.0,
                                              previewVisibleAgeMs / fadeInMs);
        const auto fadeOutAlpha = previewAgeMs <= fadeOutStartMs
            ? 1.0
            : juce::jlimit(0.0, 1.0,
                           (fadeOutStartMs + fadeOutMs - previewAgeMs) / fadeOutMs);
        const auto previewAlpha = static_cast<float>(
            juce::jmin(fadeInAlpha, fadeOutAlpha));
        juce::Graphics::ScopedSaveState previewState(g);
        g.beginTransparencyLayer(previewAlpha);
        const auto previewArea = envelopePreviewKind == 1
            ? filterEnvelopeDisplay : amplifierDisplay;
        drawDisplayFrame(previewArea,
                         envelopePreviewKind == 1 ? "FILTER ENV" : "AMP ENV");
        if (envelopePreviewKind == 1)
            drawEnvelopeGraph(previewArea,
                              parameters.filterAttack.load(), parameters.filterDecay.load(),
                              parameters.filterSustain.load(), parameters.filterRelease.load());
        else
            drawEnvelopeGraph(previewArea,
                              parameters.attack.load(), parameters.decay.load(),
                              parameters.sustain.load(), parameters.release.load());
        g.endTransparencyLayer();
    }
    topInside.removeFromLeft(2);
    auto oscillatorGroups = topInside.toFloat();
    auto commonColumn = oscillatorGroups.removeFromRight(280.0f);
    auto filterGroup = commonColumn.removeFromTop(rowHeight);
    commonColumn.removeFromTop(8.0f);
    auto filterEnvelopeGroup = commonColumn.removeFromTop(rowHeight);
    commonColumn.removeFromTop(8.0f);
    auto lfoModGroup = commonColumn.removeFromTop(rowHeight);
    filterEnvelopeGroup.removeFromLeft(52.0f);
    lfoModGroup.removeFromLeft(52.0f);
    oscillatorGroups.removeFromRight(8.0f);
    auto oscillatorTopRow = oscillatorGroups.removeFromTop(rowHeight);
    oscillatorGroups.removeFromTop(8.0f);
    auto oscillatorBGroup = oscillatorGroups.removeFromTop(rowHeight);
    oscillatorBGroup = oscillatorBGroup.withWidth(oscillatorBGroup.getWidth() + 52.0f);
    oscillatorGroups.removeFromTop(8.0f);
    drawOscillatorGroup(oscillatorTopRow, "OSCILLATOR A");
    drawOscillatorGroup(oscillatorBGroup, "OSCILLATOR B");
    auto lfoGroup = oscillatorGroups.removeFromTop(rowHeight);
    lfoGroup = lfoGroup.withWidth(lfoGroup.getWidth() + 52.0f);
    drawOscillatorGroup(lfoGroup, "LFO");
    drawOscillatorGroup(filterEnvelopeGroup, "FILTER ENV");
    drawOscillatorGroup(filterGroup, "FILTER");
    drawOscillatorGroup(lfoModGroup, "AMP ENV");

}

void AurelineMainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto header = area.removeFromTop(34);
    auto titleBounds = header.removeFromLeft(200);
    auto subtitleBounds = header.removeFromLeft(310).withTrimmedBottom(2);
    titleLabel.setBounds(titleBounds);
    titleLabel.setVisible(false);
    subtitleLabel.setBounds(subtitleBounds);
    auto libraryButtonArea = header.removeFromRight(112);
    saveLibraryButton.setBounds(libraryButtonArea.reduced(2, 2));
    auto wavButtonArea = header.removeFromRight(72);
    wavRecordButton.setBounds(wavButtonArea.reduced(2, 2));
    statusLabel.setBounds(header.withTrimmedBottom(2));

    area.removeFromTop(4);
    constexpr int unifiedRowHeight = 104;
    constexpr int unifiedRowGap = 8;
    constexpr int controlRowsInsets = 10;
    auto display = area.removeFromTop(unifiedRowHeight);
    auto displayContent = display.reduced(4, 0).withTrimmedTop(6)
                              .withHeight(unifiedRowHeight);
    auto lcdColumn = displayContent.removeFromLeft(280);
    lcdColumn.removeFromTop(55);
    auto presetRow = lcdColumn.removeFromTop(22);
    auto nextArea = presetRow.removeFromRight(38);
    presetRow.removeFromRight(4);
    auto previousArea = presetRow.removeFromRight(38);
    presetRow.removeFromRight(4);
    presetBox.setBounds(presetRow);
    previousVoiceButton.setBounds(previousArea);
    nextVoiceButton.setBounds(nextArea);
    lcdColumn.removeFromTop(3);
    auto voiceActions = lcdColumn.removeFromTop(22);
    constexpr int voiceActionGap = 2;
    const int actionWidth =
        (voiceActions.getWidth() - voiceActionGap * 5) / 6;
    for (auto* button : { &loadVoiceButton, &saveVoiceButton, &copyVoiceButton,
                          &pasteVoiceButton, &initVoiceButton, &storeVoiceButton })
    {
        button->setBounds(voiceActions.removeFromLeft(actionWidth));
        voiceActions.removeFromLeft(voiceActionGap);
    }
    displayContent.removeFromLeft(8);
    auto arpeggioDisplay = displayContent.removeFromRight(250).reduced(6, 0);
    displayContent.removeFromRight(8);
    auto mixerDisplay = displayContent.removeFromRight(228).reduced(5, 0);
    displayContent.removeFromRight(8);
    auto mixerRow = mixerDisplay;
    mixerRow.removeFromTop(14);
    mixerRow.removeFromBottom(6);
    constexpr std::array<std::size_t, 3> mixerIndices { 0, 1, 5 };
    for (std::size_t position = 0; position < mixerIndices.size(); ++position)
    {
        const auto index = mixerIndices[position];
        auto cell = juce::Rectangle<int>(mixerRow.getX() + static_cast<int>(position) * 52,
                                         mixerRow.getY(), 50, mixerRow.getHeight());
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }
    lowFrequencyButton.setBounds(mixerRow.getX() + 156, mixerRow.getY(), 32, 70);
    keyboardTrackingButton.setBounds(mixerRow.getX() + 190, mixerRow.getY(), 32, 70);
    arpeggioDisplay.removeFromTop(10);
    const int arpeggioCellWidth = arpeggioDisplay.getWidth() / 4;
    auto layoutTopArpKnob = [](juce::Rectangle<int> cell,
                               juce::Label& label, juce::Slider& knob)
    {
        label.setBounds(cell.removeFromTop(12));
        label.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
        knob.setBounds(cell.withSizeKeepingCentre(54, 62));
    };
    layoutTopArpKnob(arpeggioDisplay.removeFromLeft(arpeggioCellWidth), scaleLabel, scaleKnob);
    for (std::size_t index = 0; index < arpKnobs.size(); ++index)
        layoutTopArpKnob(arpeggioDisplay.removeFromLeft(arpeggioCellWidth),
                         arpLabels[index], arpKnobs[index]);
    area.removeFromTop(unifiedRowGap);
    auto top = area.removeFromTop(
                        unifiedRowHeight * 3 + unifiedRowGap * 2 + controlRowsInsets)
                   .reduced(4).withTrimmedTop(2);
    auto leftControls = top.removeFromLeft(70).reduced(4, 2);
    const int leftControlRowHeight = leftControls.getHeight() / 4;
    auto layoutNumberedKnobRow = [](juce::Rectangle<int> row,
                                    juce::Label& label, juce::Slider& knob,
                                    const juce::String& styleName)
    {
        label.setBounds(row.removeFromTop(15));
        label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setName(styleName);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setBounds(row.withSizeKeepingCentre(
            58, juce::jmin(66, row.getHeight())));
    };
    layoutNumberedKnobRow(leftControls.removeFromTop(leftControlRowHeight),
                          knobLabels[18], knobs[18], "numberedKnob");
    layoutNumberedKnobRow(leftControls.removeFromTop(leftControlRowHeight),
                          vintageLabel, vintageKnob, "numberedKnob");
    layoutNumberedKnobRow(leftControls.removeFromTop(leftControlRowHeight),
                          tempoLabel, tempoKnob, "tempoNumberedKnob");
    layoutNumberedKnobRow(leftControls, performanceLabels[2],
                          performanceKnobs[2], "numberedKnob");

    top.removeFromLeft(2);
    auto patch = top.removeFromLeft(244).reduced(8, 0);
    top.removeFromLeft(8);
    auto commonPanel = top.removeFromRight(280);
    top.removeFromRight(8);
    auto oscillatorPanel = top;
    const int threeRowHeight = (patch.getHeight() - 16) / 3;
    const auto fixedCell = [](juce::Rectangle<int> bounds, int offset, int width)
    {
        return juce::Rectangle<int>(bounds.getX() + 10 + offset, bounds.getY(),
                                    width, bounds.getHeight());
    };
    constexpr int waveformButtonWidth = 32;
    constexpr int waveformButtonHeight = 66;
    constexpr int waveformButtonCellWidth = 34;

    auto polyModRow = patch.removeFromTop(threeRowHeight).reduced(5, 0);
    polyModRow.removeFromTop(14);
    polyModRow.removeFromBottom(6);
    for (std::size_t position = 0; position < 2; ++position)
    {
        const auto index = position + 16;
        auto cell = fixedCell(polyModRow, static_cast<int>(position) * 52, 50);
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }
    auto polyButtonGroup = fixedCell(polyModRow, 104, 102);
    for (std::size_t index = 0; index < polyModDestinationButtons.size(); ++index)
    {
        const auto cell = polyButtonGroup.removeFromLeft(34);
        auto& button = polyModDestinationButtons[index];
        button.setBounds(cell.getCentreX() - 16, polyModRow.getY(), 32, 70);
    }

    patch.removeFromTop(8);
    auto performanceRow = patch.removeFromTop(threeRowHeight).reduced(5, 0);
    performanceRow.removeFromTop(14);
    performanceRow.removeFromBottom(6);
    auto spreadCell = fixedCell(performanceRow, 0, 50);
    spreadLabel.setBounds(spreadCell.removeFromTop(16));
    spreadKnob.setBounds(spreadCell.withSizeKeepingCentre(50, spreadCell.getHeight()));
    auto transposeCell = fixedCell(performanceRow, 52, 50);
    transposeLabel.setBounds(transposeCell.removeFromTop(16));
    transposeFader.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    transposeFader.setName({});
    transposeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    transposeFader.setBounds(transposeCell.withSizeKeepingCentre(50, transposeCell.getHeight()));
    auto performanceButtonGroup = fixedCell(performanceRow, 104, 102);
    const auto monoCell = performanceButtonGroup.removeFromLeft(34);
    const auto unisonCell = performanceButtonGroup.removeFromLeft(34);
    const auto syncCell = performanceButtonGroup.removeFromLeft(34);
    monoModeButton.setBounds(monoCell.getX(), performanceRow.getY(), 32, 70);
    unisonModeButton.setBounds(unisonCell.getX(), performanceRow.getY(), 32, 70);
    syncButton.setBounds(syncCell.getX(), performanceRow.getY(), 32, 70);
    patch.removeFromTop(8);
    auto leftLfoModRow = patch.removeFromTop(threeRowHeight).reduced(5, 0);
    leftLfoModRow.removeFromTop(14);
    leftLfoModRow.removeFromBottom(6);
    auto leftLfoButtonArea = leftLfoModRow.reduced(10, 0);
    const int lfoModButtonTravel = leftLfoButtonArea.getWidth() - 32;
    for (std::size_t index = 0; index < lfoDestinationButtons.size(); ++index)
    {
        const int x = leftLfoButtonArea.getX()
            + juce::roundToInt(static_cast<float>(lfoModButtonTravel * static_cast<int>(index))
                               / static_cast<float>(lfoDestinationButtons.size() - 1));
        lfoDestinationButtons[index].setBounds(x, leftLfoModRow.getY(), 32, 70);
    }

    const auto layoutOscillatorRow = [this, &fixedCell](juce::Rectangle<int> row,
                                                        bool oscillatorA)
    {
        row = row.reduced(5, 0);
        row.removeFromTop(14);
        row.removeFromBottom(6);
        auto rangeArea = fixedCell(row, 0, 50);
        auto& rangeLabel = oscillatorA ? oscillatorARangeLabel : oscillatorBRangeLabel;
        rangeLabel.setBounds(rangeArea.removeFromTop(16));
        auto& range = oscillatorA ? oscillatorARangeKnob : oscillatorBRangeKnob;
        range.setBounds(rangeArea.reduced(1, 0));
        auto shapeGroup = fixedCell(row, 52, 136);
        const auto shapeGroupBounds = shapeGroup;
        auto& shapeLabel = oscillatorA ? oscillatorAShapeLabel : oscillatorBShapeLabel;
        shapeLabel.setBounds(shapeGroupBounds.getX(), row.getBottom() - 14,
                             shapeGroupBounds.getWidth(), 14);
        auto& shapeButtons = oscillatorA ? waveformAButtons : waveformBButtons;
        for (std::size_t index = 0; index < shapeButtons.size(); ++index)
        {
            const auto cell = shapeGroup.removeFromLeft(waveformButtonCellWidth);
            shapeButtons[index].setBounds(cell.getX(), row.getY(),
                                          waveformButtonWidth, waveformButtonHeight);
        }
        const auto memoryIndex = static_cast<std::size_t>(oscillatorA ? 0 : 1);
        const auto pwIndex = static_cast<std::size_t>(oscillatorA ? 3 : 4);
        auto pwArea = fixedCell(row, 190, 50);
        knobLabels[pwIndex].setText("PW", juce::dontSendNotification);
        knobLabels[pwIndex].setBounds(pwArea.removeFromTop(16));
        knobs[pwIndex].setBounds(pwArea);
        const auto memoryX = row.getX() + 260;
        const auto memoryWidth = 84;
        const auto memoryHeight = 22;
        const auto memoryStackHeight = memoryHeight * 3 + 6;
        const auto memoryY = row.getCentreY() - memoryStackHeight / 2;
        waveMemoryBoxes[memoryIndex].setBounds(memoryX, memoryY,
                                               memoryWidth, memoryHeight);
        waveCharacterBoxes[memoryIndex].setBounds(memoryX, memoryY + 25,
                                                  memoryWidth, memoryHeight);
        waveMemoryButtons[memoryIndex].setBounds(memoryX, memoryY + 50,
                                                 memoryWidth, memoryHeight);

        if (!oscillatorA)
        {
            auto detuneArea = juce::Rectangle<int>(row.getRight() - 50, row.getY(),
                                                   50, row.getHeight());
            knobLabels[2].setBounds(detuneArea.removeFromTop(16));
            knobs[2].setBounds(detuneArea.withSizeKeepingCentre(
                50, detuneArea.getHeight()));
        }

    };
    auto oscillatorTopRow = oscillatorPanel.removeFromTop(threeRowHeight);
    layoutOscillatorRow(oscillatorTopRow, true);
    oscillatorPanel.removeFromTop(8);
    auto oscillatorBPanel = oscillatorPanel.removeFromTop(threeRowHeight);
    oscillatorBPanel = oscillatorBPanel.withWidth(oscillatorBPanel.getWidth() + 52);
    layoutOscillatorRow(oscillatorBPanel, false);
    oscillatorPanel.removeFromTop(8);
    auto lfoPanel = oscillatorPanel.removeFromTop(threeRowHeight);
    auto lfoRow = lfoPanel.reduced(5, 0);
    lfoRow.removeFromTop(14);
    lfoRow.removeFromBottom(6);
    auto rateCell = fixedCell(lfoRow, 0, 50);
    knobLabels[14].setBounds(rateCell.removeFromTop(16));
    knobs[14].setBounds(rateCell.withSizeKeepingCentre(50, rateCell.getHeight()));
    constexpr int lfoWaveformButtonWidth = 30;
    constexpr int lfoWaveformButtonCellWidth = 30;
    auto lfoShapeGroup = fixedCell(lfoRow, 52,
                                   lfoWaveformButtonCellWidth
                                       * static_cast<int>(lfoWaveformButtons.size()));
    const auto lfoShapeGroupBounds = lfoShapeGroup;
    lfoShapeLabel.setBounds(lfoShapeGroupBounds.getX(), lfoRow.getBottom() - 14,
                            lfoShapeGroupBounds.getWidth(), 14);
    for (std::size_t index = 0; index < lfoWaveformButtons.size(); ++index)
    {
        const auto cell = lfoShapeGroup.removeFromLeft(lfoWaveformButtonCellWidth);
        lfoWaveformButtons[index].setBounds(cell.getX(), lfoRow.getY(),
                                            lfoWaveformButtonWidth, waveformButtonHeight);
    }
    const std::array<juce::Slider*, 3> remainingLfoUnits {
        &knobs[15], &lfoDelayKnob, &lfoFadeKnob
    };
    const std::array<juce::Label*, 3> remainingLfoLabels {
        &knobLabels[15], &lfoDelayLabel, &lfoFadeLabel
    };
    for (std::size_t index = 0; index < remainingLfoUnits.size(); ++index)
    {
        auto cell = fixedCell(lfoRow, 204 + static_cast<int>(index) * 52, 50);
        remainingLfoLabels[index]->setBounds(cell.removeFromTop(16));
        remainingLfoUnits[index]->setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }
    auto retriggerCell = fixedCell(lfoRow, 360, 34);
    lfoRetriggerButton.setBounds(retriggerCell.getX() + 1, lfoRow.getY(), 32, 70);

    constexpr std::array<std::size_t, 8> filterIndices {
        6, 7, 8, 9,
        19, 20, 21, 22
    };
    constexpr int commonUnitWidth = 50;
    auto filterPanel = commonPanel.removeFromTop(threeRowHeight * 2 + 8);
    commonPanel.removeFromTop(8);
    auto lfoModPanel = commonPanel.removeFromTop(threeRowHeight);
    const auto layoutCommonKnob = [this](juce::Rectangle<int> cell, std::size_t index)
    {
        cell.removeFromTop(14);
        cell.removeFromBottom(6);
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    };
    for (std::size_t position = 0; position < filterIndices.size() + 1; ++position)
    {
        const bool filterControl = position <= 4;
        const int column = filterControl ? static_cast<int>(position)
                                         : static_cast<int>(position - 4);
        const int rowIndex = filterControl ? 0 : 1;
        const int rowY = filterPanel.getY() + rowIndex * (threeRowHeight + 8);
        auto cell = fixedCell(juce::Rectangle<int>(filterPanel.getX(), rowY,
                                                   filterPanel.getWidth(), threeRowHeight),
                              column * 52, commonUnitWidth);
        if (position == 4)
        {
            cell.removeFromTop(14);
            cell.removeFromBottom(6);
            performanceLabels[4].setBounds(cell.removeFromTop(16));
            performanceKnobs[4].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
        }
        else
        {
            const auto sourceIndex = position < 4 ? position : position - 1;
            layoutCommonKnob(cell, filterIndices[sourceIndex]);
        }
    }
    auto amplifierRow = lfoModPanel.reduced(4, 0);
    amplifierRow.removeFromTop(14);
    amplifierRow.removeFromBottom(6);
    for (std::size_t position = 0; position < 4; ++position)
    {
        const auto index = position + 10;
        auto cell = fixedCell(amplifierRow, (static_cast<int>(position) + 1) * 52, 50);
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }

    area.removeFromTop(4);
    auto performance = area.reduced(2);
    auto pitchArea = performance.removeFromLeft(44).reduced(1, 0);
    pitchLabel.setBounds(pitchArea.removeFromBottom(14).translated(0, -2));
    pitchWheel.setBounds(pitchArea.reduced(5, 0));
    auto modArea = performance.removeFromLeft(44).reduced(1, 0);
    modLabel.setBounds(modArea.removeFromBottom(14).translated(0, -2));
    modWheel.setBounds(modArea.reduced(5, 0));
    auto performanceControls = performance.removeFromLeft(100).translated(0, 3);
    struct KeyboardPerformanceControl
    {
        juce::Slider* knob;
        juce::Label* label;
    };
    const std::array<KeyboardPerformanceControl, 4> keyboardPerformanceControls {{
        { &performanceKnobs[0], &performanceLabels[0] },
        { &performanceKnobs[1], &performanceLabels[1] },
        { &modRangeKnob, &modRangeLabel },
        { &performanceKnobs[3], &performanceLabels[3] }
    }};
    for (std::size_t position = 0; position < keyboardPerformanceControls.size(); ++position)
    {
        const int row = static_cast<int>(position) / 2;
        const int column = static_cast<int>(position) % 2;
        auto cell = juce::Rectangle<int>(performanceControls.getX() + column * 50,
                                         performanceControls.getY() + row * 68,
                                         50, 68).reduced(2, 0);
        const int verticalOffset = row == 0 ? -3 : -6;
        auto& control = keyboardPerformanceControls[position];
        control.label->setBounds(cell.removeFromTop(10)
                                     .translated(0, row == 0 ? 0 : -3));
        control.knob->setBounds(cell.withSizeKeepingCentre(48, 52)
                                    .translated(0, verticalOffset));
    }
    auto performanceButtonGrid = performance.removeFromLeft(72).reduced(2, 1);
    constexpr int performanceButtonWidth = 32;
    const int performanceButtonHeight = performanceButtonGrid.getHeight() / 2;
    const int secondColumnX = performanceButtonGrid.getRight() - performanceButtonWidth;
    arpButton.setBounds(performanceButtonGrid.getX(), performanceButtonGrid.getY(),
                        performanceButtonWidth, performanceButtonHeight);
    chordButton.setBounds(secondColumnX, performanceButtonGrid.getY(),
                          performanceButtonWidth, performanceButtonHeight);
    glideLegatoButton.setBounds(performanceButtonGrid.getX(),
                                performanceButtonGrid.getY() + performanceButtonHeight,
                                performanceButtonWidth, performanceButtonHeight);
    arpHoldButton.setBounds(secondColumnX,
                            performanceButtonGrid.getY() + performanceButtonHeight,
                            performanceButtonWidth, performanceButtonHeight);
    performance.removeFromLeft(4);
    keyboard.setBounds(performance.reduced(2));
    if (waveMemoryEditorWindow != nullptr)
    {
        waveMemoryEditorWindow->setBounds(getLocalBounds());
        waveMemoryEditorWindow->toFront(false);
    }
}
