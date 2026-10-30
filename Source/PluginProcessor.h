// LX-6 - six-voice digital drum synthesizer, based on the open-source Erica Synths LXR firmware.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LxrParams.h"
#include "Sequencer.h"
#include "HardwareLink.h"
#include "Randomizer.h"

class LX6Processor : public juce::AudioProcessor
{
public:
    LX6Processor();
    ~LX6Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LX-6"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- used by the editor
    juce::AudioProcessorValueTreeState apvts;
    lx6::Sequencer sequencer;
    lx6::HardwareLink hardware;
    lx6::Randomizer randomizer { apvts };

    bool ownsEngine() const { return engineOwner; }
    void audition (int track, int velocity = 110);
    bool wasTriggered (int track);           // LED flash, clears the flag
    float getMeterLevel (int ch) const { return meter[(size_t) ch].load(); }

    // transport when the host is not playing (standalone, or "play" button)
    std::atomic<bool> internalPlay { false };
    double getDisplayTempo() const { return displayTempo.load(); }
    bool isHostSynced() const { return hostPlaying.load(); }

    // kits
    juce::String kitName { "INIT KIT" };
    void loadInitKit();
    bool saveKit (const juce::File& f);
    bool loadKit (const juce::File& f);
    static juce::File kitFolder();
    void sendWholeKitToHardware() { resendAllToHardware = true; }

    int selectedVoice = 0;

private:
    void syncParameters (juce::MidiBuffer& midiOut);
    void fireTrigger (int track, int velocity, juce::MidiBuffer& midiOut, int offset);
    void renderEngine (juce::AudioBuffer<float>& out, int start, int num, bool mixOut2IntoMain);
    void pullEngineFrames (int needed);

    static std::atomic<bool> engineInUse;
    bool engineOwner = false;

    std::array<int, 256> sentToEngine;
    std::array<int, 256> sentToHardware;
    std::vector<std::atomic<float>*> rawParams;
    std::atomic<bool> resendAllToHardware { false };

    // resampling from the engine's 44.14 kHz to the host rate
    double hostRate = 44100.0;
    std::array<juce::LagrangeInterpolator, 4> interpolators;
    std::array<std::vector<float>, 4> engineQueue;
    int queued = 0;
    std::array<std::vector<float>, 4> scratch;

    // transport
    double internalPpq = 0.0;
    std::atomic<double> displayTempo { 120.0 };
    std::atomic<bool> hostPlaying { false };
    bool wasPlaying = false;

    juce::Array<lx6::Sequencer::Trigger> triggers;
    std::array<std::atomic<int>, 7> pendingAudition {};
    std::array<std::atomic<bool>, 7> triggeredFlag {};
    std::array<std::atomic<float>, 2> meter {};

    juce::MidiBuffer midiOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LX6Processor)
};
