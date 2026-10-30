// LX-6 offline smoke test (not part of the plugin).
// Renders audio through the processor, stresses the randomizer, checks hardware-mode MIDI,
// the one-engine-per-process rule, and writes snapshots of the editor views.
// SPDX-License-Identifier: GPL-3.0-only
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

struct TestPlayHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 120;
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setIsPlaying (playing);
        p.setPpqPosition (ppq);
        return p;
    }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    LX6Processor proc;
    TestPlayHead ph;
    proc.setPlayHead (&ph);
    const double sr = 48000;
    const int bs = 512;
    proc.prepareToPlay (sr, bs);

    juce::AudioBuffer<float> buf (2, bs), all (2, (int) sr * 4);
    juce::MidiBuffer midi;

    auto block = [&]
    {
        midi.clear();
        proc.processBlock (buf, midi);
        ph.ppq += bs * ph.bpm / (60.0 * sr);
        return buf.getMagnitude (0, bs);
    };

    // 1. the starter pattern at 120 BPM
    int written = 0;
    while (written + bs <= all.getNumSamples())
    {
        block();
        for (int c = 0; c < 2; ++c)
            all.copyFrom (c, written, buf, c, 0, bs);
        written += bs;
    }
    const float peak = all.getMagnitude (0, written);
    std::printf ("rendered %d samples, peak %.3f rms %.4f\n", written, peak, all.getRMSLevel (0, 0, written));

    juce::File out ("/tmp/lx6_render.wav");
    out.deleteFile();
    if (auto w = std::unique_ptr<juce::AudioFormatWriter> (juce::WavAudioFormat().createWriterFor (new juce::FileOutputStream (out), sr, 2, 16, {}, 0)))
        w->writeFromAudioSampleBuffer (all, 0, written);

    // 2. random kits must stay audible and never crash
    int quietKits = 0;
    bool fxOk = false;
    for (int k = 0; k < 60; ++k)
    {
        proc.randomizer.amount = 1.0f;
        proc.randomizer.randomiseKit();
        float pk = 0.0f;
        for (int b = 0; b < 100; ++b)
            pk = juce::jmax (pk, block());
        if (pk < 0.05f)
            ++quietKits;
    }
    proc.randomizer.undo();
    proc.loadInitKit();
    std::printf ("quiet random kits: %d of 60\n", quietKits);

    // 3. hardware mode: parameter changes and triggers come out as MIDI
    proc.hardware.setMode (lx6::HardwareLink::Mode::Both);
    int ccs = 0, notes = 0;
    for (int b = 0; b < 50; ++b)
    {
        if (b == 10) proc.apvts.getParameter ("p37")->setValueNotifyingHost (0.3f);
        block();
        for (const auto m : midi)
        {
            if (m.getMessage().isController()) ++ccs;
            if (m.getMessage().isNoteOn()) ++notes;
        }
    }
    proc.hardware.setMode (lx6::HardwareLink::Mode::Off);
    std::printf ("hardware mode: %d CC, %d notes\n", ccs, notes);

    // 3b. FX bus: drum 1 sent to the delay must change the sound (defaults must be audible)
    {
        auto setP = [&] (int nr, int v) { auto* p = proc.apvts.getParameter ("p" + juce::String (nr)); p->setValueNotifyingHost (p->convertTo0to1 ((float) v)); };
        auto energy = [&] { double e = 0; for (int b = 0; b < 200; ++b) { block(); e += buf.getRMSLevel (0, 0, bs); } return e; };
        proc.loadInitKit();
        const double dry = energy();
        setP (234, 4); setP (215, 6);
        const double del = energy();
        setP (234, 3);
        const double comp = energy();
        setP (234, 0); setP (215, 0);
        std::printf ("fx energy: dry %.2f  delay %.2f  comp %.2f\n", dry, del, comp);
        fxOk = std::abs (del - dry) > dry * 0.05 && comp > 0.01;
    }

    // 4. a second instance must not take over the running engine
    bool secondOwns;
    {
        LX6Processor second;
        second.prepareToPlay (sr, bs);
        secondOwns = second.ownsEngine();
    }
    std::printf ("second instance owns engine: %d\n", (int) secondOwns);

    // 5. editor snapshots
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (1280, 800);
    auto snap = [&] (const char* path)
    {
        auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (path);
        f.deleteFile();
        juce::FileOutputStream s (f);
        juce::PNGImageFormat().writeImageToStream (im, s);
    };
    auto click = [&] (const juce::String& text)
    {
        std::function<juce::TextButton* (juce::Component*)> find = [&] (juce::Component* c) -> juce::TextButton*
        {
            if (auto* b = dynamic_cast<juce::TextButton*> (c))
                if (b->getButtonText() == text) return b;
            for (auto* k : c->getChildren())
                if (auto* r = find (k)) return r;
            return nullptr;
        };
        if (auto* b = find (ed.get())) { b->onClick(); return true; }
        return false;
    };
    snap ("/tmp/lx6_ui.png");
    if (click ("DEL")) snap ("/tmp/lx6_ui_del.png");
    if (click ("MOD")) snap ("/tmp/lx6_ui_mod.png");
    if (click ("LXR")) snap ("/tmp/lx6_ui_hw.png");
    ed.reset();
    std::printf ("snapshots written\n");

    const bool ok = peak > 0.01f && quietKits == 0 && ccs > 0 && notes > 0 && ! secondOwns && fxOk;
    std::printf (ok ? "ALL OK\n" : "FAILED\n");
    return ok ? 0 : 1;
}
