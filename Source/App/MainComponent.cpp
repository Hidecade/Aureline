#include "App/MainComponent.h"
#include "BinaryData.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int firstKeyboardNote = 48;
constexpr int keyboardNoteCount = 37;
constexpr int pcKeyboardTranspose = 12;

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
    const auto inactiveTick = juce::Colour(0xff34383a).withAlpha(0.82f);
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
    g.setGradientFill({ juce::Colour(0xff424039), outer.getX(), outer.getY(),
                        juce::Colour(0xff070706), outer.getRight(), outer.getBottom(), false });
    g.fillEllipse(outer);
    g.setColour(juce::Colour(0xff050505));
    g.drawEllipse(outer, 2.0f);

    const auto inner = outer.reduced(outer.getWidth() * 0.16f);
    g.setGradientFill({ juce::Colour(0xff22221f), inner.getX(), inner.getY(),
                        juce::Colour(0xff0a0a09), inner.getRight(), inner.getBottom(), false });
    g.fillEllipse(inner);
    g.setColour(juce::Colour(0xff000000).withAlpha(0.75f));
    g.drawEllipse(inner, 1.25f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.65f, -radius * 0.54f, 3.3f, radius * 0.43f, 1.2f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.setGradientFill({ juce::Colour(0xfff0f2f1), centre.x, centre.y - radius * 0.54f,
                        juce::Colour(0xff777d80), centre.x, centre.y, false });
    g.fillPath(pointer);
}

void AurelineMainComponent::AurelineLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPosition,
    float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
        return;
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
        g.setColour(juce::Colour(0xffc7cac9));
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
    g.setColour(juce::Colour(0xffd69a36));
    g.fillRoundedRectangle(juce::Rectangle<float>(track.getX() - 4.0f,
                                                   sliderPosition - 5.0f,
                                                   track.getWidth() + 8.0f, 10.0f), 2.0f);
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
                            down ? juce::Colour(0xffd39328) : juce::Colour(0xff050403),
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
    if (sawUp)
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

