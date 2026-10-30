// LX-6 - editor, following the black "LX-6" mockup.
// SPDX-License-Identifier: GPL-3.0-only
#include "PluginEditor.h"

namespace lx6
{
using LAF = LX6LookAndFeel;
static constexpr int designW = 1280, designH = 800;

static void drawSmallCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r,
                           juce::Colour c, float h = 10.0f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setColour (c);
    g.setFont (LAF::uiFont (h).withExtraKerningFactor (0.12f));
    g.drawText (t, r, j, false);
}

static int paramValue (APVTS& s, int lxrNr)
{
    return juce::roundToInt (s.getRawParameterValue (paramId (lxrNr))->load());
}

static void setParamValue (APVTS& s, int lxrNr, int v)
{
    if (auto* p = s.getParameter (paramId (lxrNr)))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
}

//==============================================================================
Knob::Knob (APVTS& s, const ParamDef& d, juce::Colour pointer, int dia) : def (d), diameter (dia)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::thumbColourId, pointer);
    slider.setMouseDragSensitivity (d.maxValue < 16 ? 60 : 180);
    slider.setDoubleClickReturnValue (true, (double) juce::jlimit (0, d.maxValue, (int) lxr::initKitDefaults[d.lxrNr]));
    slider.setTooltip (d.name);
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (s, paramId (d.lxrNr), slider);
    slider.onValueChange = [this] { repaint(); };
}

void Knob::resized()
{
    slider.setBounds (getLocalBounds().removeFromTop (diameter + 2).withSizeKeepingCentre (diameter + 2, diameter + 2));
}

void Knob::paint (juce::Graphics& g)
{
    const bool showValue = slider.isMouseOverOrDragging();
    const auto text = showValue ? valueToText (def, juce::roundToInt (slider.getValue())) : def.label;
    g.setColour (showValue ? colours::textBright : colours::label);
    g.setFont (LAF::uiFont (9.5f).withExtraKerningFactor (0.08f));
    g.drawFittedText (text, 0, diameter + 4, getWidth(), 12, juce::Justification::centredTop, 1, 0.8f);
}

//==============================================================================
ValueRow::ValueRow (APVTS& s, const ParamDef& d) : apvts (s), def (d)
{
    apvts.addParameterListener (paramId (d.lxrNr), this);
    setTooltip (d.name);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

ValueRow::~ValueRow() { apvts.removeParameterListener (paramId (def.lxrNr), this); }

void ValueRow::paint (juce::Graphics& g)
{
    if (isMouseOver())
    {
        g.setColour (colours::control);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);
    }
    g.setFont (LAF::monoFont (11.0f));
    g.setColour (colours::label);
    g.drawText (def.label, 4, 0, getWidth() / 2, getHeight(), juce::Justification::centredLeft);
    g.setColour (colours::text);
    g.drawText (valueToText (def, paramValue (apvts, def.lxrNr)), getWidth() / 3, 0, getWidth() * 2 / 3 - 4, getHeight(),
                juce::Justification::centredRight);
}

void ValueRow::mouseDown (const juce::MouseEvent&)
{
    const int current = paramValue (apvts, def.lxrNr);
    juce::PopupMenu m;
    if (def.kind == lxr::Kind::modtarget)
    {
        m.addItem (1, "OFF", true, current == 0);
        juce::PopupMenu perVoice[6], other;
        for (int i = 1; i < lxr::numModTargets; ++i)
        {
            const auto* pd = findByLxr (lxr::modTargetParams[i]);
            auto& menu = (pd != nullptr && pd->voice >= 0) ? perVoice[pd->voice] : other;
            menu.addItem (i + 1, pd != nullptr ? sectionName (pd->section).substring (0, 4) + "  " + pd->label
                                               : modTargetName (i), true, current == i);
        }
        for (int v = 0; v < 6; ++v)
            m.addSubMenu (voiceNames()[v], perVoice[v]);
        if (other.getNumItems() > 0)
            m.addSubMenu ("GLOBAL", other);
    }
    else
    {
        for (int v = 0; v <= def.maxValue; ++v)
            m.addItem (v + 1, valueToText (def, v), true, v == current);
    }
    m.setLookAndFeel (&getLookAndFeel());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [this] (int r) { if (r > 0) setParamValue (apvts, def.lxrNr, r - 1); });
}

//==============================================================================
void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = getToggleState() ? colours::accent : (over ? colours::textBright : colours::dim);
    g.setColour (c.withAlpha (down ? 0.6f : 1.0f));
    if (icon == Dice)
    {
        auto sq = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), r.getHeight()), juce::jmin (r.getWidth(), r.getHeight()));
        g.drawRoundedRectangle (sq, 2.5f, 1.3f);
        const float d = sq.getWidth() * 0.16f;
        for (auto p : { juce::Point<float> (0.3f, 0.3f), { 0.7f, 0.7f }, { 0.5f, 0.5f }, { 0.7f, 0.3f }, { 0.3f, 0.7f } })
            g.fillEllipse (sq.getX() + p.x * sq.getWidth() - d * 0.5f, sq.getY() + p.y * sq.getHeight() - d * 0.5f, d, d);
    }
    else
    {
        const float w = juce::jmin (r.getWidth(), r.getHeight());
        auto body = juce::Rectangle<float> (w * 0.7f, w * 0.5f).withCentre ({ r.getCentreX(), r.getCentreY() + w * 0.18f });
        g.fillRoundedRectangle (body, 1.5f);
        juce::Path shackle;
        const float sw = body.getWidth() * 0.55f;
        shackle.addCentredArc (body.getCentreX(), body.getY(), sw * 0.5f, sw * 0.55f, 0.0f,
                               -juce::MathConstants<float>::halfPi, getToggleState() ? juce::MathConstants<float>::halfPi : 0.4f, true);
        g.strokePath (shackle, juce::PathStrokeType (1.4f));
    }
}

