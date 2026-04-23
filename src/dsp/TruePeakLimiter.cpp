#include "TruePeakLimiter.h"
#include <cmath>
#include <algorithm>

void TruePeakLimiter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;

    // Recreate oversampling with the correct channel count
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t)numChannels,
        2,   // 2^2 = 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    oversampling->initProcessing((size_t)maxBlockSize);
    oversampling->reset();

    // 1 ms lookahead
    lookaheadSamples = (int)std::round(sampleRate * 0.001);
    const int bufLen = lookaheadSamples + maxBlockSize + 16;
    lookaheadBuffer.setSize(numChannels, bufLen, false, true, false);
    writePos    = 0;
    smoothedGain = 1.0f;
    gainReductionDb.store(0.0f);
}

void TruePeakLimiter::reset()
{
    if (oversampling) oversampling->reset();
    lookaheadBuffer.clear();
    writePos     = 0;
    smoothedGain = 1.0f;
    gainReductionDb.store(0.0f);
}

void TruePeakLimiter::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!enabled.load() || !oversampling) return;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min(buffer.getNumChannels(), numChannels_);
    const float ceiling  = std::pow(10.0f, ceilingDb.load() / 20.0f);

    // ── Upsample to detect true peaks ────────────────────────────────────────
    juce::dsp::AudioBlock<float> block(buffer);
    auto upBlock = oversampling->processSamplesUp(block);

    const int upSamples = (int)upBlock.getNumSamples();
    float peakLinear = 0.0f;

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* ptr = upBlock.getChannelPointer((size_t)ch);
        for (int i = 0; i < upSamples; ++i)
            peakLinear = std::max(peakLinear, std::abs(ptr[i]));
    }

    // ── Compute required gain reduction ──────────────────────────────────────
    float targetGain = 1.0f;
    if (peakLinear > ceiling && peakLinear > 1e-10f)
        targetGain = ceiling / peakLinear;

    // ── Smooth gain (fast attack ~0.1 ms, slow release ~50 ms) ───────────────
    const float attCoeff = std::exp(-1.0f / (float)(0.0001 * sampleRate_));
    const float relCoeff = std::exp(-1.0f / (float)(0.050  * sampleRate_));

    const float coeff = (targetGain < smoothedGain) ? attCoeff : relCoeff;
    smoothedGain = coeff * smoothedGain + (1.0f - coeff) * targetGain;
    smoothedGain = std::min(smoothedGain, 1.0f); // never amplify

    // ── Apply gain via lookahead buffer ───────────────────────────────────────
    const int bufLen = lookaheadBuffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const int readPos = (writePos - lookaheadSamples + bufLen) % bufLen;

        for (int ch = 0; ch < numCh; ++ch)
        {
            lookaheadBuffer.setSample(ch, writePos, buffer.getSample(ch, i));
            buffer.setSample(ch, i, lookaheadBuffer.getSample(ch, readPos) * smoothedGain);
        }

        writePos = (writePos + 1) % bufLen;
    }

    // Publish gain reduction
    const float grDb = (smoothedGain > 1e-10f)
                       ? 20.0f * std::log10(smoothedGain)
                       : -144.0f;
    gainReductionDb.store(grDb);
}