AurelineMainComponent::AurelineMainComponent(bool useStandaloneAudio)
    : ownsStandaloneAudio(useStandaloneAudio), keyboard(*this)
{
    woodBackground = juce::ImageFileFormat::loadFrom(
        BinaryData::aurelinewoodbackground_png,
        BinaryData::aurelinewoodbackground_pngSize);
    setLookAndFeel(&lookAndFeel);
    titleLabel.setText("AURELINE", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff050607));
    titleLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(titleLabel);
    subtitleLabel.setText("8-VOICE ANALOG MODELING SYNTHESIZER", juce::dontSendNotification);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff08090a));
    subtitleLabel.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(subtitleLabel);
    statusLabel.setText("Audio ready  |  MIDI: all inputs", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8f8068));
    statusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusLabel);

    presetBox.addItem("INIT ANALOG", 1);
    for (std::size_t index = 0; index < factoryVoices.size(); ++index)
        presetBox.addItem(factoryVoices[index].name, static_cast<int>(index) + 2);
    presetBox.setSelectedId(1);
    presetBox.addListener(this);
    addAndMakeVisible(presetBox);
    initVoiceButton.addListener(this);
    initVoiceButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff251b14));
    initVoiceButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffbf5728));
    initVoiceButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffad55));
    initVoiceButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(initVoiceButton);
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
    for (auto* button : { &waveformAButtons[0], &waveformAButtons[1], &waveformAButtons[2] })
    {
        button->addListener(this);
        addAndMakeVisible(*button);
    }
    for (auto* button : { &waveformBButtons[0], &waveformBButtons[1], &waveformBButtons[2] })
    {
        button->addListener(this);
        addAndMakeVisible(*button);
    }
    waveformAButtons[0].setToggleState(true, juce::dontSendNotification);
    waveformBButtons[0].setToggleState(true, juce::dontSendNotification);
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
        label->setColour(juce::Label::textColourId, juce::Colour(0xffc7c9c8));
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

    const std::array<const char*, 23> names { "VCO A", "VCO B", "B FINE", "PW A", "PW B",
        "NOISE", "CUTOFF", "RESONANCE", "FILTER ENV", "KEY TRACK", "ATTACK", "DECAY",
        "SUSTAIN", "RELEASE", "LFO RATE", "INITIAL AMT", "FILTER ENV", "OSC B", "VOLUME",
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

    const auto useZeroToTenDisplay = [](juce::Slider& slider)
    {
        slider.setNumDecimalPlacesToDisplay(0);
        slider.textFromValueFunction = [&slider](double value)
        {
            const auto position = slider.valueToProportionOfLength(value);
            return juce::String(juce::roundToInt(position * 10.0));
        };
        slider.valueFromTextFunction = [&slider](const juce::String& text)
        {
            const auto position = juce::jlimit(0.0, 1.0, text.getDoubleValue() / 10.0);
            return slider.proportionOfLengthToValue(position);
        };
        slider.updateText();
    };
    constexpr std::array<std::size_t, 21> zeroToTenKnobIndices {
        0, 1, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17,
        19, 20, 21, 22
    };
    for (const auto index : zeroToTenKnobIndices)
        useZeroToTenDisplay(knobs[index]);
    for (auto* slider : { &spreadKnob, &vintageKnob, &lfoDelayKnob, &lfoFadeKnob,
                          &performanceKnobs[3], &arpKnobs[2] })
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
        label->setColour(juce::Label::textColourId, juce::Colour(0xffc7c9c8));
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    pitchLabel.setText("PITCH", juce::dontSendNotification);
    modLabel.setText("MOD", juce::dontSendNotification);
    transposeLabel.setText("TRANSPOSE", juce::dontSendNotification);
    transposeLabel.setFont(juce::FontOptions(8.0f));
    addAndMakeVisible(keyboard);

    for (auto& note : heldNotes)
        note.store(false);
    pcKeyboardHeldNotes.fill(false);
    for (auto& sample : scopeSamples)
        sample.store(0.0f);
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
    label.setColour(juce::Label::textColourId, juce::Colour(0xffc7c9c8));
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
    patch.oscillatorB.sawEnabled = (waveformMaskB & 1) != 0;
    patch.oscillatorB.triangleEnabled = (waveformMaskB & 2) != 0;
    patch.oscillatorB.pulseEnabled = (waveformMaskB & 4) != 0;
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
    patch.lfoDelaySeconds = parameters.lfoDelay.load();
    patch.lfoFadeSeconds = parameters.lfoFade.load();
    patch.lfoRetrigger = parameters.lfoRetrigger.load();
    // Keep the full knob/wheel range musical: pitch remains vibrato rather than
    // an octave trill, while PW and filter still reach clearly audible depths.
    patch.lfoPitchDepthASemitones = parameters.lfoDestinations[0].load() ? 1.0 : 0.0;
    patch.lfoPitchDepthBSemitones = parameters.lfoDestinations[1].load() ? 1.0 : 0.0;
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
    const int stride = juce::jmax(1, info.numSamples / 128);
    auto writeIndex = scopeWriteIndex.load(std::memory_order_relaxed);
    for (int sample = 0; sample < info.numSamples; sample += stride)
    {
        scopeSamples[writeIndex].store(left[sample], std::memory_order_relaxed);
        writeIndex = (writeIndex + 1) % scopeSize;
    }
    scopeWriteIndex.store(writeIndex, std::memory_order_release);
}

void AurelineMainComponent::releaseResources() { engine.panic(); }

void AurelineMainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message);
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
    midiCollector.addMessageToQueue(message);
}