//==============================================================================
VoiceStrip::VoiceStrip (LX6Processor& p, int v) : proc (p), voice (v)
{
    const auto col = colours::voice (v);
    for (int nr : { coarseParam (v), decayParam (v), panParam (v) })
    {
        auto* def = findByLxr (nr);
        auto* k = knobs.add (new Knob (proc.apvts, *def, col, 38));
        addAndMakeVisible (k);
    }
    // nicer strip labels than the editor ones
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    fader.setTooltip (voiceNames()[v] + " volume");
    addAndMakeVisible (fader);
    faderAttachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramId (volumeParam (v)), fader);

    for (auto* b : { &mute, &solo })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    mute.setColour (juce::TextButton::buttonOnColourId, colours::dim);
    solo.setColour (juce::TextButton::buttonOnColourId, col);
    muteAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ids::mute + juce::String (v), mute);
    soloAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ids::solo + juce::String (v), solo);
    startTimerHz (30);
}

void VoiceStrip::timerCallback()
{
    bool hit = proc.wasTriggered (voice);
    if (voice == 5) hit |= proc.wasTriggered (6);
    if (hit) flash = 1.0f;
    else flash *= 0.8f;
    repaint (ledArea.expanded (6));
    repaint (envArea);
}

void VoiceStrip::resized()
{
    auto r = getLocalBounds().reduced (6, 10);
    ledArea = r.removeFromTop (16);
    r.removeFromTop (10);
    envArea = r.removeFromTop (34).withSizeKeepingCentre (76, 34);
    r.removeFromTop (10);
    for (auto* k : knobs)
    {
        k->setBounds (r.removeFromTop (54).withSizeKeepingCentre (60, 54));
        r.removeFromTop (6);
    }
    auto bottom = r.removeFromBottom (24);
    mute.setBounds (bottom.withWidth (34).withX (bottom.getCentreX() - 36));
    solo.setBounds (bottom.withWidth (34).withX (bottom.getCentreX() + 2));
    r.removeFromBottom (10);
    fader.setBounds (r.withSizeKeepingCentre (30, r.getHeight()));
}

void VoiceStrip::paint (juce::Graphics& g)
{
    const auto col = colours::voice (voice);
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (selected ? juce::Colour (0xff22272c) : juce::Colour (0xff1b1f23));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (selected ? col : colours::border);
    g.drawRoundedRectangle (b, 6.0f, 1.0f);

    // LED + name (click the LED to audition)
    g.setFont (LAF::uiFont (12.0f, true).withExtraKerningFactor (0.08f));
    const auto name = voiceNames()[voice];
    const int textW = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), name);
    const int startX = (getWidth() - (textW + 14)) / 2;
    const auto led = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ (float) startX + 4.0f, (float) ledArea.getCentreY() });
    g.setColour (col.withAlpha (0.25f + 0.5f * flash));
    g.fillEllipse (led.expanded (2.0f + 4.0f * flash));
    g.setColour (col.interpolatedWith (juce::Colours::white, 0.4f * flash));
    g.fillEllipse (led);
    g.setColour (colours::text);
    g.drawText (name, startX + 14, ledArea.getY(), textW + 2, ledArea.getHeight(), juce::Justification::centredLeft);

    // envelope preview from attack / decay / slope
    g.setColour (colours::well);
    g.fillRoundedRectangle (envArea.toFloat(), 3.0f);
    const float att = paramValue (proc.apvts, 49 + 2 * voice) / 127.0f;
    const float dec = paramValue (proc.apvts, 50 + 2 * voice) / 127.0f;
    const float slope = paramValue (proc.apvts, 62 + voice) / 127.0f;
    auto e = envArea.toFloat().reduced (2.0f, 3.0f);
    juce::Path p;
    p.startNewSubPath (e.getX(), e.getBottom());
    const float ax = e.getX() + juce::jmax (1.0f, att * e.getWidth() * 0.35f);
    p.lineTo (ax, e.getY());
    const float dw = juce::jmax (4.0f, dec * (e.getRight() - ax));
    const float curve = 0.15f + slope * 0.8f;
    for (int i = 1; i <= 24; ++i)
    {
        const float t = i / 24.0f;
        p.lineTo (ax + t * dw, e.getY() + (1.0f - std::pow (1.0f - t, 1.0f + 4.0f * (1.0f - curve))) * e.getHeight());
    }
    p.lineTo (e.getRight(), e.getBottom());
    g.setColour (col.withAlpha (0.12f + 0.25f * flash));
    juce::Path fill (p);
    fill.closeSubPath();
    g.fillPath (fill);
    g.setColour (col);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

void VoiceStrip::mouseDown (const juce::MouseEvent& e)
{
    if (ledArea.expanded (0, 4).contains (e.getPosition()))
        proc.audition (voice);
    if (onSelect) onSelect (voice);
}

//==============================================================================
SectionBox::SectionBox (LX6Processor& p, int v, Section s) : proc (p), voice (v), section (s)
{
    for (auto* d : paramsFor (v, s))
    {
        juce::Component* c;
        if (d->kind == lxr::Kind::modtarget)
            c = controls.add (new ValueRow (proc.apvts, *d));
        else
            c = controls.add (new Knob (proc.apvts, *d, colours::text.brighter (0.3f), 34));
        addAndMakeVisible (c);
    }
    dice.setTooltip ("Randomize this section");
    dice.onClick = [this] { proc.randomizer.randomiseSection (voice, section); };
    lock.setClickingTogglesState (true);
    lock.setToggleState (proc.randomizer.locked.count (section) > 0, juce::dontSendNotification);
    lock.setTooltip ("Lock: keep this section when randomizing the kit or voice");
    lock.onClick = [this]
    {
        if (lock.getToggleState()) proc.randomizer.locked.insert (section);
        else proc.randomizer.locked.erase (section);
    };
    addAndMakeVisible (dice);
    addAndMakeVisible (lock);
}

