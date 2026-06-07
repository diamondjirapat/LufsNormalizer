#include "GainReductionMeter.h"
#include <cmath>
#include <algorithm>

GainReductionMeter::GainReductionMeter()
{
    setOpaque(false);
}

float GainReductionMeter::grToWidth(float grDb, float totalWidth) const noexcept
{
    // grDb is <= 0; map 0..-kMaxGR to 0..totalWidth/2
    const float clamped = std::clamp(-grDb, 0.0f, kMaxGR);
    return (clamped / kMaxGR) * (totalWidth * 0.5f);
}

void GainReductionMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();
    const float centerX = x + w * 0.5f;

    // ── Draw background grid (dotted style) ──────────────────────────────────
    for (float db = 6.0f; db <= kMaxGR; db += 6.0f)
    {
        float offset = (db / kMaxGR) * (w * 0.5f);
        // Dotted vertical lines
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        for (float dy = y; dy < y + h; dy += 4.0f)
        {
            g.fillRect(centerX - offset, dy, 1.0f, 2.0f);
            g.fillRect(centerX + offset, dy, 1.0f, 2.0f);
        }
    }

    const float rowSpacing = 3.0f;
    const float numRows = 4.0f;
    const float rowH = (h - rowSpacing * (numRows - 1.0f)) / numRows;

    auto drawRow = [&](float rowY, float grDb, juce::Colour col, const juce::String& name, bool isLeveler)
    {
        // ── Compute bar geometry ─────────────────────────────────────────────
        float barW;
        float barX;
        if (isLeveler)
        {
            float clamped = std::clamp(grDb, -kMaxGR, kMaxGR);
            barW = std::abs(clamped) / kMaxGR * (w * 0.5f);
            barX = (clamped >= 0.0f) ? centerX : centerX - barW;
        }
        else
        {
            barW = grToWidth(grDb, w);
            barX = centerX - barW;
        }

        // ── Bar with horizontal gradient glow ────────────────────────────────
        if (barW > 0.5f)
        {
            // Rounded bar
            g.setColour(col.withAlpha(0.75f));
            g.fillRoundedRectangle(barX, rowY + 0.5f, barW, rowH - 1.0f, 3.0f);

            // Glow: horizontal gradient from center outward
            bool goesRight = (isLeveler && grDb >= 0.0f);
            float glowStart = goesRight ? barX : barX + barW;
            float glowEnd   = goesRight ? barX + barW : barX;

            juce::ColourGradient glow(col.withAlpha(0.4f),  glowStart, rowY,
                                      col.withAlpha(0.05f), glowEnd,   rowY, false);
            g.setGradientFill(glow);
            g.fillRoundedRectangle(barX, rowY + 0.5f, barW, rowH - 1.0f, 3.0f);

            // Bright edge at the tip
            float edgeX = goesRight ? barX + barW - 2.0f : barX;
            g.setColour(col.withAlpha(0.5f));
            g.fillRoundedRectangle(edgeX, rowY + 1.0f, 2.0f, rowH - 2.0f, 1.0f);
        }

        // ── Pill badge label ─────────────────────────────────────────────────
        {
            g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
            juce::String text = name + "  " + juce::String(grDb, 1) + " dB";

            // Position: right of center for left-going bars, left of center for right-going
            float textX;
            auto justification = juce::Justification::centredLeft;

            if (isLeveler && grDb >= 0.0f)
            {
                textX = centerX - 5.0f;
                justification = juce::Justification::centredRight;
            }
            else
            {
                textX = centerX + 5.0f;
            }

            juce::Rectangle<float> textBounds(x, rowY, w, rowH);

            // Text shadow for readability
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            if (justification == juce::Justification::centredLeft)
                g.drawText(text, textBounds.translated(1, 1).withTrimmedLeft(textX - x), justification, false);
            else
                g.drawText(text, textBounds.translated(1, 1).withTrimmedRight(x + w - textX), justification, false);

            g.setColour(juce::Colour(0xffe0e4f0));
            if (justification == juce::Justification::centredLeft)
                g.drawText(text, textBounds.withTrimmedLeft(textX - x), justification, false);
            else
                g.drawText(text, textBounds.withTrimmedRight(x + w - textX), justification, false);
        }
    };

    drawRow(y, expanderGR.load(), juce::Colour(0xff2d7ad4), "EXP", false);
    drawRow(y + rowH + rowSpacing, compGR.load(), juce::Colour(0xff00d4ff), "COMP", false);
    drawRow(y + (rowH + rowSpacing) * 2.0f, levelerGain.load(), juce::Colour(0xff00e676), "LVL", true);
    drawRow(y + (rowH + rowSpacing) * 3.0f, limiterGR.load(), juce::Colour(0xffff4455), "LIM", false);

    // ── Center line with glow ────────────────────────────────────────────────
    // Soft glow
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.fillRect(centerX - 2.0f, y, 4.0f, h);
    // Crisp line
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawLine(centerX, y, centerX, y + h, 1.0f);

    // ── Ticks at the bottom ──────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.setColour(juce::Colour(0xff505568));
    for (float db = 0.0f; db <= kMaxGR; db += 6.0f)
    {
        const float tickXLeft = centerX - (db / kMaxGR) * (w * 0.5f);
        const float tickXRight = centerX + (db / kMaxGR) * (w * 0.5f);

        if (db > 0.0f)
        {
            g.drawText("-" + juce::String((int)db), (int)(tickXLeft - 10.0f), (int)(y + h - 11.0f), 20, 11, juce::Justification::centredBottom, false);
            g.drawText("+" + juce::String((int)db), (int)(tickXRight - 10.0f), (int)(y + h - 11.0f), 20, 11, juce::Justification::centredBottom, false);
        }
        else
        {
            g.drawText("0", (int)(centerX - 10.0f), (int)(y + h - 11.0f), 20, 11, juce::Justification::centredBottom, false);
        }
    }
}