juce::ValueTree AurelineMainComponent::capturePluginState() const
{
    juce::ValueTree state("AurelineState");
    state.setProperty("version", 1, nullptr);
#define SAVE_FLOAT(name) state.setProperty(#name, parameters.name.load(), nullptr)
#define SAVE_INT(name) state.setProperty(#name, parameters.name.load(), nullptr)
#define SAVE_BOOL(name) state.setProperty(#name, parameters.name.load(), nullptr)
    SAVE_FLOAT(oscillatorALevel); SAVE_FLOAT(oscillatorBLevel); SAVE_FLOAT(oscillatorBFine);
    SAVE_FLOAT(pulseWidthA); SAVE_FLOAT(pulseWidthB); SAVE_FLOAT(noiseLevel);
    SAVE_FLOAT(cutoff); SAVE_FLOAT(resonance); SAVE_FLOAT(filterEnvelope);
    SAVE_FLOAT(filterKeyboardTracking); SAVE_FLOAT(filterAttack); SAVE_FLOAT(filterDecay);
    SAVE_FLOAT(filterSustain); SAVE_FLOAT(filterRelease); SAVE_FLOAT(attack);
    SAVE_FLOAT(decay); SAVE_FLOAT(sustain); SAVE_FLOAT(release); SAVE_FLOAT(lfoRate);
    SAVE_FLOAT(lfoAmount); SAVE_FLOAT(lfoDelay); SAVE_FLOAT(lfoFade);
    SAVE_FLOAT(polyModFilterEnvelope); SAVE_FLOAT(polyModOscillatorB); SAVE_FLOAT(spread);
    SAVE_FLOAT(vintage); SAVE_FLOAT(tempoBpm); SAVE_FLOAT(master); SAVE_FLOAT(transpose);
    SAVE_FLOAT(pitchBendRange); SAVE_FLOAT(glide); SAVE_FLOAT(masterTune);
    SAVE_FLOAT(unisonDetune); SAVE_FLOAT(filterVelocity);
    SAVE_FLOAT(oscillatorAOctave); SAVE_FLOAT(oscillatorBOctave); SAVE_FLOAT(arpGate);
    SAVE_INT(scaleRoot); SAVE_INT(voiceMode); SAVE_INT(waveformMaskA);
    SAVE_INT(waveformMaskB); SAVE_INT(lfoWaveformMask); SAVE_INT(arpRate);
    SAVE_INT(arpDirection);
    SAVE_BOOL(lfoRetrigger); SAVE_BOOL(glideLegatoOnly); SAVE_BOOL(oscillatorSync);
    SAVE_BOOL(oscillatorBLowFrequency); SAVE_BOOL(oscillatorBKeyboardTracking);
    SAVE_BOOL(polyModToFrequencyA); SAVE_BOOL(polyModToPulseWidthA);
    SAVE_BOOL(polyModToFilter); SAVE_BOOL(arpEnabled); SAVE_BOOL(chordEnabled);
    SAVE_BOOL(arpHold);
    for (std::size_t index = 0; index < parameters.lfoDestinations.size(); ++index)
        state.setProperty("lfoDestination" + juce::String(index),
                          parameters.lfoDestinations[index].load(), nullptr);
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
    LOAD_FLOAT(lfoAmount); LOAD_FLOAT(lfoDelay); LOAD_FLOAT(lfoFade);
    LOAD_FLOAT(polyModFilterEnvelope); LOAD_FLOAT(polyModOscillatorB); LOAD_FLOAT(spread);
    LOAD_FLOAT(vintage); LOAD_FLOAT(tempoBpm); LOAD_FLOAT(master); LOAD_FLOAT(transpose);
    LOAD_FLOAT(pitchBendRange); LOAD_FLOAT(glide); LOAD_FLOAT(masterTune);
    LOAD_FLOAT(unisonDetune); LOAD_FLOAT(filterVelocity);
    LOAD_FLOAT(oscillatorAOctave); LOAD_FLOAT(oscillatorBOctave); LOAD_FLOAT(arpGate);
    LOAD_INT(scaleRoot); LOAD_INT(voiceMode); LOAD_INT(waveformMaskA);
    LOAD_INT(waveformMaskB); LOAD_INT(lfoWaveformMask); LOAD_INT(arpRate);
    LOAD_INT(arpDirection);
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
        const int selectedIndex = presetBox.getSelectedId() - 2;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(factoryVoices.size()))
            loadFactoryVoice(static_cast<std::size_t>(selectedIndex));
    }
    else if (box == &voiceModeBox)
    {
        parameters.voiceMode.store(juce::jlimit(0, 2, voiceModeBox.getSelectedId() - 1));
        repaint();
    }
}

