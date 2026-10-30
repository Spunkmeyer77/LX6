// LX-6 - hardware editor mode for the Erica Synths LXR-02.
// SPDX-License-Identifier: GPL-3.0-only
#include "HardwareLink.h"
#include "LxrParamTable.h"

namespace lx6
{
HardwareLink::HardwareLink()
{
    for (size_t i = 0; i < voiceChannels.size(); ++i)
        voiceChannels[i] = (int) i + 1; // voices on channels 1-7 by default
    pending.ensureSize (8192);
}

HardwareLink::~HardwareLink()
{
    openDevice ({});
}

void HardwareLink::setVoiceChannel (int track, int ch)
{
    if (juce::isPositiveAndBelow (track, (int) voiceChannels.size()))
        voiceChannels[(size_t) track] = juce::jlimit (1, 16, ch);
}

int HardwareLink::getVoiceChannel (int track) const
{
    return juce::isPositiveAndBelow (track, (int) voiceChannels.size()) ? voiceChannels[(size_t) track].load() : 1;
}

juce::StringArray HardwareLink::getDeviceNames() const
{
    juce::StringArray names;
    for (auto& d : juce::MidiOutput::getAvailableDevices())
        names.add (d.name);
    return names;
}

bool HardwareLink::openDevice (const juce::String& name)
{
    std::unique_ptr<juce::MidiOutput> newDevice;
    if (name.isNotEmpty())
        for (auto& d : juce::MidiOutput::getAvailableDevices())
            if (d.name == name)
            {
                newDevice = juce::MidiOutput::openDevice (d.identifier);
                if (newDevice != nullptr)
                    newDevice->startBackgroundThread();
                break;
            }

    std::unique_ptr<juce::MidiOutput> old;
    {
        const juce::SpinLock::ScopedLockType sl (deviceLock);
        old = std::move (device);
        device = std::move (newDevice);
        deviceName = device != nullptr ? name : juce::String();
    }
    return name.isEmpty() || deviceName.isNotEmpty();
}

juce::String HardwareLink::getDeviceName() const
{
    const juce::SpinLock::ScopedLockType sl (deviceLock);
    return deviceName;
}

bool HardwareLink::canSendOverMidi (int lxrNr)
{
    // velocity / LFO destinations are indices the LXR only accepts from its own panel
    if ((lxrNr >= 149 && lxrNr <= 154) || (lxrNr >= 167 && lxrNr <= 172))
        return false;
    return lxrNr > 0 && lxrNr < lxr::numParamInfos && lxrNr != 5 && lxrNr != 97 && lxrNr != 98 && lxrNr != 127;
}

void HardwareLink::add (const juce::MidiMessage& m, juce::MidiBuffer& pluginOut, int sampleOffset)
{
    pluginOut.addEvent (m, sampleOffset);
    pending.addEvent (m, sampleOffset);
}

void HardwareLink::sendParam (int lxrNr, int value, juce::MidiBuffer& pluginOut, int offset)
{
    if (! isActive() || ! canSendOverMidi (lxrNr))
        return;
    const int ch = paramChannel.load();
    value = juce::jlimit (0, 127, value);

    if (lxrNr < 127)
    {
        // the LXR reads CC n as parameter n-1
        add (juce::MidiMessage::controllerEvent (ch, lxrNr + 1, value), pluginOut, offset);
    }
    else
    {
        // parameters from 128 up: NRPN (MSB 99, LSB 98) + data entry (6)
        const int nrpn = lxrNr - 128;
        add (juce::MidiMessage::controllerEvent (ch, 99, (nrpn >> 7) & 0x7f), pluginOut, offset);
        add (juce::MidiMessage::controllerEvent (ch, 98, nrpn & 0x7f), pluginOut, offset);
        add (juce::MidiMessage::controllerEvent (ch, 6, value), pluginOut, offset);
    }
}

void HardwareLink::sendTrigger (int track, int velocity, juce::MidiBuffer& pluginOut, int offset)
{
    if (! isActive())
        return;
    const int ch = getVoiceChannel (track);
    add (juce::MidiMessage::noteOn (ch, 60, (juce::uint8) juce::jlimit (1, 127, velocity)), pluginOut, offset);
    add (juce::MidiMessage::noteOff (ch, 60), pluginOut, offset);
}

void HardwareLink::flush (double sampleRate)
{
    if (pending.isEmpty())
        return;
    {
        const juce::SpinLock::ScopedTryLockType sl (deviceLock);
        if (sl.isLocked() && device != nullptr)
            device->sendBlockOfMessages (pending, juce::Time::getMillisecondCounterHiRes() + 1.0, sampleRate);
    }
    pending.clear();
}
}
