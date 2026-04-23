#include "GainSmoother.h"
#include "DspUtils.h"
#include <algorithm>

// ── prepare / reset ──────────────────────────────────────────────────────────
void GainSmoother::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;

    smoothedGainDb = 0.0f;
    targetGainDb_  = 0.0f;
    currentGainDb.store(0.0f);

    // Allocate lookahead buffer (max 50 ms)
    const int maxLookahead = (int)std::ceil(sampleRate * 0.050);
    delayBuffer.setSize(numChannels, maxLookahead + maxBlockSize, false, true, false);
    delayWritePos = 0;
}

void GainSmoother::reset()
{
    smoothedGainDb = 0.0f;
    targetGainDb_  = 0.0f;
    currentGainDb.store(0.0f);
    delayBuffer.clear();
    delayWritePos = 0;
}

// ── setLookaheadMs ────────────────────────────────────────────────────────────
void GainSmoother::setLookaheadMs(float ms, bool enabled)
{
    lookaheadEnabled = enabled;
    lookaheadSamples = enabled
        ? (int)std::round(sampleRate_ * (double)ms * 0.001)
        : 0;
}

// ── setTargetGainDb ───────────────────────────────────────────────────────────
void GainSmoother::setTargetGainDb(float dB) noexcept
{
    const float clamped = std::clamp(dB, minGainDb.load(), maxGainDb.load());
    targetGainDb_ = clamped;
}

// ── processBlock ─────────────────────────────────────────────────────────────
void GainSmoother::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min(buffer.getNumChannels(), numChannels_);

    const float attCoeff = DspUtils::msToCoeff(attackMs .load(), sampleRate_);
    const float relCoeff = DspUtils::msToCoeff(releaseMs.load(), sampleRate_);

    if (lookaheadEnabled && lookaheadSamples > 0)
    {
        // ── Lookahead path ────────────────────────────────────────────────────
        // Write incoming audio into delay buffer, read delayed audio out.
        const int bufLen = delayBuffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            // Smooth gain
            const float coeff = (targetGainDb_ > smoothedGainDb) ? attCoeff : relCoeff;
            smoothedGainDb = coeff * smoothedGainDb + (1.0f - coeff) * targetGainDb_;
            const float linGain = std::exp2(smoothedGainDb * 0.16609640474f);

            // Write current sample into delay buffer, read delayed sample
            const int readPos = (delayWritePos - lookaheadSamples + bufLen) % bufLen;

            for (int ch = 0; ch < numCh; ++ch)
            {
                delayBuffer.setSample(ch, delayWritePos, buffer.getSample(ch, i));
                buffer.setSample(ch, i, delayBuffer.getSample(ch, readPos) * linGain);
            }

            delayWritePos = (delayWritePos + 1) % bufLen;
        }
    }
    else
    {
        // ── Direct path ───────────────────────────────────────────────────────
        for (int i = 0; i < numSamples; ++i)
        {
            // Attack when gain needs to increase, release when decreasing
            const float coeff = (targetGainDb_ > smoothedGainDb) ? attCoeff : relCoeff;
            smoothedGainDb = coeff * smoothedGainDb + (1.0f - coeff) * targetGainDb_;
            const float linGain = std::exp2(smoothedGainDb * 0.16609640474f);

            for (int ch = 0; ch < numCh; ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * linGain);
        }
    }

    currentGainDb.store(smoothedGainDb);
}
