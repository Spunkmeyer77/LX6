// LX-6 - six-voice digital drum synthesizer, based on the open-source Erica Synths LXR firmware.
// SPDX-License-Identifier: GPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "lx6_engine.h"

extern "C" void lx6_engine_onMidiOut (uint8_t, uint8_t, uint8_t) {}

// the LXR engine is one set of C globals: only one plugin instance per process can run it
std::atomic<bool> LX6Processor::engineInUse { false };

static constexpr int queueCapacity = 4096;

LX6Processor::LX6Processor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Main", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Out 2", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "LX6", lx6::createLayout())
{
    for (auto& d : lx6::allParams())
        rawParams.push_back (apvts.getRawParameterValue (lx6::paramId (d.lxrNr)));
    sentToEngine.fill (-1);
    sentToHardware.fill (-1);

    for (auto& q : engineQueue) q.assign (queueCapacity, 0.0f);
    for (auto& f : triggeredFlag) f = false;
    for (auto& a : pendingAudition) a = 0;
    for (auto& m : meter) m = 0.0f;
}

LX6Processor::~LX6Processor()
{
    if (engineOwner)
        engineInUse = false;
}

bool LX6Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.outputBuses.size() > 1)
    {
        const auto& aux = layouts.outputBuses[1];
        if (! aux.isDisabled() && aux != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void LX6Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate;
    for (auto& i : interpolators) i.reset();
    for (auto& s : scratch) s.assign ((size_t) juce::jmax (512, samplesPerBlock), 0.0f);
    queued = 0;
    midiOut.ensureSize (8192);
    triggers.ensureStorageAllocated (256);

    if (! engineOwner)
    {
        bool expected = false;
        if (engineInUse.compare_exchange_strong (expected, true))
        {
            engineOwner = true;
            lx6_engine_init();
            sentToEngine.fill (-1);
        }
    }
}

//==============================================================================
void LX6Processor::audition (int track, int velocity)
{
    if (juce::isPositiveAndBelow (track, 7))
        pendingAudition[(size_t) track] = velocity;
}

bool LX6Processor::wasTriggered (int track)
{
    return juce::isPositiveAndBelow (track, 7) && triggeredFlag[(size_t) track].exchange (false);
}

void LX6Processor::syncParameters (juce::MidiBuffer& out)
{
    const auto& defs = lx6::allParams();
    const bool resend = resendAllToHardware.exchange (false);

    for (size_t i = 0; i < defs.size(); ++i)
    {
        const int nr = defs[i].lxrNr;
        const int v = juce::roundToInt (rawParams[i]->load());

        if (engineOwner && sentToEngine[(size_t) nr] != v)
        {
            lx6_engine_setParam ((uint16_t) nr, (uint8_t) v);
            sentToEngine[(size_t) nr] = v;
        }
        if (hardware.isActive() && (resend || sentToHardware[(size_t) nr] != v))
        {
            hardware.sendParam (nr, v, out, 0);
            sentToHardware[(size_t) nr] = v;
        }
    }
    if (! hardware.isActive())
        sentToHardware.fill (-1); // send everything once hardware mode is switched on
}

void LX6Processor::fireTrigger (int track, int velocity, juce::MidiBuffer& out, int offset)
{
    const int voice = juce::jmin (track, 5);

    // mute / solo
    bool anySolo = false;
    for (int v = 0; v < lx6::numVoices; ++v)
        anySolo |= apvts.getRawParameterValue (lx6::ids::solo + juce::String (v))->load() > 0.5f;
    const bool muted = apvts.getRawParameterValue (lx6::ids::mute + juce::String (voice))->load() > 0.5f;
    const bool soloed = apvts.getRawParameterValue (lx6::ids::solo + juce::String (voice))->load() > 0.5f;
    if (muted || (anySolo && ! soloed))
        return;

    if (engineOwner && hardware.internalEngineEnabled())
        lx6_engine_trigger ((uint8_t) track, (uint8_t) juce::jlimit (1, 127, velocity), 60);
    hardware.sendTrigger (track, velocity, out, offset);
    triggeredFlag[(size_t) track] = true;
}

void LX6Processor::pullEngineFrames (int needed)
{
    int16_t o1[LX6_BLOCK_FRAMES * 2], o2[LX6_BLOCK_FRAMES * 2];
    while (queued < needed && queued + LX6_BLOCK_FRAMES <= queueCapacity)
    {
        lx6_engine_render (o1, o2);
        constexpr float scale = 0.5f / 32768.0f; // 6 dB headroom: both LXR outs can sum into the main bus
        for (int i = 0; i < LX6_BLOCK_FRAMES; ++i)
        {
            engineQueue[0][(size_t) (queued + i)] = o1[2 * i] * scale;
            engineQueue[1][(size_t) (queued + i)] = o1[2 * i + 1] * scale;
            engineQueue[2][(size_t) (queued + i)] = o2[2 * i] * scale;
            engineQueue[3][(size_t) (queued + i)] = o2[2 * i + 1] * scale;
        }
        queued += LX6_BLOCK_FRAMES;
    }
}

void LX6Processor::renderEngine (juce::AudioBuffer<float>& out, int start, int num, bool mixOut2IntoMain)
{
    const double ratio = LX6_ENGINE_RATE / hostRate;
    while (num > 0)
    {
        const int n = juce::jmin (num, 256, (int) scratch[0].size());
        pullEngineFrames ((int) std::ceil (n * ratio) + 4);

        int used = 0;
        for (int c = 0; c < 4; ++c)
            used = interpolators[(size_t) c].process (ratio, engineQueue[(size_t) c].data(), scratch[(size_t) c].data(), n);

        used = juce::jmin (used, queued);
        for (auto& q : engineQueue)
            std::memmove (q.data(), q.data() + used, sizeof (float) * (size_t) (queued - used));
        queued -= used;

        const int outChans = out.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            float l = scratch[0][(size_t) i], r = scratch[1][(size_t) i];
            if (mixOut2IntoMain)
            {
                l += scratch[2][(size_t) i];
                r += scratch[3][(size_t) i];
            }
            out.addSample (0, start + i, l);
            out.addSample (1, start + i, r);
            if (! mixOut2IntoMain && outChans >= 4)
            {
                out.addSample (2, start + i, scratch[2][(size_t) i]);
                out.addSample (3, start + i, scratch[3][(size_t) i]);
            }
        }
        start += n;
        num -= n;
    }
}

