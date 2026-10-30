// LX-6 - sound randomizer with section locks, amount and undo.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "LxrParams.h"

namespace lx6
{
class Randomizer
{
public:
    explicit Randomizer (juce::AudioProcessorValueTreeState& state) : apvts (state) {}

    // amount 0..1: 0 = tiny variation around the current sound, 1 = completely new values
    float amount = 0.5f;
    // sections that are protected from randomizing
    std::set<Section> locked { Section::Voice };

    void randomiseKit();
    void randomiseVoice (int voice);
    void randomiseSection (int voice, Section s);

    bool canUndo() const { return ! undoStack.empty(); }
    void undo();

private:
    void pushUndo();
    void randomiseParam (const ParamDef& d);
    juce::AudioProcessorValueTreeState& apvts;
    juce::Random rng;
    std::vector<std::vector<std::pair<juce::String, float>>> undoStack;
};
}
