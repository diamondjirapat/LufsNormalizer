#include "LufsMeter.h"
#include <cmath>

// ── Filter coefficient factories ─────────────────────────────────────────────
// Coefficients from ITU-R BS.1770-4 Annex 1.

juce::dsp::IIR::Coefficients<float>::Ptr
LufsMeter::makePreFilterCoeffs(double fs)
{
    // High-shelf pre-filter
    // Derived from the analogue prototype in BS.1770-4
    const double Vh = 1.58486;
    const double Vb = 1.52433;
    const double Vl = 1.0;
    const double Q  = 0.7071;
    const double fc = 1681.974450955533;
    const double K  = std::tan(juce::MathConstants<double>::pi * fc / fs);

    const double a0 =       1.0 + K / Q + K * K;
    const double b0 = (Vh + Vb * K / Q + Vl * K * K) / a0;
    const double b1 = 2.0 * (Vl * K * K - Vh) / a0;
    const double b2 = (Vh - Vb * K / Q + Vl * K * K) / a0;
    const double a1 = 2.0 * (K * K - 1.0) / a0;
    const double a2 = (1.0 - K / Q + K * K) / a0;

    return juce::dsp::IIR::Coefficients<float>::Ptr(
        new juce::dsp::IIR::Coefficients<float>(
            (float)b0, (float)b1, (float)b2,
            1.0f,      (float)a1, (float)a2));
}

juce::dsp::IIR::Coefficients<float>::Ptr
LufsMeter::makeRLBFilterCoeffs(double fs)
{
    // High-pass RLB weighting filter (2nd order Butterworth, fc = 38.13547 Hz)
    const double fc = 38.13547087602444;
    const double K  = std::tan(juce::MathConstants<double>::pi * fc / fs);
    const double Q  = 0.5003270373238773;

    const double a0 =  1.0 + K / Q + K * K;
    const double b0 =  1.0 / a0;
    const double b1 = -2.0 / a0;
    const double b2 =  1.0 / a0;
    const double a1 =  2.0 * (K * K - 1.0) / a0;
    const double a2 = (1.0 - K / Q + K * K) / a0;

    return juce::dsp::IIR::Coefficients<float>::Ptr(
        new juce::dsp::IIR::Coefficients<float>(
            (float)b0, (float)b1, (float)b2,
            1.0f,      (float)a1, (float)a2));
}

// ── prepare ──────────────────────────────────────────────────────────────────
void LufsMeter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    sampleRate_  = sampleRate;
    numChannels_ = numChannels;

    // Build per-channel K-weighting filters
    filters.resize((size_t)numChannels);
    auto preCoeffs = makePreFilterCoeffs(sampleRate);
    auto rlbCoeffs = makeRLBFilterCoeffs(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 4096;
    spec.numChannels      = 1;

    for (auto& ch : filters)
    {
        *ch.preFilter.coefficients = *preCoeffs;
        *ch.rlbFilter.coefficients = *rlbCoeffs;
        ch.preFilter.prepare(spec);
        ch.rlbFilter.prepare(spec);
        ch.preFilter.reset();
        ch.rlbFilter.reset();
    }

    // Window sizes in blocks (one block = processBlock call)
    // We approximate using 10 ms sub-blocks internally, but here we use
    // the actual block size. For accuracy we track samples per window.
    // Momentary = 400 ms, Short-term = 3000 ms
    // We'll recalculate block counts dynamically in processBlock.
    momentaryWindow  = {};
    shortTermWindow  = {};

    // Gated integration: 100 ms blocks
    gatedBlockSize    = (int)std::round(sampleRate * 0.1);
    gatedBlockAccum   = 0.0;
    gatedBlockSamples = 0;
    gatedBlocks.clear();

    reset();
}

void LufsMeter::reset()
{
    momentaryWindow  = {};
    shortTermWindow  = {};
    gatedBlocks.clear();
    gatedBlockAccum   = 0.0;
    gatedBlockSamples = 0;

    for (auto& ch : filters)
    {
        ch.preFilter.reset();
        ch.rlbFilter.reset();
    }

    momentaryLUFS .store(-144.0f);
    shortTermLUFS .store(-144.0f);
    integratedLUFS.store(-144.0f);
}

