#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class AutoGain
{
public:
    AutoGain() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    
    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    void setTargetRmsDb(float targetDb)   { targetRmsDb = targetDb; }
    void setSpeedMs(float speedMs); // Controls how fast the gain rides

    void processBlock(juce::AudioBuffer<float>& buffer);
    
    float getCurrentGainDb() const { return currentGainDb.load(); }

private:
    bool  enabled = true;
    float targetRmsDb = -18.0f;
    std::atomic<float> currentGainDb { 0.0f };
    
    double sampleRate { 44100.0 };
    
    // Low-pass filter to calculate slow RMS
    juce::dsp::FirstOrderTPTFilter<float> rmsFilter;
    
    // Smoothing for the applied gain
    float gainMultiplier { 1.0f };
    float smoothCoeff { 0.001f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoGain)
};