void SectionBox::resized()
{
    lock.setBounds (getWidth() - 26, 6, 18, 18);
    dice.setBounds (getWidth() - 48, 6, 18, 18);
    auto r = getLocalBounds().reduced (12, 8);
    r.removeFromTop (22);
    const int cellW = 52, cellH = 52;
    const int perRow = juce::jmax (1, r.getWidth() / cellW);
    int i = 0;
    for (auto* c : controls)
    {
        if (dynamic_cast<ValueRow*> (c) != nullptr)
            continue;
        const int col = i % perRow, row = i / perRow;
        c->setBounds (r.getX() + col * cellW, r.getY() + row * cellH, cellW, cellH);
        ++i;
    }
    int rowY = r.getY() + ((i + perRow - 1) / perRow) * cellH + 2;
    for (auto* c : controls)
        if (dynamic_cast<ValueRow*> (c) != nullptr)
        {
            c->setBounds (r.getX(), rowY, juce::jmin (r.getWidth(), 220), 20);
            rowY += 22;
        }
}

void SectionBox::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (colours::border);
    g.drawRoundedRectangle (b, 6.0f, 1.0f);
    drawSmallCaps (g, sectionName (section), { 12, 8, 200, 16 }, colours::voice (voice));
}

//==============================================================================
VoiceEditor::VoiceEditor (LX6Processor& p) : proc (p)
{
    for (int i = 0; i < 2; ++i)
    {
        pageButtons[i].setClickingTogglesState (false);
        pageButtons[i].onClick = [this, i] { page = i; rebuild(); };
        addAndMakeVisible (pageButtons[i]);
    }
    dice.setTooltip ("Randomize this voice (locked sections are kept)");
    dice.onClick = [this] { proc.randomizer.randomiseVoice (voice); };
    addAndMakeVisible (dice);
    rebuild();
}

void VoiceEditor::setVoice (int v)
{
    if (v != voice) { voice = v; rebuild(); repaint(); }
}

void VoiceEditor::rebuild()
{
    boxes.clear();
    const Section sound[] { Section::Osc, Section::Pitch, Section::Filter, Section::Amp };
    const Section mod[] { Section::Transient, Section::Voice, Section::Velocity, Section::Amp };
    for (int i = 0; i < 4; ++i)
    {
        Section s = page == 0 ? sound[i] : mod[i];
        if (page == 0 && s == Section::Pitch && voice >= 4)
            s = Section::Transient; // cymbal / hi-hat have no pitch envelope
        if (page == 1 && i == 3)
            continue;
        if (page == 1 && s == Section::Transient && voice >= 4)
            continue;
        auto* b = boxes.add (new SectionBox (proc, voice, s));
        addAndMakeVisible (b);
    }
    for (int i = 0; i < 2; ++i)
    {
        pageButtons[i].setToggleState (i == page, juce::dontSendNotification);
        pageButtons[i].setColour (juce::TextButton::buttonOnColourId, colours::voice (voice));
    }
    resized();
}

void VoiceEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 14);
    auto header = r.removeFromTop (24);
    dice.setBounds (header.removeFromRight (22).withSizeKeepingCentre (20, 20));
    header.removeFromRight (10);
    pageButtons[1].setBounds (header.removeFromRight (60));
    header.removeFromRight (4);
    pageButtons[0].setBounds (header.removeFromRight (60));
    r.removeFromTop (12);

    const int gap = 10;
    const int w = (r.getWidth() - gap) / 2, h = (r.getHeight() - gap) / 2;
    for (int i = 0; i < boxes.size(); ++i)
    {
        const bool lastAlone = i == boxes.size() - 1 && i % 2 == 0;
        boxes[i]->setBounds (r.getX() + (i % 2) * (w + gap), r.getY() + (i / 2) * (h + gap), lastAlone ? r.getWidth() : w, h);
    }
}

void VoiceEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (16, 14).removeFromTop (24);
    const auto col = colours::voice (voice);
    g.setColour (col);
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ r.getX() + 5.0f, (float) r.getCentreY() }));
    g.setColour (colours::text);
    g.setFont (LAF::uiFont (16.0f, true).withExtraKerningFactor (0.06f));
    g.drawText (voiceNames()[voice] + juce::String::fromUTF8 (" \xc2\xb7 EDIT"), r.withTrimmedLeft (20), juce::Justification::centredLeft);
    g.setColour (colours::dim);
    g.setFont (LAF::uiFont (11.0f));
    g.drawText ("click a strip to edit its voice", r.withTrimmedRight (170), juce::Justification::centredRight);
}

//==============================================================================
FxLfoPanel::FxLfoPanel (LX6Processor& p) : proc (p)
{
    const char* names[] { "OFF", "DRV", "RNG", "CMP", "DEL" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = typeButtons[i];
        b.setButtonText (names[i]);
        b.onClick = [this, i] { setParamValue (proc.apvts, 234, i); };
        addAndMakeVisible (b);
    }
    for (int nr : { 236, 238, 239, 240, 241, 114 })
        addAndMakeVisible (fxKnobs.add (new Knob (proc.apvts, *findByLxr (nr), colours::text.brighter (0.3f), 34)));
    for (int nr : { 237, 242, 243, 245, 235 })
        addAndMakeVisible (fxRows.add (new ValueRow (proc.apvts, *findByLxr (nr))));
    proc.apvts.addParameterListener (paramId (234), this);
    for (int v = 0; v < numVoices; ++v)
        for (int which : { 1, 2 })
            proc.apvts.addParameterListener (paramId (lfo (v, which)), this);
    handleAsyncUpdate();
    rebuildLfo();
}

