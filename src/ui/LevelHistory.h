#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <mutex>

/**
 * Scrolling LUFS history graph.
 * Displays short-term LUFS over the last N seconds as a waveform.
 */
class LevelHistory : public juce::Component
{
public:
    explicit LevelHistory(int historySeconds = 30);

    /** Push a new short-term LUFS value (call from timer on message thread). */
    void pushValue(float lufs);

    void setTargetLUFS(float lufs) noexcept { targetLUFS = lufs; }

    void paint(juce::Graphics& g) override;

private:
    static constexpr float kMinLUFS = -40.0f;
    static constexpr float kMaxLUFS =   0.0f;

    int maxPoints;
    std::deque<float> history;
    float targetLUFS = -16.0f;

    float lufsToY(float lufs, float height) const noexcept;
};
