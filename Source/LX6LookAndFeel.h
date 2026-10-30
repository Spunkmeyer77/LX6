// LX-6 - look and feel (dark panel, voice colours, round knobs).
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace lx6
{
namespace colours
{
    const juce::Colour bg        { 0xff15181b };
    const juce::Colour panel     { 0xff1d2125 };
    const juce::Colour panelDark { 0xff191c20 };
    const juce::Colour seqBg     { 0xff1a1d21 };
    const juce::Colour well      { 0xff0f1214 };
    const juce::Colour border    { 0xff2a3036 };
    const juce::Colour control   { 0xff262b30 };
    const juce::Colour ctrlEdge  { 0xff353c43 };
    const juce::Colour knobEdge  { 0xff3a424a };
    const juce::Colour text      { 0xffd7dde2 };
    const juce::Colour textBright{ 0xfff2f4f5 };
    const juce::Colour dim       { 0xff8d969e };
    const juce::Colour label     { 0xff9aa3ad };
    const juce::Colour accent    { 0xffe0623a };
    const juce::Colour lfo       { 0xff4fa3d9 };

    inline juce::Colour voice (int v)
    {
        static const juce::uint32 c[] { 0xffe0623a, 0xffd9b23a, 0xff4fbf8a, 0xff4fa3d9, 0xffa77fe0, 0xffe07fb0, 0xffe07fb0 };
        return juce::Colour (c[juce::jlimit (0, 6, v)]);
    }
}

class LX6LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LX6LookAndFeel();

    static juce::Font uiFont (float height, bool bold = false);
    static juce::Font monoFont (float height);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (12.0f); }
    juce::Font getPopupMenuFont() override { return uiFont (14.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return monoFont (13.0f); }
    juce::Font getLabelFont (juce::Label&) override { return uiFont (13.0f); }
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
};
}
