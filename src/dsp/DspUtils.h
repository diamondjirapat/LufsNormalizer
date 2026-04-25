#pragma once
#include <cmath>

/**
 * Shared DSP utilities used across multiple modules.
 */
namespace DspUtils
{
    constexpr float kMinLinear = 1.0e-10f;
    constexpr float kSilenceDb = -144.0f;

    /**
     * Convert a time constant in milliseconds to a 1-pole smoothing coefficient.
     * coeff = exp(-1 / (τ * sampleRate)), where τ = ms / 1000.
     *
     * @param ms  Time constant in milliseconds (must be > 0).
     * @param sr  Sample rate in Hz.
     * @return    Smoothing coefficient in [0, 1).
     */
    inline float msToCoeff(float ms, double sr) noexcept
    {
        if (ms <= 0.0f || sr <= 0.0) return 0.0f;
        return std::exp(-1.0f / (float)(ms * 0.001 * sr));
    }

    inline float dbToGain(float db) noexcept
    {
        return std::exp2(db / 6.020599913f);
    }

    inline float gainToDb(float gain) noexcept
    {
        return gain > kMinLinear ? 6.020599913f * std::log2(gain) : kSilenceDb;
    }

    inline float powerToDb(float power) noexcept
    {
        return power > kMinLinear ? 10.0f * std::log10(power) : kSilenceDb;
    }
}
