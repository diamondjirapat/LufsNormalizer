#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <deque>
#include <atomic>

/**
 * EBU R128 / ITU-R BS.1770-4 loudness meter.
 *
 * Implements:
 *   - K-weighting filter (pre-filter + RLB weighting)
 *   - Momentary loudness  (400 ms sliding window)
 *   - Short-term loudness (3 s  sliding window)
 *   - Integrated loudness (gated, full programme)
 *
 * All public getters are safe to call from any thread (atomic reads).
 * processBlock() must be called from the audio thread only.
 */
class LufsMeter
{
public:
    LufsMeter() = default;

    /** Call once before playback starts. */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Reset all accumulators. */
    void reset();

    /** Process a block of audio. Channels beyond the prepared count are ignored. */
    void processBlock(const juce::AudioBuffer<float>& buffer);

    // ── Results (thread-safe reads) ──────────────────────────────────────────
    float getMomentaryLUFS()  const noexcept { return momentaryLUFS.load();  }
    float getShortTermLUFS()  const noexcept { return shortTermLUFS.load();  }
    float getIntegratedLUFS() const noexcept { return integratedLUFS.load(); }

private:
    // ── K-weighting filters (one set per channel) ────────────────────────────
    // Stage 1: high-shelf pre-filter
    // Stage 2: high-pass RLB weighting filter
    struct ChannelFilters
    {
        juce::dsp::IIR::Filter<float> preFilter;
        juce::dsp::IIR::Filter<float> rlbFilter;
    };
    std::vector<ChannelFilters> filters;

    // ── Sliding-window mean-square accumulators ──────────────────────────────
    // We store per-block mean-square values in a ring buffer and sum them.
    struct Window
    {
        std::deque<double> blocks;   // mean-square per block
        double             sum  = 0.0;
        int                maxBlocks = 1;

        void push(double ms)
        {
            blocks.push_back(ms);
            sum += ms;
            while ((int)blocks.size() > maxBlocks)
            {
                sum -= blocks.front();
                blocks.pop_front();
            }
            if (sum < 0.0) sum = 0.0; // guard floating-point drift
        }

        double meanSquare() const
        {
            return blocks.empty() ? 0.0 : sum / (double)blocks.size();
        }
    };

    Window momentaryWindow;   // 400 ms
    Window shortTermWindow;   // 3 s

    // ── Integrated loudness (gated) ──────────────────────────────────────────
    // Pass 1: absolute gate  -70 LUFS
    // Pass 2: relative gate  -10 LU below ungated mean
    struct GatedBlock
    {
        double meanSquare = 0.0;
    };
    std::vector<GatedBlock> gatedBlocks;   // 100 ms blocks
    double                  gatedBlockAccum = 0.0;
    int                     gatedBlockSamples = 0;
    int                     gatedBlockSize    = 0;   // samples per 100 ms

    // ── State ────────────────────────────────────────────────────────────────
    double sampleRate_  = 44100.0;
    int    numChannels_ = 2;

    // ── Atomic results ───────────────────────────────────────────────────────
    std::atomic<float> momentaryLUFS  { -144.0f };
    std::atomic<float> shortTermLUFS  { -144.0f };
    std::atomic<float> integratedLUFS { -144.0f };

    // ── Helpers ──────────────────────────────────────────────────────────────
    static float msToLUFS(double ms) noexcept;
    void         updateIntegrated();

    static juce::dsp::IIR::Coefficients<float>::Ptr
        makePreFilterCoeffs(double fs);

    static juce::dsp::IIR::Coefficients<float>::Ptr
        makeRLBFilterCoeffs(double fs);
};