void AurelineMainComponent::loadFactoryVoice(std::size_t index)
{
    if (index >= factoryVoices.size())
        return;

    const auto& voice = factoryVoices[index];
    resetToInitialVoice();

    const auto setWaveform = [](auto& buttons, int mask)
    {
        for (std::size_t buttonIndex = 0; buttonIndex < buttons.size(); ++buttonIndex)
            buttons[buttonIndex].setToggleState((mask & (1 << static_cast<int>(buttonIndex))) != 0,
                                                juce::dontSendNotification);
    };
    setWaveform(waveformAButtons, voice.waveA);
    setWaveform(waveformBButtons, voice.waveB);
    parameters.waveformMaskA.store(voice.waveA);
    parameters.waveformMaskB.store(voice.waveB);

    oscillatorARangeKnob.setValue(voice.octaveA, juce::sendNotificationSync);
    oscillatorBRangeKnob.setValue(voice.octaveB, juce::sendNotificationSync);
    knobs[0].setValue(voice.levelA, juce::sendNotificationSync);
    knobs[1].setValue(voice.levelB, juce::sendNotificationSync);
    knobs[2].setValue(voice.fine, juce::sendNotificationSync);
    knobs[5].setValue(voice.noise, juce::sendNotificationSync);
    knobs[6].setValue(voice.cutoff, juce::sendNotificationSync);
    knobs[7].setValue(voice.resonance, juce::sendNotificationSync);
    knobs[8].setValue(voice.filterEnvelope, juce::sendNotificationSync);
    knobs[9].setValue(0.35, juce::sendNotificationSync);

    knobs[10].setValue(voice.attack, juce::sendNotificationSync);
    knobs[11].setValue(voice.decay, juce::sendNotificationSync);
    knobs[12].setValue(voice.sustain, juce::sendNotificationSync);
    knobs[13].setValue(voice.release, juce::sendNotificationSync);
    const auto filterAttack = voice.filterAttack >= 0.0f
        ? voice.filterAttack : juce::jmin(voice.attack, 1.5f);
    const auto filterDecay = voice.filterDecay >= 0.0f
        ? voice.filterDecay : juce::jmax(0.08f, voice.decay * 0.8f);
    const auto filterSustain = voice.filterSustain >= 0.0f
        ? voice.filterSustain : juce::jlimit(0.0f, 1.0f, voice.sustain * 0.72f);
    const auto filterRelease = voice.filterRelease >= 0.0f
        ? voice.filterRelease : voice.release;
    knobs[19].setValue(filterAttack, juce::sendNotificationSync);
    knobs[20].setValue(filterDecay, juce::sendNotificationSync);
    knobs[21].setValue(filterSustain, juce::sendNotificationSync);
    knobs[22].setValue(filterRelease, juce::sendNotificationSync);

    knobs[14].setValue(voice.lfoRate, juce::sendNotificationSync);
    knobs[15].setValue(voice.lfoAmount, juce::sendNotificationSync);
    spreadKnob.setValue(voice.spread, juce::sendNotificationSync);
    vintageKnob.setValue(voice.vintage, juce::sendNotificationSync);

    const bool hasVibrato = voice.lfoAmount > 0.001f;
    for (std::size_t destination = 0; destination < lfoDestinationButtons.size(); ++destination)
    {
        const bool enabled = hasVibrato && destination < 2;
        lfoDestinationButtons[destination].setToggleState(enabled, juce::dontSendNotification);
        parameters.lfoDestinations[destination].store(enabled);
    }

    syncButton.setToggleState(voice.sync, juce::dontSendNotification);
    parameters.oscillatorSync.store(voice.sync);
    monoModeButton.setToggleState(voice.voiceMode == 1, juce::dontSendNotification);
    unisonModeButton.setToggleState(voice.voiceMode == 2, juce::dontSendNotification);
    parameters.voiceMode.store(voice.voiceMode);
    voiceModeBox.setSelectedId(voice.voiceMode + 1, juce::dontSendNotification);

    presetBox.setSelectedId(static_cast<int>(index) + 2, juce::dontSendNotification);
    applyParameters();
    repaint();
}

void AurelineMainComponent::resetToInitialVoice()
{
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

    presetBox.setSelectedId(1, juce::dontSendNotification);
    applyParameters();
    repaint();
}

