#pragma once
#include <cmath>

/**
 * Shared DSP utilities used across multiple modules.
 */
namespace DspUtils
{
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
        if (ms <= 0.0f) return 0.0f;
        return std::exp(-1.0f / (float)(ms * 0.001 * sr));
    }
}
