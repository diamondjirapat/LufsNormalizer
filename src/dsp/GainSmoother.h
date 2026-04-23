#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

/**
 * Smoothed gain compensation stage.
 *
 * Converts a target LUFS error into a linear gain correction and applies it
 * with configurable attack/release smoothing to prevent pumping.
 *
 * Optionally includes a lookahead delay (ring buffer) so gain changes are
 * applied slightly before the transient arrives.
 */
class GainSmoother
{
public:
    GainSmoother() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    /**
     * Set the desired output gain in dB.
     * Called from the audio thread after LUFS measurement.
     */
    void setTargetGainDb(float dB) noexcept;

    /** Apply smoothed gain to buffer in-place. */
    void processBlock(juce::AudioBuffer<float>& buffer);

    // ── Parameters ───────────────────────────────────────────────────────────
    void setAttackMs (float ms)  noexcept { attackMs .store(ms);  }
    void setReleaseMs(float ms)  noexcept { releaseMs.store(ms);  }
    void setMaxGainDb(float dB)  noexcept { maxGainDb.store(dB);  }
    void setMinGainDb(float dB)  noexcept { minGainDb.store(dB);  }

    /** Enable/disable lookahead (requires latency compensation). */
    void setLookaheadMs(float ms, bool enabled);

    /** Returns current applied gain in dB. Thread-safe. */
    float getCurrentGainDb() const noexcept { return currentGainDb.load(); }

    /** Latency introduced by lookahead in samples. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

private:
    double sampleRate_  = 44100.0;
    int    numChannels_ = 2;

    // ── Smoothing state ───────────────────────────────────────────────────────
    float smoothedGainDb = 0.0f;
    float targetGainDb_  = 0.0f;

    std::atomic<float> attackMs  { 200.0f };
    std::atomic<float> releaseMs { 500.0f };
    std::atomic<float> maxGainDb {  24.0f };
    std::atomic<float> minGainDb { -24.0f };

    std::atomic<float> currentGainDb { 0.0f };

    // ── Lookahead delay line ──────────────────────────────────────────────────
    juce::AudioBuffer<float> delayBuffer;
    int  delayWritePos    = 0;
    int  lookaheadSamples = 0;
    bool lookaheadEnabled = false;
};
