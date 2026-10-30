// LX-6 - look and feel (dark panel, voice colours, round knobs).
// SPDX-License-Identifier: GPL-3.0-only
#include "LX6LookAndFeel.h"
#include "BinaryData.h"

namespace lx6
{
namespace
{
    juce::Typeface::Ptr typeface (const void* data, size_t size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, size);
    }
    juce::Typeface::Ptr medium() { static auto t = typeface (BinaryData::SpaceGroteskMedium_ttf, BinaryData::SpaceGroteskMedium_ttfSize); return t; }
    juce::Typeface::Ptr bold()   { static auto t = typeface (BinaryData::SpaceGroteskBold_ttf, BinaryData::SpaceGroteskBold_ttfSize); return t; }
    juce::Typeface::Ptr mono()   { static auto t = typeface (BinaryData::JetBrainsMonoMedium_ttf, BinaryData::JetBrainsMonoMedium_ttfSize); return t; }
}

juce::Font LX6LookAndFeel::uiFont (float height, bool isBold)
{
    return juce::Font (juce::FontOptions (isBold ? bold() : medium()).withHeight (height));
}

juce::Font LX6LookAndFeel::monoFont (float height)
{
    return juce::Font (juce::FontOptions (mono()).withHeight (height));
}

LX6LookAndFeel::LX6LookAndFeel()
{
    using namespace colours;
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Label::textColourId, text);
    setColour (juce::TextButton::buttonColourId, control);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, bg);
    setColour (juce::ComboBox::backgroundColourId, well);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, ctrlEdge);
    setColour (juce::ComboBox::arrowColourId, dim);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, bg);
    setColour (juce::PopupMenu::headerTextColourId, dim);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::TooltipWindow::backgroundColourId, panel);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::ToggleButton::tickColourId, accent);
    setColour (juce::ToggleButton::tickDisabledColourId, dim);
    setDefaultSansSerifTypeface (medium());
}

void LX6LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider& s)
{
    const float size = (float) juce::jmin (w, h) - 2.0f;
    const auto bounds = juce::Rectangle<float> (size, size).withCentre ({ x + w * 0.5f, y + h * 0.5f });
    const auto pointer = s.findColour (juce::Slider::thumbColourId);

    g.setColour (colours::control);
    g.fillEllipse (bounds);
    g.setColour (s.isMouseOverOrDragging() ? colours::knobEdge.brighter (0.3f) : colours::knobEdge);
    g.drawEllipse (bounds.reduced (1.0f), 2.0f);

    // pointer, like the mockup: a short rounded bar from near the edge
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float r = size * 0.5f;
    juce::Path p;
    const float pw = juce::jmax (3.0f, size * 0.085f);
    p.addRoundedRectangle (-pw * 0.5f, -r + 3.0f, pw, r * 0.38f, pw * 0.5f);
    g.setColour (pointer);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (bounds.getCentre()));
}

void LX6LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical)
        return LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);

    const float cx = x + w * 0.5f;
    g.setColour (colours::well);
    g.fillRoundedRectangle (cx - 3.0f, (float) y, 6.0f, (float) h, 3.0f);

    const auto cap = juce::Rectangle<float> (24.0f, 12.0f).withCentre ({ cx, pos });
    g.setColour (s.isMouseOverOrDragging() ? colours::textBright : juce::Colour (0xffc9cfd4));
    g.fillRoundedRectangle (cap, 2.0f);
    g.setColour (colours::bg.withAlpha (0.6f));
    g.fillRect (cap.withSizeKeepingCentre (16.0f, 1.0f));
}

void LX6LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto fill = on ? b.findColour (juce::TextButton::buttonOnColourId) : b.findColour (juce::TextButton::buttonColourId);
    if (over && ! on) fill = fill.brighter (0.12f);
    if (down) fill = fill.darker (0.15f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 3.0f);
    if (! on)
    {
        g.setColour (colours::ctrlEdge);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }
}

void LX6LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (uiFont (juce::jmin (12.0f, b.getHeight() * 0.55f), b.getToggleState()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1);
}

void LX6LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& c)
{
    auto r = juce::Rectangle<float> ((float) w, (float) h).reduced (0.5f);
    g.setColour (c.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (c.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (colours::dim);
    g.setFont (uiFont (12.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbe"), w - 18, 0, 14, h, juce::Justification::centred);
}

void LX6LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (colours::panel);
    g.setColour (colours::ctrlEdge);
    g.drawRect (0, 0, w, h);
}

void LX6LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& t, int w, int h)
{
    g.fillAll (colours::panel);
    g.setColour (colours::ctrlEdge);
    g.drawRect (0, 0, w, h);
    g.setColour (colours::text);
    g.setFont (uiFont (13.0f));
    g.drawFittedText (t, 6, 2, w - 12, h - 4, juce::Justification::centredLeft, 3);
}
}
