// LX-6 - overlay panels: About window and LXR hardware settings.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "HardwareLink.h"

namespace lx6
{
// a dimmed full-window overlay with a centred card; click outside or press CLOSE to dismiss
class Overlay : public juce::Component
{
public:
    Overlay (int cardW, int cardH);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onClose;

protected:
    juce::Rectangle<int> card;
    juce::TextButton closeButton { "CLOSE" };
    virtual void layoutCard (juce::Rectangle<int>) {}
    virtual void paintCard (juce::Graphics&, juce::Rectangle<int>) {}

private:
    int cw, ch;
};

class AboutOverlay : public Overlay
{
public:
    AboutOverlay();
private:
    void paintCard (juce::Graphics&, juce::Rectangle<int>) override;
    void layoutCard (juce::Rectangle<int>) override;
    juce::HyperlinkButton store { "ticcoross.gumroad.com", juce::URL ("https://ticcoross.gumroad.com/") };
    juce::HyperlinkButton mail { "ticcoross@gmail.com", juce::URL ("mailto:ticcoross@gmail.com") };
    juce::HyperlinkButton source { "Source code (GPL-3.0)", juce::URL ("https://github.com/Spunkmeyer77/LX6") };
};

class HardwareOverlay : public Overlay, private juce::Timer
{
public:
    HardwareOverlay (HardwareLink&, std::function<void()> sendKit);
private:
    void paintCard (juce::Graphics&, juce::Rectangle<int>) override;
    void layoutCard (juce::Rectangle<int>) override;
    void timerCallback() override;
    void refreshDevices();

    HardwareLink& link;
    juce::TextButton modeButtons[3] { juce::TextButton ("OFF"), juce::TextButton ("LXR ONLY"), juce::TextButton ("LX-6 + LXR") };
    juce::ComboBox device, paramChannel;
    juce::OwnedArray<juce::ComboBox> voiceChannels;
    juce::TextButton sendKitButton { "SEND KIT TO LXR" };
    juce::String status;
};
}
