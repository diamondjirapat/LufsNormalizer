#include "PluginEditor.h"
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
// DarkLookAndFeel
// ═════════════════════════════════════════════════════════════════════════════
DarkLookAndFeel::DarkLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,           juce::Colour(0xff00c8ff));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00c8ff));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333355));
    setColour(juce::Slider::textBoxTextColourId,     juce::Colours::lightgrey);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a2e));
    setColour(juce::Slider::textBoxOutlineColourId,  juce::Colour(0xff333355));
    setColour(juce::Label::textColourId,             juce::Colours::lightgrey);
    setColour(juce::ToggleButton::textColourId,      juce::Colours::lightgrey);
    setColour(juce::ComboBox::backgroundColourId,    juce::Colour(0xff1a1a2e));
    setColour(juce::ComboBox::textColourId,          juce::Colours::lightgrey);
    setColour(juce::ComboBox::outlineColourId,       juce::Colour(0xff333355));
    setColour(juce::TextButton::buttonColourId,      juce::Colour(0xff1a1a2e));
    setColour(juce::TextButton::textColourOffId,     juce::Colours::lightgrey);
    setColour(juce::PopupMenu::backgroundColourId,   juce::Colour(0xff1a1a2e));
    setColour(juce::PopupMenu::textColourId,         juce::Colours::lightgrey);
}

void DarkLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    const float radius = (float)std::min(width, height) * 0.5f - 4.0f;
    const float centreX = (float)x + (float)width  * 0.5f;
    const float centreY = (float)y + (float)height * 0.5f;
    const float angle   = startAngle + sliderPos * (endAngle - startAngle);

    // Track arc
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius, radius,
                        0.0f, startAngle, endAngle, true);
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(track, juce::PathStrokeType(3.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Fill arc
    juce::Path fill;
    fill.addCentredArc(centreX, centreY, radius, radius,
                       0.0f, startAngle, angle, true);
    g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(fill, juce::PathStrokeType(3.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    const float knobR = radius * 0.65f;
    g.setColour(juce::Colour(0xff252540));
    g.fillEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f);

    g.setColour(juce::Colour(0xff333355));
    g.drawEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

    // Pointer line
    const float pointerLen = knobR * 0.7f;
    const float px = centreX + std::sin(angle) * pointerLen;
    const float py = centreY - std::cos(angle) * pointerLen;
    g.setColour(findColour(juce::Slider::thumbColourId));
    g.drawLine(centreX, centreY, px, py, 2.0f);
}

void DarkLookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& button,
    bool /*highlighted*/, bool /*down*/)
{
    const bool on = button.getToggleState();
    const auto bounds = button.getLocalBounds().toFloat();

    // Pill background
    const float h = bounds.getHeight() * 0.6f;
    const float w = h * 1.8f;
    const float bx = bounds.getX() + 4.0f;
    const float by = bounds.getCentreY() - h * 0.5f;

    g.setColour(on ? juce::Colour(0xff00c8ff) : juce::Colour(0xff333355));
    g.fillRoundedRectangle(bx, by, w, h, h * 0.5f);

    // Thumb
    const float thumbR = h * 0.45f;
    const float thumbX = on ? bx + w - thumbR * 2.0f - 2.0f : bx + 2.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX, by + (h - thumbR * 2.0f) * 0.5f, thumbR * 2.0f, thumbR * 2.0f);

    // Label
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(on ? juce::Colours::white : juce::Colours::grey);
    g.drawText(button.getButtonText(),
               (int)(bx + w + 6.0f), 0,
               (int)(bounds.getWidth() - w - 10.0f), (int)bounds.getHeight(),
               juce::Justification::centredLeft, false);
}

juce::Font DarkLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(10.0f);
}

// ═════════════════════════════════════════════════════════════════════════════
// LabelledKnob
// ═════════════════════════════════════════════════════════════════════════════
void LabelledKnob::setup(juce::Component* parent, const juce::String& labelText)
{
    slider.setScrollWheelEnabled(true);
    parent->addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(10.0f));
    parent->addAndMakeVisible(label);
}

void LabelledKnob::setBounds(juce::Rectangle<int> area)
{
    const int labelH = 14;
    label .setBounds(area.removeFromBottom(labelH));
    slider.setBounds(area);
}

