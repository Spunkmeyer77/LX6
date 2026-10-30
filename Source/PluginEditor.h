// LX-6 - editor, following the black "LX-6" mockup.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PluginProcessor.h"
#include "LX6LookAndFeel.h"
#include "AboutWindow.h"

namespace lx6
{
using APVTS = juce::AudioProcessorValueTreeState;

// rotary knob with a label underneath; shows the value while hovered or dragged
class Knob : public juce::Component
{
public:
    Knob (APVTS&, const ParamDef&, juce::Colour pointer, int diameter);
    void resized() override;
    void paint (juce::Graphics&) override;
    const ParamDef& def;
private:
    struct Rot : juce::Slider { using Slider::Slider; void mouseEnter (const juce::MouseEvent& e) override { Slider::mouseEnter (e); getParentComponent()->repaint(); }
                                void mouseExit (const juce::MouseEvent& e) override { Slider::mouseExit (e); getParentComponent()->repaint(); } };
    Rot slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
    int diameter;
};

// "LABEL      VALUE" row that opens a menu (LFO shape, destinations, FX modes...)
class ValueRow : public juce::Component, public juce::SettableTooltipClient, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater
{
public:
    ValueRow (APVTS&, const ParamDef&);
    ~ValueRow() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }
    APVTS& apvts;
    const ParamDef& def;
};

class IconButton : public juce::Button
{
public:
    enum Icon { Dice, Lock };
    IconButton (Icon i) : juce::Button ("icon"), icon (i) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    Icon icon;
};

class VoiceStrip : public juce::Component, private juce::Timer
{
public:
    VoiceStrip (LX6Processor&, int voice);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void (int)> onSelect;
    bool selected = false;
private:
    void timerCallback() override;
    LX6Processor& proc;
    int voice;
    juce::OwnedArray<Knob> knobs;
    juce::Slider fader;
    std::unique_ptr<APVTS::SliderAttachment> faderAttachment;
    juce::TextButton mute { "M" }, solo { "S" };
    std::unique_ptr<APVTS::ButtonAttachment> muteAtt, soloAtt;
    float flash = 0.0f;
    juce::Rectangle<int> ledArea, envArea;
};

class SectionBox : public juce::Component
{
public:
    SectionBox (LX6Processor&, int voice, Section);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    LX6Processor& proc;
    int voice;
    Section section;
    juce::OwnedArray<juce::Component> controls;
    IconButton dice { IconButton::Dice }, lock { IconButton::Lock };
};

class VoiceEditor : public juce::Component
{
public:
    explicit VoiceEditor (LX6Processor&);
    void setVoice (int v);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void rebuild();
    LX6Processor& proc;
    int voice = 0, page = 0;
    juce::TextButton pageButtons[2] { juce::TextButton ("SOUND"), juce::TextButton ("MOD") };
    IconButton dice { IconButton::Dice };
    juce::OwnedArray<SectionBox> boxes;
};

class FxLfoPanel : public juce::Component, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater
{
public:
    explicit FxLfoPanel (LX6Processor&);
    ~FxLfoPanel() override;
    void setVoice (int v);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    void rebuildLfo();
    LX6Processor& proc;
    int voice = 0;
    juce::TextButton typeButtons[5];
    juce::OwnedArray<Knob> fxKnobs;
    juce::OwnedArray<ValueRow> fxRows;
    juce::OwnedArray<juce::Component> lfoControls;
    juce::Rectangle<int> lfoDisplay;
};

class SequencerPanel : public juce::Component, private juce::Timer
{
public:
    explicit SequencerPanel (LX6Processor&);
    void setTrack (int t);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void editLane (juce::Point<int>);
    void showEuclid();
    int stepAt (juce::Point<int>) const;
    LX6Processor& proc;
    int track = 0;
    enum Lane { Velocity, Probability } lane = Velocity;
    juce::TextButton patternButtons[8];
    juce::TextButton trackButtons[7];
    juce::TextButton euclidButton { "EUCLID" }, velButton { "VEL" }, probButton { "PROB" },
                     rndButton { "RND" }, clrButton { "CLR" }, copyButton { "COPY" }, playButton { "PLAY" };
    juce::Rectangle<int> stepsArea;
    juce::Rectangle<int> laneRect (int step) const;
    juce::Rectangle<int> stepRect (int step) const;
    int lastShownStep = -2;
};

class TopBar : public juce::Component, private juce::Timer
{
public:
    explicit TopBar (LX6Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    std::function<void()> onAbout, onHardware, onKitChanged;
private:
    void timerCallback() override;
    void showKitMenu();
    void saveKitAs();
    LX6Processor& proc;
    juce::HyperlinkButton store { "ticcoross.gumroad.com", juce::URL ("https://ticcoross.gumroad.com/") };
    juce::TextButton saveButton { "SAVE" }, randomButton { "RANDOM" }, undoButton { "UNDO" },
                     hwButton { "LXR" }, aboutButton { "i" };
    juce::Slider amount, master;
    std::unique_ptr<APVTS::SliderAttachment> masterAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> kitBox, tempoBox, meterBox;
    float tempoDragStart = 0.0f;
    juce::String shownKit;
    double shownTempo = 0.0;
};
}

class LX6Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LX6Editor (LX6Processor&);
    ~LX6Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectVoice (int v);
    void showOverlay (std::unique_ptr<lx6::Overlay>);

    LX6Processor& proc;
    lx6::LX6LookAndFeel laf;
    juce::Component content;
    std::unique_ptr<lx6::TopBar> top;
    juce::OwnedArray<lx6::VoiceStrip> strips;
    std::unique_ptr<lx6::VoiceEditor> voiceEditor;
    std::unique_ptr<lx6::FxLfoPanel> fxLfo;
    std::unique_ptr<lx6::SequencerPanel> seq;
    std::unique_ptr<lx6::Overlay> overlay;
    juce::TooltipWindow tooltips { this, 600 };
    bool engineBusyShown = false;
};
