#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class Compressor
{
public:
    Compressor() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate_ = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)maxBlockSize, (juce::uint32)numChannels };
        compressor.prepare(spec);
        reset();
    }

    void reset()
    {
        compressor.reset();
        gainReductionDb.store(0.0f);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled.load())
        {
            gainReductionDb.store(0.0f);
            return;
        }

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        // Update parameters from atomics
        float t = thresholdDb.load();
        float r = ratio.load();
        compressor.setThreshold(t);
        compressor.setRatio(r);
        compressor.setAttack(attackMs.load());
        compressor.setRelease(releaseMs.load());

        // Measure input RMS/peak for GR calculation
        float preMax = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto mag = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
            if (mag > preMax) preMax = mag;
        }

        compressor.process(context);

        // Apply makeup gain
        float totalMakeupDb = makeupDb.load();
        if (autoMakeup.load() && t < 0.0f && r > 1.0f)
        {
            totalMakeupDb += -t * (1.0f - 1.0f / r);
        }

        const float makeupLinear = juce::Decibels::decibelsToGain(totalMakeupDb);
        buffer.applyGain(makeupLinear);

        // Calculate a rough GR for visualization
        float postMax = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto mag = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
            if (mag > postMax) postMax = mag;
        }

        // Simple GR calculation based on peak magnitudes
        float currentGrDb = 0.0f;
        if (preMax > 1e-5f && postMax > 1e-5f)
        {
            // Divide out the makeup gain to find the compressor's true attenuation
            float attenuation = (postMax / makeupLinear) / preMax;
            currentGrDb = juce::Decibels::gainToDecibels(attenuation);
            if (currentGrDb > 0.0f) currentGrDb = 0.0f;
        }

        // Smooth GR for display (time constant roughly 50ms)
        const float coeff = (currentGrDb < gainReductionDb.load()) 
            ? std::exp(-1.0f / (float)(0.010 * sampleRate_)) 
            : std::exp(-1.0f / (float)(0.050 * sampleRate_));
            
        float smoothedGr = gainReductionDb.load();
        smoothedGr = coeff * smoothedGr + (1.0f - coeff) * currentGrDb;
        gainReductionDb.store(smoothedGr);
    }

    void setEnabled(bool e)          noexcept { enabled.store(e); }
    void setThresholdDb(float dB)    noexcept { thresholdDb.store(dB); }
    void setRatio(float r)           noexcept { ratio.store(r); }
    void setAttackMs(float ms)       noexcept { attackMs.store(ms); }
    void setReleaseMs(float ms)      noexcept { releaseMs.store(ms); }
    void setMakeupDb(float dB)       noexcept { makeupDb.store(dB); }
    void setAutoMakeup(bool a)       noexcept { autoMakeup.store(a); }

    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    juce::dsp::Compressor<float> compressor;
    double sampleRate_ = 48000.0;

    std::atomic<bool>  enabled     { true };
    std::atomic<float> thresholdDb { -20.0f };
    std::atomic<float> ratio       { 2.0f };
    std::atomic<float> attackMs    { 10.0f };
    std::atomic<float> releaseMs   { 100.0f };
    std::atomic<float> makeupDb    { 0.0f };
    std::atomic<bool>  autoMakeup  { true };

    std::atomic<float> gainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};
