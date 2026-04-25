#include "GainSmoother.h"
#include "DspUtils.h"
#include <algorithm>
#include <vector>

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
    const int maxSamples = std::max(0, delayBuffer.getNumSamples() - 1);
    const int requestedSamples = enabled
        ? (int)std::round(sampleRate_ * (double)std::max(0.0f, ms) * 0.001)
        : 0;
    lookaheadSamples = std::clamp(requestedSamples, 0, maxSamples);
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

    if (numSamples == 0 || numCh == 0)
    {
        currentGainDb.store(smoothedGainDb);
        return;
    }

    const float attCoeff = DspUtils::msToCoeff(attackMs .load(), sampleRate_);
    const float relCoeff = DspUtils::msToCoeff(releaseMs.load(), sampleRate_);
    std::vector<float*> writePointers((size_t) numCh);
    std::vector<float*> delayPointers((size_t) numCh);

    for (int ch = 0; ch < numCh; ++ch)
    {
        writePointers[(size_t) ch] = buffer.getWritePointer(ch);
        delayPointers[(size_t) ch] = delayBuffer.getWritePointer(ch);
    }

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
                const float input = writePointers[(size_t) ch][i];
                delayPointers[(size_t) ch][delayWritePos] = input;
                writePointers[(size_t) ch][i] = delayPointers[(size_t) ch][readPos] * linGain;
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
                writePointers[(size_t) ch][i] *= linGain;
        }
    }

    currentGainDb.store(smoothedGainDb);
}
