#include "LevelHistory.h"
#include <algorithm>
#include <cmath>

LevelHistory::LevelHistory(int historySeconds)
    : maxPoints(historySeconds * 10) // 10 updates/sec
{
}

void LevelHistory::pushInputValue(float db)
{
    inputHistory.push_back(db);
    while ((int)inputHistory.size() > maxPoints)
        inputHistory.pop_front();
    repaint();
}

void LevelHistory::pushOutputValue(float db)
{
    outputHistory.push_back(db);
    while ((int)outputHistory.size() > maxPoints)
        outputHistory.pop_front();
    repaint();
}

void LevelHistory::pushValue(float lufs)
{
    pushInputValue(lufs);
}

float LevelHistory::dbToY(float db, float height) const noexcept
{
    const float clamped = std::clamp(db, kMinDb, kMaxDb);
    const float norm    = (kMaxDb - clamped) / (kMaxDb - kMinDb);
    return norm * height;
}

void LevelHistory::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();

    const bool hasInput  = !inputHistory.empty();
    const bool hasOutput = !outputHistory.empty();

    if (!hasInput && !hasOutput)
    {
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("No signal", bounds.toNearestInt(), juce::Justification::centred);
        return;
    }

    // ── Target line ───────────────────────────────────────────────────────────
    const float targetY = y + dbToY(targetLUFS, h);
    g.setColour(juce::Colour(0xffff9900).withAlpha(0.5f));
    g.drawLine(x, targetY, x + w, targetY, 1.5f);

    // ── Helper lambda to draw a history trace ─────────────────────────────────
    auto drawTrace = [&](const std::deque<float>& history, juce::Colour strokeCol,
                         juce::Colour fillCol, float strokeWidth)
    {
        if (history.empty()) return;

        juce::Path path;
        const int  n    = (int)history.size();
        const float step = w / (float)std::max(maxPoints - 1, 1);

        bool started = false;
        for (int i = 0; i < n; ++i)
        {
            const float px = x + (float)(maxPoints - n + i) * step;
            const float py = y + dbToY(history[(size_t)i], h);

            if (!started) { path.startNewSubPath(px, py); started = true; }
            else          { path.lineTo(px, py); }
        }

        // Fill under the curve
        juce::Path filled = path;
        filled.lineTo(x + (float)(maxPoints - 1) * step, y + h);
        filled.lineTo(x + (float)(maxPoints - n) * step, y + h);
        filled.closeSubPath();

        g.setColour(fillCol);
        g.fillPath(filled);

        // Stroke
        g.setColour(strokeCol);
        g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved));
    };

    // ── Draw input trace (blue, behind) ───────────────────────────────────────
    drawTrace(inputHistory,
              juce::Colour(0xff3a8fd4),            // blue stroke
              juce::Colour(0xff3a8fd4).withAlpha(0.12f), // blue fill
              1.5f);

    // ── Draw output trace (green, in front) ───────────────────────────────────
    drawTrace(outputHistory,
              juce::Colour(0xff2ecc71),            // green stroke
              juce::Colour(0xff2ecc71).withAlpha(0.12f), // green fill
              2.0f);

    // ── Scale labels ─────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.setColour(juce::Colours::grey);
    for (float db = kMaxDb; db >= kMinDb; db -= 12.0f)
    {
        const float labelY = y + dbToY(db, h);
        g.drawText(juce::String((int)db),
                   (int)x, (int)(labelY - 5.0f), 20, 10,
                   juce::Justification::left, false);
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
        g.drawLine(x + 20.0f, labelY, x + w, labelY, 0.5f);
        g.setColour(juce::Colours::grey);
    }

    // ── Legend in top-right corner ────────────────────────────────────────────
    const float legendX = x + w - 90.0f;
    const float legendY = y + 4.0f;

    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));

    // Input (blue) legend
    g.setColour(juce::Colour(0xff3a8fd4));
    g.fillRect(legendX, legendY + 2.0f, 10.0f, 3.0f);
    g.drawText("Input", (int)(legendX + 14.0f), (int)legendY, 30, 10,
               juce::Justification::left, false);

    // Output (green) legend
    g.setColour(juce::Colour(0xff2ecc71));
    g.fillRect(legendX + 48.0f, legendY + 2.0f, 10.0f, 3.0f);
    g.drawText("Output", (int)(legendX + 62.0f), (int)legendY, 36, 10,
               juce::Justification::left, false);
}
