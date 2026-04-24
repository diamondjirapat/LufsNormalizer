#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <cstring>

inline float fast_log10(float x) {
    uint32_t vx;
    std::memcpy(&vx, &x, sizeof(x));
    float y = (float)vx;
    y *= 1.1920928955078125e-7f;
    y -= 126.94269504f;
    return y * 0.30102999566f;
}

inline float fast_pow10(float x) {
    float y = x * 3.321928094887362f;
    uint32_t vi = (uint32_t)((1 << 23) * (y + 126.94269504f));
    float v;
    std::memcpy(&v, &vi, sizeof(vi));
    return v;
}

int main() {
    constexpr int numSamples = 44100 * 60; // 60 seconds
    std::vector<float> input(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float normalized = (float)i / numSamples;
        input[i] = std::max(1e-10f, normalized * normalized);
    }

    float targetRmsDb = -18.0f;
    float gateThresholdDb = -60.0f;

    // Baseline
    float volatile sumGain1 = 0.0f;
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numSamples; ++i) {
        float currentRmsDb = 10.0f * std::log10(input[i]);
        float requiredGainDb = 0.0f;
        if (currentRmsDb > gateThresholdDb) {
            requiredGainDb = std::clamp(targetRmsDb - currentRmsDb, -12.0f, 12.0f);
        }
        sumGain1 += std::pow(10.0f, requiredGainDb / 20.0f);
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto dur1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();

    // Fast
    float volatile sumGain2 = 0.0f;
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numSamples; ++i) {
        float currentRmsDb = 10.0f * fast_log10(input[i]);
        float requiredGainDb = 0.0f;
        if (currentRmsDb > gateThresholdDb) {
            requiredGainDb = std::clamp(targetRmsDb - currentRmsDb, -12.0f, 12.0f);
        }
        sumGain2 += fast_pow10(requiredGainDb / 20.0f);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto dur2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();

    std::cout << "Baseline: " << dur1 << " us" << std::endl;
    std::cout << "Optimized: " << dur2 << " us" << std::endl;
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) << (float)dur1 / dur2 << "x" << std::endl;

    return 0;
}
