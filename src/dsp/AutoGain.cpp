#include "AutoGain.h"
#include <cmath>
#include <algorithm>

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

    // Get raw channel pointers for direct access (avoids per-sample bounds checks)
    jassert((int)channelPointers.size() >= numChannels);
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
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float sample = channelPointers[(size_t)ch][i];
            sumSq += sample * sample;
        }
        float meanSq = sumSq / (float)numChannels;

        // 2. Filter the mean squared value to get slow changing envelope
        float smoothedSq = rmsFilter.processSample(0, meanSq); // channel 0
        smoothedSq = std::max(smoothedSq, 1e-10f); // prevent log(0)

        // 3. Calculate required multiplier to hit target
        float requiredMultiplier = 1.0f;
        
        // If the signal is above the gate threshold, calculate gain
        if (smoothedSq > gateThresholdSq)
        {
            requiredMultiplier = targetRmsLinear / std::sqrt(smoothedSq);
            // Clamp the required multiplier to prevent extreme behavior (e.g., max +/- 12 dB)
            requiredMultiplier = std::clamp(requiredMultiplier, minGainMult, maxGainMult);
        }

        // 4. Smooth the gain multiplier
        gainMultiplier += smoothCoeff * (requiredMultiplier - gainMultiplier);

        // 5. Apply to signal
        for (int ch = 0; ch < numChannels; ++ch)
            channelPointers[(size_t)ch][i] *= gainMultiplier;
    }
    
    // Store current applied gain in dB for the UI
    currentGainDb.store(20.0f * std::log10(std::max(gainMultiplier, 1e-5f)));
}
