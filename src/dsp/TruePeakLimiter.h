#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

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

    /** Latency introduced by the 1 ms lookahead. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

private:
    double sampleRate_  = 44100.0;
    int    numChannels_ = 2;

    std::atomic<float> ceilingDb      { -1.0f };
    std::atomic<bool>  enabled        {  true  };
    std::atomic<float> gainReductionDb{  0.0f  };

    // 4x oversampling — recreated in prepare() with the correct channel count
    static constexpr int kOversamplingFactor = 4;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Lookahead delay (1 ms)
    juce::AudioBuffer<float> lookaheadBuffer;
    int  lookaheadSamples = 0;
    int  writePos         = 0;

    // Smoothed gain for the limiter
    float smoothedGain = 1.0f;
};