void AurelineMainComponent::buttonClicked(juce::Button* button)
{
    if (button == &initVoiceButton)
        resetToInitialVoice();
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
                    : waveformButton.waveform() == aureline::Waveform::triangle ? 2 : 4;
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
                    : waveformButton.waveform() == aureline::Waveform::triangle ? 2 : 4;
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
    syncPcKeyboardNotes();
    repaint();
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
        g.setGradientFill({ juce::Colour(0xff151719), panel.getX(), panel.getY(),
                            juce::Colour(0xff030405), panel.getX(), panel.getBottom(), false });
        g.fillRoundedRectangle(panel, radius);
        g.setColour(juce::Colour(0xff050504));
        g.drawRoundedRectangle(panel, radius, 2.0f);
        g.setColour(juce::Colour(0xff202326));
        g.drawRoundedRectangle(panel.reduced(2.0f), radius - 1.0f, 1.0f);
    };
    g.fillAll(juce::Colour(0xff11120f));
    const auto woodenChassis = getLocalBounds().reduced(7).toFloat();
    {
        juce::Graphics::ScopedSaveState savedState(g);
        juce::Path chassisClip;
        chassisClip.addRoundedRectangle(woodenChassis, 6.0f);
        g.reduceClipRegion(chassisClip);
        if (woodBackground.isValid())
            g.drawImage(woodBackground, woodenChassis,
                        juce::RectanglePlacement::fillDestination);
        else
        {
            g.setColour(juce::Colour(0xff4a2412));
            g.fillRect(woodenChassis);
        }
    }

    auto headerArea = getLocalBounds().reduced(20).removeFromTop(34)
                          .removeFromLeft(190).toFloat().reduced(0.0f, 1.0f);
    g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff050607));
    g.drawText("Aureline", headerArea.removeFromLeft(112.0f),
               juce::Justification::centredLeft);
    auto version = juce::String(JUCE_APPLICATION_VERSION_STRING);
    if (version.endsWith(".0"))
        version = version.dropLastCharacters(2);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff303335));
    g.drawText("v" + version, headerArea.withTrimmedBottom(6.0f),
               juce::Justification::bottomLeft);

    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(34);
    area.removeFromTop(4);
    const auto display = area.removeFromTop(103);
    drawPanel(display.toFloat(), 2.0f);
    area.removeFromTop(4);
    const auto top = area.removeFromTop(339);
    drawPanel(top.toFloat(), 2.0f);
    area.removeFromTop(4);
    drawPanel(area.toFloat(), 2.0f);

    auto displayContent = display.reduced(8, 7).toFloat();
    const auto drawDisplayFrame = [&g](juce::Rectangle<float> bounds, const juce::String& title)
    {
        g.setGradientFill({ juce::Colour(0xff101719), bounds.getX(), bounds.getY(),
                            juce::Colour(0xff030708), bounds.getX(), bounds.getBottom(), false });
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xff51443c));
        g.drawRoundedRectangle(bounds, 4.0f, 1.2f);
        g.setColour(juce::Colour(0xffc7c9c8));
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
            g.setColour(juce::Colour(0xff737779));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 0.75f);
        }
        g.setColour(juce::Colour(0xffd5d7d6));
        g.setFont(captionFont);
        g.drawText(title, caption.reduced(4.0f, 0.0f),
                   juce::Justification::centredLeft, false);
    };

    auto lcdColumn = displayContent.removeFromLeft(280.0f);
    auto lcd = lcdColumn.removeFromTop(56.0f);
    displayContent.removeFromLeft(8.0f);
    auto arpeggioDisplay = displayContent.removeFromRight(250.0f);
    displayContent.removeFromRight(8.0f);
    const auto displayMeterWidth = (displayContent.getWidth() - 16.0f) / 3.0f;
    auto scope = displayContent.removeFromLeft(displayMeterWidth);
    displayContent.removeFromLeft(8.0f);
    auto filterEnvelopeDisplay = displayContent.removeFromLeft(displayMeterWidth);
    displayContent.removeFromLeft(8.0f);
    auto amplifierDisplay = displayContent;
    g.setGradientFill({ juce::Colour(0xff28170d), lcd.getX(), lcd.getY(),
                        juce::Colour(0xff0b0704), lcd.getX(), lcd.getBottom(), false });
    g.fillRoundedRectangle(lcd, 4.0f);
    g.setColour(juce::Colour(0xff594235));
    g.drawRoundedRectangle(lcd, 4.0f, 1.0f);
    drawDisplayFrame(scope, "WAVEFORM");
    drawDisplayFrame(filterEnvelopeDisplay, "FILTER ENV");
    drawDisplayFrame(amplifierDisplay, "AMP ENV");
    drawControlFrame(arpeggioDisplay, "ARPEGGIO");

    scope = scope.reduced(8.0f, 20.0f).withTrimmedTop(2.0f);
    juce::Path wave;
    const auto writeIndex = scopeWriteIndex.load(std::memory_order_acquire);
    std::array<float, scopeSize> waveformSamples {};
    float peak = 0.0f;
    for (std::size_t index = 0; index < scopeSize; ++index)
    {
        waveformSamples[index] = scopeSamples[(writeIndex + index) % scopeSize]
            .load(std::memory_order_relaxed);
        peak = juce::jmax(peak, std::abs(waveformSamples[index]));
    }

    const float triggerThreshold = juce::jmax(0.002f, peak * 0.05f);
    std::size_t earlierCrossing = 0;
    std::size_t previousCrossing = 0;
    std::size_t latestCrossing = 0;
    bool hasEarlierCrossing = false;
    bool hasPreviousCrossing = false;
    bool hasLatestCrossing = false;
    bool triggerArmed = false;
    for (std::size_t index = 1; index < scopeSize; ++index)
    {
        if (waveformSamples[index] <= -triggerThreshold)
            triggerArmed = true;
        if (!triggerArmed || waveformSamples[index] < triggerThreshold)
            continue;
        if (!hasLatestCrossing || index - latestCrossing >= 8)
        {
            earlierCrossing = previousCrossing;
            hasEarlierCrossing = hasPreviousCrossing;
            previousCrossing = latestCrossing;
            hasPreviousCrossing = hasLatestCrossing;
            latestCrossing = index;
            hasLatestCrossing = true;
        }
        triggerArmed = false;
    }

    std::size_t cycleLength = juce::jmin<std::size_t>(512, scopeSize - 1);
    std::size_t cycleStart = scopeSize - cycleLength - 1;
    if (hasEarlierCrossing && hasPreviousCrossing && hasLatestCrossing
        && previousCrossing > earlierCrossing)
    {
        const auto singleCycleLength = previousCrossing - earlierCrossing;
        const auto halfCycleLength = singleCycleLength / 2;
        const auto proposedStart = earlierCrossing >= halfCycleLength
            ? earlierCrossing - halfCycleLength : 0;
        const auto proposedLength = singleCycleLength * 2;
        if (proposedStart + proposedLength < scopeSize)
        {
            cycleStart = proposedStart;
            cycleLength = proposedLength;
        }
    }
    for (int pixel = 0; pixel <= static_cast<int>(scope.getWidth()); ++pixel)
    {
        const auto normalized = static_cast<float>(pixel) / juce::jmax(1.0f, scope.getWidth());
        const auto samplePosition = normalized * static_cast<float>(cycleLength);
        const auto sampleOffset = static_cast<std::size_t>(samplePosition);
        const auto nextOffset = juce::jmin(sampleOffset + 1, cycleLength);
        const auto fraction = samplePosition - static_cast<float>(sampleOffset);
        const auto firstSample = waveformSamples[cycleStart + sampleOffset];
        const auto secondSample = waveformSamples[cycleStart + nextOffset];
        constexpr float displayGain = 3.5f;
        const auto sample = juce::jlimit(-1.0f, 1.0f,
            juce::jmap(fraction, firstSample, secondSample) * displayGain);
        const auto point = juce::Point<float>(scope.getX() + static_cast<float>(pixel),
                                              scope.getCentreY() - sample * scope.getHeight() * 0.47f);
        if (pixel == 0)
            wave.startNewSubPath(point);
        else
            wave.lineTo(point);
    }
    g.setColour(juce::Colour(0xffff7a28).withAlpha(0.14f));
    g.strokePath(wave, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved));
    g.setColour(juce::Colour(0xffff9a42));
    g.strokePath(wave, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));

    const auto innerLcd = lcd.reduced(6.0f);
    constexpr int columnsPerChar = 5;
    constexpr int rowsPerChar = 8;
    constexpr int characterCount = 18;
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
    const auto tempoText = juce::String(juce::roundToInt(parameters.tempoBpm.load()));
    const auto playLine = ("PLAY " + modeText + " TEMPO" + tempoText)
                              .substring(0, characterCount).paddedRight(' ', characterCount);
    auto voiceLine = presetBox.getText().toUpperCase();
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
            g.setColour(juce::Colour(0xff737779));
            g.drawRoundedRectangle(group.reduced(0.5f), 4.0f, 0.75f);
        }
        g.setColour(juce::Colour(0xffd5d7d6));
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
    drawEnvelopeGraph(filterEnvelopeDisplay,
                      parameters.filterAttack.load(), parameters.filterDecay.load(),
                      parameters.filterSustain.load(), parameters.filterRelease.load());
    drawEnvelopeGraph(amplifierDisplay,
                      parameters.attack.load(), parameters.decay.load(),
                      parameters.sustain.load(), parameters.release.load());
    topInside.removeFromLeft(2);
    auto oscillatorGroups = topInside.toFloat();
    auto commonColumn = oscillatorGroups.removeFromRight(228.0f);
    auto commonGroup = commonColumn.removeFromTop(rowHeight * 2.0f + 8.0f);
    commonColumn.removeFromTop(8.0f);
    auto lfoModGroup = commonColumn.removeFromTop(rowHeight);
    oscillatorGroups.removeFromRight(8.0f);
    auto oscillatorTopRow = oscillatorGroups.removeFromTop(rowHeight);
    auto mixerGroup = oscillatorTopRow.removeFromRight(174.0f);
    oscillatorTopRow.removeFromRight(8.0f);
    oscillatorGroups.removeFromTop(8.0f);
    auto oscillatorBGroup = oscillatorGroups.removeFromTop(rowHeight);
    oscillatorGroups.removeFromTop(8.0f);
    drawOscillatorGroup(oscillatorTopRow, "OSCILLATOR A");
    drawOscillatorGroup(mixerGroup, "MIXER");
    drawOscillatorGroup(oscillatorBGroup, "OSCILLATOR B");
    drawOscillatorGroup(oscillatorGroups.removeFromTop(rowHeight), "LFO");
    drawOscillatorGroup(commonGroup, "FILTER");
    drawOscillatorGroup(lfoModGroup, "AMPLIFIER");

}

void AurelineMainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto header = area.removeFromTop(34);
    auto titleBounds = header.removeFromLeft(190);
    auto subtitleBounds = header.removeFromLeft(300).withTrimmedBottom(4);
    titleLabel.setBounds(titleBounds);
    titleLabel.setVisible(false);
    subtitleLabel.setBounds(subtitleBounds);
    statusLabel.setBounds(header);

    area.removeFromTop(4);
    auto display = area.removeFromTop(103).reduced(2);
    auto displayContent = display.reduced(6, 5);
    auto lcdColumn = displayContent.removeFromLeft(280);
    lcdColumn.removeFromTop(61);
    auto presetRow = lcdColumn.removeFromTop(24);
    auto initButtonArea = presetRow.removeFromRight(76);
    presetRow.removeFromRight(4);
    presetBox.setBounds(presetRow);
    initVoiceButton.setBounds(initButtonArea);
    displayContent.removeFromLeft(8);
    auto arpeggioDisplay = displayContent.removeFromRight(250).reduced(6, 0);
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
    area.removeFromTop(4);
    auto top = area.removeFromTop(339).reduced(4).withTrimmedTop(2);
    auto leftControls = top.removeFromLeft(70).reduced(4, 2);
    const int leftControlRowHeight = leftControls.getHeight() / 3;
    auto layoutNumberedKnobRow = [](juce::Rectangle<int> row,
                                    juce::Label& label, juce::Slider& knob,
                                    const juce::String& styleName)
    {
        label.setBounds(row.removeFromTop(15));
        label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setName(styleName);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setBounds(row.withSizeKeepingCentre(62, 82));
    };
    layoutNumberedKnobRow(leftControls.removeFromTop(leftControlRowHeight),
                          knobLabels[18], knobs[18], "numberedKnob");
    layoutNumberedKnobRow(leftControls.removeFromTop(leftControlRowHeight),
                          vintageLabel, vintageKnob, "numberedKnob");
    layoutNumberedKnobRow(leftControls, tempoLabel, tempoKnob, "tempoNumberedKnob");

    top.removeFromLeft(2);
    auto patch = top.removeFromLeft(244).reduced(8, 0);
    top.removeFromLeft(8);
    auto commonPanel = top.removeFromRight(228);
    top.removeFromRight(8);
    auto oscillatorPanel = top;
    const int threeRowHeight = (patch.getHeight() - 16) / 3;
    const auto fixedCell = [](juce::Rectangle<int> bounds, int offset, int width)
    {
        return juce::Rectangle<int>(bounds.getX() + 10 + offset, bounds.getY(),
                                    width, bounds.getHeight());
    };

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
        auto shapeGroup = fixedCell(row, 52, 102);
        const auto shapeGroupBounds = shapeGroup;
        auto& shapeLabel = oscillatorA ? oscillatorAShapeLabel : oscillatorBShapeLabel;
        shapeLabel.setBounds(shapeGroupBounds.getX(), row.getBottom() - 14,
                             shapeGroupBounds.getWidth(), 14);
        auto& shapeButtons = oscillatorA ? waveformAButtons : waveformBButtons;
        for (std::size_t index = 0; index < shapeButtons.size(); ++index)
        {
            const auto cell = shapeGroup.removeFromLeft(34);
            shapeButtons[index].setBounds(cell.getX(), row.getY(), 32, 66);
        }
        const auto pwIndex = static_cast<std::size_t>(oscillatorA ? 3 : 4);
        auto pwArea = fixedCell(row, 156, 50);
        knobLabels[pwIndex].setText("PW", juce::dontSendNotification);
        knobLabels[pwIndex].setBounds(pwArea.removeFromTop(16));
        knobs[pwIndex].setBounds(pwArea);

        if (!oscillatorA)
        {
            auto fineArea = fixedCell(row, 208, 50);
            knobLabels[2].setBounds(fineArea.removeFromTop(16));
            knobs[2].setBounds(fineArea.withSizeKeepingCentre(50, fineArea.getHeight()));
            auto oscillatorButtonGroup = fixedCell(row, 260, 68);
            const auto lfCell = oscillatorButtonGroup.removeFromLeft(34);
            const auto kbCell = oscillatorButtonGroup.removeFromLeft(34);
            lowFrequencyButton.setBounds(lfCell.getCentreX() - 16, row.getY(), 32, 70);
            keyboardTrackingButton.setBounds(kbCell.getCentreX() - 16, row.getY(), 32, 70);
        }
    };
    auto oscillatorTopRow = oscillatorPanel.removeFromTop(threeRowHeight);
    auto mixerPanel = oscillatorTopRow.removeFromRight(174);
    oscillatorTopRow.removeFromRight(8);
    layoutOscillatorRow(oscillatorTopRow, true);

    auto mixerRow = mixerPanel;
    mixerRow.removeFromTop(14);
    mixerRow.removeFromBottom(6);
    constexpr std::array<std::size_t, 3> mixerIndices { 0, 1, 5 };
    for (std::size_t position = 0; position < mixerIndices.size(); ++position)
    {
        const auto index = mixerIndices[position];
        auto cell = fixedCell(mixerRow, static_cast<int>(position) * 52, 50);
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }
    oscillatorPanel.removeFromTop(8);
    auto oscillatorBPanel = oscillatorPanel.removeFromTop(threeRowHeight);
    layoutOscillatorRow(oscillatorBPanel, false);
    oscillatorPanel.removeFromTop(8);
    auto lfoPanel = oscillatorPanel.removeFromTop(threeRowHeight);
    auto lfoRow = lfoPanel.reduced(5, 0);
    lfoRow.removeFromTop(14);
    lfoRow.removeFromBottom(6);
    auto rateCell = fixedCell(lfoRow, 0, 50);
    knobLabels[14].setBounds(rateCell.removeFromTop(16));
    knobs[14].setBounds(rateCell.withSizeKeepingCentre(50, rateCell.getHeight()));
    auto lfoShapeGroup = fixedCell(lfoRow, 52, 140);
    const auto lfoShapeGroupBounds = lfoShapeGroup;
    lfoShapeLabel.setBounds(lfoShapeGroupBounds.getX(), lfoRow.getBottom() - 14,
                            lfoShapeGroupBounds.getWidth(), 14);
    for (std::size_t index = 0; index < lfoWaveformButtons.size(); ++index)
    {
        const auto cell = lfoShapeGroup.removeFromLeft(28);
        lfoWaveformButtons[index].setBounds(cell.getX(), lfoRow.getY(), 26, 66);
    }
    const std::array<juce::Slider*, 3> remainingLfoUnits {
        &knobs[15], &lfoDelayKnob, &lfoFadeKnob
    };
    const std::array<juce::Label*, 3> remainingLfoLabels {
        &knobLabels[15], &lfoDelayLabel, &lfoFadeLabel
    };
    for (std::size_t index = 0; index < remainingLfoUnits.size(); ++index)
    {
        auto cell = fixedCell(lfoRow, 194 + static_cast<int>(index) * 52, 50);
        remainingLfoLabels[index]->setBounds(cell.removeFromTop(16));
        remainingLfoUnits[index]->setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }
    auto retriggerCell = fixedCell(lfoRow, 350, 34);
    lfoRetriggerButton.setBounds(retriggerCell.getX() + 1, lfoRow.getY(), 32, 70);

    constexpr std::array<std::size_t, 8> filterIndices {
        6, 7, 8, 9,
        19, 20, 21, 22
    };
    constexpr int commonColumns = 4;
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
    for (std::size_t position = 0; position < filterIndices.size(); ++position)
    {
        const int column = static_cast<int>(position) % commonColumns;
        const int rowIndex = static_cast<int>(position) / commonColumns;
        const int rowY = filterPanel.getY() + rowIndex * (threeRowHeight + 8);
        auto cell = fixedCell(juce::Rectangle<int>(filterPanel.getX(), rowY,
                                                   filterPanel.getWidth(), threeRowHeight),
                              column * 52, commonUnitWidth);
        layoutCommonKnob(cell, filterIndices[position]);
    }
    auto amplifierRow = lfoModPanel.reduced(4, 0);
    amplifierRow.removeFromTop(14);
    amplifierRow.removeFromBottom(6);
    for (std::size_t position = 0; position < 4; ++position)
    {
        const auto index = position + 10;
        auto cell = fixedCell(amplifierRow, static_cast<int>(position) * 52, 50);
        knobLabels[index].setBounds(cell.removeFromTop(16));
        knobs[index].setBounds(cell.withSizeKeepingCentre(50, cell.getHeight()));
    }

    area.removeFromTop(4);
    auto performance = area.reduced(2);
    auto pitchArea = performance.removeFromLeft(44).reduced(1, 0);
    pitchLabel.setBounds(pitchArea.removeFromBottom(16));
    pitchWheel.setBounds(pitchArea.reduced(5, 0));
    auto modArea = performance.removeFromLeft(44).reduced(1, 0);
    modLabel.setBounds(modArea.removeFromBottom(16));
    modWheel.setBounds(modArea.reduced(5, 0));
    auto performanceControls = performance.removeFromLeft(150).translated(0, 3);
    for (std::size_t index = 0; index < performanceKnobs.size(); ++index)
    {
        const int row = static_cast<int>(index) / 3;
        const int column = static_cast<int>(index) % 3;
        auto cell = juce::Rectangle<int>(performanceControls.getX() + column * 50,
                                         performanceControls.getY() + row * 68,
                                         50, 68).reduced(2, 0);
        const int verticalOffset = row == 0 ? -3 : -6;
        performanceLabels[index].setBounds(cell.removeFromTop(10)
                                               .translated(0, row == 0 ? 0 : -3));
        performanceKnobs[index].setBounds(cell.withSizeKeepingCentre(48, 52)
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
}
