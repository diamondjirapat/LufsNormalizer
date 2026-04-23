#include "Expander.h"
#include <cmath>
#include <algorithm>

// ── prepare / reset ──────────────────────────────────────────────────────────
void Expander::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;
    rmsState.assign((size_t)numChannels, 0.0f);
    smoothedGainDb = 0.0f;
    gainReductionDb.store(0.0f);
}

void Expander::reset()
{
    std::fill(rmsState.begin(), rmsState.end(), 0.0f);
    smoothedGainDb = 0.0f;
    gainReductionDb.store(0.0f);
}

// ── processBlock ─────────────────────────────────────────────────────────────
void Expander::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!enabled.load()) return;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min(buffer.getNumChannels(), numChannels_);

    // Snapshot parameters (avoid repeated atomic loads in inner loop)
    const float thresh  = thresholdDb.load();
    const float rat     = ratio.load();
    const float knee    = kneeDb.load();
    const float attMs   = attackMs.load();
    const float relMs   = releaseMs.load();

    // 1-pole RMS detector time constant (~10 ms window)
    const float rmsCoeff    = msToCoeff(10.0f, sampleRate_);
    const float attCoeff    = msToCoeff(attMs,  sampleRate_);
    const float relCoeff    = msToCoeff(relMs,  sampleRate_);

    float maxGrDb = 0.0f; // track peak gain reduction this block

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Level detection: max RMS across channels ─────────────────────────
        float maxRms = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float s = buffer.getSample(ch, i);
            // 1-pole IIR on squared signal → RMS
            rmsState[(size_t)ch] = rmsCoeff * rmsState[(size_t)ch]
                                 + (1.0f - rmsCoeff) * s * s;
            const float rms = std::sqrt(std::max(0.0f, rmsState[(size_t)ch]));
            maxRms = std::max(maxRms, rms);
        }

        // ── Gain computer ────────────────────────────────────────────────────
        const float inputDb  = (maxRms > 1e-10f)
                               ? 20.0f * std::log10(maxRms)
                               : -144.0f;
        const float targetGainDb = computeGainDb(inputDb, thresh, rat, knee);

        // ── Ballistics ───────────────────────────────────────────────────────
        // Expander: gain goes DOWN when signal is quiet.
        // Attack  = how fast gain drops  (signal falls below threshold)
        // Release = how fast gain recovers (signal rises above threshold)
        const float coeff = (targetGainDb < smoothedGainDb) ? attCoeff : relCoeff;
        smoothedGainDb = coeff * smoothedGainDb + (1.0f - coeff) * targetGainDb;

        // Track peak gain reduction
        if (smoothedGainDb < maxGrDb) maxGrDb = smoothedGainDb;

        // ── Apply gain ───────────────────────────────────────────────────────
        const float linGain = std::pow(10.0f, smoothedGainDb / 20.0f);
        for (int ch = 0; ch < numCh; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * linGain);
    }

    gainReductionDb.store(maxGrDb);
}

// ── computeGainDb ─────────────────────────────────────────────────────────────
// Downward expander gain computer with soft knee.
// Below (threshold - knee/2): full expansion applied.
// Within knee region:         smooth transition.
// Above threshold:            unity gain (0 dB).
float Expander::computeGainDb(float inputDb, float thresh,
                               float rat, float knee) const noexcept
{
    const float halfKnee = knee * 0.5f;
    const float lower    = thresh - halfKnee;
    const float upper    = thresh + halfKnee;

    if (inputDb >= upper)
    {
        // Above threshold – no expansion
        return 0.0f;
    }
    else if (inputDb <= lower)
    {
        // Below knee – full expansion
        // Expander gain = (1/ratio - 1) * (threshold - inputDb)
        // For ratio > 1 this is negative (gain reduction).
        return (1.0f / rat - 1.0f) * (thresh - inputDb);
    }
    else
    {
        // Soft knee region – interpolate
        const float x = (inputDb - lower) / knee; // 0..1
        // Smooth step: 3x^2 - 2x^3
        const float t = x * x * (3.0f - 2.0f * x);
        // Blend between full expansion and unity
        const float fullExpansion = (1.0f / rat - 1.0f) * halfKnee;
        return fullExpansion * (1.0f - t);
    }
}

// ── msToCoeff ────────────────────────────────────────────────────────────────
float Expander::msToCoeff(float ms, double sr) noexcept
{
    if (ms <= 0.0f) return 0.0f;
    // τ = ms / 1000, coeff = exp(-1 / (τ * sr))
    return std::exp(-1.0f / (float)(ms * 0.001 * sr));
}
