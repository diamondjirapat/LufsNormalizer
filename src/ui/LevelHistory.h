#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

/**
 * Scrolling level history graph.
 * Displays both input (blue) and output (green) levels over the last N seconds.
 */
class LevelHistory : public juce::Component
{
public:
    explicit LevelHistory(int historySeconds = 30);

    /** Push a new input level value in dBFS (call from timer on message thread). */
    void pushInputValue(float db);

    /** Push a new output level value in dBFS (call from timer on message thread). */
    void pushOutputValue(float db);

    /** Legacy single-value push (pushes to input). */
    void pushValue(float lufs);

    void setTargetLUFS(float lufs) noexcept { targetLUFS = lufs; }

    void paint(juce::Graphics& g) override;

private:
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb =   0.0f;

    int maxPoints;
    std::deque<float> inputHistory;
    std::deque<float> outputHistory;
    float targetLUFS = -16.0f;

    float dbToY(float db, float height) const noexcept;
};
