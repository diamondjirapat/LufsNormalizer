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

    // Draw background grid
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    for (float db = 0.0f; db <= kMaxGR; db += 6.0f)
    {
        float offset = (db / kMaxGR) * (w * 0.5f);
        g.drawLine(centerX - offset, y, centerX - offset, y + h, 1.0f);
        g.drawLine(centerX + offset, y, centerX + offset, y + h, 1.0f);
    }

    const float rowSpacing = 2.0f;
    const float numRows = 4.0f;
    const float rowH = (h - rowSpacing * (numRows - 1.0f)) / numRows;

    auto drawRow = [&](float rowY, float grDb, juce::Colour col, const juce::String& name, bool isLeveler)
    {
        // Bar background
        juce::Rectangle<float> rowBounds(x, rowY, w, rowH);
        
        // Draw actual bar
        float barW;
        float barX;
        if (isLeveler)
        {
            // Leveler can be positive or negative
            float clamped = std::clamp(grDb, -kMaxGR, kMaxGR);
            barW = std::abs(clamped) / kMaxGR * (w * 0.5f);
            barX = (clamped >= 0.0f) ? centerX : centerX - barW;
        }
        else
        {
            barW = grToWidth(grDb, w);
            barX = centerX - barW; // Expand from center to left
        }

        g.setColour(col.withAlpha(0.8f));
        g.fillRect(barX, rowY, barW, rowH);

        // Label
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        juce::String text = name + " (" + juce::String(grDb, 1) + " dB)";
        
        // Text Position
        float textX = centerX + 5.0f;
        auto justification = juce::Justification::centredLeft;
        
        // Bar goes right text goes left so it doesn't overlap
        if (isLeveler && grDb >= 0.0f)
        {
            textX = centerX - 5.0f;
            justification = juce::Justification::centredRight;
        }
        
        juce::Rectangle<float> textBounds(x, rowY, w, rowH);
        
        // Shadow for text readability
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        if (justification == juce::Justification::centredLeft)
            g.drawText(text, textBounds.translated(1, 1).withTrimmedLeft(textX - x), justification, false);
        else
            g.drawText(text, textBounds.translated(1, 1).withTrimmedRight(x + w - textX), justification, false);
        
        g.setColour(juce::Colours::white);
        if (justification == juce::Justification::centredLeft)
            g.drawText(text, textBounds.withTrimmedLeft(textX - x), justification, false);
        else
            g.drawText(text, textBounds.withTrimmedRight(x + w - textX), justification, false);
    };

    drawRow(y, expanderGR.load(), juce::Colour(0xff0a58ca), "EXP", false);
    drawRow(y + rowH + rowSpacing, compGR.load(), juce::Colour(0xff00d4ff), "COMP", false);
    drawRow(y + (rowH + rowSpacing) * 2.0f, levelerGain.load(), juce::Colour(0xff2ecc71), "LVL", true);
    drawRow(y + (rowH + rowSpacing) * 3.0f, limiterGR.load(), juce::Colour(0xffc92a2a), "LIM", false);

    // Center line (stronger)
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawLine(centerX, y, centerX, y + h, 1.5f);

    // Ticks at the bottom
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colour(0xff808595));
    for (float db = 0.0f; db <= kMaxGR; db += 6.0f)
    {
        const float tickXLeft = centerX - (db / kMaxGR) * (w * 0.5f);
        const float tickXRight = centerX + (db / kMaxGR) * (w * 0.5f);
        
        if (db > 0.0f)
        {
            g.drawText("-" + juce::String((int)db), (int)(tickXLeft - 10.0f), (int)(y + h - 12.0f), 20, 12, juce::Justification::centredBottom, false);
            g.drawText("+" + juce::String((int)db), (int)(tickXRight - 10.0f), (int)(y + h - 12.0f), 20, 12, juce::Justification::centredBottom, false);
        }
        else
        {
            g.drawText("0", (int)(centerX - 10.0f), (int)(y + h - 12.0f), 20, 12, juce::Justification::centredBottom, false);
        }
    }
}