FxLfoPanel::~FxLfoPanel()
{
    proc.apvts.removeParameterListener (paramId (234), this);
    for (int v = 0; v < numVoices; ++v)
        for (int which : { 1, 2 })
            proc.apvts.removeParameterListener (paramId (lfo (v, which)), this);
}

void FxLfoPanel::setVoice (int v)
{
    if (v != voice) { voice = v; rebuildLfo(); }
}

void FxLfoPanel::handleAsyncUpdate()
{
    const int type = paramValue (proc.apvts, 234);
    for (int i = 0; i < 5; ++i)
        typeButtons[i].setToggleState (i == type, juce::dontSendNotification);
    // only the menus that matter for this effect
    fxRows[0]->setVisible (type == 1); // dist type
    fxRows[1]->setVisible (type == 3); // ratio
    fxRows[2]->setVisible (type == 2); // ring wave
    fxRows[3]->setVisible (type == 4); // delay type
    resized();
    repaint();
}

void FxLfoPanel::rebuildLfo()
{
    lfoControls.clear();
    const auto col = colours::lfo;
    for (int which : { 0, 1, 7 })
        addAndMakeVisible (lfoControls.add (new Knob (proc.apvts, *findByLxr (lfo (voice, which)), col, 34)));
    for (int which : { 2, 6, 3, 5 })
        addAndMakeVisible (lfoControls.add (new ValueRow (proc.apvts, *findByLxr (lfo (voice, which)))));
    resized();
    repaint();
}

void FxLfoPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 14);
    r.removeFromTop (18);
    auto typeRow = r.removeFromTop (24);
    const int bw = (typeRow.getWidth() - 4 * 3) / 5;
    for (int i = 0; i < 5; ++i)
        typeButtons[i].setBounds (typeRow.getX() + i * (bw + 3), typeRow.getY(), bw, 24);
    r.removeFromTop (10);
    for (int i = 0; i < fxKnobs.size(); ++i)
        fxKnobs[i]->setBounds (r.getX() + (i % 3) * 58, r.getY() + (i / 3) * 52, 56, 52);
    r.removeFromTop (106);
    for (auto* row : fxRows)
        if (row->isVisible())
            row->setBounds (r.removeFromTop (20)), r.removeFromTop (2);

    r.removeFromTop (12);
    r.removeFromTop (18); // LFO title
    lfoDisplay = r.removeFromTop (46);
    r.removeFromTop (8);
    int k = 0;
    for (auto* c : lfoControls)
    {
        if (dynamic_cast<Knob*> (c) != nullptr)
            c->setBounds (r.getX() + (k++) * 58, r.getY(), 56, 52);
    }
    r.removeFromTop (56);
    for (auto* c : lfoControls)
        if (dynamic_cast<ValueRow*> (c) != nullptr)
            c->setBounds (r.removeFromTop (20)), r.removeFromTop (2);
}

void FxLfoPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::panelDark);
    g.setColour (colours::border);
    g.fillRect (0, 0, 1, getHeight());

    auto r = getLocalBounds().reduced (14, 14);
    drawSmallCaps (g, "FX BUS", r.removeFromTop (14), colours::dim);

    // LFO title + shape preview
    const int y = lfoDisplay.getY() - 22;
    g.setColour (colours::border);
    g.fillRect (14, y - 8, getWidth() - 28, 1);
    drawSmallCaps (g, juce::String::fromUTF8 ("LFO \xc2\xb7 ") + voiceNames()[voice], { 14, y, getWidth() - 28, 14 }, colours::dim);

    g.setColour (colours::well);
    g.fillRoundedRectangle (lfoDisplay.toFloat(), 4.0f);
    const int shape = paramValue (proc.apvts, lfo (voice, 2));
    const float depth = 0.25f + 0.75f * paramValue (proc.apvts, lfo (voice, 1)) / 127.0f;
    auto d = lfoDisplay.toFloat().reduced (4.0f, 6.0f);
    juce::Path p;
    juce::Random rnd (7);
    float held = 0.0f;
    for (int i = 0; i <= 120; ++i)
    {
        const float t = i / 120.0f, ph = std::fmod (t * 2.0f, 1.0f);
        float v = 0.0f;
        switch (shape)
        {
            case 0: v = std::sin (ph * juce::MathConstants<float>::twoPi); break;
            case 1: v = 1.0f - 4.0f * std::abs (ph - 0.5f); break;
            case 2: v = ph * 2.0f - 1.0f; break;
            case 3: v = 1.0f - ph * 2.0f; break;
            case 4: v = ph < 0.5f ? 1.0f : -1.0f; break;
            case 5: if (i % 10 == 0) held = rnd.nextFloat() * 2.0f - 1.0f; v = held; break;
            case 6: v = std::pow (ph, 3.0f) * 2.0f - 1.0f; break;
            default: v = std::pow (1.0f - ph, 3.0f) * 2.0f - 1.0f; break;
        }
        const float x = d.getX() + t * d.getWidth(), yy = d.getCentreY() - v * depth * d.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    g.setColour (colours::lfo);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

//==============================================================================
SequencerPanel::SequencerPanel (LX6Processor& p) : proc (p)
{
    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.setTooltip ("Click: play and edit pattern. Shift-click: edit only. Alt-click: copy the edited pattern here");
        b.onClick = [this, i]
        {
            const auto mods = juce::ModifierKeys::getCurrentModifiers();
            if (mods.isAltDown())
                proc.sequencer.copyPattern (proc.sequencer.getEditPattern(), i);
            else
            {
                proc.sequencer.setEditPattern (i);
                if (! mods.isShiftDown())
                    proc.sequencer.queuePattern (i);
            }
            repaint();
        };
        addAndMakeVisible (b);
    }
    const char* shortNames[] { "D1", "D2", "D3", "SN", "CY", "HH", "OH" };
    for (int t = 0; t < 7; ++t)
    {
        auto& b = trackButtons[t];
        b.setButtonText (shortNames[t]);
        b.setColour (juce::TextButton::buttonOnColourId, colours::voice (t));
        b.setTooltip (trackNames()[t]);
        b.onClick = [this, t] { setTrack (t); };
        addAndMakeVisible (b);
    }
    euclidButton.onClick = [this] { showEuclid(); };
    velButton.onClick = [this] { lane = Velocity; repaint(); };
    probButton.onClick = [this] { lane = Probability; repaint(); };
    rndButton.setTooltip ("Random steps for this track (shift: all tracks)");
    rndButton.onClick = [this]
    {
        juce::Random r;
        const bool all = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
        proc.sequencer.randomise (proc.sequencer.getEditPattern(), all ? -1 : track, 0.3f, r);
        repaint();
    };
    clrButton.onClick = [this] { proc.sequencer.clearTrack (proc.sequencer.getEditPattern(), track); repaint(); };
    copyButton.setTooltip ("Copy this pattern to the next slot");
    copyButton.onClick = [this]
    {
        const int from = proc.sequencer.getEditPattern();
        proc.sequencer.copyPattern (from, (from + 1) % 8);
        proc.sequencer.setEditPattern ((from + 1) % 8);
        repaint();
    };
    playButton.setTooltip ("Play with LX-6's own clock when the host is stopped (TEMPO sets the speed)");
    playButton.setClickingTogglesState (true);
    playButton.onClick = [this] { proc.internalPlay = playButton.getToggleState(); };
    for (auto* b : { &euclidButton, &velButton, &probButton, &rndButton, &clrButton, &copyButton, &playButton })
        addAndMakeVisible (b);
    setTrack (0);
    startTimerHz (30);
}

