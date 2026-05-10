#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "dsp/AutoGain.h"
#include <juce_audio_basics/juce_audio_basics.h>

constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;

void processSilence(AutoGain& autoGain, int numBlocks) {
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    for (int i = 0; i < numBlocks; ++i) {
        buffer.clear();
        autoGain.processBlock(buffer);
    }
}

void processSineWave(AutoGain& autoGain, int numBlocks, float amplitude) {
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    double phase = 0.0;
    double phaseInc = 2.0 * M_PI * 440.0 / kSampleRate;

    for (int i = 0; i < numBlocks; ++i) {
        for (int sample = 0; sample < kBlockSize; ++sample) {
            float val = amplitude * static_cast<float>(std::sin(phase));
            buffer.setSample(0, sample, val);
            buffer.setSample(1, sample, val);
            phase += phaseInc;
        }
        autoGain.processBlock(buffer);
    }
}

void testInitialization() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);

    // Should initialize to 0 dB gain
    assert(std::abs(autoGain.getCurrentGainDb() - 0.0f) < 1e-4f);
    std::cout << "testInitialization passed" << std::endl;
}

void testDisabled() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);
    autoGain.setEnabled(false);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    // Fill with 1.0f
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < kBlockSize; ++i) {
            buffer.setSample(ch, i, 1.0f);
        }
    }

    autoGain.processBlock(buffer);

    // Bypassed, output should remain 1.0f, gain should be 0.0f
    assert(std::abs(autoGain.getCurrentGainDb() - 0.0f) < 1e-4f);
    assert(std::abs(buffer.getSample(0, 0) - 1.0f) < 1e-4f);
    std::cout << "testDisabled passed" << std::endl;
}

void testSilenceBelowGate() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);
    autoGain.setGateThresholdDb(-60.0f);

    // Process silence
    processSilence(autoGain, 10);

    // Gain should stay at 0 dB (multiplier 1.0) because signal is below gate
    assert(std::abs(autoGain.getCurrentGainDb() - 0.0f) < 1e-4f);
    std::cout << "testSilenceBelowGate passed" << std::endl;
}

void testTargetTracking() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);
    autoGain.setTargetRmsDb(-12.0f);
    autoGain.setAttackMs(10.0f);
    autoGain.setReleaseMs(10.0f);
    autoGain.setGateThresholdDb(-60.0f);
    autoGain.setMaxGainDb(20.0f);

    // Target RMS is -12 dB.
    // -12 dB RMS sine wave amplitude is approx 10^(-12/20) * sqrt(2)
    // 10^(-12/20) = 0.2511886
    // If we input a signal with an RMS of -24 dB, AutoGain should boost it.
    // -24 dB RMS sine wave amplitude = 10^(-24/20) * sqrt(2) = 0.0630957 * 1.414 = ~0.089

    float inputAmplitude = 0.089f; // roughly -24 dB RMS

    // Run it for enough blocks to adapt
    processSineWave(autoGain, 200, inputAmplitude);

    // Required gain to get from -24 to -12 is +12 dB
    float finalGainDb = autoGain.getCurrentGainDb();

    // Since our RMS calculation is lowpass filtered and approximate, we allow some tolerance
    assert(finalGainDb > 10.0f && finalGainDb < 14.0f);
    std::cout << "testTargetTracking (boost) passed. Final gain: " << finalGainDb << " dB" << std::endl;
}

void testMaxGainLimit() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);
    autoGain.setTargetRmsDb(-12.0f);
    autoGain.setAttackMs(10.0f);
    autoGain.setReleaseMs(10.0f);
    autoGain.setGateThresholdDb(-80.0f);

    // Set a very strict max gain
    autoGain.setMaxGainDb(6.0f);

    // Input a very quiet signal, e.g. -40 dB RMS
    // Should want +28 dB, but capped at +6 dB
    float inputAmplitude = 0.014f; // roughly -40 dB RMS
    processSineWave(autoGain, 200, inputAmplitude);

    float finalGainDb = autoGain.getCurrentGainDb();

    // Ensure we are near the max gain cap, but not exceeding it
    assert(finalGainDb <= 6.1f);
    assert(finalGainDb > 5.0f);
    std::cout << "testMaxGainLimit passed. Final gain: " << finalGainDb << " dB" << std::endl;
}

void testGainReduction() {
    AutoGain autoGain;
    autoGain.prepare(kSampleRate, kBlockSize, 2);
    autoGain.setTargetRmsDb(-24.0f); // very quiet target
    autoGain.setAttackMs(10.0f);
    autoGain.setReleaseMs(10.0f);
    autoGain.setGateThresholdDb(-60.0f);

    // Allow reduction is FALSE by default. So it shouldn't reduce below 0 dB.
    float inputAmplitude = 0.5f; // roughly -9 dB RMS. Needs -15 dB gain.
    processSineWave(autoGain, 200, inputAmplitude);

    float finalGainDb = autoGain.getCurrentGainDb();
    // It should stay at 0 dB because allowReduce is false
    assert(std::abs(finalGainDb - 0.0f) < 1e-4f);

    // Now allow reduction
    autoGain.setAllowReduce(true);
    // Needs enough time to decay back down
    processSineWave(autoGain, 400, inputAmplitude);

    finalGainDb = autoGain.getCurrentGainDb();
    // It should now reduce gain. Target is -24, input is -9, so roughly -15 dB of gain.
    assert(finalGainDb < -10.0f && finalGainDb > -20.0f);
    std::cout << "testGainReduction passed. Final gain: " << finalGainDb << " dB" << std::endl;
}

int main() {
    std::cout << "Running AutoGain tests..." << std::endl;

    testInitialization();
    testDisabled();
    testSilenceBelowGate();
    testTargetTracking();
    testMaxGainLimit();
    testGainReduction();

    std::cout << "All AutoGain tests passed successfully!" << std::endl;
    return 0;
}
