#include "LufsDisplay.h"
#include <cmath>
#include <algorithm>

LufsDisplay::LufsDisplay()
{
    setOpaque(false);
}

void LufsDisplay::resized()
{
    // Leave room for scale labels on the left
    meterArea = getLocalBounds().reduced(4).withTrimmedLeft(24);
}

float LufsDisplay::dbToY(float db, float height) const noexcept
{
    const float clamped = std::clamp(db, kMinDb, kMaxDb);
    const float norm    = (kMaxDb - clamped) / (kMaxDb - kMinDb);
    return norm * height;
}

void LufsDisplay::paint(juce::Graphics& g)
{
    const float w = (float)meterArea.getWidth();
    const float h = (float)meterArea.getHeight();
    const float x = (float)meterArea.getX();
    const float y = (float)meterArea.getY();

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xff1c1d24));
    g.fillRoundedRectangle(meterArea.toFloat(), 4.0f);

    // ── Colour scheme based on mode ───────────────────────────────────────────
    const bool isInput = (mode == Input);
    const juce::Colour barColour   = isInput ? juce::Colour(0xff3a8fd4)  // blue
                                             : juce::Colour(0xff2ecc71); // green
    const juce::Colour peakColour  = isInput ? juce::Colour(0xff5bb5ff)  // bright blue
                                             : juce::Colour(0xff5dff9e); // bright green
    const juce::Colour rmsColour   = isInput ? juce::Colour(0xcc2a6fa8)  // darker blue
                                             : juce::Colour(0xcc1fa85b); // darker green
    const juce::Colour glowColour  = isInput ? juce::Colour(0xff00a0ff)
                                             : juce::Colour(0xff00ff88);

    // ── Gradient background zones (subtle) ────────────────────────────────────
    {
        // Red zone: above -6 dB
        const float redBot = y + dbToY(-6.0f, h);
        g.setColour(juce::Colour(0xff4a1a1a).withAlpha(0.25f));
        g.fillRect(x, y, w, redBot - y);

        // Yellow zone: -6 to -18
        const float yellowBot = y + dbToY(-18.0f, h);
        g.setColour(juce::Colour(0xff4a4a1a).withAlpha(0.15f));
        g.fillRect(x, redBot, w, yellowBot - redBot);

        // Green zone: below -18
        g.setColour(juce::Colour(0xff1a3a1a).withAlpha(0.15f));
        g.fillRect(x, yellowBot, w, (y + h) - yellowBot);
    }

    // ── Read current values ───────────────────────────────────────────────────
    const float curPeakDb = peakDb.load();
    const float curRmsDb  = rmsDb.load();

    // ── Peak-hold logic ───────────────────────────────────────────────────────
    if (curPeakDb > peakHoldDb)
    {
        peakHoldDb = curPeakDb;
        peakHoldTicks = kPeakHoldFrames;
    }
    else if (peakHoldTicks > 0)
    {
        --peakHoldTicks;
    }
    else
    {
        peakHoldDb -= kPeakFallRate;
        if (peakHoldDb < kMinDb) peakHoldDb = kMinDb;
    }

    // ── RMS bar (wider, semi-transparent fill) ────────────────────────────────
    if (curRmsDb > kMinDb)
    {
        const float barTop = y + dbToY(curRmsDb, h);
        const float barH   = (y + h) - barTop;

        // Gradient fill for the RMS bar
        juce::ColourGradient grad(rmsColour.withAlpha(0.6f), x, barTop,
                                  rmsColour.withAlpha(0.2f), x, y + h, false);
        g.setGradientFill(grad);
        g.fillRect(x + 1.0f, barTop, w - 2.0f, barH);
    }

    // ── Peak bar (narrow, bright, centred) ────────────────────────────────────
    if (curPeakDb > kMinDb)
    {
        const float barTop = y + dbToY(curPeakDb, h);
        const float barH   = (y + h) - barTop;
        const float peakW  = w * 0.4f;
        const float peakX  = x + (w - peakW) * 0.5f;

        juce::ColourGradient grad(peakColour.withAlpha(0.9f), peakX, barTop,
                                  barColour.withAlpha(0.5f), peakX, y + h, false);
        g.setGradientFill(grad);
        g.fillRect(peakX, barTop, peakW, barH);
    }

    // ── Peak-hold tick ────────────────────────────────────────────────────────
    if (peakHoldDb > kMinDb)
    {
        const float tickY = y + dbToY(peakHoldDb, h);
        g.setColour(glowColour.withAlpha(0.9f));
        g.fillRect(x + 2.0f, tickY - 1.0f, w - 4.0f, 2.0f);
    }

    // ── Target line (only for output meter) ───────────────────────────────────
    if (!isInput)
    {
        const float tgtLufs = targetLUFS.load();
        // Map LUFS target to our dBFS scale (approximate: LUFS ≈ dBFS for calibrated signals)
        const float tgtY = y + dbToY(tgtLufs, h);
        if (tgtY > y && tgtY < y + h)
        {
            g.setColour(juce::Colour(0xffffcc00).withAlpha(0.7f));
            g.drawLine(x, tgtY, x + w, tgtY, 1.5f);
        }
    }

    // ── Scale labels ─────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.setColour(juce::Colours::lightgrey.withAlpha(0.7f));

    const float scaleX = (float)getLocalBounds().getX();
    for (float db = kMaxDb; db >= kMinDb; db -= 6.0f)
    {
        const float labelY = y + dbToY(db, h);
        g.drawText(juce::String((int)db),
                   (int)scaleX, (int)(labelY - 5.0f), 22, 10,
                   juce::Justification::centredRight, false);

        // Tick mark
        g.setColour(juce::Colours::grey.withAlpha(0.25f));
        g.drawLine(x, labelY, x + w, labelY, 0.5f);
        g.setColour(juce::Colours::lightgrey.withAlpha(0.7f));
    }

    // ── Mode label at bottom ──────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.setColour(barColour);
    g.drawText(isInput ? "IN" : "OUT",
               (int)x, (int)(y + h - 16.0f), (int)w, 14,
               juce::Justification::centred, false);

    // ── dB readout at top ─────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.setColour(juce::Colours::white);
    juce::String dbText = (curPeakDb > -99.0f) ? juce::String(curPeakDb, 1) + " dB" : "-inf";
    g.drawText(dbText, (int)x, (int)y + 2, (int)w, 12,
               juce::Justification::centred, false);
}