void SequencerPanel::setTrack (int t)
{
    track = juce::jlimit (0, 6, t);
    for (int i = 0; i < 7; ++i)
        trackButtons[i].setToggleState (i == track, juce::dontSendNotification);
    repaint();
}

void SequencerPanel::timerCallback()
{
    const int s = proc.sequencer.isRunning() ? proc.sequencer.getCurrentStep() : -1;
    const int edit = proc.sequencer.getEditPattern(), play = proc.sequencer.getPlayingPattern(), q = proc.sequencer.getQueuedPattern();
    const bool blink = (juce::Time::getMillisecondCounter() / 250) % 2 == 0;
    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        const bool on = i == play || (i == q && blink);
        b.setToggleState (on, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonColourId, i == edit && i != play ? colours::knobEdge : colours::control);
    }
    velButton.setToggleState (lane == Velocity, juce::dontSendNotification);
    probButton.setToggleState (lane == Probability, juce::dontSendNotification);
    playButton.setToggleState (proc.internalPlay.load(), juce::dontSendNotification);
    if (s != lastShownStep) { lastShownStep = s; repaint (stepsArea.expanded (2)); }
}

void SequencerPanel::resized()
{
    auto r = getLocalBounds().reduced (16, 10);
    auto top = r.removeFromTop (24);
    top.removeFromLeft (64);
    for (auto& b : patternButtons)
    {
        b.setBounds (top.removeFromLeft (30));
        top.removeFromLeft (5);
    }
    top.removeFromLeft (14);
    for (auto* b : { &playButton, &copyButton, &clrButton, &rndButton, &probButton, &velButton, &euclidButton })
    {
        const int w = b == &euclidButton ? 70 : (b == &playButton ? 52 : 44);
        b->setBounds (top.removeFromRight (w));
        top.removeFromRight (5);
    }
    top.removeFromRight (8);
    for (int t = 6; t >= 0; --t)
    {
        trackButtons[t].setBounds (top.removeFromRight (30));
        top.removeFromRight (3);
    }
    r.removeFromTop (8);
    stepsArea = r;
}

juce::Rectangle<int> SequencerPanel::laneRect (int s) const
{
    const int gap = 5, w = (stepsArea.getWidth() - 15 * gap) / 16;
    return { stepsArea.getX() + s * (w + gap), stepsArea.getY(), w, 34 };
}

juce::Rectangle<int> SequencerPanel::stepRect (int s) const
{
    return laneRect (s).translated (0, 38).withHeight (40);
}

int SequencerPanel::stepAt (juce::Point<int> p) const
{
    for (int s = 0; s < 16; ++s)
    {
        const auto r = laneRect (s);
        if (p.x >= r.getX() - 2 && p.x < r.getRight() + 3)
            return s;
    }
    return -1;
}

void SequencerPanel::editLane (juce::Point<int> p)
{
    const int s = stepAt (p);
    if (s < 0) return;
    const auto lr = laneRect (s);
    const float frac = juce::jlimit (0.0f, 1.0f, (lr.getBottom() - p.y) / (float) lr.getHeight());
    const int pat = proc.sequencer.getEditPattern();
    auto st = proc.sequencer.getStep (pat, track, s);
    if (lane == Velocity) st.velocity = juce::jlimit (1, 127, juce::roundToInt (frac * 127.0f));
    else st.probability = juce::jlimit (0, 100, juce::roundToInt (frac * 100.0f));
    proc.sequencer.setStep (pat, track, s, st);
    repaint (stepsArea);
}

