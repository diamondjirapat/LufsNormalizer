#include "dsp/Expander.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

void testInitialization() {
    std::cout << "Running testInitialization..." << std::endl;
    Expander expander;
    expander.prepare(44100.0, 512, 2);
    assert(expander.getGainReductionDb() == 0.0f);
    std::cout << "Passed!" << std::endl;
}

void testNoExpansionAboveThreshold() {
    std::cout << "Running testNoExpansionAboveThreshold..." << std::endl;
    Expander expander;
    expander.prepare(44100.0, 512, 1);
    expander.setThresholdDb(-10.0f);
    expander.setRatio(2.0f);
    expander.setKneeDb(0.0f);
    expander.setAttackMs(10.0f);
    expander.setReleaseMs(10.0f);
    expander.setEnabled(true);

    juce::AudioBuffer<float> buffer(1, 1024);
    for (int i = 0; i < 1024; ++i) buffer.getWritePointer(0)[i] = 1.0f;

    for(int i=0; i<100; ++i) expander.processBlock(buffer);

    float gr = expander.getGainReductionDb();
    std::cout << "Gain Reduction at 0dB (settled): " << gr << " dB" << std::endl;
    assert(std::abs(gr) < 0.01f);
    std::cout << "Passed!" << std::endl;
}

void testDisabledState() {
    std::cout << "Running testDisabledState..." << std::endl;
    Expander expander;
    expander.prepare(44100.0, 512, 1);
    expander.setThresholdDb(-10.0f);
    expander.setEnabled(false);

    juce::AudioBuffer<float> buffer(1, 1024);
    for (int i = 0; i < 1024; ++i) buffer.getWritePointer(0)[i] = 0.001f;

    expander.processBlock(buffer);
    assert(expander.getGainReductionDb() == 0.0f);
    std::cout << "Passed!" << std::endl;
}

void testExpansionLogic() {
    std::cout << "Running testExpansionLogic..." << std::endl;
    Expander expander;
    expander.prepare(44100.0, 512, 1);
    expander.setThresholdDb(-10.0f);
    expander.setRatio(2.0f);
    expander.setKneeDb(0.0f);
    expander.setAttackMs(0.01f);
    expander.setReleaseMs(0.01f);
    expander.setEnabled(true);

    juce::AudioBuffer<float> buffer(1, 1024);
    float level = std::pow(10.0f, -30.0f / 20.0f);
    for (int i = 0; i < 1024; ++i) buffer.getWritePointer(0)[i] = (i % 2 == 0) ? level : -level;

    expander.processBlock(buffer);
    float gr = expander.getGainReductionDb();
    std::cout << "Gain Reduction after one block: " << gr << " dB" << std::endl;
    assert(gr < -1.0f);
    std::cout << "Passed!" << std::endl;
}

void testStereoLinking() {
    std::cout << "Running testStereoLinking..." << std::endl;
    Expander expander;
    expander.prepare(44100.0, 512, 2);
    expander.setThresholdDb(-10.0f);
    expander.setEnabled(true);

    juce::AudioBuffer<float> buffer(2, 1024);
    // Channel 0 is loud (0 dB), Channel 1 is quiet (-60 dB)
    for (int i = 0; i < 1024; ++i) {
        buffer.getWritePointer(0)[i] = 1.0f;
        buffer.getWritePointer(1)[i] = 0.001f;
    }

    for(int i=0; i<100; ++i) expander.processBlock(buffer);
    float gr = expander.getGainReductionDb();
    std::cout << "Stereo linked GR (one channel loud): " << gr << " dB" << std::endl;
    assert(std::abs(gr) < 0.1f);
    std::cout << "Passed!" << std::endl;
}

int main() {
    try {
        testInitialization();
        testNoExpansionAboveThreshold();
        testDisabledState();
        testExpansionLogic();
        testStereoLinking();
        std::cout << "\nTests passed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
