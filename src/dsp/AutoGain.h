#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

class AutoGain
{
public:
    AutoGain() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    
    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    void setTargetRmsDb(float targetDb);
    void setSpeedMs(float speedMs); // Kept for backward compat – sets both attack & release (MayB)
    void setAttackMs(float ms);
    void setReleaseMs(float ms);
    void setMaxGainDb(float db);
    void setGateThresholdDb(float threshDb) { gateThresholdDb = threshDb; }
    void setAllowReduce(bool allow);

    void processBlock(juce::AudioBuffer<float>& buffer);
    
    float getCurrentGainDb() const { return currentGainDb.load(); }

private:
    bool  enabled = true;
    float targetRmsDb = -18.0f;
    std::atomic<float> currentGainDb { 0.0f };
    float gateThresholdDb = -60.0f;
    float maxGainDb = 12.0f;
    bool  allowReduce = false;

    // Cached linear values to avoid redundant pow calls
    float targetRmsLinear = 0.125892541f; // 10^(-18/20)
    float maxGainMult = 3.981071706f;      // 10^(12/20)
    float minGainMult = 1.0f;              // allowReduce is false by default

    double sampleRate { 44100.0 };
    
    // Low-pass filter to calculate slow RMS
    juce::dsp::FirstOrderTPTFilter<float> rmsFilter;
    
    // Smoothing for the applied gain
    float gainMultiplier { 1.0f };
    float attackCoeff  { 0.001f };
    float releaseCoeff { 0.001f };
    std::vector<float*> channelPointers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoGain)
};
