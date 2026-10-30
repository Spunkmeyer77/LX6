// LX-6 - overlay panels: About window and LXR hardware settings.
// SPDX-License-Identifier: GPL-3.0-only
#include "AboutWindow.h"
#include "LX6LookAndFeel.h"
#include "LxrParams.h"

namespace lx6
{
using LAF = LX6LookAndFeel;

Overlay::Overlay (int w, int h) : cw (w), ch (h)
{
    addAndMakeVisible (closeButton);
    closeButton.onClick = [this] { if (onClose) onClose(); };
}

void Overlay::resized()
{
    card = getLocalBounds().withSizeKeepingCentre (cw, ch);
    closeButton.setBounds (card.getRight() - 96, card.getBottom() - 44, 80, 28);
    layoutCard (card);
}

void Overlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.72f));
    g.setColour (colours::panel);
    g.fillRoundedRectangle (card.toFloat(), 10.0f);
    g.setColour (colours::border.brighter (0.2f));
    g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 10.0f, 1.0f);
    // accent strip
    g.setColour (colours::accent);
    g.fillRoundedRectangle (card.toFloat().removeFromTop (4.0f).reduced (14.0f, 0.0f), 2.0f);
    paintCard (g, card);
}

void Overlay::mouseDown (const juce::MouseEvent& e)
{
    if (! card.contains (e.getPosition()) && onClose)
        onClose();
}

//==============================================================================
AboutOverlay::AboutOverlay() : Overlay (560, 420)
{
    for (auto* l : { &store, &mail, &source })
    {
        l->setFont (LAF::uiFont (15.0f), false, juce::Justification::centredLeft);
        l->setColour (juce::HyperlinkButton::textColourId, colours::accent);
        addAndMakeVisible (*l);
    }
}

void AboutOverlay::layoutCard (juce::Rectangle<int> c)
{
    store.setBounds (c.getX() + 40, c.getY() + 248, 260, 22);
    mail.setBounds (c.getX() + 40, c.getY() + 274, 260, 22);
    source.setBounds (c.getX() + 40, c.getY() + 300, 260, 22);
}

void AboutOverlay::paintCard (juce::Graphics& g, juce::Rectangle<int> c)
{
    const int x = c.getX() + 40;
    int y = c.getY() + 36;

    // big logo with voice-colour dots
    g.setColour (colours::textBright);
    g.setFont (LAF::uiFont (64.0f, true));
    g.drawText ("LX-6", x, y, 300, 70, juce::Justification::centredLeft);
    for (int v = 0; v < 6; ++v)
    {
        g.setColour (colours::voice (v));
        g.fillEllipse ((float) (x + 4 + v * 18), (float) (y + 78), 10.0f, 10.0f);
    }
    g.setColour (colours::dim);
    g.setFont (LAF::monoFont (13.0f));
    g.drawText ("v" LX6_VERSION_STRING, c.getRight() - 160, y + 18, 120, 20, juce::Justification::centredRight);

    y += 104;
    g.setColour (colours::text);
    g.setFont (LAF::uiFont (17.0f, true));
    g.drawText ("Six-voice digital drum synthesizer", x, y, 480, 24, juce::Justification::centredLeft);
    g.setColour (colours::label);
    g.setFont (LAF::uiFont (14.0f));
    g.drawFittedText ("Sound engine based on the open-source Erica Synths LXR firmware.\n"
                      "LX-6 is an independent product, not made or endorsed by Erica Synths.",
                      x, y + 28, 480, 44, juce::Justification::topLeft, 3);

    g.setColour (colours::dim);
    g.setFont (LAF::uiFont (12.0f));
    g.drawFittedText ("Made by Ticco Ross. Free software under the GNU GPL v3: you can study, change and share it. "
                      "No warranty. Fonts: Space Grotesk, JetBrains Mono (SIL OFL).",
                      x, c.getBottom() - 86, 360, 60, juce::Justification::topLeft, 4);
}

