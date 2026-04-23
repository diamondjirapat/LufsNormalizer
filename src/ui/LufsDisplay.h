#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Vertical LUFS bar meter with scale markings.
 * Displays momentary (fast) and short-term (slow) values simultaneously.
 */
class LufsDisplay : public juce::Component
{
public:
    LufsDisplay();

    void setMomentaryLUFS (float lufs) noexcept;
    void setShortTermLUFS (float lufs) noexcept;
    void setIntegratedLUFS(float lufs) noexcept;
    void setTargetLUFS    (float lufs) noexcept;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Range displayed on the meter
    static constexpr float kMinLUFS = -40.0f;
    static constexpr float kMaxLUFS =   0.0f;

    std::atomic<float> momentaryLUFS  { kMinLUFS };
    std::atomic<float> shortTermLUFS  { kMinLUFS };
    std::atomic<float> integratedLUFS { kMinLUFS };
    std::atomic<float> targetLUFS     { -16.0f   };

    // Smoothed display values (updated in paint on message thread)
    float displayMomentary  = kMinLUFS;
    float displayShortTerm  = kMinLUFS;

    float lufsToY(float lufs, float height) const noexcept;

    juce::Rectangle<int> meterArea;
};
