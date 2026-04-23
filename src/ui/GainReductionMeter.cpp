#include "GainReductionMeter.h"
#include <cmath>
#include <algorithm>

GainReductionMeter::GainReductionMeter()
{
    setOpaque(false);
}

float GainReductionMeter::grToWidth(float grDb, float totalWidth) const noexcept
{
    // grDb is ≤ 0; map 0..-kMaxGR to 0..totalWidth
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

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRoundedRectangle(bounds, 3.0f);

    const float rowH = h / 3.0f;

    // ── Leveler gain (green = positive, orange = negative) ────────────────────
    {
        const float gainDb = levelerGain.load();
        const float barW   = std::abs(gainDb) / kMaxGR * w;
        const float barX   = (gainDb >= 0.0f) ? x + w * 0.5f : x + w * 0.5f - barW;
        g.setColour(gainDb >= 0.0f ? juce::Colour(0xff00cc66) : juce::Colour(0xffff8800));
        g.fillRect(barX, y, barW, rowH - 1.0f);

        g.setFont(juce::Font(8.0f));
        g.setColour(juce::Colours::lightgrey);
        g.drawText("LVL " + juce::String(gainDb, 1) + " dB",
                   (int)x, (int)y, (int)w, (int)rowH,
                   juce::Justification::centred, false);
    }

    // ── Expander GR (blue) ────────────────────────────────────────────────────
    {
        const float grDb = expanderGR.load();
        const float barW = grToWidth(grDb, w);
        g.setColour(juce::Colour(0xff0088ff).withAlpha(0.8f));
        g.fillRect(x, y + rowH, barW, rowH - 1.0f);

        g.setFont(juce::Font(8.0f));
        g.setColour(juce::Colours::lightgrey);
        g.drawText("EXP " + juce::String(grDb, 1) + " dB",
                   (int)x, (int)(y + rowH), (int)w, (int)rowH,
                   juce::Justification::centred, false);
    }

    // ── Limiter GR (red) ─────────────────────────────────────────────────────
    {
        const float grDb = limiterGR.load();
        const float barW = grToWidth(grDb, w);
        g.setColour(juce::Colour(0xffff2244).withAlpha(0.8f));
        g.fillRect(x, y + rowH * 2.0f, barW, rowH - 1.0f);

        g.setFont(juce::Font(8.0f));
        g.setColour(juce::Colours::lightgrey);
        g.drawText("LIM " + juce::String(grDb, 1) + " dB",
                   (int)x, (int)(y + rowH * 2.0f), (int)w, (int)rowH,
                   juce::Justification::centred, false);
    }

    // ── Centre line (0 dB) ────────────────────────────────────────────────────
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawLine(x + w * 0.5f, y, x + w * 0.5f, y + h, 0.5f);

    // ── Scale ticks ───────────────────────────────────────────────────────────
    g.setFont(juce::Font(7.0f));
    g.setColour(juce::Colours::grey);
    for (float db = 0.0f; db <= kMaxGR; db += 6.0f)
    {
        const float tickX = x + (db / kMaxGR) * w;
        g.drawLine(tickX, y + h - 4.0f, tickX, y + h, 0.5f);
        if (db > 0.0f)
            g.drawText("-" + juce::String((int)db),
                       (int)(tickX - 8.0f), (int)(y + h - 12.0f), 16, 10,
                       juce::Justification::centred, false);
    }

    // ── Border ────────────────────────────────────────────────────────────────
    g.setColour(juce::Colours::grey.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}
