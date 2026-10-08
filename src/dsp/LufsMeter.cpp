#include "LufsMeter.h"
#include "DspUtils.h"
#include <cmath>

// ── Filter coefficient factories ─────────────────────────────────────────────
// Coefficients from ITU-R BS.1770-4 Annex 1.

juce::dsp::IIR::Coefficients<float>::Ptr
LufsMeter::makePreFilterCoeffs(double fs)
{
    // High-shelf pre-filter
    // Derived from the analogue prototype in BS.1770-4
    const double Vh = std::pow(10.0, 3.999843853973347 / 20.0);
    const double Vb = std::pow(Vh, 0.4996667741545416);
    const double Vl = 1.0;
    const double Q  = 0.7071752369554196;
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
    const double b0 =  1.0;
    const double b1 = -2.0;
    const double b2 =  1.0;
    const double a1 =  2.0 * (K * K - 1.0) / a0;
    const double a2 = (1.0 - K / Q + K * K) / a0;

    return juce::dsp::IIR::Coefficients<float>::Ptr(
        new juce::dsp::IIR::Coefficients<float>(
            (float)b0, (float)b1, (float)b2,
            1.0f,      (float)a1, (float)a2));
}

// ── prepare ──────────────────────────────────────────────────────────────────
void LufsMeter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    numChannels_ = numChannels;

    // Build per-channel K-weighting filters
    filters.resize((size_t)numChannels);
    auto preCoeffs = makePreFilterCoeffs(sampleRate);
    auto rlbCoeffs = makeRLBFilterCoeffs(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32)std::max(1, maxBlockSize);
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

    momentaryWindow.allocate(std::max(1, (int)std::round(sampleRate * 0.400)));
    shortTermWindow.allocate(std::max(1, (int)std::round(sampleRate * 3.000)));
    gatedBlockSize = std::max(1, (int)std::round(sampleRate * 0.1));
    reset();
}

void LufsMeter::reset()
{
    momentaryWindow.clear();
    shortTermWindow.clear();
    histogram.fill(0);
    histogramEnergy.fill(0.0);
    gatedBlockSamples = (int)momentaryWindow.energies.size();

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

    for (int i = 0; i < numSamples; ++i)
    {
        double energy = 0.0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const double weight = numCh > 4 && ch == 3 ? 0.0
                                : numCh > 4 && ch >= 4 ? 1.41 : 1.0;
            if (weight == 0.0) continue;
            float s = filters[(size_t)ch].preFilter.processSample(buffer.getReadPointer(ch)[i]);
            s = filters[(size_t)ch].rlbFilter.processSample(s);
            energy += weight * (double)s * (double)s;
        }
        momentaryWindow.push(energy);
        shortTermWindow.push(energy);

        // Full 400 ms gating blocks, advanced every 100 ms (75% overlap).
        if (--gatedBlockSamples <= 0)
        {
            const double mean = momentaryWindow.meanSquare();
            const float lufs = msToLUFS(mean);
            if (lufs > HISTOGRAM_MIN_LUFS)
            {
                const int bin = std::clamp((int)std::floor((lufs - HISTOGRAM_MIN_LUFS) / 0.1f),
                                           0, (int)HISTOGRAM_BINS - 1);
                ++histogram[(size_t)bin];
                histogramEnergy[(size_t)bin] += mean;
            }
            updateIntegrated();
            gatedBlockSamples = gatedBlockSize;
        }
    }
    momentaryLUFS.store(msToLUFS(momentaryWindow.meanSquare()));
    shortTermLUFS.store(msToLUFS(shortTermWindow.meanSquare()));
}

void LufsMeter::resetIntegrated() noexcept
{
    histogram.fill(0);
    histogramEnergy.fill(0.0);
    gatedBlockSamples = (int)momentaryWindow.energies.size();
    integratedLUFS.store(-144.0f);
}

// ── calculateHistogramSummary ────────────────────────────────────────────────────────────
LufsMeter::HistogramSummary LufsMeter::calculateHistogramSummary(size_t startBin) const noexcept
{
    double sum = 0.0;
    uint64_t count = 0;

    for (size_t i = startBin; i < HISTOGRAM_BINS; ++i)
    {
        if (histogram[i] > 0)
        {
            sum += histogramEnergy[i];
            count += histogram[i];
        }
    }

    return { sum, count };
}

// ── updateIntegrated ────────────────────────────────────────────────────────────
void LufsMeter::updateIntegrated()
{
    // Pass 1: find average energy of all blocks >= -70 LUFS (absolute gate)
    auto pass1 = calculateHistogramSummary(0);

    if (pass1.count == 0)
    {
        integratedLUFS.store(-144.0f);
        return;
    }

    // Relative gate: -10 LU below ungated mean
    const double ungatedMean = pass1.sum / (double)pass1.count;
    const double relGateMS = ungatedMean * std::pow(10.0, -10.0 / 10.0);
    const float relGateLUFS = msToLUFS(relGateMS);

    // Pass 2: find average energy above relative gate
    int startBin = (int)std::ceil((relGateLUFS - HISTOGRAM_MIN_LUFS) / 0.1f);
    startBin = std::max(0, std::min(startBin, (int)HISTOGRAM_BINS - 1));

    auto pass2 = calculateHistogramSummary((size_t)startBin);

    if (pass2.count == 0)
    {
        integratedLUFS.store(-144.0f);
        return;
    }

    integratedLUFS.store(msToLUFS(pass2.sum / (double)pass2.count));
}

// ── msToLUFS ─────────────────────────────────────────────────────────────────
float LufsMeter::msToLUFS(double ms) noexcept
{
    if (ms <= 0.0) return DspUtils::kSilenceDb;
    // BS.1770: L_K = -0.691 + 10 * log10(sum of channel mean-squares)
    return (float)(-0.691 + 10.0 * std::log10(ms));
}
