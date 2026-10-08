#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

/**
 * True-peak brickwall limiter.
 *
 * Uses 4x oversampling (polyphase upsampling) to detect inter-sample peaks,
 * then applies a brickwall gain reduction to keep true peaks below the ceiling.
 *
 * The limiter uses a lookahead of ~1 ms for transparent operation.
 */
class TruePeakLimiter
{
public:
    TruePeakLimiter() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    /** Process in-place. */
    void processBlock(juce::AudioBuffer<float>& buffer);

    void setCeilingDb(float dB)  noexcept { ceilingDb.store(dB);  }
    void setEnabled(bool e)      noexcept { enabled.store(e);      }

    /** Peak gain reduction applied this block (≤ 0 dB). Thread-safe. */
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

    /** Lookahead latency: at least 1 ms, extended to cover the FIR detector. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

private:
    double sampleRate_  = 48000.0;
    int    numChannels_ = 2;

    std::atomic<float> ceilingDb      { -1.0f };
    std::atomic<bool>  enabled        {  true  };
    std::atomic<float> gainReductionDb{  0.0f  };

    // 4x FIR oversampling used for detection only, recreated in prepare().
    static constexpr int kOversamplingFactor = 4;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Lookahead delay (at least 1 ms)
    juce::AudioBuffer<float> lookaheadBuffer;
    std::vector<float> perSamplePeak;
    struct Peak { uint64_t sample; float value; };
    std::vector<Peak> peakQueue;
    size_t peakHead = 0, peakCount = 0;
    uint64_t sampleIndex = 0;
    int maximumBlockSize = 1;
    int  lookaheadSamples = 0;
    int  writePos         = 0;

    // Smoothed gain for the limiter
    float smoothedGain = 1.0f;
    void processChunk(juce::AudioBuffer<float>& buffer);
};
