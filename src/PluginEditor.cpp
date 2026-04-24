#include "PluginEditor.h"
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
// DarkLookAndFeel
// ═════════════════════════════════════════════════════════════════════════════
DarkLookAndFeel::DarkLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,           juce::Colour(0xff00d4ff));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d4ff));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff252936));
    setColour(juce::Slider::textBoxTextColourId,     juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff161821));
    setColour(juce::Slider::textBoxOutlineColourId,  juce::Colour(0xff3a3d4a));
    setColour(juce::Label::textColourId,             juce::Colour(0xffa0a5b5));
    setColour(juce::ToggleButton::textColourId,      juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId,    juce::Colour(0xff161821));
    setColour(juce::ComboBox::textColourId,          juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId,       juce::Colour(0xff3a3d4a));
    setColour(juce::TextButton::buttonColourId,      juce::Colour(0xff2a2d3e));
    setColour(juce::TextButton::textColourOffId,     juce::Colours::white);
    setColour(juce::PopupMenu::backgroundColourId,   juce::Colour(0xff1a1c23));
    setColour(juce::PopupMenu::textColourId,         juce::Colours::white);
}

void DarkLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& /*slider*/)
{
    const float radius = (float)std::min(width, height) * 0.5f - 6.0f;
    const float centreX = (float)x + (float)width  * 0.5f;
    const float centreY = (float)y + (float)height * 0.5f;
    const float angle   = startAngle + sliderPos * (endAngle - startAngle);

    // Inner dark circle (background)
    g.setColour(juce::Colour(0xff161821));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    
    // Slight shadow or gradient inside
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2d3e), centreX, centreY - radius,
                                           juce::Colour(0xff12131b), centreX, centreY + radius, false));
    g.fillEllipse(centreX - radius + 2, centreY - radius + 2, (radius - 2) * 2.0f, (radius - 2) * 2.0f);

    // Outer track arc
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius, radius,
                        0.0f, startAngle, endAngle, true);
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(track, juce::PathStrokeType(4.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Fill arc
    juce::Path fill;
    fill.addCentredArc(centreX, centreY, radius, radius,
                       0.0f, startAngle, angle, true);
    g.setColour(findColour(juce::Slider::rotarySliderFillColourId).withAlpha(0.2f));
    g.strokePath(fill, juce::PathStrokeType(8.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(fill, juce::PathStrokeType(4.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer dot
    const float dotR = 3.0f;
    const float px = centreX + std::sin(angle) * (radius - 6.0f);
    const float py = centreY - std::cos(angle) * (radius - 6.0f);
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(px - dotR, py - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.fillEllipse(px - dotR - 2.0f, py - dotR - 2.0f, (dotR + 2.0f) * 2.0f, (dotR + 2.0f) * 2.0f);
}

void DarkLookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& button,
    bool /*highlighted*/, bool /*down*/)
{
    const bool on = button.getToggleState();
    const auto bounds = button.getLocalBounds().toFloat();

    const float h = 16.0f;
    const float w = 28.0f;
    const float bx = bounds.getX() + 4.0f;
    const float by = bounds.getCentreY() - h * 0.5f;

    g.setColour(on ? juce::Colour(0xff2ecc71) : juce::Colour(0xff3a3d4a));
    g.fillRoundedRectangle(bx, by, w, h, h * 0.5f);

    const float thumbR = h * 0.5f - 2.0f;
    const float thumbX = on ? bx + w - thumbR * 2.0f - 2.0f : bx + 2.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX, bounds.getCentreY() - thumbR, thumbR * 2.0f, thumbR * 2.0f);

    g.setFont(juce::Font(14.0f).withStyle(juce::Font::bold));
    g.setColour(on ? juce::Colours::white : juce::Colour(0xffa0a5b5));
    g.drawText(button.getButtonText(),
               (int)(bx + w + 8.0f), 0,
               (int)(bounds.getWidth() - w - 12.0f), (int)bounds.getHeight(),
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
    setSize(960, 560);
    setResizable(true, true);
    setResizeLimits(720, 420, 1600, 900);

    // ── Meters ────────────────────────────────────────────────────────────────
    addAndMakeVisible(lufsDisplay);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(levelHistory);

    // ── Gate section ──────────────────────────────────────────────────────────
    addAndMakeVisible(gateToggle);
    gateKnob       .setup(this, "Threshold");
    gateAttackKnob .setup(this, "Attack");
    gateReleaseKnob.setup(this, "Release");

    // ── LUFS Leveler section ──────────────────────────────────────────────────
    addAndMakeVisible(levelerToggle);
    targetKnob .setup(this, "Target");
    attackKnob .setup(this, "Attack");
    releaseKnob.setup(this, "Release");
    maxGainKnob.setup(this, "Max Gain");

    // ── AutoGain section ──────────────────────────────────────────────────────
    addAndMakeVisible(autoGainToggle);
    autoGainTargetKnob.setup(this, "Target");
    autoGainSpeedKnob.setup(this, "Speed");

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

    // ── Dry/Wet ───────────────────────────────────────────────────────────────
    dryWetKnob.setup(this, "Mix %");

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
        lbl->setFont(juce::Font(11.0f).withStyle(juce::Font::bold));
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

    gateAtt        = std::make_unique<SliderAttachment>(apvts, ParamID::GATE_THRESHOLD, gateKnob.slider);
    gateAttackAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::GATE_ATTACK,    gateAttackKnob.slider);
    gateReleaseAtt = std::make_unique<SliderAttachment>(apvts, ParamID::GATE_RELEASE,   gateReleaseKnob.slider);
    gateOnAtt      = std::make_unique<ButtonAttachment>(apvts, ParamID::GATE_ENABLED,   gateToggle);

    autoGainTargetAtt = std::make_unique<SliderAttachment>(apvts, ParamID::AUTOGAIN_TARGET, autoGainTargetKnob.slider);
    autoGainSpeedAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::AUTOGAIN_SPEED,  autoGainSpeedKnob.slider);
    autoGainOnAtt     = std::make_unique<ButtonAttachment>(apvts, ParamID::AUTOGAIN_ENABLED, autoGainToggle);

    expThreshAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_THRESHOLD, expThreshKnob.slider);
    expRatioAtt   = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_RATIO,     expRatioKnob.slider);
    expAttackAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_ATTACK,    expAttackKnob.slider);
    expReleaseAtt = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_RELEASE,   expReleaseKnob.slider);
    expKneeAtt    = std::make_unique<SliderAttachment>(apvts, ParamID::EXP_KNEE,      expKneeKnob.slider);

    ceilingAtt     = std::make_unique<SliderAttachment>(apvts, ParamID::LIMITER_CEILING,  ceilingKnob.slider);
    lookaheadMsAtt = std::make_unique<SliderAttachment>(apvts, ParamID::LOOKAHEAD_MS,     lookaheadKnob.slider);
    dryWetAtt      = std::make_unique<SliderAttachment>(apvts, ParamID::DRY_WET,          dryWetKnob.slider);

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
    auto historyArea = rightArea.removeFromTop(120);
    levelHistory.setBounds(historyArea);
    rightArea.removeFromTop(6);

    // GR meter (Horizontal stacked bars now)
    auto grArea = rightArea.removeFromTop(50);
    grMeter.setBounds(grArea);
    rightArea.removeFromTop(6);

    // ── Control sections ──────────────────────────────────────────────────────
    // Split remaining area into two rows
    auto topControls = rightArea.removeFromTop(rightArea.getHeight() * 0.5f - 3);
    rightArea.removeFromTop(6);
    auto botControls = rightArea;

    // Row 1: Gate (left) | Expander (mid) | AutoGain (right)
    auto gateArea = topControls.removeFromLeft(topControls.getWidth() / 3 - 3);
    topControls.removeFromLeft(6);
    auto expanderArea = topControls.removeFromLeft(topControls.getWidth() * 0.65f - 3);
    topControls.removeFromLeft(6);
    auto autoGainArea = topControls;

    // Gate section
    gateArea.removeFromTop(8); 
    auto gateTitleRow = gateArea.removeFromTop(24);
    gateTitleRow.removeFromLeft(8); 
    gateToggle.setBounds(gateTitleRow.removeFromLeft(100));
    gateArea.removeFromTop(8);
    const int gateKnobW = gateArea.getWidth() / 3;
    gateKnob       .setBounds(gateArea.removeFromLeft(gateKnobW));
    gateAttackKnob .setBounds(gateArea.removeFromLeft(gateKnobW));
    gateReleaseKnob.setBounds(gateArea);

    // Expander section
    expanderArea.removeFromTop(8); 
    auto expTitleRow = expanderArea.removeFromTop(24);
    expTitleRow.removeFromLeft(8); 
    expanderToggle.setBounds(expTitleRow.removeFromLeft(100));
    expanderArea.removeFromTop(8);
    const int expKnobW = expanderArea.getWidth() / 5;
    expThreshKnob .setBounds(expanderArea.removeFromLeft(expKnobW));
    expRatioKnob  .setBounds(expanderArea.removeFromLeft(expKnobW));
    expAttackKnob .setBounds(expanderArea.removeFromLeft(expKnobW));
    expReleaseKnob.setBounds(expanderArea.removeFromLeft(expKnobW));
    expKneeKnob   .setBounds(expanderArea);

    // AutoGain section
    autoGainArea.removeFromTop(8);
    auto autoGainTitleRow = autoGainArea.removeFromTop(24);
    autoGainTitleRow.removeFromLeft(8);
    autoGainToggle.setBounds(autoGainTitleRow.removeFromLeft(100));
    autoGainArea.removeFromTop(8);
    const int agKnobW = autoGainArea.getWidth() / 2;
    autoGainTargetKnob.setBounds(autoGainArea.removeFromLeft(agKnobW));
    autoGainSpeedKnob.setBounds(autoGainArea);


    // Row 2: Leveler (left) | Limiter (mid) | Lookahead (right)
    auto levelerArea = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
    botControls.removeFromLeft(6);
    auto limiterArea    = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
    botControls.removeFromLeft(6);
    auto lookaheadArea  = botControls;

    // Leveler section
    levelerArea.removeFromTop(8); 
    auto titleRow = levelerArea.removeFromTop(24);
    titleRow.removeFromLeft(8); 
    levelerToggle.setBounds(titleRow.removeFromLeft(100));
    levelerArea.removeFromTop(8);
    const int knobW = levelerArea.getWidth() / 4;
    targetKnob .setBounds(levelerArea.removeFromLeft(knobW));
    attackKnob .setBounds(levelerArea.removeFromLeft(knobW));
    releaseKnob.setBounds(levelerArea.removeFromLeft(knobW));
    maxGainKnob.setBounds(levelerArea);

    // Limiter
    limiterArea.removeFromTop(8);
    auto limTitleRow = limiterArea.removeFromTop(24);
    limTitleRow.removeFromLeft(8);
    limiterToggle.setBounds(limTitleRow.removeFromLeft(150));
    limiterArea.removeFromTop(8);
    ceilingKnob.setBounds(limiterArea.removeFromLeft(limiterArea.getWidth() / 2));

    // Lookahead + Mix
    lookaheadArea.removeFromTop(8);
    auto lookTitleRow = lookaheadArea.removeFromTop(24);
    lookTitleRow.removeFromLeft(8);
    lookaheadToggle.setBounds(lookTitleRow.removeFromLeft(150));
    lookaheadArea.removeFromTop(8);
    const int lookKnobW = lookaheadArea.getWidth() / 2;
    lookaheadKnob.setBounds(lookaheadArea.removeFromLeft(lookKnobW));
    dryWetKnob.setBounds(lookaheadArea);
}

