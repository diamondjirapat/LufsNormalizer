#include "TruePeakLimiter.h"
#include "DspUtils.h"
#include <cmath>
#include <algorithm>

void TruePeakLimiter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;
    perSamplePeak.assign((size_t)maxBlockSize, 0.0f);

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
    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min(buffer.getNumChannels(), numChannels_);
    if (!enabled.load() || !oversampling)
    {
        smoothedGain = 1.0f;
        gainReductionDb.store(0.0f);
        return;
    }

    if (numSamples == 0 || numCh == 0)
    {
        gainReductionDb.store(0.0f);
        return;
    }

    const float ceiling  = DspUtils::dbToGain(ceilingDb.load());
    if (numSamples > (int)perSamplePeak.size())
        perSamplePeak.resize((size_t) numSamples, 0.0f);

    // ── Store original input to lookahead buffer ──────────────────────────────
    const int bufLen = lookaheadBuffer.getNumSamples();
    std::vector<float*> inputPointers((size_t) numCh);
    std::vector<float*> lookaheadPointers((size_t) numCh);
    for (int ch = 0; ch < numCh; ++ch)
    {
        inputPointers[(size_t) ch] = buffer.getWritePointer(ch);
        lookaheadPointers[(size_t) ch] = lookaheadBuffer.getWritePointer(ch);
    }
    int tempWritePos = writePos;
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
            lookaheadPointers[(size_t) ch][tempWritePos] = inputPointers[(size_t) ch][i];
        tempWritePos = (tempWritePos + 1) % bufLen;
    }

    // ── Upsample to detect true peaks per-sample ──────────────────────────────
    juce::dsp::AudioBlock<float> fullBlock(buffer);
    juce::dsp::AudioBlock<float> activeBlock = fullBlock.getSubBlock(0, (size_t)numSamples);
    auto upBlock = oversampling->processSamplesUp(activeBlock);

    // Build a per-original-sample peak table from the oversampled signal
    // Each original sample corresponds to kOversamplingFactor upsampled samples
    std::fill(perSamplePeak.begin(), perSamplePeak.begin() + numSamples, 0.0f);

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* ptr = upBlock.getChannelPointer((size_t)ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float peak = 0.0f;
            const float* subPtr = ptr + (i * kOversamplingFactor);
            for (int j = 0; j < kOversamplingFactor; ++j)
                peak = std::max(peak, std::abs(subPtr[j]));

            perSamplePeak[(size_t)i] = std::max(perSamplePeak[(size_t)i], peak);
        }
    }

    // ── Smooth gain per-sample (fast attack ~0.1 ms, slow release ~50 ms) ────
    const float attCoeff = std::exp(-1.0f / (float)(0.0001 * sampleRate_));
    const float relCoeff = std::exp(-1.0f / (float)(0.050  * sampleRate_));

    // Flush oversampling state. This will overwrite `buffer`, which is fine
    // because we have the original samples in the lookahead buffer.
    oversampling->processSamplesDown(activeBlock);

    // ── Apply per-sample smoothed gain via lookahead buffer ───────────────────
    float minGainThisBlock = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Compute target gain for this sample
        float targetGain = 1.0f;
        if (perSamplePeak[(size_t)i] > ceiling && perSamplePeak[(size_t)i] > 1e-10f)
            targetGain = ceiling / perSamplePeak[(size_t)i];

        // Smooth: fast attack, slow release
        const float coeff = (targetGain < smoothedGain) ? attCoeff : relCoeff;
        smoothedGain = coeff * smoothedGain + (1.0f - coeff) * targetGain;
        smoothedGain = std::min(smoothedGain, 1.0f); // never amplify

        minGainThisBlock = std::min(minGainThisBlock, smoothedGain);

        const int readPos = (writePos - lookaheadSamples + bufLen) % bufLen;

        for (int ch = 0; ch < numCh; ++ch)
        {
            inputPointers[(size_t) ch][i] = lookaheadPointers[(size_t) ch][readPos] * smoothedGain;
        }

        writePos = (writePos + 1) % bufLen;
    }

    // Publish peak gain reduction for this block
    const float grDb = DspUtils::gainToDb(std::max(minGainThisBlock, DspUtils::kMinLinear));
    gainReductionDb.store(grDb);
}
