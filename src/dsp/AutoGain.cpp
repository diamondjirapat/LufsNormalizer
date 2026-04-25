#include "AutoGain.h"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {

// Fast log10 approximation using IEEE-754 bit manipulation
// Max error is ~0.17 dB
inline float fast_log10(float x) {
    uint32_t vx;
    std::memcpy(&vx, &x, sizeof(x));
    float y = (float)vx;
    y *= 1.1920928955078125e-7f; // 1 / (1 << 23)
    y -= 126.94269504f;
    return y * 0.30102999566f; // log10(2)
}

// Fast pow10 approximation using IEEE-754 bit manipulation
// Max relative error is ~3.8%
inline float fast_pow10(float x) {
    float y = x * 3.321928094887362f; // x * log2(10)
    uint32_t vi = (uint32_t)((1 << 23) * (y + 126.94269504f));
    float v;
    std::memcpy(&v, &vi, sizeof(vi));
    return v;
}

} // namespace

void AutoGain::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    channelPointers.resize((size_t)numChannels);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };

    // Very low frequency cutoff for RMS smoothing (e.g., 2 Hz)
    rmsFilter.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    rmsFilter.setCutoffFrequency(2.0f);
    rmsFilter.prepare(spec);

    setSpeedMs(500.0f); // default speed
    reset();
}

void AutoGain::reset()
{
    rmsFilter.reset();
    currentGainDb.store(0.0f);
    gainMultiplier = 1.0f;
}

void AutoGain::setSpeedMs(float speedMs)
{
    // Convert ms to a 1-pole smoothing coefficient
    smoothCoeff = 1.0f - std::exp(-1.0f / (speedMs * 0.001f * (float)sampleRate));
}

void AutoGain::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!enabled)
    {
        currentGainDb.store(0.0f);
        return;
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Get raw channel pointers for direct access (avoids per-sample bounds checks)
    if ((int)channelPointers.size() < numChannels) channelPointers.resize((size_t)numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        channelPointers[(size_t)ch] = buffer.getWritePointer(ch);

    // Precalculate loop constants for performance
    const float targetRmsLinear = std::pow(10.0f, targetRmsDb / 20.0f);
    const float gateThresholdSq = std::pow(10.0f, gateThresholdDb / 10.0f);
    const float maxGainMult = std::pow(10.0f, 12.0f / 20.0f);
    const float minGainMult = std::pow(10.0f, -12.0f / 20.0f);

    // Process sample by sample
    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Calculate squared sum for RMS
        float sumSq = 0.0f;
        for (int ch = 0; ch < safeChannels; ++ch)
        {
            const float sample = channelPointers[(size_t)ch][i];
            sumSq += sample * sample;
        }
        float meanSq = safeChannels > 0 ? sumSq / (float)safeChannels : 0.0f;

        // 2. Filter the mean squared value to get slow changing envelope
        float smoothedSq = rmsFilter.processSample(0, meanSq); // channel 0
        smoothedSq = std::max(smoothedSq, 1e-10f); // prevent log(0)

        // 3. Convert to dB
        float currentRmsDb = 10.0f * fast_log10(smoothedSq);

        // 4. Calculate required gain to hit target
        float requiredGainDb = 0.0f;

        // If the signal is below the gate threshold, return gain to 0 dB
        if (currentRmsDb > gateThresholdDb)
        {
            requiredMultiplier = targetRmsLinear / std::sqrt(smoothedSq);
            // Clamp the required multiplier to prevent extreme behavior (e.g., max +/- 12 dB)
            requiredMultiplier = std::clamp(requiredMultiplier, minGainMult, maxGainMult);
        }

        // 5. Smooth the gain multiplier
        float requiredMultiplier = fast_pow10(requiredGainDb / 20.0f);
        gainMultiplier += smoothCoeff * (requiredMultiplier - gainMultiplier);

        // 6. Apply to signal
        for (int ch = 0; ch < safeChannels; ++ch)
            channelPointers[(size_t)ch][i] *= gainMultiplier;
    }

    // Store current applied gain in dB for the UI
    currentGainDb.store(20.0f * std::log10(std::max(gainMultiplier, 1e-5f)));
}
