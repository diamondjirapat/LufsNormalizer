#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <deque>
#include <atomic>
#include <cstdint>

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
        std::vector<double> energies;
        std::vector<int>    samples;
        double              totalEnergy = 0.0;
        int64_t             totalSamples = 0;
        int                 maxBlocks = 1;
        int                 writeIndex = 0;
        int                 count = 0;

        void allocate(int maxB)
        {
            maxBlocks = maxB;
            energies.assign((size_t)maxB, 0.0);
            samples.assign((size_t)maxB, 0);
            totalEnergy = 0.0;
            totalSamples = 0;
            writeIndex = 0;
            count = 0;
        }

        void setMaxBlocks(int newMax)
        {
            if (newMax == maxBlocks) return;
            if (newMax <= 0 || newMax > (int)energies.size()) return;

            double newEnergy = 0.0;
            int64_t newSampleCount = 0;
            int newCount = std::min(count, newMax);
            int cap = (int)energies.size();

            for (int i = 0; i < newCount; ++i)
            {
                int idx = (writeIndex - 1 - i + cap) % cap;
                newEnergy += energies[(size_t)idx];
                newSampleCount += samples[(size_t)idx];
            }

            totalEnergy = newEnergy;
            totalSamples = newSampleCount;
            count = newCount;
            maxBlocks = newMax;
        }

        void push(double meanSquare, int numSamples)
        {
            if (energies.empty() || maxBlocks == 0) return;
            int cap = (int)energies.size();
            const double energy = meanSquare * (double) numSamples;

            if (count >= maxBlocks)
            {
                int oldestIdx = (writeIndex - maxBlocks + cap) % cap;
                totalEnergy -= energies[(size_t)oldestIdx];
                totalSamples -= samples[(size_t)oldestIdx];
            }
            else
            {
                count++;
            }

            energies[(size_t)writeIndex] = energy;
            samples[(size_t)writeIndex] = numSamples;
            totalEnergy += energy;
            totalSamples += numSamples;
            writeIndex = (writeIndex + 1) % cap;

            if (totalEnergy < 0.0) totalEnergy = 0.0;
            if (totalSamples < 0) totalSamples = 0;
        }

        double meanSquare() const
        {
            return totalSamples == 0 ? 0.0 : totalEnergy / (double) totalSamples;
        }
    };

    Window momentaryWindow;   // 400 ms
    Window shortTermWindow;   // 3 s

    // ── Integrated loudness (gated) ──────────────────────────────────────────
    // Pass 1: absolute gate  -70 LUFS
    // Pass 2: relative gate  -10 LU below ungated mean
    static constexpr float HISTOGRAM_MIN_LUFS = -70.0f;
    static constexpr size_t HISTOGRAM_BINS = 900; // -70 to +20 LUFS in 0.1 steps
    std::array<int, HISTOGRAM_BINS> histogram;
    double                  gatedBlockAccum = 0.0;
    int                     gatedBlockSamples = 0;
    int                     gatedBlockSize    = 0;   // samples per 100 ms

    // ── State ────────────────────────────────────────────────────────────────
    double sampleRate_  = 48000.0;
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
