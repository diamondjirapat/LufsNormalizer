#include "GainReductionMeter.h"
#include <cmath>
#include <algorithm>

GainReductionMeter::GainReductionMeter()
{
    setOpaque(false);
}

float GainReductionMeter::grToWidth(float grDb, float totalWidth) const noexcept
{
    // grDb is <= 0; map 0..-kMaxGR to 0..totalWidth
    const float clamped = std::clamp(-grDb, 0.0f, kMaxGR);
    return (clamped / kMaxGR) * totalWidth;
}

void GainReductionMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();

    // The container background is drawn by PluginEditor now (panelCol).
    // So we don't need to draw a background here unless we want an inner background.

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
            barW = std::abs(grDb) / kMaxGR * w;
            barX = (grDb >= 0.0f) ? x + w * 0.5f : x + w * 0.5f - barW;
        }
        else
        {
            barW = grToWidth(grDb, w);
            barX = x + w - barW; // Expand from right to left
        }

        g.setColour(col);
        g.fillRect(barX, rowY, barW, rowH);

        // Label
        g.setFont(juce::Font(11.0f).withStyle(juce::Font::bold));
        
        juce::String text = name + " (" + juce::String(grDb, 1) + " dB)";
        
        // Shadow for text readability
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawText(text, rowBounds.translated(1, 1).withTrimmedLeft(w * 0.5f - 50.0f), juce::Justification::centredLeft, false);
        
        g.setColour(juce::Colours::white);
        if (isLeveler && grDb >= 0.0f) g.setColour(col); // text color
        else if (isLeveler) g.setColour(col);
        else g.setColour(col); // tint the text with the bar color
        
        g.drawText(text, rowBounds.withTrimmedLeft(w * 0.5f - 50.0f), juce::Justification::centredLeft, false);
    };

    drawRow(y, expanderGR.load(), juce::Colour(0xff0a58ca), "EXPANDER", false);
    drawRow(y + rowH + rowSpacing, autoGainDb.load(), juce::Colour(0xff00d4ff), "AUTOGAIN", true);
    drawRow(y + (rowH + rowSpacing) * 2.0f, levelerGain.load(), juce::Colour(0xff2ecc71), "LEVELER", true);
    drawRow(y + (rowH + rowSpacing) * 3.0f, limiterGR.load(), juce::Colour(0xffc92a2a), "LIMITER", false);

    // Center line
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(x + w * 0.5f, y, x + w * 0.5f, y + h, 1.0f);

    // Ticks at the bottom
    g.setFont(juce::Font(9.0f));
    g.setColour(juce::Colour(0xff606575));
    for (float db = 0.0f; db <= kMaxGR; db += 6.0f)
    {
        const float tickXLeft = x + w * 0.5f - (db / kMaxGR) * (w * 0.5f);
        const float tickXRight = x + w * 0.5f + (db / kMaxGR) * (w * 0.5f);
        
        if (db > 0.0f)
        {
            g.drawText("-" + juce::String((int)db), (int)(tickXLeft - 10.0f), (int)(y + h - 10.0f), 20, 10, juce::Justification::centred, false);
            g.drawText("+" + juce::String((int)db), (int)(tickXRight - 10.0f), (int)(y + h - 10.0f), 20, 10, juce::Justification::centred, false);
        }
    }
}
