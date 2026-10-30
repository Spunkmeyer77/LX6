// LX-6 - hardware editor mode: mirrors every edit and trigger to an Erica Synths LXR-02
// over MIDI (CC for parameters 1-127, NRPN for parameters 128+, notes per voice channel).
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>

namespace lx6
{
class HardwareLink
{
public:
    enum class Mode { Off = 0, Hardware = 1, Both = 2 }; // Hardware = internal engine silent

    HardwareLink();
    ~HardwareLink();

    // --- settings (message thread)
    void setMode (Mode m)               { mode = (int) m; }
    Mode getMode() const                { return (Mode) mode.load(); }
    bool isActive() const               { return mode.load() != (int) Mode::Off; }
    bool internalEngineEnabled() const  { return mode.load() != (int) Mode::Hardware; }

    void setParamChannel (int ch)       { paramChannel = juce::jlimit (1, 16, ch); }
    int getParamChannel() const         { return paramChannel.load(); }
    void setVoiceChannel (int track, int ch);
    int getVoiceChannel (int track) const;

    juce::StringArray getDeviceNames() const;
    bool openDevice (const juce::String& name); // "" closes
    juce::String getDeviceName() const;

    // --- messages (audio thread). They go to the chosen MIDI port and to the plugin's MIDI out.
    void sendParam (int lxrNr, int value, juce::MidiBuffer& pluginOut, int sampleOffset);
    void sendTrigger (int track, int velocity, juce::MidiBuffer& pluginOut, int sampleOffset);
    void flush (double sampleRate);     // sends this block's messages to the device

    static bool canSendOverMidi (int lxrNr);

private:
    void add (const juce::MidiMessage& m, juce::MidiBuffer& pluginOut, int sampleOffset);

    std::atomic<int> mode { (int) Mode::Off };
    std::atomic<int> paramChannel { 16 };
    std::array<std::atomic<int>, 7> voiceChannels;

    juce::SpinLock deviceLock;
    std::unique_ptr<juce::MidiOutput> device;
    juce::String deviceName;
    juce::MidiBuffer pending;
};
}
