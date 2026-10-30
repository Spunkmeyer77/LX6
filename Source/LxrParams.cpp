// LX-6 - parameter map between the plugin and the LXR engine.
// SPDX-License-Identifier: GPL-3.0-only
#include "LxrParams.h"

namespace lx6
{
namespace
{
    const int oscWaveParam[numVoices] = { 1, 2, 3, 4, 6, 7 };
    const int panParams[numVoices] = { 94, 95, 96, 99, 100, 101 };

    const lxr::ParamInfo& info (int nr)
    {
        jassert (nr >= 0 && nr < lxr::numParamInfos);
        return lxr::paramInfos[nr];
    }

    struct Builder
    {
        std::vector<ParamDef> defs;

        void add (int nr, int voice, Section s, const juce::String& label)
        {
            const auto& pi = info (nr);
            ParamDef d;
            d.lxrNr = nr;
            d.voice = voice;
            d.section = s;
            d.label = label;
            d.name = (voice >= 0 ? voiceNames()[voice].toLowerCase() + " " : juce::String ("fx "))
                         + label.toLowerCase();
            // nicer capitalisation for hosts: "Drum 1 Cutoff"
            juce::StringArray words;
            words.addTokens (d.name, " ", "");
            for (auto& w : words)
                w = w.substring (0, 1).toUpperCase() + w.substring (1);
            d.name = words.joinIntoString (" ");
            d.kind = pi.kind;
            d.maxValue = pi.maxValue;
            if (pi.kind == lxr::Kind::modtarget)
                d.maxValue = lxr::numModTargets - 1;
            if (pi.kind == lxr::Kind::menu)
                d.options.addTokens (pi.options, "|", "");
            defs.push_back (d);
        }
    };