// ═════════════════════════════════════════════════════════════════════════════
// LufsNormalizerEditor
// ═════════════════════════════════════════════════════════════════════════════
LufsNormalizerEditor::LufsNormalizerEditor(LufsNormalizerProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&laf);
    setSize(820, 560);

    // ── Meters ────────────────────────────────────────────────────────────────
    addAndMakeVisible(lufsDisplay);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(levelHistory);

    // ── Leveler section ───────────────────────────────────────────────────────
    addAndMakeVisible(levelerToggle);
    targetKnob .setup(this, "Target");
    attackKnob .setup(this, "Attack");
    releaseKnob.setup(this, "Release");
    maxGainKnob.setup(this, "Max Gain");

    // ── Expander section ──────────────────────────────────────────────────────
    addAndMakeVisible(expanderToggle);
    expThreshKnob .setup(this, "Threshold");
    expRatioKnob  .setup(this, "Ratio");
    expAttackKnob .setup(this, "Attack");
    expReleaseKnob.setup(this, "Release");
    expKneeKnob   .setup(this, "Knee");

    // ── Limiter section ───────────────────────────────────────────────────────
    addAndMakeVisible(limiterToggle);
    ceilingKnob.setup(this, "Ceiling");

    // ── Lookahead section ─────────────────────────────────────────────────────
    addAndMakeVisible(lookaheadToggle);
    lookaheadKnob.setup(this, "Time");

    // ── Preset combo ─────────────────────────────────────────────────────────
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetCombo.addItem(processor.getProgramName(i), i + 1);
    presetCombo.setSelectedId(processor.getCurrentProgram() + 1,
                              juce::dontSendNotification);
    presetCombo.onChange = [this]
    {
        processor.setCurrentProgram(presetCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(presetCombo);

    // ── Reset button ──────────────────────────────────────────────────────────
    resetButton.onClick = [this] { processor.resetIntegrated(); };
    addAndMakeVisible(resetButton);

    // ── LUFS readout labels ───────────────────────────────────────────────────
    for (auto* lbl : { &momentaryLabel, &shortTermLabel, &integratedLabel })
    {
        lbl->setJustificationType(juce::Justification::centred);
        lbl->setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(*lbl);
    }

    buildAttachments();
    startTimerHz(20); // 20 fps meter refresh
}

LufsNormalizerEditor::~LufsNormalizerEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ── buildAttachments ─────────────────────────────────────────────────────────
void LufsNormalizerEditor::buildAttachments()
{
    auto& apvts = processor.getAPVTS();

    targetAtt   = std::make_unique<SliderAttachment>(apvts, ParamID::TARGET_LUFS,  targetKnob.slider);
    attackAtt   = std::make_unique<SliderAttachment>(apvts, ParamID::ATTACK_MS,    attackKnob.slider);
    releaseAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::RELEASE_MS,   releaseKnob.slider);
    maxGainAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::MAX_GAIN_DB,  maxGainKnob.slider);

    expThreshAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_THRESHOLD, expThreshKnob.slider);
    expRatioAtt   = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_RATIO,     expRatioKnob.slider);
    expAttackAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_ATTACK,    expAttackKnob.slider);
    expReleaseAtt = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_RELEASE,   expReleaseKnob.slider);
    expKneeAtt    = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_KNEE,      expKneeKnob.slider);

    ceilingAtt     = std::make_unique<SliderAttachment>(apvts, ParamID::LIMITER_CEILING,  ceilingKnob.slider);
    lookaheadMsAtt = std::make_unique<SliderAttachment>(apvts, ParamID::LOOKAHEAD_MS,     lookaheadKnob.slider);

    levelerOnAtt   = std::make_unique<ButtonAttachment>(apvts, ParamID::LEVELER_ENABLED,  levelerToggle);
    expanderOnAtt  = std::make_unique<ButtonAttachment>(apvts, ParamID::EXP_ENABLED,      expanderToggle);
    limiterOnAtt   = std::make_unique<ButtonAttachment>(apvts, ParamID::LIMITER_ENABLED,  limiterToggle);
    lookaheadOnAtt = std::make_unique<ButtonAttachment>(apvts, ParamID::LOOKAHEAD_ENABLED, lookaheadToggle);
}

// ── resized ───────────────────────────────────────────────────────────────────
void LufsNormalizerEditor::resized()
{
    layoutComponents();
}

