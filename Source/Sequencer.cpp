// LX-6 - 16-step pattern sequencer (7 tracks x 8 patterns), host-synced.
// SPDX-License-Identifier: GPL-3.0-only
#include "Sequencer.h"

namespace lx6
{
Sequencer::Sequencer()
{
    for (auto& d : data)
        d.store (pack ({}));

    // a simple starter beat in pattern A so the plugin makes sound right away
    for (int s : { 0, 4, 8, 12 }) setStep (0, 0, s, { true, 110, 100 });
    for (int s : { 4, 12 })       setStep (0, 3, s, { true, 100, 100 });
    for (int s = 0; s < 16; s += 2) setStep (0, 5, s, { true, s % 4 == 0 ? 90 : 60, 100 });
    setStep (0, 6, 14, { true, 80, 100 });
}

uint32_t Sequencer::pack (Step s)
{
    return (uint32_t) (s.on ? 1 : 0)
         | ((uint32_t) juce::jlimit (1, 127, s.velocity) << 1)
         | ((uint32_t) juce::jlimit (0, 100, s.probability) << 8);
}

Sequencer::Step Sequencer::unpack (uint32_t v)
{
    return { (v & 1u) != 0, (int) ((v >> 1) & 0x7f), (int) ((v >> 8) & 0x7f) };
}

Sequencer::Step Sequencer::getStep (int p, int t, int s) const { return unpack (data[(size_t) index (p, t, s)].load()); }
void Sequencer::setStep (int p, int t, int s, Step st)          { data[(size_t) index (p, t, s)].store (pack (st)); }

void Sequencer::toggleStep (int p, int t, int s)
{
    auto st = getStep (p, t, s);
    st.on = ! st.on;
    setStep (p, t, s, st);
}

void Sequencer::clearTrack (int p, int t)
{
    for (int s = 0; s < numSteps; ++s)
    {
        auto st = getStep (p, t, s);
        st.on = false;
        setStep (p, t, s, st);
    }
}

void Sequencer::euclid (int p, int t, int pulses, int rotation, int velocity)
{
    pulses = juce::jlimit (0, numSteps, pulses);
    for (int s = 0; s < numSteps; ++s)
    {
        // Bjorklund-equivalent: step is on when the running remainder wraps
        const int i = ((s - rotation) % numSteps + numSteps) % numSteps;
        const bool on = pulses > 0 && ((i * pulses) % numSteps) < pulses;
        auto st = getStep (p, t, s);
        st.on = on;
        if (on) st.velocity = velocity;
        setStep (p, t, s, st);
    }
}

void Sequencer::randomise (int p, int track, float density, juce::Random& r)
{
    for (int t = 0; t < numTracks; ++t)
    {
        if (track >= 0 && t != track)
            continue;
        // kicks/snares sparser, hats busier
        const float d = juce::jlimit (0.0f, 1.0f, density * (t >= 5 ? 1.6f : (t == 3 || t == 4 ? 0.7f : 1.0f)));
        for (int s = 0; s < numSteps; ++s)
        {
            Step st;
            st.on = r.nextFloat() < d * (s % 4 == 0 ? 1.5f : 1.0f);
            st.velocity = 50 + r.nextInt (78);
            st.probability = r.nextFloat() < 0.15f ? 50 + r.nextInt (50) : 100;
            setStep (p, t, s, st);
        }
    }
}

void Sequencer::copyPattern (int from, int to)
{
    for (int t = 0; t < numTracks; ++t)
        for (int s = 0; s < numSteps; ++s)
            setStep (to, t, s, getStep (from, t, s));
}

void Sequencer::queuePattern (int p)
{
    p = juce::jlimit (0, numPatterns - 1, p);
    if (running.load())
        queuedPattern = p;
    else
    {
        playingPattern = p;
        queuedPattern = -1;
    }
}

void Sequencer::stop()
{
    running = false;
    currentStep = -1;
    lastSixteenth = -1;
    const int q = queuedPattern.exchange (-1);
    if (q >= 0) playingPattern = q;
}

void Sequencer::process (double ppqStart, double ppqPerSample, int numSamples, float swing,
                         juce::Array<Trigger>& out)
{
    if (ppqStart < 0.0 || ppqPerSample <= 0.0)
    {
        if (running.load()) stop();
        return;
    }

    const double ppqEnd = ppqStart + ppqPerSample * numSamples;
    const double swingDelay = 0.25 * 0.5 * juce::jlimit (0.0, 0.75, (double) swing / 100.0);

    // a jump backwards (loop / relocate): forget what we played
    if (lastSixteenth >= 0 && ppqStart < (double) lastSixteenth * 0.25 - 1.0e-6)
        lastSixteenth = (juce::int64) std::ceil (ppqStart * 4.0) - 1;
    if (! running.load())
        lastSixteenth = (juce::int64) std::ceil (ppqStart * 4.0 - 1.0e-9) - 1;
    running = true;

    for (juce::int64 k = lastSixteenth + 1;; ++k)
    {
        const double t = (double) k * 0.25 + ((k & 1) ? swingDelay : 0.0);
        if (t >= ppqEnd)
            break;
        lastSixteenth = k;
        if (t < ppqStart - 1.0e-9)
            continue;

        const int step = (int) (k % numSteps);
        if (step == 0)
        {
            const int q = queuedPattern.exchange (-1);
            if (q >= 0) playingPattern = q;
        }
        currentStep = step;

        const int offset = juce::jlimit (0, numSamples - 1, (int) ((t - ppqStart) / ppqPerSample));
        const int pat = playingPattern.load();
        for (int tr = 0; tr < numTracks; ++tr)
        {
            const auto st = getStep (pat, tr, step);
            if (st.on && (st.probability >= 100 || rng.nextInt (100) < st.probability))
                out.add ({ offset, tr, st.velocity });
        }
    }
}

juce::String Sequencer::toString() const
{
    juce::MemoryOutputStream mo;
    for (auto& d : data)
        mo.writeInt ((int) d.load());
    return mo.getMemoryBlock().toBase64Encoding();
}

void Sequencer::fromString (const juce::String& s)
{
    juce::MemoryBlock mb;
    if (! mb.fromBase64Encoding (s) || mb.getSize() != data.size() * 4)
        return;
    juce::MemoryInputStream mi (mb, false);
    for (auto& d : data)
        d.store ((uint32_t) mi.readInt());
}
}