    std::vector<ParamDef> build()
    {
        Builder b;
        for (int v = 0; v < numVoices; ++v)
        {
            const bool drum = v < 3, snare = v == 3, cym = v == 4, hat = v == 5;

            // oscillator
            b.add (oscWaveParam[v], v, Section::Osc, "WAVE");
            b.add (8 + 2 * v, v, Section::Osc, "COARSE");
            b.add (9 + 2 * v, v, Section::Osc, "FINE");
            b.add (228 + v, v, Section::Osc, "PWM");
            if (drum)
            {
                b.add (20 + v, v, Section::Osc, "MOD WAVE");
                b.add (82 + 2 * v, v, Section::Osc, "FM AMT");
                b.add (83 + 2 * v, v, Section::Osc, "FM FREQ");
                b.add (134 + v, v, Section::Osc, "FM/MIX");
            }
            if (snare)
            {
                b.add (27, v, Section::Osc, "NOISE FREQ");
                b.add (28, v, Section::Osc, "NOISE MIX");
            }
            if (cym)
            {
                b.add (23, v, Section::Osc, "WAVE 2");
                b.add (24, v, Section::Osc, "WAVE 3");
                b.add (29, v, Section::Osc, "MOD1 FREQ");
                b.add (30, v, Section::Osc, "MOD2 FREQ");
                b.add (31, v, Section::Osc, "MOD1 GAIN");
                b.add (32, v, Section::Osc, "MOD2 GAIN");
            }
            if (hat)
            {
                b.add (25, v, Section::Osc, "WAVE 2");
                b.add (26, v, Section::Osc, "WAVE 3");
                b.add (33, v, Section::Osc, "MOD1 FREQ");
                b.add (34, v, Section::Osc, "MOD2 FREQ");
                b.add (35, v, Section::Osc, "MOD1 GAIN");
                b.add (36, v, Section::Osc, "MOD2 GAIN");
            }

            // pitch envelope (drums + snare)
            if (v < 4)
            {
                b.add (70 + v, v, Section::Pitch, "P DECAY");
                b.add (78 + v, v, Section::Pitch, "P SLOPE");
                b.add (74 + v, v, Section::Pitch, "P AMOUNT");
            }

            // filter
            b.add (191 + v, v, Section::Filter, "TYPE");
            b.add (37 + v, v, Section::Filter, "CUTOFF");
            b.add (43 + v, v, Section::Filter, "RESO");
            b.add (128 + v, v, Section::Filter, "F DRIVE");

            // amp envelope
            b.add (49 + 2 * v, v, Section::Amp, "ATTACK");
            b.add (50 + 2 * v, v, Section::Amp, hat ? "DECAY CL" : "DECAY");
            if (hat)
                b.add (61, v, Section::Amp, "DECAY OP");
            b.add (62 + v, v, Section::Amp, "SLOPE");
            if (snare) b.add (68, v, Section::Amp, "REPEAT");
            if (cym)   b.add (69, v, Section::Amp, "REPEAT");

            // transient
            b.add (203 + v, v, Section::Transient, "T WAVE");
            b.add (197 + v, v, Section::Transient, "T LEVEL");
            b.add (209 + v, v, Section::Transient, "T FREQ");

            // voice
            b.add (88 + v, v, Section::Voice, "VOLUME");
            b.add (panParams[v], v, Section::Voice, "PAN");
            b.add (102 + v, v, Section::Voice, "DRIVE");
            b.add (108 + v, v, Section::Voice, "S.RATE");
            b.add (215 + v, v, Section::Voice, "OUTPUT");

            // velocity modulation
            b.add (143 + v, v, Section::Velocity, "VEL AMT");
            b.add (149 + v, v, Section::Velocity, "VEL DEST");
            b.add (137 + v, v, Section::Velocity, "VEL>VOL");

            // LFO
            b.add (115 + v, v, Section::Lfo, "RATE");
            b.add (121 + v, v, Section::Lfo, "DEPTH");
            b.add (155 + v, v, Section::Lfo, "SHAPE");
            b.add (167 + v, v, Section::Lfo, "DEST");
            b.add (161 + v, v, Section::Lfo, "DEST VOICE");
            b.add (173 + v, v, Section::Lfo, "RETRIG");
            b.add (179 + v, v, Section::Lfo, "SYNC");
            b.add (185 + v, v, Section::Lfo, "OFFSET");
        }

        // FX bus
        b.add (234, -1, Section::Fx, "TYPE");
        b.add (236, -1, Section::Fx, "DRY/WET");
        b.add (238, -1, Section::Fx, "P1");
        b.add (239, -1, Section::Fx, "P2");
        b.add (240, -1, Section::Fx, "P3");
        b.add (241, -1, Section::Fx, "P4");
        b.add (237, -1, Section::Fx, "DIST TYPE");
        b.add (242, -1, Section::Fx, "RATIO");
        b.add (243, -1, Section::Fx, "RING WAVE");
        b.add (245, -1, Section::Fx, "DELAY TYPE");
        b.add (235, -1, Section::Fx, "FX OUT");
        b.add (114, -1, Section::Fx, "S.RATE ALL");
        return b.defs;
    }
}

const juce::StringArray& voiceNames()
{
    static const juce::StringArray n { "DRUM 1", "DRUM 2", "DRUM 3", "SNARE", "CYMBAL", "HI-HAT" };
    return n;
}

const juce::StringArray& trackNames()
{
    static const juce::StringArray n { "DRUM 1", "DRUM 2", "DRUM 3", "SNARE", "CYMBAL", "HI-HAT", "OPEN HH" };
    return n;
}

const juce::String& sectionName (Section s)
{
    static const juce::String names[] { "OSCILLATOR", "PITCH MOD", "FILTER", "AMP ENVELOPE",
                                        "TRANSIENT", "VOICE", "VELOCITY", "LFO", "FX BUS" };
    return names[(int) s];
}

const std::vector<ParamDef>& allParams()
{
    static const std::vector<ParamDef> defs = build();
    return defs;
}

const ParamDef* findByLxr (int lxrNr)
{
    static const auto index = []
    {
        std::array<int, lxr::numParamInfos> idx;
        idx.fill (-1);
        const auto& d = allParams();
        for (int i = 0; i < (int) d.size(); ++i)
            idx[(size_t) d[(size_t) i].lxrNr] = i;
        return idx;
    }();
    if (lxrNr < 0 || lxrNr >= lxr::numParamInfos || index[(size_t) lxrNr] < 0)
        return nullptr;
    return &allParams()[(size_t) index[(size_t) lxrNr]];
}

juce::String paramId (int lxrNr) { return "p" + juce::String (lxrNr); }

std::vector<const ParamDef*> paramsFor (int voice, Section s)
{
    std::vector<const ParamDef*> out;
    for (auto& d : allParams())
        if (d.voice == voice && d.section == s)
            out.push_back (&d);
    return out;
}

int volumeParam (int v) { return 88 + v; }
int panParam (int v)    { return panParams[v]; }
int coarseParam (int v) { return 8 + 2 * v; }
int decayParam (int v)  { return 50 + 2 * v; }

int lfo (int v, int which)
{
    static const int base[] { 115, 121, 155, 167, 161, 173, 179, 185 };
    return base[which] + v;
}

juce::String modTargetName (int index)
{
    if (index <= 0 || index >= lxr::numModTargets)
        return "OFF";
    const int p = lxr::modTargetParams[index];
    if (const auto* d = findByLxr (p))
    {
        static const char* shortVoice[] { "D1", "D2", "D3", "SN", "CY", "HH" };
        return (d->voice >= 0 ? juce::String (shortVoice[d->voice]) + " " : juce::String()) + d->label;
    }
    if (p == 114)
        return "S.RATE ALL";
    return "P" + juce::String (p);
}

juce::String valueToText (const ParamDef& d, int value)
{
    switch (d.kind)
    {
        case lxr::Kind::menu:
            if (juce::isPositiveAndBelow (value, d.options.size()))
                return d.options[value].toUpperCase();
            return juce::String (value);
        case lxr::Kind::pm63:      return juce::String (value - 63);
        case lxr::Kind::pm100:     return juce::String (juce::roundToInt ((value - 63.5f) / 63.5f * 100.0f));
        case lxr::Kind::onoff:     return value ? "ON" : "OFF";
        case lxr::Kind::u1b16:     return juce::String (value + 1);
        case lxr::Kind::modtarget: return modTargetName (value);
        case lxr::Kind::voice:     return value == 0 ? "OFF" : voiceNames()[juce::jlimit (0, 5, value - 1)];
        case lxr::Kind::u127: case lxr::Kind::u255: case lxr::Kind::u15: case lxr::Kind::u100:
        default:                   return juce::String (value);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (auto& d : allParams())
    {
        const int def = juce::jlimit (0, d.maxValue, (int) lxr::initKitDefaults[d.lxrNr]);
        const auto* dp = &d;
        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { paramId (d.lxrNr), 1 }, d.name, 0, d.maxValue, def,
            juce::AudioParameterIntAttributes()
                .withStringFromValueFunction ([dp] (int v, int) { return valueToText (*dp, v); })));
    }

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::master, 1 }, "Master",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::tempo, 1 }, "Tempo (standalone)",
        juce::NormalisableRange<float> (40.0f, 250.0f, 0.1f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::swing, 1 }, "Swing",
        juce::NormalisableRange<float> (0.0f, 75.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    for (int t = 0; t < numVoices; ++t)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::mute + juce::String (t), 1 }, voiceNames()[t] + " Mute", false));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::solo + juce::String (t), 1 }, voiceNames()[t] + " Solo", false));
    }
    return layout;
}
}