//==============================================================================
HardwareOverlay::HardwareOverlay (HardwareLink& l, std::function<void()> sendKit) : Overlay (640, 470), link (l)
{
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[i];
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { link.setMode ((HardwareLink::Mode) i); timerCallback(); };
        addAndMakeVisible (b);
    }

    device.setTextWhenNothingSelected ("Choose MIDI output...");
    device.onChange = [this]
    {
        const auto name = device.getText();
        status = link.openDevice (device.getSelectedId() > 1 ? name : juce::String())
                   ? (device.getSelectedId() > 1 ? "Connected to " + name : "No MIDI port: use the plugin's MIDI out")
                   : "Could not open " + name;
        repaint();
    };
    addAndMakeVisible (device);

    for (int ch = 1; ch <= 16; ++ch)
        paramChannel.addItem ("CH " + juce::String (ch), ch);
    paramChannel.setSelectedId (link.getParamChannel(), juce::dontSendNotification);
    paramChannel.onChange = [this] { link.setParamChannel (paramChannel.getSelectedId()); };
    addAndMakeVisible (paramChannel);

    for (int t = 0; t < 7; ++t)
    {
        auto* c = voiceChannels.add (new juce::ComboBox());
        for (int ch = 1; ch <= 16; ++ch)
            c->addItem (juce::String (ch), ch);
        c->setSelectedId (link.getVoiceChannel (t), juce::dontSendNotification);
        c->onChange = [this, t, c] { link.setVoiceChannel (t, c->getSelectedId()); };
        addAndMakeVisible (c);
    }

    sendKitButton.onClick = [sendKit] { if (sendKit) sendKit(); };
    addAndMakeVisible (sendKitButton);

    refreshDevices();
    timerCallback();
    startTimerHz (4);
}

void HardwareOverlay::refreshDevices()
{
    device.clear (juce::dontSendNotification);
    device.addItem ("None (plugin MIDI out only)", 1);
    const auto names = link.getDeviceNames();
    for (int i = 0; i < names.size(); ++i)
        device.addItem (names[i], i + 2);
    const auto current = link.getDeviceName();
    const int idx = names.indexOf (current);
    device.setSelectedId (idx >= 0 ? idx + 2 : 1, juce::dontSendNotification);
}

void HardwareOverlay::timerCallback()
{
    const int m = (int) link.getMode();
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setToggleState (i == m, juce::dontSendNotification);
    const bool on = link.isActive();
    device.setEnabled (on);
    paramChannel.setEnabled (on);
    sendKitButton.setEnabled (on);
    for (auto* c : voiceChannels) c->setEnabled (on);
}

void HardwareOverlay::layoutCard (juce::Rectangle<int> c)
{
    const int x = c.getX() + 32;
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setBounds (x + i * 124, c.getY() + 96, 116, 30);
    device.setBounds (x, c.getY() + 166, 370, 28);
    paramChannel.setBounds (x + 390, c.getY() + 166, 90, 28);
    for (int t = 0; t < 7; ++t)
        voiceChannels[t]->setBounds (x + t * 82, c.getY() + 240, 70, 26);
    sendKitButton.setBounds (x, c.getY() + 290, 170, 30);
}

void HardwareOverlay::paintCard (juce::Graphics& g, juce::Rectangle<int> c)
{
    const int x = c.getX() + 32;
    g.setColour (colours::textBright);
    g.setFont (LAF::uiFont (22.0f, true));
    g.drawText ("LXR-02 HARDWARE EDITOR", x, c.getY() + 26, 480, 30, juce::Justification::centredLeft);
    g.setColour (colours::label);
    g.setFont (LAF::uiFont (13.0f));
    g.drawText ("Every knob, the randomizer and the sequencer can drive an Erica Synths LXR-02 over MIDI.",
                x, c.getY() + 56, 580, 20, juce::Justification::centredLeft);

    g.setColour (colours::dim);
    g.setFont (LAF::uiFont (11.0f));
    g.drawText ("MODE", x, c.getY() + 80, 200, 14, juce::Justification::centredLeft);
    g.drawText ("MIDI OUTPUT", x, c.getY() + 150, 200, 14, juce::Justification::centredLeft);
    g.drawText ("PARAMETER CH", x + 390, c.getY() + 150, 120, 14, juce::Justification::centredLeft);
    for (int t = 0; t < 7; ++t)
        g.drawText (trackNames()[t], x + t * 82, c.getY() + 224, 80, 14, juce::Justification::centredLeft);

    g.setColour (colours::accent);
    g.setFont (LAF::uiFont (13.0f));
    g.drawText (status, x + 190, c.getY() + 290, 400, 30, juce::Justification::centredLeft);

    g.setColour (colours::label);
    g.setFont (LAF::uiFont (12.5f));
    g.drawFittedText ("On the LXR, set the global MIDI channel to the PARAMETER CH above and each voice's MIDI channel "
                      "to the numbers shown for its track. Parameters go out as CC (1-127) and NRPN (128+); "
                      "LFO and velocity destinations can only be set on the LXR itself.\n"
                      "LXR ONLY silences LX-6's own engine so you hear just the hardware.",
                      x, c.getY() + 334, 576, 76, juce::Justification::topLeft, 6);
}
}
