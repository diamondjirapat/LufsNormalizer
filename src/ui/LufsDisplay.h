#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Vertical peak/RMS bar meter.
 * Can operate in Input mode (blue tones) or Output mode (green tones).
 * Shows peak bar, RMS bar, and a peak-hold tick.
 */
class LufsDisplay : public juce::Component
{
public:
    enum Mode { Input, Output };

    LufsDisplay();

    void setMode(Mode m) noexcept { mode = m; }

    void setPeakDb (float db) noexcept { peakDb .store(db); }
    void setRmsDb  (float db) noexcept { rmsDb  .store(db); }

    /** Still accepts LUFS values for the integrated/target overlay. */
    void setTargetLUFS    (float lufs) noexcept { targetLUFS    .store(lufs); }

    void updatePeakHold();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Mode mode = Input;

    // Range displayed on the meter (dBFS)
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb =   0.0f;

    std::atomic<float> peakDb  { kMinDb };
    std::atomic<float> rmsDb   { kMinDb };

    // Legacy LUFS fields (for target line / integrated tick on the meter)
    std::atomic<float> targetLUFS     { -16.0f  };

    // Peak-hold state
    float peakHoldDb   = kMinDb;
    int   peakHoldTicks = 0;
    static constexpr int kPeakHoldFrames = 40; // ~2 sec at 20 fps
    static constexpr float kPeakFallRate = 0.3f; // dB per frame

    float dbToY(float db, float height) const noexcept;

    juce::Rectangle<int> meterArea;
};