void LufsNormalizerEditor::layoutComponents()
{
    auto area = getLocalBounds().reduced(8);

    // ── Top bar: preset + reset ───────────────────────────────────────────────
    auto topBar = area.removeFromTop(28);
    presetLabel.setBounds(topBar.removeFromLeft(50));
    presetCombo.setBounds(topBar.removeFromLeft(130));
    topBar.removeFromLeft(8);
    resetButton.setBounds(topBar.removeFromLeft(110).reduced(0, 2));

    area.removeFromTop(6);

    // ── Left column: LUFS meter ───────────────────────────────────────────────
    auto leftCol = area.removeFromLeft(80);
    lufsDisplay.setBounds(leftCol.removeFromTop(leftCol.getHeight() - 60));

    // LUFS readout labels below meter
    momentaryLabel .setBounds(leftCol.removeFromTop(20));
    shortTermLabel .setBounds(leftCol.removeFromTop(20));
    integratedLabel.setBounds(leftCol.removeFromTop(20));

    area.removeFromLeft(6);

    // ── Right column: controls ────────────────────────────────────────────────
    auto rightArea = area;

    // History graph at top
    auto historyArea = rightArea.removeFromTop(100);
    levelHistory.setBounds(historyArea);
    rightArea.removeFromTop(6);

    // GR meter
    auto grArea = rightArea.removeFromTop(70);
    grMeter.setBounds(grArea);
    rightArea.removeFromTop(6);

    // ── Control sections ──────────────────────────────────────────────────────
    // Split remaining area into two rows
    auto topControls = rightArea.removeFromTop(rightArea.getHeight() / 2 - 3);
    rightArea.removeFromTop(6);
    auto botControls = rightArea;

    // Row 1: Leveler (left) | Expander (right)
    {
        auto levelerArea = topControls.removeFromLeft(topControls.getWidth() / 2 - 3);
        topControls.removeFromLeft(6);
        auto expanderArea = topControls;

        // Leveler section
        levelerToggle.setBounds(levelerArea.removeFromTop(22));
        levelerArea.removeFromTop(4);
        const int knobW = levelerArea.getWidth() / 4;
        targetKnob .setBounds(levelerArea.removeFromLeft(knobW));
        attackKnob .setBounds(levelerArea.removeFromLeft(knobW));
        releaseKnob.setBounds(levelerArea.removeFromLeft(knobW));
        maxGainKnob.setBounds(levelerArea);

        // Expander section
        expanderToggle.setBounds(expanderArea.removeFromTop(22));
        expanderArea.removeFromTop(4);
        const int expKnobW = expanderArea.getWidth() / 5;
        expThreshKnob .setBounds(expanderArea.removeFromLeft(expKnobW));
        expRatioKnob  .setBounds(expanderArea.removeFromLeft(expKnobW));
        expAttackKnob .setBounds(expanderArea.removeFromLeft(expKnobW));
        expReleaseKnob.setBounds(expanderArea.removeFromLeft(expKnobW));
        expKneeKnob   .setBounds(expanderArea);
    }

    // Row 2: Limiter (left) | Lookahead (right)
    {
        auto limiterArea    = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
        botControls.removeFromLeft(6);
        auto lookaheadArea  = botControls;

        // Limiter
        limiterToggle.setBounds(limiterArea.removeFromTop(22));
        limiterArea.removeFromTop(4);
        ceilingKnob.setBounds(limiterArea.removeFromLeft(limiterArea.getWidth() / 3));

        // Lookahead
        lookaheadToggle.setBounds(lookaheadArea.removeFromTop(22));
        lookaheadArea.removeFromTop(4);
        lookaheadKnob.setBounds(lookaheadArea.removeFromLeft(lookaheadArea.getWidth() / 3));
    }
}