// ── processBlock ─────────────────────────────────────────────────────────────
void LufsMeter::processBlock(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min(buffer.getNumChannels(), numChannels_);

    if (numSamples == 0 || numCh == 0) return;

    // ── Apply K-weighting and accumulate mean-square ─────────────────────────
    // Channel weights per BS.1770: L/R/C = 1.0, LFE = 0, Ls/Rs = 1.41
    // For stereo we use 1.0 for both channels.
    double blockMeanSquare = 0.0;

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* src = buffer.getReadPointer(ch);

        double chSum = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            float s = filters[(size_t)ch].preFilter.processSample(src[i]);
            s       = filters[(size_t)ch].rlbFilter.processSample(s);

            // Flush denormals
            if (std::abs(s) < 1e-15f) s = 0.0f;

            chSum += (double)s * (double)s;
        }
        blockMeanSquare += chSum / (double)numSamples;
    }

    // Average across channels (equal weights for stereo)
    blockMeanSquare /= (double)numCh;

    // ── Update sliding windows ────────────────────────────────────────────────
    // We store the mean-square for this block and track how many samples
    // each window should span.
    const double blockDuration = (double)numSamples / sampleRate_;

    // Recalculate max blocks for each window based on current block size
    const int momentaryMaxBlocks  = std::max(1, (int)std::ceil(0.400 / blockDuration));
    const int shortTermMaxBlocks  = std::max(1, (int)std::ceil(3.000 / blockDuration));

    momentaryWindow.maxBlocks = momentaryMaxBlocks;
    shortTermWindow.maxBlocks = shortTermMaxBlocks;

    momentaryWindow.push(blockMeanSquare);
    shortTermWindow.push(blockMeanSquare);

    // ── Publish momentary / short-term ───────────────────────────────────────
    momentaryLUFS .store(msToLUFS(momentaryWindow.meanSquare()));
    shortTermLUFS .store(msToLUFS(shortTermWindow.meanSquare()));

    // ── Gated integration (100 ms blocks) ────────────────────────────────────
    gatedBlockAccum   += blockMeanSquare * (double)numSamples;
    gatedBlockSamples += numSamples;

    if (gatedBlockSamples >= gatedBlockSize)
    {
        const double ms = gatedBlockAccum / (double)gatedBlockSamples;
        gatedBlocks.push_back({ ms });
        gatedBlockAccum   = 0.0;
        gatedBlockSamples = 0;
        updateIntegrated();
    }
}

// ── updateIntegrated ─────────────────────────────────────────────────────────
void LufsMeter::updateIntegrated()
{
    if (gatedBlocks.empty()) return;

    // Absolute gate: -70 LUFS  →  mean-square threshold
    // LUFS = -0.691 + 10*log10(ms)  →  ms = 10^((LUFS+0.691)/10)
    constexpr double absGateLUFS = -70.0;
    const double     absGateMS   = std::pow(10.0, (absGateLUFS + 0.691) / 10.0);

    // Pass 1: collect blocks above absolute gate
    double sum1 = 0.0;
    int    cnt1 = 0;
    for (const auto& b : gatedBlocks)
    {
        if (b.meanSquare >= absGateMS)
        {
            sum1 += b.meanSquare;
            ++cnt1;
        }
    }

    if (cnt1 == 0)
    {
        integratedLUFS.store(-144.0f);
        return;
    }

    // Relative gate: -10 LU below ungated mean
    const double ungatedMean  = sum1 / (double)cnt1;
    const double relGateMS    = ungatedMean * std::pow(10.0, -10.0 / 10.0);

    // Pass 2: collect blocks above relative gate
    double sum2 = 0.0;
    int    cnt2 = 0;
    for (const auto& b : gatedBlocks)
    {
        if (b.meanSquare >= absGateMS && b.meanSquare >= relGateMS)
        {
            sum2 += b.meanSquare;
            ++cnt2;
        }
    }

    if (cnt2 == 0)
    {
        integratedLUFS.store(-144.0f);
        return;
    }

    integratedLUFS.store(msToLUFS(sum2 / (double)cnt2));
}

// ── msToLUFS ─────────────────────────────────────────────────────────────────
float LufsMeter::msToLUFS(double ms) noexcept
{
    if (ms <= 0.0) return -144.0f;
    // BS.1770: L_K = -0.691 + 10 * log10(sum of channel mean-squares)
    return (float)(-0.691 + 10.0 * std::log10(ms));
}
