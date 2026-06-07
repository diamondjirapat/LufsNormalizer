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
        g.setColour(juce::Colour(0xff404560).withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("No signal", bounds.toNearestInt(), juce::Justification::centred);
        return;
    }

    // ── Target line — dashed with glow ───────────────────────────────────────
    {
        const float targetY = y + dbToY(targetLUFS.load(), h);

        // Glow behind the line
        g.setColour(juce::Colour(0xffffab00).withAlpha(0.10f));
        g.fillRect(x, targetY - 2.0f, w, 4.0f);

        // Dashed line
        g.setColour(juce::Colour(0xffffab00).withAlpha(0.55f));
        const float dashLen = 5.0f;
        const float gapLen  = 3.0f;
        float cx = x;
        while (cx < x + w)
        {
            float endX = std::min(cx + dashLen, x + w);
            g.drawLine(cx, targetY, endX, targetY, 1.2f);
            cx += dashLen + gapLen;
        }
    }

    // ── Helper lambda to draw a history trace with glow ──────────────────────
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

        // Fill under the curve — pronounced gradient
        juce::Path filled = path;
        filled.lineTo(x + (float)(maxPoints - 1) * step, y + h);
        filled.lineTo(x + (float)(maxPoints - n) * step, y + h);
        filled.closeSubPath();

        // Multi-stop fill gradient (more opaque near the line, fading out)
        {
            juce::ColourGradient grad(fillCol.withAlpha(0.25f), x, y,
                                      fillCol.withAlpha(0.02f), x, y + h, false);
            grad.addColour(0.4, fillCol.withAlpha(0.12f));
            g.setGradientFill(grad);
            g.fillPath(filled);
        }

        // Glow stroke (wide, soft)
        g.setColour(strokeCol.withAlpha(0.12f));
        g.strokePath(path, juce::PathStrokeType(strokeWidth * 3.0f, juce::PathStrokeType::curved));

        // Crisp stroke
        g.setColour(strokeCol.withAlpha(0.85f));
        g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved));
    };

    // ── Draw input trace (blue, behind) ──────────────────────────────────────
    drawTrace(inputHistory,
              juce::Colour(0xff3a8fd4),
              juce::Colour(0xff3a8fd4),
              1.5f);

    // ── Draw output trace (green, in front) ──────────────────────────────────
    drawTrace(outputHistory,
              juce::Colour(0xff00e676),
              juce::Colour(0xff00e676),
              2.0f);

    // ── Scale labels (dotted grid) ───────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(7.5f)));
    for (float db = kMaxDb; db >= kMinDb; db -= 12.0f)
    {
        const float labelY = y + dbToY(db, h);

        g.setColour(juce::Colour(0xff505568));
        g.drawText(juce::String((int)db),
                   (int)x, (int)(labelY - 5.0f), 20, 10,
                   juce::Justification::left, false);

        // Dotted grid line
        g.setColour(juce::Colour(0xff252830));
        float gx = x + 22.0f;
        while (gx < x + w)
        {
            g.fillRect(gx, labelY, 2.0f, 0.5f);
            gx += 5.0f;
        }
    }

    // ── Legend in top-right corner (circles instead of rectangles) ────────────
    {
        const float legendX = x + w - 95.0f;
        const float legendY = y + 5.0f;

        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));

        // Input (blue) legend
        g.setColour(juce::Colour(0xff3a8fd4));
        g.fillEllipse(legendX, legendY + 1.5f, 6.0f, 6.0f);
        g.drawText("Input", (int)(legendX + 10.0f), (int)legendY, 30, 10,
                   juce::Justification::left, false);

        // Output (green) legend
        g.setColour(juce::Colour(0xff00e676));
        g.fillEllipse(legendX + 44.0f, legendY + 1.5f, 6.0f, 6.0f);
        g.drawText("Output", (int)(legendX + 54.0f), (int)legendY, 36, 10,
                   juce::Justification::left, false);
    }
}