void SequencerPanel::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.getPosition());
    if (s < 0) return;
    if (stepRect (s).contains (e.getPosition()))
    {
        proc.sequencer.toggleStep (proc.sequencer.getEditPattern(), track, s);
        repaint (stepsArea);
    }
    else if (laneRect (s).contains (e.getPosition()))
        editLane (e.getPosition());
}

void SequencerPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (e.getMouseDownY() < stepsArea.getY() + 36)
        editLane (e.getPosition());
}

void SequencerPanel::showEuclid()
{
    struct EuclidBox : juce::Component
    {
        juce::Label l1 { {}, "PULSES" }, l2 { {}, "ROTATE" };
        juce::Slider pulses { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                     rot { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
        juce::TextButton go { "APPLY" };
        EuclidBox()
        {
            pulses.setRange (0, 16, 1); pulses.setValue (5);
            rot.setRange (0, 15, 1);    rot.setValue (0);
            for (auto* c : std::initializer_list<juce::Component*> { &l1, &l2, &pulses, &rot, &go })
                addAndMakeVisible (c);
            l1.setBounds (8, 10, 60, 24);  pulses.setBounds (66, 10, 150, 24);
            l2.setBounds (8, 42, 60, 24);  rot.setBounds (66, 42, 150, 24);
            go.setBounds (8, 80, 208, 26);
            setSize (224, 114);
        }
    };
    auto box = std::make_unique<EuclidBox>();
    box->setLookAndFeel (&getLookAndFeel());
    auto* raw = box.get();
    raw->go.onClick = [this, raw]
    {
        proc.sequencer.euclid (proc.sequencer.getEditPattern(), track, (int) raw->pulses.getValue(), (int) raw->rot.getValue());
        repaint();
    };
    auto& cob = juce::CallOutBox::launchAsynchronously (std::move (box), euclidButton.getScreenBounds(), nullptr);
    cob.setLookAndFeel (&getLookAndFeel());
}

void SequencerPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::seqBg);
    g.setColour (colours::border);
    g.fillRect (0, 0, getWidth(), 1);

    auto r = getLocalBounds().reduced (16, 10);
    drawSmallCaps (g, "PATTERN", r.removeFromTop (24).withWidth (64), colours::dim);

    const int pat = proc.sequencer.getEditPattern();
    const int playing = proc.sequencer.isRunning() && proc.sequencer.getPlayingPattern() == pat ? proc.sequencer.getCurrentStep() : -1;
    const auto col = colours::voice (track);

    for (int s = 0; s < 16; ++s)
    {
        const auto st = proc.sequencer.getStep (pat, track, s);
        auto lr = laneRect (s).toFloat();
        g.setColour (colours::well);
        g.fillRoundedRectangle (lr, 2.0f);
        const float frac = lane == Velocity ? st.velocity / 127.0f : st.probability / 100.0f;
        g.setColour (st.on ? col : colours::knobEdge.withAlpha (0.35f));
        g.fillRoundedRectangle (lr.withTop (lr.getBottom() - lr.getHeight() * frac), 2.0f);

        auto sr = stepRect (s).toFloat();
        const bool group = (s / 4) % 2 == 1;
        g.setColour (s == playing ? colours::knobEdge : (st.on ? col : (group ? juce::Colour (0xff23282d) : juce::Colour (0xff2a3036))));
        g.fillRoundedRectangle (sr, 4.0f);
        g.setColour (s == playing ? colours::textBright : colours::ctrlEdge);
        g.drawRoundedRectangle (sr.reduced (0.5f), 4.0f, 1.0f);
        if (st.on && st.probability < 100)
        {
            g.setColour (colours::bg.withAlpha (0.7f));
            g.setFont (LAF::monoFont (10.0f));
            g.drawText (juce::String (st.probability) + "%", sr, juce::Justification::centred);
        }

        g.setColour (s == playing ? colours::textBright : colours::dim);
        g.setFont (LAF::monoFont (10.0f));
        g.drawText (juce::String (s + 1), stepRect (s).translated (0, 42).withHeight (14), juce::Justification::centred);
    }
}

//==============================================================================
TopBar::TopBar (LX6Processor& p) : proc (p)
{
    store.setFont (LAF::uiFont (11.0f), false, juce::Justification::centredLeft);
    store.setColour (juce::HyperlinkButton::textColourId, colours::label);
    addAndMakeVisible (store);

    saveButton.onClick = [this] { saveKitAs(); };
    randomButton.setTooltip ("Randomize the whole kit (locked sections are kept). Amount knob: how far");
    randomButton.onClick = [this]
    {
        proc.randomizer.amount = (float) amount.getValue();
        proc.randomizer.randomiseKit();
        proc.kitName = "RANDOM KIT";
        if (onKitChanged) onKitChanged();
    };
    undoButton.setTooltip ("Undo the last randomize");
    undoButton.onClick = [this] { proc.randomizer.undo(); };
    hwButton.setTooltip ("LXR-02 hardware editor settings");
    hwButton.onClick = [this] { if (onHardware) onHardware(); };
    aboutButton.onClick = [this] { if (onAbout) onAbout(); };
    for (auto* b : { &saveButton, &randomButton, &undoButton, &hwButton, &aboutButton })
        addAndMakeVisible (b);

    amount.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amount.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amount.setRange (0.0, 1.0);
    amount.setValue (0.5);
    amount.setColour (juce::Slider::thumbColourId, colours::accent);
    amount.setTooltip ("Random amount: small = variations, full = brand new sounds");
    amount.onValueChange = [this] { proc.randomizer.amount = (float) amount.getValue(); };
    addAndMakeVisible (amount);

    master.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    master.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    master.setColour (juce::Slider::thumbColourId, colours::textBright);
    master.setTooltip ("Master volume");
    addAndMakeVisible (master);
    masterAttachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, ids::master, master);
    master.setDoubleClickReturnValue (true, 0.0);

    startTimerHz (24);
}

