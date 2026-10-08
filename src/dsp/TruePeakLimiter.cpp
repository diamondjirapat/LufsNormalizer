#include "TruePeakLimiter.h"
#include "DspUtils.h"
#include <cmath>
#include <algorithm>

void TruePeakLimiter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    maximumBlockSize = std::max(1, maxBlockSize);
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;
    perSamplePeak.assign((size_t)maximumBlockSize, 0.0f);

    // Recreate oversampling with the correct channel count
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t)numChannels,
        2,   // 2^2 = 4x
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true);

    oversampling->initProcessing((size_t)maximumBlockSize);
    oversampling->reset();

    // 1 ms lookahead
    // At low sample rates, the FIR detector can take longer than 1 ms. Keep
    // enough delay for its reconstructed peaks to arrive before the audio.
    lookaheadSamples = std::max((int)std::round(sampleRate * 0.001),
                               (int)std::ceil(oversampling->getLatencyInSamples()));
    const int bufLen = lookaheadSamples + maxBlockSize + 16;
    lookaheadBuffer.setSize(numChannels, bufLen, false, true, false);
    lookaheadBuffer.clear();
    peakQueue.resize((size_t)lookaheadSamples + 2);
    peakHead = peakCount = 0;
    sampleIndex = 0;
    writePos    = 0;
    smoothedGain = 1.0f;
    gainReductionDb.store(0.0f);
}

void TruePeakLimiter::reset()
{
    if (oversampling) oversampling->reset();
    lookaheadBuffer.clear();
    peakHead = peakCount = 0;
    sampleIndex = 0;
    writePos     = 0;
    smoothedGain = 1.0f;
    gainReductionDb.store(0.0f);
}

void TruePeakLimiter::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!oversampling) return;
    for (int start = 0; start < buffer.getNumSamples(); start += maximumBlockSize)
    {
        const int length = std::min(maximumBlockSize, buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(),
                                      buffer.getNumChannels(), start, length);
        processChunk(chunk);
    }
}

void TruePeakLimiter::processChunk(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = std::min(buffer.getNumChannels(), numChannels_);
    if (numSamples == 0 || numCh == 0) return;
    const bool limiting = enabled.load();
    const float ceiling = DspUtils::dbToGain(ceilingDb.load());
    // Reserve a small margin for reconstruction between output samples.
    const float detectionCeiling = ceiling * DspUtils::dbToGain(-0.5f);
    const float relCoeff = DspUtils::msToCoeff(50.0f, sampleRate_);

    juce::dsp::AudioBlock<float> block(buffer);
    auto upBlock = oversampling->processSamplesUp(block);
    std::fill(perSamplePeak.begin(), perSamplePeak.begin() + numSamples, 0.0f);
    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* up = upBlock.getChannelPointer((size_t)ch);
        const float* input = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float peak = std::abs(input[i]);
            for (int j = 0; j < kOversamplingFactor; ++j)
                peak = std::max(peak, std::abs(up[i * kOversamplingFactor + j]));
            perSamplePeak[(size_t)i] = std::max(perSamplePeak[(size_t)i], peak);
        }
    }

    const int bufLen = lookaheadBuffer.getNumSamples();
    float minGain = 1.0f;
    for (int i = 0; i < numSamples; ++i, ++sampleIndex)
    {
        // Monotonic queue: retain the largest peak until its delayed audio has
        // actually left the limiter. Instant attack prevents isolated overshoots.
        while (peakCount > 0 && sampleIndex > lookaheadSamples
               && peakQueue[peakHead].sample < sampleIndex - (uint64_t)lookaheadSamples)
        {
            peakHead = (peakHead + 1) % peakQueue.size();
            --peakCount;
        }
        const float peak = perSamplePeak[(size_t)i];
        while (peakCount > 0
               && peakQueue[(peakHead + peakCount - 1) % peakQueue.size()].value <= peak)
            --peakCount;
        peakQueue[(peakHead + peakCount) % peakQueue.size()] = { sampleIndex, peak };
        ++peakCount;
        const float maxPeak = peakQueue[peakHead].value;
        const float target = maxPeak > detectionCeiling ? detectionCeiling / maxPeak : 1.0f;
        smoothedGain = limiting ? std::min(target, relCoeff * smoothedGain + (1.0f - relCoeff)) : 1.0f;
        minGain = std::min(minGain, smoothedGain);

        const int readPos = (writePos - lookaheadSamples + bufLen) % bufLen;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* delay = lookaheadBuffer.getWritePointer(ch);
            float* audio = buffer.getWritePointer(ch);
            delay[writePos] = audio[i];
            audio[i] = delay[readPos] * smoothedGain;
        }
        writePos = (writePos + 1) % bufLen;
    }
    gainReductionDb.store(DspUtils::gainToDb(minGain));
}
