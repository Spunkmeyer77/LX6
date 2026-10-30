// LX-6 - parameter map between the plugin and the LXR engine.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LxrParamTable.h"

namespace lx6
{
    constexpr int numVoices = 6;
    constexpr int numTracks = 7; // + open hi-hat

    enum class Section { Osc, Pitch, Filter, Amp, Transient, Voice, Velocity, Lfo, Fx };

    struct ParamDef
    {
        int lxrNr;            // LXR parameter number (Parameters.h)
        int voice;            // 0-5, -1 = global
        Section section;
        juce::String label;   // short label for knobs, e.g. "CUTOFF"
        juce::String name;    // full host name, e.g. "Drum 1 Cutoff"
        int maxValue;
        lxr::Kind kind;
        juce::StringArray options;
    };

    const juce::StringArray& voiceNames();      // "DRUM 1" ... "HI-HAT"
    const juce::StringArray& trackNames();      // + "OPEN HH"
    const juce::String& sectionName (Section s);

    // all parameters exposed by the plugin, in a stable order
    const std::vector<ParamDef>& allParams();
    const ParamDef* findByLxr (int lxrNr);
    juce::String paramId (int lxrNr);           // "p37"

    // parameters of one voice in one section, in panel order
    std::vector<const ParamDef*> paramsFor (int voice, Section s);

    // key per-voice controls used on the channel strips
    int volumeParam (int voice);
    int panParam (int voice);
    int coarseParam (int voice);
    int decayParam (int voice);
    int lfo (int voice, int which); // which: 0 rate,1 amount,2 wave,3 target,4 voice,5 retrigger,6 sync,7 offset

    // value display, e.g. "-12", "LP", "1/8", "D1 Coarse"
    juce::String valueToText (const ParamDef& d, int value);
    juce::String modTargetName (int index);

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // global (non-LXR) plugin parameter IDs
    namespace ids
    {
        inline const juce::String master = "master";
        inline const juce::String tempo = "tempo";
        inline const juce::String swing = "swing";
        inline const juce::String mute = "mute";  // + track index
        inline const juce::String solo = "solo";  // + track index
    }
}