void LX6Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    midiOut.clear();

    // a second instance takes over the engine when the first one is removed
    if (! engineOwner)
    {
        bool expected = false;
        if (engineInUse.compare_exchange_strong (expected, true))
        {
            engineOwner = true;
            lx6_engine_init();
            sentToEngine.fill (-1);
        }
    }

    syncParameters (midiOut);

    // --- transport
    double bpm = apvts.getRawParameterValue (lx6::ids::tempo)->load();
    double ppq = -1.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing)
                if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    hostPlaying = playing;

    if (! playing && internalPlay.load())
    {
        if (! wasPlaying) internalPpq = 0.0;
        ppq = internalPpq;
        internalPpq += numSamples * bpm / (60.0 * hostRate);
        playing = true;
    }
    wasPlaying = playing && ! hostPlaying.load();
    displayTempo = bpm;
    if (engineOwner)
        lx6_engine_setBpm ((uint16_t) juce::roundToInt (bpm));

    // --- triggers: sequencer, MIDI notes, auditions
    triggers.clearQuick();
    const float swing = apvts.getRawParameterValue (lx6::ids::swing)->load();
    sequencer.process (playing ? ppq : -1.0, bpm / (60.0 * hostRate), numSamples, swing, triggers);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            // C1 (36) .. F#1 (42): Drum 1, Drum 2, Drum 3, Snare, Cymbal, Closed HH, Open HH
            const int track = m.getNoteNumber() - 36;
            if (juce::isPositiveAndBelow (track, 7))
                triggers.add ({ meta.samplePosition, track, m.getVelocity() });
        }
    }
    for (int t = 0; t < 7; ++t)
        if (const int v = pendingAudition[(size_t) t].exchange (0))
            triggers.add ({ 0, t, v });

    std::sort (triggers.begin(), triggers.end(),
               [] (const auto& a, const auto& b) { return a.sampleOffset < b.sampleOffset; });

    // --- render, splitting the block at each trigger
    const bool aux = buffer.getNumChannels() >= 4 && getBus (false, 1) != nullptr && getBus (false, 1)->isEnabled();
    const bool renderAudio = engineOwner && hardware.internalEngineEnabled();
    int pos = 0, ti = 0;
    for (;;)
    {
        while (ti < triggers.size() && triggers.getReference (ti).sampleOffset <= pos)
        {
            const auto& t = triggers.getReference (ti++);
            fireTrigger (t.track, t.velocity, midiOut, pos);
        }
        const int next = ti < triggers.size() ? juce::jmin (numSamples, triggers.getReference (ti).sampleOffset)
                                              : numSamples;
        if (renderAudio && next > pos)
            renderEngine (buffer, pos, next - pos, ! aux);
        pos = next;
        if (pos >= numSamples)
            break;
    }

    // --- master and meters
    const float gain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (lx6::ids::master)->load(), -60.0f);
    buffer.applyGain (gain);
    for (int c = 0; c < 2; ++c)
    {
        const float peak = buffer.getMagnitude (c, 0, numSamples);
        meter[(size_t) c] = juce::jmax (peak, meter[(size_t) c].load() * 0.85f);
    }

    hardware.flush (hostRate);
    midi.swapWith (midiOut);
}