// ── paint ─────────────────────────────────────────────────────────────────────
void LufsNormalizerEditor::paint(juce::Graphics& g)
{
    // Background gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff0d0d1a), 0.0f, 0.0f,
        juce::Colour(0xff1a1a2e), (float)getWidth(), (float)getHeight(),
        false));
    g.fillAll();

    // Section backgrounds
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(34); // top bar
    area.removeFromLeft(86); // meter column

    auto historyArea = area.removeFromTop(100);
    area.removeFromTop(6);
    auto grArea = area.removeFromTop(70);
    area.removeFromTop(6);

    auto topControls = area.removeFromTop(area.getHeight() / 2 - 3);
    area.removeFromTop(6);
    auto botControls = area;

    auto levelerArea  = topControls.removeFromLeft(topControls.getWidth() / 2 - 3);
    topControls.removeFromLeft(6);
    auto expanderArea = topControls;

    auto limiterArea   = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
    botControls.removeFromLeft(6);
    auto lookaheadArea = botControls;

    drawSectionBackground(g, historyArea,  "LUFS History",  juce::Colour(0xff0d1a2e));
    drawSectionBackground(g, grArea,       "Gain Reduction", juce::Colour(0xff0d1a2e));
    drawSectionBackground(g, levelerArea,  "LUFS Leveler",  juce::Colour(0xff0d1a1a));
    drawSectionBackground(g, expanderArea, "Expander",      juce::Colour(0xff0d0d1a));
    drawSectionBackground(g, limiterArea,  "True Peak Limiter", juce::Colour(0xff1a0d0d));
    drawSectionBackground(g, lookaheadArea,"Lookahead",     juce::Colour(0xff0d1a0d));

    // Plugin title
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff00c8ff));
    g.drawText("LUFS NORMALIZER",
               getLocalBounds().removeFromTop(28).withTrimmedLeft(200),
               juce::Justification::centredLeft, false);

    g.setFont(juce::Font(9.0f));
    g.setColour(juce::Colours::grey);
    g.drawText("v1.0  |  EBU R128",
               getLocalBounds().removeFromTop(28).withTrimmedLeft(360),
               juce::Justification::centredLeft, false);
}

void LufsNormalizerEditor::drawSectionBackground(juce::Graphics& g,
    juce::Rectangle<int> area, const juce::String& title,
    juce::Colour colour) const
{
    g.setColour(colour.withAlpha(0.6f));
    g.fillRoundedRectangle(area.toFloat(), 6.0f);
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);

    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(juce::Colours::grey.withAlpha(0.7f));
    g.drawText(title, area.withHeight(14), juce::Justification::centredTop, false);
}

// ── timerCallback ─────────────────────────────────────────────────────────────
void LufsNormalizerEditor::timerCallback()
{
    const float mom = processor.getMomentaryLUFS();
    const float st  = processor.getShortTermLUFS();
    const float intg = processor.getIntegratedLUFS();

    // Update LUFS display
    lufsDisplay.setMomentaryLUFS (mom);
    lufsDisplay.setShortTermLUFS (st);
    lufsDisplay.setIntegratedLUFS(intg);
    lufsDisplay.setTargetLUFS(
        processor.getAPVTS().getRawParameterValue(ParamID::TARGET_LUFS)->load());
    lufsDisplay.repaint();

    // Update GR meter
    grMeter.setExpanderGR (processor.getExpanderGrDb());
    grMeter.setLimiterGR  (processor.getLimiterGrDb());
    grMeter.setLevelerGain(processor.getLevelerGainDb());
    grMeter.repaint();

    // Update history (push every other tick → ~10 Hz)
    static int histTick = 0;
    if (++histTick >= 2)
    {
        histTick = 0;
        levelHistory.pushValue(st);
        levelHistory.setTargetLUFS(
            processor.getAPVTS().getRawParameterValue(ParamID::TARGET_LUFS)->load());
    }

    // Update readout labels
    auto fmt = [](float v) -> juce::String
    {
        if (v <= -140.0f) return "-inf";
        return juce::String(v, 1);
    };

    momentaryLabel .setText("M: "  + fmt(mom)  + " LU", juce::dontSendNotification);
    shortTermLabel .setText("ST: " + fmt(st)   + " LU", juce::dontSendNotification);
    integratedLabel.setText("I: "  + fmt(intg) + " LU", juce::dontSendNotification);

    // Colour-code integrated label
    const float target = processor.getAPVTS().getRawParameterValue(ParamID::TARGET_LUFS)->load();
    const float diff   = intg - target;
    juce::Colour intgColour = juce::Colours::lightgrey;
    if (intg > -140.0f)
    {
        if      (std::abs(diff) < 1.0f) intgColour = juce::Colour(0xff00ff88);
        else if (std::abs(diff) < 3.0f) intgColour = juce::Colour(0xffffcc00);
        else                            intgColour = juce::Colour(0xffff4444);
    }
    integratedLabel.setColour(juce::Label::textColourId, intgColour);
}
