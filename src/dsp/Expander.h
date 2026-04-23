#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * Downward expander with soft knee.
 *
 * Signal flow (per sample):
 *   1. Level detection (RMS via 1-pole IIR)
 *   2. Gain computer (threshold / ratio / knee)
 *   3. Ballistics (attack / release smoothing)
 *   4. Apply gain
 *
 * All parameter setters are real-time safe (atomic stores).
 * processBlock() must be called from the audio thread.
 */
class Expander
{
public:
    Expander() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    /** Process in-place. */
    void processBlock(juce::AudioBuffer<float>& buffer);

    // ── Parameter setters (real-time safe) ───────────────────────────────────
    void setThresholdDb(float dB)    noexcept { thresholdDb.store(dB);    }
    void setRatio(float r)           noexcept { ratio.store(r);           }
    void setAttackMs(float ms)       noexcept { attackMs.store(ms);       }
    void setReleaseMs(float ms)      noexcept { releaseMs.store(ms);      }
    void setKneeDb(float dB)         noexcept { kneeDb.store(dB);         }
    void setEnabled(bool e)          noexcept { enabled.store(e);         }

    /** Returns the current gain reduction in dB (≤ 0). Thread-safe read. */
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    double sampleRate_  = 44100.0;
    int    numChannels_ = 2;

    // ── Parameters (atomic for thread-safe writes from message thread) ───────
    std::atomic<float> thresholdDb { -40.0f };
    std::atomic<float> ratio       {   2.0f };
    std::atomic<float> attackMs    {  10.0f };
    std::atomic<float> releaseMs   { 100.0f };
    std::atomic<float> kneeDb      {   6.0f };
    std::atomic<bool>  enabled     {  true  };

    // ── State ────────────────────────────────────────────────────────────────
    // Per-channel RMS detector state (1-pole IIR)
    std::vector<float> rmsState;

    // Smoothed gain (single value – linked stereo)
    float smoothedGainDb = 0.0f;

    // ── Published result ─────────────────────────────────────────────────────
    std::atomic<float> gainReductionDb { 0.0f };

    // ── Helpers ──────────────────────────────────────────────────────────────
    /** Gain computer: returns gain in dB to apply for a given input level. */
    float computeGainDb(float inputDb, float thresh, float rat, float knee) const noexcept;
};