void TopBar::resized()
{
    store.setBounds (20, 40, 150, 14);
    kitBox = { 196, 26, 220, 30 };
    saveButton.setBounds (428, 26, 56, 30);
    randomButton.setBounds (490, 26, 70, 30);
    amount.setBounds (566, 26, 30, 30);
    undoButton.setBounds (602, 26, 52, 30);

    aboutButton.setBounds (getWidth() - 52, 16, 32, 32);
    hwButton.setBounds (getWidth() - 106, 16, 46, 32);
    meterBox = { getWidth() - 132, 14, 15, 36 };
    master.setBounds (getWidth() - 182, 24, 36, 36);
    tempoBox = { getWidth() - 330, 26, 130, 30 };
}

void TopBar::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);
    g.setColour (colours::border);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);

    g.setColour (colours::textBright);
    g.setFont (LAF::uiFont (26.0f, true).withExtraKerningFactor (0.08f));
    g.drawText ("LX-6", 20, 8, 140, 32, juce::Justification::centredLeft);
    g.setColour (colours::border.brighter (0.1f));
    g.fillRect (176, 14, 1, 36);

    drawSmallCaps (g, "KIT", kitBox.withY (10).withHeight (14), colours::dim);
    g.setColour (colours::well);
    g.fillRoundedRectangle (kitBox.toFloat(), 4.0f);
    g.setColour (colours::ctrlEdge);
    g.drawRoundedRectangle (kitBox.toFloat().reduced (0.5f), 4.0f, 1.0f);
    g.setColour (colours::accent);
    g.setFont (LAF::monoFont (13.0f));
    g.drawText (proc.kitName, kitBox.reduced (10, 0).withTrimmedRight (14), juce::Justification::centredLeft);
    g.setColour (colours::dim);
    g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbe"), kitBox.removeFromRight (22), juce::Justification::centred);

    drawSmallCaps (g, "AMT", amount.getBounds().withY (10).withHeight (14), colours::dim, 9.0f, juce::Justification::centred);

    const bool synced = proc.isHostSynced();
    drawSmallCaps (g, synced ? juce::String::fromUTF8 ("TEMPO \xc2\xb7 HOST SYNC") : juce::String::fromUTF8 ("TEMPO \xc2\xb7 INTERNAL"),
                   tempoBox.withY (10).withHeight (14).expanded (20, 0), colours::dim, 10.0f, juce::Justification::centred);
    g.setColour (colours::well);
    g.fillRoundedRectangle (tempoBox.toFloat(), 4.0f);
    g.setColour (colours::ctrlEdge);
    g.drawRoundedRectangle (tempoBox.toFloat().reduced (0.5f), 4.0f, 1.0f);
    g.setColour (colours::accent);
    g.setFont (LAF::monoFont (22.0f));
    g.drawText (juce::String (proc.getDisplayTempo(), 2), tempoBox, juce::Justification::centred);

    drawSmallCaps (g, "MASTER", master.getBounds().withY (10).withHeight (14).expanded (10, 0), colours::dim, 10.0f,
                   juce::Justification::centred);

    // meter
    for (int c = 0; c < 2; ++c)
    {
        auto bar = juce::Rectangle<float> ((float) meterBox.getX() + c * 9.0f, (float) meterBox.getY(), 6.0f, (float) meterBox.getHeight());
        g.setColour (colours::well);
        g.fillRect (bar);
        const float lvl = juce::jlimit (0.0f, 1.0f, juce::jmap (juce::Decibels::gainToDecibels (proc.getMeterLevel (c), -60.0f), -60.0f, 0.0f, 0.0f, 1.0f));
        auto filled = bar.withTop (bar.getBottom() - bar.getHeight() * lvl);
        juce::ColourGradient grad (juce::Colour (0xffe0623a), bar.getX(), bar.getY(), juce::Colour (0xff4fbf8a), bar.getX(), bar.getBottom(), false);
        grad.addColour (0.3, juce::Colour (0xffd9b23a));
        g.setGradientFill (grad);
        g.fillRect (filled);
    }

    if (! proc.ownsEngine())
    {
        g.setColour (colours::accent);
        g.setFont (LAF::uiFont (11.0f));
        g.drawFittedText ("Another LX-6 is running the sound engine.\nThis one works as LXR editor + sequencer.",
                          670, 14, 270, 36, juce::Justification::centredLeft, 2);
    }
    hwButton.setToggleState (proc.hardware.isActive(), juce::dontSendNotification);
}

void TopBar::timerCallback()
{
    repaint (meterBox.expanded (2));
    if (proc.getDisplayTempo() != shownTempo || proc.kitName != shownKit)
    {
        shownTempo = proc.getDisplayTempo();
        shownKit = proc.kitName;
        repaint();
    }
}

void TopBar::mouseDown (const juce::MouseEvent& e)
{
    if (kitBox.contains (e.getPosition()))
        showKitMenu();
    tempoDragStart = proc.apvts.getRawParameterValue (ids::tempo)->load();
}

void TopBar::mouseDrag (const juce::MouseEvent& e)
{
    if (! tempoBox.contains (e.getMouseDownPosition()) || proc.isHostSynced())
        return;
    if (auto* p = proc.apvts.getParameter (ids::tempo))
    {
        const float bpm = juce::jlimit (40.0f, 250.0f, tempoDragStart - e.getDistanceFromDragStartY() * 0.25f);
        p->setValueNotifyingHost (p->convertTo0to1 (bpm));
        repaint();
    }
}

