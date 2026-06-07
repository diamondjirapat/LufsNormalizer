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

void LufsDisplay::updatePeakHold()
{
    const float curPeakDb = peakDb.load();
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
}

void LufsDisplay::paint(juce::Graphics& g)
{
    const float w = (float)meterArea.getWidth();
    const float h = (float)meterArea.getHeight();
    const float x = (float)meterArea.getX();
    const float y = (float)meterArea.getY();

    // ── Background with subtle radial gradient ────────────────────────────────
    {
        g.setColour(juce::Colour(0xff0f1018));
        g.fillRoundedRectangle(meterArea.toFloat(), 5.0f);

        // Radial-like gradient (simulate with vertical gradient from center)
        juce::ColourGradient radial(
            juce::Colour(0xff181b28), x + w * 0.5f, y + h * 0.4f,
            juce::Colour(0xff0b0c14), x + w * 0.5f, y + h, false);
        g.setGradientFill(radial);
        g.fillRoundedRectangle(meterArea.toFloat().reduced(1.0f), 4.0f);
    }

    // ── Colour scheme based on mode ───────────────────────────────────────────
    const bool isInput = (mode == Input);
    const juce::Colour barColour   = isInput ? juce::Colour(0xff3a8fd4)
                                             : juce::Colour(0xff00e676);
    const juce::Colour peakColour  = isInput ? juce::Colour(0xff5bb5ff)
                                             : juce::Colour(0xff69ffaa);
    const juce::Colour rmsColour   = isInput ? juce::Colour(0xcc2a6fa8)
                                             : juce::Colour(0xcc1aaa5e);
    const juce::Colour glowColour  = isInput ? juce::Colour(0xff00a0ff)
                                             : juce::Colour(0xff00ff88);

    // ── Gradient background zones (subtle) ────────────────────────────────────
    {
        // Red zone: above -6 dB
        const float redBot = y + dbToY(-6.0f, h);
        g.setColour(juce::Colour(0xff3a1018).withAlpha(0.20f));
        g.fillRect(x + 1.0f, y + 1.0f, w - 2.0f, redBot - y - 1.0f);

        // Yellow zone: -6 to -18
        const float yellowBot = y + dbToY(-18.0f, h);
        g.setColour(juce::Colour(0xff3a3818).withAlpha(0.12f));
        g.fillRect(x + 1.0f, redBot, w - 2.0f, yellowBot - redBot);

        // Green zone: below -18
        g.setColour(juce::Colour(0xff18301a).withAlpha(0.12f));
        g.fillRect(x + 1.0f, yellowBot, w - 2.0f, (y + h) - yellowBot - 1.0f);
    }

    // ── Read current values ───────────────────────────────────────────────────
    const float curPeakDb = peakDb.load();
    const float curRmsDb  = rmsDb.load();

    // ── RMS bar (wider, gradient fill with glow edge) ─────────────────────────
    if (curRmsDb > kMinDb)
    {
        const float barTop = y + dbToY(curRmsDb, h);
        const float barH   = (y + h) - barTop;

        // Multi-stop gradient fill
        juce::ColourGradient grad(rmsColour.withAlpha(0.7f), x, barTop,
                                  rmsColour.withAlpha(0.15f), x, y + h, false);
        grad.addColour(0.3, rmsColour.withAlpha(0.5f));
        g.setGradientFill(grad);
        g.fillRect(x + 2.0f, barTop, w - 4.0f, barH);

        // Glow edge at top of RMS bar
        g.setColour(glowColour.withAlpha(0.20f));
        g.fillRect(x + 2.0f, barTop, w - 4.0f, 3.0f);
    }

    // ── Peak bar (narrow, bright, centred, with inner highlight) ──────────────
    if (curPeakDb > kMinDb)
    {
        const float barTop = y + dbToY(curPeakDb, h);
        const float barH   = (y + h) - barTop;
        const float peakW  = w * 0.36f;
        const float peakX  = x + (w - peakW) * 0.5f;

        juce::ColourGradient grad(peakColour.withAlpha(0.85f), peakX, barTop,
                                  barColour.withAlpha(0.35f), peakX, y + h, false);
        g.setGradientFill(grad);
        g.fillRect(peakX, barTop, peakW, barH);

        // Inner highlight stripe
        const float hlW = peakW * 0.25f;
        const float hlX = peakX + (peakW - hlW) * 0.5f;
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRect(hlX, barTop, hlW, barH);
    }

    // ── Peak-hold tick — triple-layer glow ────────────────────────────────────
    if (peakHoldDb > kMinDb)
    {
        const float tickY = y + dbToY(peakHoldDb, h);
        // Wide soft glow
        g.setColour(glowColour.withAlpha(0.15f));
        g.fillRect(x + 2.0f, tickY - 3.0f, w - 4.0f, 6.0f);
        // Medium glow
        g.setColour(glowColour.withAlpha(0.45f));
        g.fillRect(x + 2.0f, tickY - 1.5f, w - 4.0f, 3.0f);
        // Crisp core
        g.setColour(glowColour.withAlpha(0.9f));
        g.fillRect(x + 2.0f, tickY - 0.5f, w - 4.0f, 1.0f);
    }

    // ── Target line (only for output meter) — dashed with glow ────────────────
    if (!isInput)
    {
        const float tgtLufs = targetLUFS.load();
        const float tgtY = y + dbToY(tgtLufs, h);
        if (tgtY > y && tgtY < y + h)
        {
            // Glow behind the line
            g.setColour(juce::Colour(0xffffab00).withAlpha(0.15f));
            g.fillRect(x, tgtY - 2.0f, w, 4.0f);

            // Dashed line
            g.setColour(juce::Colour(0xffffab00).withAlpha(0.75f));
            const float dashLen = 4.0f;
            const float gapLen  = 3.0f;
            float cx = x;
            while (cx < x + w)
            {
                float endX = std::min(cx + dashLen, x + w);
                g.drawLine(cx, tgtY, endX, tgtY, 1.2f);
                cx += dashLen + gapLen;
            }
        }
    }

    // ── Scale labels ─────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(7.5f)));
    g.setColour(juce::Colour(0xff5a5e70));

    const float scaleX = (float)getLocalBounds().getX();
    for (float db = kMaxDb; db >= kMinDb; db -= 6.0f)
    {
        const float labelY = y + dbToY(db, h);
        g.setColour(juce::Colour(0xff5a5e70));
        g.drawText(juce::String((int)db),
                   (int)scaleX, (int)(labelY - 5.0f), 22, 10,
                   juce::Justification::centredRight, false);

        // Grid tick
        g.setColour(juce::Colour(0xff252830));
        g.drawLine(x, labelY, x + w, labelY, 0.5f);
    }

    // ── Mode label at bottom (pill style) ─────────────────────────────────────
    {
        const juce::String modeText = isInput ? "IN" : "OUT";
        const float pillW = isInput ? 22.0f : 30.0f;
        const float pillH = 13.0f;
        const float pillX = x + (w - pillW) * 0.5f;
        const float pillY = y + h - 18.0f;

        g.setColour(barColour.withAlpha(0.2f));
        g.fillRoundedRectangle(pillX, pillY, pillW, pillH, 4.0f);
        g.setColour(barColour.withAlpha(0.5f));
        g.drawRoundedRectangle(pillX, pillY, pillW, pillH, 4.0f, 0.6f);

        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.setColour(barColour);
        g.drawText(modeText, (int)pillX, (int)pillY, (int)pillW, (int)pillH,
                   juce::Justification::centred, false);
    }

    // ── dB readout at top (pill background) ───────────────────────────────────
    {
        juce::String dbText = (curPeakDb > -99.0f) ? juce::String(curPeakDb, 1) + " dB" : "-inf";

        const float pillW = w - 4.0f;
        const float pillH = 14.0f;
        const float pillX = x + 2.0f;
        const float pillY = y + 2.0f;

        g.setColour(juce::Colour(0x40000000));
        g.fillRoundedRectangle(pillX, pillY, pillW, pillH, 3.0f);

        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.setColour(juce::Colour(0xffe0e4f0));
        g.drawText(dbText, (int)pillX, (int)pillY, (int)pillW, (int)pillH,
                   juce::Justification::centred, false);
    }
}