// ── paint ─────────────────────────────────────────────────────────────────────
void LufsNormalizerEditor::paint(juce::Graphics& g)
{
    // Background flat dark grey
    g.fillAll(juce::Colour(0xff161618));

    // Section backgrounds
    auto area = getLocalBounds().reduced(8);
    auto topBarArea = area.removeFromTop(28); // top bar
    area.removeFromTop(6);
    area.removeFromLeft(86); // meter column

    auto historyArea = area.removeFromTop(120);
    area.removeFromTop(6);
    auto grArea = area.removeFromTop(50);
    area.removeFromTop(6);

    auto topControls = area.removeFromTop(area.getHeight() * 0.5f - 3);
    area.removeFromTop(6);
    auto botControls = area;

    auto gateArea = topControls.removeFromLeft(topControls.getWidth() / 3 - 3);
    topControls.removeFromLeft(6);
    auto expanderArea = topControls.removeFromLeft(topControls.getWidth() * 0.65f - 3);
    topControls.removeFromLeft(6);
    auto autoGainArea = topControls;

    auto levelerArea  = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
    botControls.removeFromLeft(6);
    auto limiterArea   = botControls.removeFromLeft(botControls.getWidth() / 2 - 3);
    botControls.removeFromLeft(6);
    auto lookaheadArea = botControls;

    // Use lighter grey for panels
    const juce::Colour panelCol(0xff1c1d24);

    drawSectionBackground(g, historyArea,  "",  panelCol);
    drawSectionBackground(g, grArea,       "", panelCol); // GR meter background
    drawSectionBackground(g, gateArea,     "",  panelCol);
    drawSectionBackground(g, expanderArea, "",  panelCol);
    drawSectionBackground(g, autoGainArea, "",  panelCol);
    drawSectionBackground(g, levelerArea,  "",  panelCol);
    drawSectionBackground(g, limiterArea,  "",  panelCol);
    drawSectionBackground(g, lookaheadArea,"",  panelCol);

    // Plugin title (Centered in top bar, or right-aligned)
    g.setFont(juce::Font(16.0f).withStyle(juce::Font::bold));
    g.setColour(juce::Colour(0xff00d4ff));
    g.drawText("LUFS MASTER",
               topBarArea.withTrimmedLeft(280),
               juce::Justification::centredLeft, false);

    g.setFont(juce::Font(11.0f));
    g.setColour(juce::Colour(0xff606575));
    g.drawText("v1.0  |  EBU R128",
               topBarArea.withTrimmedLeft(410),
               juce::Justification::centredLeft, false);
}

void LufsNormalizerEditor::drawSectionBackground(juce::Graphics& g,
    juce::Rectangle<int> area, const juce::String& title,
    juce::Colour colour) const
{
    g.setColour(colour);
    g.fillRoundedRectangle(area.toFloat(), 6.0f);
    
    // Very subtle border for modern flat aesthetic
    g.setColour(juce::Colour(0xff2d2e38));
    g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);

    if (title.isNotEmpty())
    {
        g.setFont(juce::Font(13.0f).withStyle(juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(title, area.withHeight(24).withTrimmedLeft(12), juce::Justification::centredLeft, false);
    }
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
    grMeter.setAutoGain   (processor.getAutoGainDb());
    grMeter.setLevelerGain(processor.getLevelerGainDb());
    grMeter.setLimiterGR  (processor.getLimiterGrDb());
    grMeter.repaint();

    // Update history (push every other tick → ~10 Hz)
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

    momentaryLabel .setText("M: "  + fmt(mom)  + " LUFS", juce::dontSendNotification);
    shortTermLabel .setText("ST: " + fmt(st)   + " LUFS", juce::dontSendNotification);
    integratedLabel.setText("I: "  + fmt(intg) + " LUFS", juce::dontSendNotification);

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
