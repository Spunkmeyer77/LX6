// LX-6 - sound randomizer with section locks, amount and undo.
// SPDX-License-Identifier: GPL-3.0-only
#include "Randomizer.h"

namespace lx6
{
void Randomizer::pushUndo()
{
    std::vector<std::pair<juce::String, float>> snap;
    for (auto& d : allParams())
        if (auto* p = apvts.getParameter (paramId (d.lxrNr)))
            snap.emplace_back (paramId (d.lxrNr), p->getValue());
    undoStack.push_back (std::move (snap));
    if (undoStack.size() > 32)
        undoStack.erase (undoStack.begin());
}

void Randomizer::undo()
{
    if (undoStack.empty())
        return;
    for (auto& [id, v] : undoStack.back())
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (v);
    undoStack.pop_back();
}

void Randomizer::randomiseParam (const ParamDef& d)
{
    auto* p = apvts.getParameter (paramId (d.lxrNr));
    if (p == nullptr)
        return;

    // things that would only make a mess when random
    if (d.section == Section::Voice && (d.label == "OUTPUT" || d.label == "VOLUME"))
        return;
    if (d.section == Section::Lfo && (d.label == "DEST" || d.label == "DEST VOICE"))
        return;
    if (d.section == Section::Velocity && d.label == "VEL DEST")
        return;

    const int range = d.maxValue;
    const int current = juce::roundToInt (p->convertFrom0to1 (p->getValue()));
    int value;

    if (d.kind == lxr::Kind::menu || d.kind == lxr::Kind::onoff)
    {
        // switches: change with a probability given by the amount
        value = rng.nextFloat() < amount ? rng.nextInt (range + 1) : current;
        if (d.label == "TYPE" && d.section == Section::Filter && value == range)
            value = 0; // avoid "filter off" from the dice
    }
    else
    {
        const float spread = juce::jmax (0.02f, amount) * (float) range;
        const float target = (float) current + (rng.nextFloat() * 2.0f - 1.0f) * spread;
        value = juce::jlimit (0, range, juce::roundToInt (target));
    }

    // keep drums audible: no slow fades, no decays that collapse to nothing
    if (d.section == Section::Amp && d.label.startsWith ("DECAY"))
        value = juce::jmax (value, 16);
    if (d.section == Section::Amp && d.label == "ATTACK")
        value = juce::jmin (value, 24);
    if (d.section == Section::Velocity && d.label == "VEL>VOL")
        return;
    // sample-rate reduction: 127 is clean, low values crush the sound to almost nothing
    if (d.lxrNr >= 108 && d.lxrNr <= 114)
        value = juce::jmax (value, d.lxrNr == 114 ? 90 : 55);
    if (d.section == Section::Filter && d.label == "CUTOFF")
    {
        // low-pass types need some top end, high-pass types some bottom
        const int type = juce::roundToInt (apvts.getRawParameterValue (paramId (191 + d.voice))->load());
        if (type == 0 || type == 6) value = juce::jmax (value, 45);
        else if (type == 1)         value = juce::jmin (value, 95);
        else                        value = juce::jlimit (25, 110, value);
    }

    p->setValueNotifyingHost (p->convertTo0to1 ((float) value));
}

void Randomizer::randomiseSection (int voice, Section s)
{
    pushUndo();
    for (auto* d : paramsFor (voice, s))
        randomiseParam (*d);
}

void Randomizer::randomiseVoice (int voice)
{
    pushUndo();
    for (auto& d : allParams())
        if (d.voice == voice && locked.count (d.section) == 0)
            randomiseParam (d);
}

void Randomizer::randomiseKit()
{
    pushUndo();
    for (auto& d : allParams())
        if (locked.count (d.section) == 0)
            randomiseParam (d);
}
}
