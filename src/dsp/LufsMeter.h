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
    LufsMeter();

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
        std::vector<double> blocks;
        double              sum = 0.0;
        int                 maxBlocks = 1;
        int                 writeIndex = 0;
        int                 count = 0;

        void allocate(int maxB)
        {
            maxBlocks = maxB;
            blocks.assign((size_t)maxB, 0.0);
            sum = 0.0;
            writeIndex = 0;
            count = 0;
        }

        void push(double ms)
        {
            if (blocks.empty() || maxBlocks == 0) return;

            if (count >= maxBlocks)
                sum -= blocks[(size_t)writeIndex];
            else
                count++;

            blocks[(size_t)writeIndex] = ms;
            sum += ms;
            writeIndex = (writeIndex + 1) % maxBlocks;

            if (sum < 0.0) sum = 0.0; // guard floating-point drift
        }

        double meanSquare() const
        {
            return count == 0 ? 0.0 : sum / (double)count;
        }
    };

    Window momentaryWindow;   // 400 ms
    Window shortTermWindow;   // 3 s

    // ── Integrated loudness (gated) ──────────────────────────────────────────
    // Pass 1: absolute gate  -70 LUFS
    // Pass 2: relative gate  -10 LU below ungated mean
    static constexpr float HISTOGRAM_MIN_LUFS = -70.0f;
    static constexpr size_t HISTOGRAM_BINS = 900; // -70 to +20 LUFS in 0.1 steps
    std::array<int, HISTOGRAM_BINS> histogram{};
    std::array<double, HISTOGRAM_BINS> binToMsLookup{};
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
