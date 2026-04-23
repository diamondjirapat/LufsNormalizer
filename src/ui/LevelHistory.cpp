#include "LevelHistory.h"
#include <algorithm>
#include <cmath>

LevelHistory::LevelHistory(int historySeconds)
    : maxPoints(historySeconds * 10) // 10 updates/sec
{
}

void LevelHistory::pushValue(float lufs)
{
    history.push_back(lufs);
    while ((int)history.size() > maxPoints)
        history.pop_front();
    repaint();
}

float LevelHistory::lufsToY(float lufs, float height) const noexcept
{
    const float clamped = std::clamp(lufs, kMinLUFS, kMaxLUFS);
    const float norm    = (kMaxLUFS - clamped) / (kMaxLUFS - kMinLUFS);
    return norm * height;
}

void LevelHistory::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();

    // The background is handled by the parent panel now, 
    // but we can add a very faint inner grid or just let it be transparent.

    if (history.empty())
    {
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.setFont(juce::Font(11.0f));
        g.drawText("No signal", bounds.toNearestInt(), juce::Justification::centred);
        return;
    }

    // ── Target line ───────────────────────────────────────────────────────────
    const float targetY = y + lufsToY(targetLUFS, h);
    g.setColour(juce::Colour(0xffff9900).withAlpha(0.6f));
    g.drawLine(x, targetY, x + w, targetY, 1.5f);

    // ── History path ─────────────────────────────────────────────────────────
    juce::Path path;
    const int  n     = (int)history.size();
    const float step = w / (float)std::max(maxPoints - 1, 1);

    bool started = false;
    for (int i = 0; i < n; ++i)
    {
        const float px = x + (float)(maxPoints - n + i) * step;
        const float py = y + lufsToY(history[(size_t)i], h);

        if (!started) { path.startNewSubPath(px, py); started = true; }
        else          { path.lineTo(px, py); }
    }

    // Fill under the curve
    juce::Path filled = path;
    filled.lineTo(x + (float)(maxPoints - 1) * step, y + h);
    filled.lineTo(x, y + h);
    filled.closeSubPath();

    g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.15f));
    g.fillPath(filled);

    // Stroke
    g.setColour(juce::Colour(0xff00d4ff));
    g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

    // ── Scale labels ─────────────────────────────────────────────────────────
    g.setFont(juce::Font(8.0f));
    g.setColour(juce::Colours::grey);
    for (float lufs = kMaxLUFS; lufs >= kMinLUFS; lufs -= 10.0f)
    {
        const float labelY = y + lufsToY(lufs, h);
        g.drawText(juce::String((int)lufs),
                   (int)x, (int)(labelY - 5.0f), 20, 10,
                   juce::Justification::left, false);
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
        g.drawLine(x + 20.0f, labelY, x + w, labelY, 0.5f);
        g.setColour(juce::Colours::grey);
    }

    // Border is handled by parent panel.
}
