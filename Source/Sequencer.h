// LX-6 - 16-step pattern sequencer (7 tracks x 8 patterns), host-synced.
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <array>

namespace lx6
{
class Sequencer
{
public:
    static constexpr int numTracks = 7;
    static constexpr int numSteps = 16;
    static constexpr int numPatterns = 8;

    struct Step { bool on = false; int velocity = 100; int probability = 100; };

    Sequencer();

    // --- editing (any thread)
    Step getStep (int pattern, int track, int step) const;
    void setStep (int pattern, int track, int step, Step s);
    void toggleStep (int pattern, int track, int step);
    void clearTrack (int pattern, int track);
    void euclid (int pattern, int track, int pulses, int rotation, int velocity = 100);
    void randomise (int pattern, int track, float density, juce::Random& rng); // track -1 = all
    void copyPattern (int from, int to);

    int getEditPattern() const          { return editPattern.load(); }
    void setEditPattern (int p)         { editPattern = juce::jlimit (0, numPatterns - 1, p); }
    int getPlayingPattern() const       { return playingPattern.load(); }
    int getQueuedPattern() const        { return queuedPattern.load(); }
    void queuePattern (int p);          // switches at the end of the bar when running
    int getCurrentStep() const          { return currentStep.load(); }
    bool isRunning() const              { return running.load(); }

    // --- playback (audio thread)
    struct Trigger { int sampleOffset; int track; int velocity; };

    // advance over a block. ppqStart = musical position at block start, ppqPerSample = tempo
    // ppqStart < 0 means "not playing". Triggers are appended to 'out'.
    void process (double ppqStart, double ppqPerSample, int numSamples, float swing,
                  juce::Array<Trigger>& out);
    void stop();

    // --- state
    juce::String toString() const;
    void fromString (const juce::String& s);

private:
    // packed step: bit 0 on, bits 1-7 velocity, bits 8-14 probability
    std::array<std::atomic<uint32_t>, numPatterns * numTracks * numSteps> data;
    static int index (int p, int t, int s) { return (p * numTracks + t) * numSteps + s; }
    static uint32_t pack (Step s);
    static Step unpack (uint32_t v);

    std::atomic<int> editPattern { 0 }, playingPattern { 0 }, queuedPattern { -1 }, currentStep { -1 };
    std::atomic<bool> running { false };
    juce::int64 lastSixteenth = -1;
    juce::Random rng;
};
}
