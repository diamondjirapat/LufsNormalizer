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
    const int safeChannels = std::min(numChannels, (int)channelPointers.size());
    for (int ch = 0; ch < safeChannels; ++ch)
        channelPointers[(size_t)ch] = buffer.getWritePointer(ch);

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
        float currentRmsDb = 10.0f * std::log10(smoothedSq);

        // 4. Calculate required gain to hit target
        float requiredGainDb = 0.0f;
        
        // If the signal is below the gate threshold, return gain to 0 dB
        if (currentRmsDb > gateThresholdDb)
        {
            requiredGainDb = targetRmsDb - currentRmsDb;
            // Clamp the required gain to prevent extreme behavior (e.g., max +/- 12 dB)
            requiredGainDb = std::clamp(requiredGainDb, -12.0f, 12.0f);
        }

        // 5. Smooth the gain multiplier
        float requiredMultiplier = std::pow(10.0f, requiredGainDb / 20.0f);
        gainMultiplier += smoothCoeff * (requiredMultiplier - gainMultiplier);

        // 6. Apply to signal
        for (int ch = 0; ch < safeChannels; ++ch)
            channelPointers[(size_t)ch][i] *= gainMultiplier;
    }
    
    // Store current applied gain in dB for the UI
    currentGainDb.store(20.0f * std::log10(std::max(gainMultiplier, 1e-5f)));
}