//==============================================================================
void LX6Processor::loadInitKit()
{
    for (auto& d : lx6::allParams())
        if (auto* p = apvts.getParameter (lx6::paramId (d.lxrNr)))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, d.maxValue, lx6::defaultValue (d.lxrNr))));
    kitName = "INIT KIT";
}

juce::File LX6Processor::kitFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("LX-6").getChildFile ("Kits");
    f.createDirectory();
    return f;
}

bool LX6Processor::saveKit (const juce::File& f)
{
    juce::XmlElement kit ("LX6KIT");
    kit.setAttribute ("name", f.getFileNameWithoutExtension());
    kit.setAttribute ("version", LX6_VERSION_STRING);
    for (auto& d : lx6::allParams())
        if (auto* p = apvts.getParameter (lx6::paramId (d.lxrNr)))
            kit.setAttribute (lx6::paramId (d.lxrNr), juce::roundToInt (p->convertFrom0to1 (p->getValue())));
    if (! kit.writeTo (f))
        return false;
    kitName = f.getFileNameWithoutExtension().toUpperCase();
    return true;
}

bool LX6Processor::loadKit (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("LX6KIT"))
        return false;
    for (auto& d : lx6::allParams())
    {
        const auto id = lx6::paramId (d.lxrNr);
        if (xml->hasAttribute (id))
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, d.maxValue, xml->getIntAttribute (id))));
    }
    kitName = xml->getStringAttribute ("name", f.getFileNameWithoutExtension()).toUpperCase();
    return true;
}

//==============================================================================
void LX6Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    xml->setAttribute ("kitName", kitName);
    xml->setAttribute ("selectedVoice", selectedVoice);
    xml->setAttribute ("editPattern", sequencer.getEditPattern());
    xml->setAttribute ("playPattern", sequencer.getPlayingPattern());

    auto* seq = xml->createNewChildElement ("SEQUENCER");
    seq->setAttribute ("data", sequencer.toString());

    auto* hw = xml->createNewChildElement ("HARDWARE");
    hw->setAttribute ("mode", (int) hardware.getMode());
    hw->setAttribute ("device", hardware.getDeviceName());
    hw->setAttribute ("paramChannel", hardware.getParamChannel());
    for (int t = 0; t < 7; ++t)
        hw->setAttribute ("voiceChannel" + juce::String (t), hardware.getVoiceChannel (t));

    copyXmlToBinary (*xml, dest);
}

void LX6Processor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    kitName = xml->getStringAttribute ("kitName", "INIT KIT");
    selectedVoice = juce::jlimit (0, 5, xml->getIntAttribute ("selectedVoice", 0));
    sequencer.setEditPattern (xml->getIntAttribute ("editPattern", 0));
    sequencer.queuePattern (xml->getIntAttribute ("playPattern", 0));

    if (auto* seq = xml->getChildByName ("SEQUENCER"))
        sequencer.fromString (seq->getStringAttribute ("data"));

    if (auto* hw = xml->getChildByName ("HARDWARE"))
    {
        hardware.setMode ((lx6::HardwareLink::Mode) juce::jlimit (0, 2, hw->getIntAttribute ("mode", 0)));
        hardware.setParamChannel (hw->getIntAttribute ("paramChannel", 16));
        for (int t = 0; t < 7; ++t)
            hardware.setVoiceChannel (t, hw->getIntAttribute ("voiceChannel" + juce::String (t), t + 1));
        const auto dev = hw->getStringAttribute ("device");
        if (dev.isNotEmpty())
            hardware.openDevice (dev);
    }

    // drop our own child elements before handing the tree to the APVTS
    xml->deleteAllChildElementsWithTagName ("SEQUENCER");
    xml->deleteAllChildElementsWithTagName ("HARDWARE");
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LX6Processor::createEditor() { return new LX6Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LX6Processor(); }
