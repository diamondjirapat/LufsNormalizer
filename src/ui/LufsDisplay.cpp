#include "LufsDisplay.h"
#include <cmath>
#include <algorithm>

LufsDisplay::LufsDisplay()
{
    setOpaque(false);
}

void LufsDisplay::setMomentaryLUFS (float lufs) noexcept { momentaryLUFS .store(lufs); }
void LufsDisplay::setShortTermLUFS (float lufs) noexcept { shortTermLUFS .store(lufs); }
void LufsDisplay::setIntegratedLUFS(float lufs) noexcept { integratedLUFS.store(lufs); }
void LufsDisplay::setTargetLUFS    (float lufs) noexcept { targetLUFS    .store(lufs); }

void LufsDisplay::resized()
{
    // Leave room for scale labels on the left
    meterArea = getLocalBounds().reduced(4).withTrimmedLeft(28);
}

float LufsDisplay::lufsToY(float lufs, float height) const noexcept
{
    const float clamped = std::clamp(lufs, kMinLUFS, kMaxLUFS);
    const float norm    = (kMaxLUFS - clamped) / (kMaxLUFS - kMinLUFS);
    return norm * height;
}

void LufsDisplay::paint(juce::Graphics& g)
{
    const float w = (float)meterArea.getWidth();
    const float h = (float)meterArea.getHeight();
    const float x = (float)meterArea.getX();
    const float y = (float)meterArea.getY();

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xff1c1d24)); // lighter panel grey
    g.fillRoundedRectangle(meterArea.toFloat(), 4.0f);

    // ── Colour gradient zones ─────────────────────────────────────────────────
    // Green zone: below target, Yellow: near target, Red: above target
    const float targetY = y + lufsToY(targetLUFS.load(), h);

    // Draw coloured background bands
    const float greenTop = y + lufsToY(-10.0f, h);
    const float yellowTop = targetY - 2.0f;

    g.setColour(juce::Colour(0xff2d4a2d).withAlpha(0.3f));
    g.fillRect(x, greenTop, w, h - (greenTop - y));

    g.setColour(juce::Colour(0xff4a4a1a).withAlpha(0.3f));
    g.fillRect(x, yellowTop, w, greenTop - yellowTop);

    g.setColour(juce::Colour(0xff4a1a1a).withAlpha(0.3f));
    g.fillRect(x, y, w, yellowTop - y);

    // ── Short-term bar (wider, semi-transparent) ──────────────────────────────
    const float stLufs = shortTermLUFS.load();
    if (stLufs > kMinLUFS)
    {
        const float barTop = y + lufsToY(stLufs, h);
        const float barH   = h - (barTop - y);
        g.setColour(juce::Colour(0x8800d4ff));
        g.fillRect(x, barTop, w, barH);
    }

    // ── Momentary bar (narrow, bright) ───────────────────────────────────────
    const float momLufs = momentaryLUFS.load();
    if (momLufs > kMinLUFS)
    {
        const float barTop = y + lufsToY(momLufs, h);
        const float barH   = h - (barTop - y);
        g.setColour(juce::Colour(0xcc2ecc71));
        g.fillRect(x + w * 0.3f, barTop, w * 0.4f, barH);
    }

    // ── Integrated LUFS tick ──────────────────────────────────────────────────
    const float intLufs = integratedLUFS.load();
    if (intLufs > kMinLUFS - 1.0f)
    {
        const float tickY = y + lufsToY(intLufs, h);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillRect(x, tickY - 1.0f, w, 2.0f);

        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(juce::Colours::white);
        g.drawText(juce::String(intLufs, 1) + " I",
                   (int)x, (int)(tickY - 10.0f), (int)w, 10,
                   juce::Justification::centred, false);
    }

    // ── Target line ───────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xffffcc00));
    g.drawLine(x, targetY, x + w, targetY, 1.5f);

    // ── Scale labels ─────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colours::lightgrey);

    const float scaleX = (float)getLocalBounds().getX();
    for (float lufs = kMaxLUFS; lufs >= kMinLUFS; lufs -= 5.0f)
    {
        const float labelY = y + lufsToY(lufs, h);
        g.drawText(juce::String((int)lufs),
                   (int)scaleX, (int)(labelY - 5.0f), 26, 10,
                   juce::Justification::centredRight, false);

        // Tick mark
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.drawLine(x, labelY, x + w, labelY, 0.5f);
        g.setColour(juce::Colours::lightgrey);
    }

    // Border handled by parent

    // ── Labels ────────────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xff00d4ff));
    g.drawText("ST", (int)x, (int)(y + h - 14.0f), 14, 12,
               juce::Justification::left, false);

    g.setColour(juce::Colour(0xff2ecc71));
    g.drawText("M", (int)(x + w * 0.3f), (int)(y + h - 14.0f), 14, 12,
               juce::Justification::left, false);
}
