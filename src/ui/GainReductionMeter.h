#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

/**
 * Horizontal gain-reduction meter.
 * Shows expander GR (blue) and limiter GR (red) stacked.
 */
class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter();

    void setExpanderGR(float dB)  noexcept { expanderGR.store(dB);  }
    void setLimiterGR (float dB)  noexcept { limiterGR .store(dB);  }
    void setLevelerGain(float dB) noexcept { levelerGain.store(dB); }

    void paint(juce::Graphics& g) override;

private:
    static constexpr float kMaxGR = 30.0f; // display range in dB

    std::atomic<float> expanderGR  { 0.0f };
    std::atomic<float> limiterGR   { 0.0f };
    std::atomic<float> levelerGain { 0.0f };

    float grToWidth(float grDb, float totalWidth) const noexcept;
};