void TopBar::showKitMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("FACTORY");
    m.addItem (1, "Init Kit");
    const auto kits = LX6Processor::kitFolder().findChildFiles (juce::File::findFiles, false, "*.lx6kit");
    juce::Array<juce::File> sorted (kits);
    sorted.sort();
    if (! sorted.isEmpty())
    {
        m.addSectionHeader ("YOUR KITS");
        for (int i = 0; i < sorted.size(); ++i)
            m.addItem (100 + i, sorted[i].getFileNameWithoutExtension());
    }
    m.addSeparator();
    m.addItem (2, "Save kit as...");
    m.addItem (3, "Load kit file...");
    m.addItem (4, "Show kits folder");
    m.setLookAndFeel (&getLookAndFeel());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (kitBox)),
                     [this, sorted] (int r)
                     {
                         if (r == 1) proc.loadInitKit();
                         else if (r == 2) saveKitAs();
                         else if (r == 3)
                         {
                             chooser = std::make_unique<juce::FileChooser> ("Load LX-6 kit", LX6Processor::kitFolder(), "*.lx6kit");
                             chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                                   [this] (const juce::FileChooser& fc)
                                                   {
                                                       if (fc.getResult().existsAsFile()) proc.loadKit (fc.getResult());
                                                       repaint();
                                                   });
                         }
                         else if (r == 4) LX6Processor::kitFolder().revealToUser();
                         else if (r >= 100) proc.loadKit (sorted[r - 100]);
                         repaint();
                     });
}

void TopBar::saveKitAs()
{
    const auto suggested = LX6Processor::kitFolder().getChildFile (proc.kitName.toLowerCase().replaceCharacter (' ', '-') + ".lx6kit");
    chooser = std::make_unique<juce::FileChooser> ("Save LX-6 kit", suggested, "*.lx6kit");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f != juce::File())
                                  proc.saveKit (f.withFileExtension ("lx6kit"));
                              repaint();
                          });
}
}

//==============================================================================
LX6Editor::LX6Editor (LX6Processor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, lx6::designW, lx6::designH);

    top = std::make_unique<lx6::TopBar> (proc);
    content.addAndMakeVisible (*top);
    top->onAbout = [this] { showOverlay (std::make_unique<lx6::AboutOverlay>()); };
    top->onHardware = [this]
    {
        showOverlay (std::make_unique<lx6::HardwareOverlay> (proc.hardware, [this] { proc.sendWholeKitToHardware(); }));
    };
    top->onKitChanged = [this] { repaint(); };

    for (int v = 0; v < 6; ++v)
    {
        auto* s = strips.add (new lx6::VoiceStrip (proc, v));
        s->onSelect = [this] (int voice) { selectVoice (voice); };
        content.addAndMakeVisible (s);
    }
    voiceEditor = std::make_unique<lx6::VoiceEditor> (proc);
    fxLfo = std::make_unique<lx6::FxLfoPanel> (proc);
    seq = std::make_unique<lx6::SequencerPanel> (proc);
    content.addAndMakeVisible (*voiceEditor);
    content.addAndMakeVisible (*fxLfo);
    content.addAndMakeVisible (*seq);

    selectVoice (proc.selectedVoice);

    setResizable (true, true);
    setResizeLimits (lx6::designW / 2, lx6::designH / 2, lx6::designW * 2, lx6::designH * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) lx6::designW / lx6::designH);
    setSize (lx6::designW, lx6::designH);
    startTimerHz (2);
}

LX6Editor::~LX6Editor()
{
    setLookAndFeel (nullptr);
}

void LX6Editor::selectVoice (int v)
{
    proc.selectedVoice = v;
    for (int i = 0; i < strips.size(); ++i)
    {
        strips[i]->selected = i == v;
        strips[i]->repaint();
    }
    voiceEditor->setVoice (v);
    fxLfo->setVoice (v);
    seq->setTrack (v);
}

void LX6Editor::showOverlay (std::unique_ptr<lx6::Overlay> o)
{
    overlay = std::move (o);
    overlay->onClose = [this] { juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<LX6Editor> (this)]
                                                                 { if (sp) sp->overlay.reset(); }); };
    content.addAndMakeVisible (*overlay);
    overlay->setBounds (0, 0, lx6::designW, lx6::designH);
}

void LX6Editor::timerCallback()
{
    if (! proc.ownsEngine() != engineBusyShown)
    {
        engineBusyShown = ! proc.ownsEngine();
        repaint();
    }
}

void LX6Editor::resized()
{
    const float scale = (float) getWidth() / (float) lx6::designW;
    content.setTransform (juce::AffineTransform::scale (scale));

    top->setBounds (0, 0, lx6::designW, 64);
    const int seqH = 168, midY = 64, midH = lx6::designH - 64 - seqH;
    for (int v = 0; v < 6; ++v)
        strips[v]->setBounds (12 + v * 98, midY + 14, 92, midH - 28);
    const int stripsRight = 12 + 6 * 98 + 6;
    fxLfo->setBounds (lx6::designW - 200, midY, 200, midH);
    voiceEditor->setBounds (stripsRight, midY, lx6::designW - 200 - stripsRight, midH);
    seq->setBounds (0, lx6::designH - seqH, lx6::designW, seqH);
    if (overlay != nullptr)
        overlay->setBounds (0, 0, lx6::designW, lx6::designH);
}

void LX6Editor::paint (juce::Graphics& g)
{
    g.fillAll (lx6::colours::bg);
}
