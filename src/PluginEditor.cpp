#include "PluginEditor.h"
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
// DarkLookAndFeel
// ═════════════════════════════════════════════════════════════════════════════
DarkLookAndFeel::DarkLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,              juce::Colour(0xff00d4ff));
    setColour(juce::Slider::rotarySliderFillColourId,   juce::Colour(0xff00d4ff));
    setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff1e2030));
    setColour(juce::Slider::textBoxTextColourId,        juce::Colour(0xffe0e4f0));
    setColour(juce::Slider::textBoxBackgroundColourId,  juce::Colour(0xff0c0d14));
    setColour(juce::Slider::textBoxOutlineColourId,     juce::Colour(0xff2a2d3e));
    setColour(juce::Label::textColourId,                juce::Colour(0xff8890a5));
    setColour(juce::ToggleButton::textColourId,         juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId,       juce::Colour(0xff0f1018));
    setColour(juce::ComboBox::textColourId,             juce::Colour(0xffe0e4f0));
    setColour(juce::ComboBox::outlineColourId,          juce::Colour(0xff2a2d3e));
    setColour(juce::TextButton::buttonColourId,         juce::Colour(0xff1a1d2e));
    setColour(juce::TextButton::textColourOffId,        juce::Colour(0xffe0e4f0));
    setColour(juce::PopupMenu::backgroundColourId,      juce::Colour(0xff111320));
    setColour(juce::PopupMenu::textColourId,            juce::Colour(0xffe0e4f0));
}

void DarkLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& /*slider*/)
{
    const float radius  = (float)std::min(width, height) * 0.5f - 6.0f;
    const float centreX = (float)x + (float)width  * 0.5f;
    const float centreY = (float)y + (float)height * 0.5f;
    const float angle   = startAngle + sliderPos * (endAngle - startAngle);
    const auto  accentCol = findColour(juce::Slider::rotarySliderFillColourId);

    // ── Shadow ring (depth cue) ──────────────────────────────────────────────
    g.setColour(juce::Colour(0x30000000));
    g.fillEllipse(centreX - radius - 1, centreY - radius + 1,
                  (radius + 1) * 2.0f, (radius + 1) * 2.0f);

    // ── Outer ring background ────────────────────────────────────────────────
    g.setColour(juce::Colour(0xff0a0b12));
    g.fillEllipse(centreX - radius, centreY - radius,
                  radius * 2.0f, radius * 2.0f);

    // ── Concave knob surface (radial-like gradient) ──────────────────────────
    const float inner = radius - 4.0f;
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff22253a), centreX, centreY - inner * 0.6f,
        juce::Colour(0xff0c0d16), centreX, centreY + inner * 0.8f, false));
    g.fillEllipse(centreX - inner, centreY - inner,
                  inner * 2.0f, inner * 2.0f);

    // ── Subtle rim highlight (top edge catch-light) ──────────────────────────
    {
        juce::Path rimArc;
        rimArc.addCentredArc(centreX, centreY, radius - 0.5f, radius - 0.5f,
                             0.0f, -2.4f, -0.7f, true);
        g.setColour(juce::Colour(0x18ffffff));
        g.strokePath(rimArc, juce::PathStrokeType(1.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Track arc (inactive portion) ─────────────────────────────────────────
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius, radius,
                        0.0f, startAngle, endAngle, true);
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId).withAlpha(0.5f));
    g.strokePath(track, juce::PathStrokeType(3.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // ── Value arc — triple-layer neon glow ───────────────────────────────────
    if (sliderPos > 0.001f)
    {
        juce::Path fill;
        fill.addCentredArc(centreX, centreY, radius, radius,
                           0.0f, startAngle, angle, true);

        // Layer 1: wide soft glow
        g.setColour(accentCol.withAlpha(0.10f));
        g.strokePath(fill, juce::PathStrokeType(10.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // Layer 2: medium glow
        g.setColour(accentCol.withAlpha(0.25f));
        g.strokePath(fill, juce::PathStrokeType(6.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // Layer 3: crisp core
        g.setColour(accentCol);
        g.strokePath(fill, juce::PathStrokeType(2.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Pointer line (replaces dot) ──────────────────────────────────────────
    {
        const float pInner = radius * 0.38f;
        const float pOuter = radius - 5.0f;
        const float x1 = centreX + std::sin(angle) * pInner;
        const float y1 = centreY - std::cos(angle) * pInner;
        const float x2 = centreX + std::sin(angle) * pOuter;
        const float y2 = centreY - std::cos(angle) * pOuter;

        // Glow behind the line
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawLine(x1, y1, x2, y2, 5.0f);
        // Crisp pointer
        g.setColour(juce::Colours::white.withAlpha(0.95f));
        g.drawLine(x1, y1, x2, y2, 1.8f);
    }
}

void DarkLookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& button,
    bool /*highlighted*/, bool /*down*/)
{
    const bool on = button.getToggleState();
    const auto bounds = button.getLocalBounds().toFloat();

    const float h  = 16.0f;
    const float w  = 30.0f;
    const float bx = bounds.getX() + 4.0f;
    const float by = bounds.getCentreY() - h * 0.5f;

    const auto onColour = juce::Colour(0xff00e676);

    // ── Glow halo behind pill when ON ────────────────────────────────────────
    if (on)
    {
        g.setColour(onColour.withAlpha(0.12f));
        g.fillRoundedRectangle(bx - 3.0f, by - 3.0f, w + 6.0f, h + 6.0f, (h + 6.0f) * 0.5f);
    }

    // ── Pill track ────────────────────────────────────────────────────────────
    g.setColour(on ? onColour : juce::Colour(0xff282c3a));
    g.fillRoundedRectangle(bx, by, w, h, h * 0.5f);

    // Subtle inner border
    g.setColour(on ? onColour.brighter(0.2f).withAlpha(0.3f)
                   : juce::Colour(0xff3a3f50));
    g.drawRoundedRectangle(bx, by, w, h, h * 0.5f, 0.8f);

    // ── Thumb ────────────────────────────────────────────────────────────────
    const float thumbR = h * 0.5f - 2.5f;
    const float thumbX = on ? bx + w - thumbR * 2.0f - 2.5f : bx + 2.5f;
    const float thumbY = bounds.getCentreY() - thumbR;

    // Shadow under thumb
    g.setColour(juce::Colour(0x30000000));
    g.fillEllipse(thumbX + 0.5f, thumbY + 0.5f, thumbR * 2.0f, thumbR * 2.0f);
    // Thumb
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX, thumbY, thumbR * 2.0f, thumbR * 2.0f);

    // ── Label text ───────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
    g.setColour(on ? juce::Colour(0xfff0f2f8) : juce::Colour(0xff6b7080));
    g.drawText(button.getButtonText(),
               (int)(bx + w + 7.0f), 0,
               button.getWidth() - (int)(w + 12.0f), (int)bounds.getHeight(),
               juce::Justification::centredLeft, true);
}

juce::Font DarkLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(10.5f));
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
    label.setFont(juce::Font(juce::FontOptions(11.0f)));
    parent->addAndMakeVisible(label);
}

void LabelledKnob::setBounds(juce::Rectangle<int> area)
{
    const int labelH = 16;
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
    setSize(1020, 600);
    setResizable(true, true);
    setResizeLimits(780, 460, 1600, 960);

    // ── Meters ────────────────────────────────────────────────────────────────
    inputMeter.setMode(LufsDisplay::Input);
    outputMeter.setMode(LufsDisplay::Output);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(levelHistory);

    // ── Gate section ──────────────────────────────────────────────────────────
    addAndMakeVisible(gateToggle);
    gateKnob       .setup(this, "Thresh");
    gateAttackKnob .setup(this, "Attack");
    gateReleaseKnob.setup(this, "Release");

    // ── LUFS Leveler section ──────────────────────────────────────────────────
    addAndMakeVisible(levelerToggle);
    targetKnob .setup(this, "Target");
    attackKnob .setup(this, "Attack");
    releaseKnob.setup(this, "Release");
    maxGainKnob.setup(this, "Max Gain");

    // ── Compressor section ──────────────────────────────────────────────────
    addAndMakeVisible(compToggle);
    compThreshKnob.setup(this, "Thresh");
    compRatioKnob.setup(this, "Ratio");
    compAttackKnob.setup(this, "Attack");
    compReleaseKnob.setup(this, "Release");
    compMakeupKnob.setup(this, "Makeup");
    addAndMakeVisible(compAutoMakeupToggle);

    // ── Expander section ──────────────────────────────────────────────────────
    addAndMakeVisible(expanderToggle);
    expThreshKnob .setup(this, "Thresh");
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
    for (auto* lbl : { &momentaryLabel, &shortTermLabel, &integratedLabel,
                        &outMomentaryLabel, &outShortTermLabel, &outIntegratedLabel })
    {
        lbl->setJustificationType(juce::Justification::centred);
        lbl->setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        lbl->setMinimumHorizontalScale(0.7f);
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

    compThreshAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::COMP_THRESHOLD, compThreshKnob.slider);
    compRatioAtt   = std::make_unique<SliderAttachment>(apvts, ParamID::COMP_RATIO,     compRatioKnob.slider);
    compAttackAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::COMP_ATTACK,    compAttackKnob.slider);
    compReleaseAtt = std::make_unique<SliderAttachment>(apvts, ParamID::COMP_RELEASE,   compReleaseKnob.slider);
    compMakeupAtt  = std::make_unique<SliderAttachment>(apvts, ParamID::COMP_MAKEUP,    compMakeupKnob.slider);
    compOnAtt      = std::make_unique<ButtonAttachment>(apvts, ParamID::COMP_ENABLED,   compToggle);
    compAutoMakeupAtt = std::make_unique<ButtonAttachment>(apvts, ParamID::COMP_AUTO_MAKEUP, compAutoMakeupToggle);

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
    auto area = getLocalBounds().reduced(12);


    // ── Top bar: preset + reset ───────────────────────────────────────────────
    topBarArea = area.removeFromTop(28);
    auto topBarCopy = topBarArea; // Use a copy for sub-layout
    presetLabel.setBounds(topBarCopy.removeFromLeft(50));
    presetCombo.setBounds(topBarCopy.removeFromLeft(280));
    topBarCopy.removeFromLeft(8);
    resetButton.setBounds(topBarCopy.removeFromLeft(132).reduced(0, 2));

    area.removeFromTop(6);

    // ── Left column: Input meter ──────────────────────────────────────────────
    auto leftCol = area.removeFromLeft(80);
    inputMeter.setBounds(leftCol.removeFromTop(leftCol.getHeight() - 60));

    // LUFS readout labels below meter
    momentaryLabel .setBounds(leftCol.removeFromTop(20));
    shortTermLabel .setBounds(leftCol.removeFromTop(20));
    integratedLabel.setBounds(leftCol.removeFromTop(20));

    area.removeFromLeft(6);

    // ── Right column: Output meter ──────────────────────────────────────────
    auto rightCol = area.removeFromRight(80);
    outputMeter.setBounds(rightCol.removeFromTop(rightCol.getHeight() - 60));

    // Output LUFS readout labels below meter
    outMomentaryLabel .setBounds(rightCol.removeFromTop(20));
    outShortTermLabel .setBounds(rightCol.removeFromTop(20));
    outIntegratedLabel.setBounds(rightCol.removeFromTop(20));

    area.removeFromRight(6);

    // ── Right column: controls ────────────────────────────────────────────────
    auto rightArea = area;

    // History graph at top
    historyArea = rightArea.removeFromTop(120);
    levelHistory.setBounds(historyArea);
    rightArea.removeFromTop(6);

    // GR meter (Horizontal stacked bars now)
    grArea = rightArea.removeFromTop(50);
    grMeter.setBounds(grArea);
    rightArea.removeFromTop(6);

    // ── Control sections ──────────────────────────────────────────────────────
    // Split remaining area into two rows
    auto topControls = rightArea.removeFromTop(juce::roundToInt((float) rightArea.getHeight() * 0.5f - 3.0f));
    rightArea.removeFromTop(6);
    auto botControls = rightArea;

    // Row 1: Gate (left) | Expander (mid) | Compressor (right)
    int row1Usable = topControls.getWidth() - 12;
    gateArea = topControls.removeFromLeft(juce::roundToInt(row1Usable * (3.0f / 12.0f)));
    topControls.removeFromLeft(6);
    expanderArea = topControls.removeFromLeft(juce::roundToInt(row1Usable * (4.0f / 12.0f)));
    topControls.removeFromLeft(6);
    compArea = topControls;

    // Row 2: Leveler (left) | Limiter (mid) | Lookahead (right)
    int row2Usable = botControls.getWidth() - 12;
    levelerArea = botControls.removeFromLeft(juce::roundToInt(row2Usable * (4.0f / 7.5f)));
    botControls.removeFromLeft(6);
    limiterArea = botControls.removeFromLeft(juce::roundToInt(row2Usable * (1.5f / 7.5f)));
    botControls.removeFromLeft(6);
    lookaheadArea = botControls;

    // Process Titles for Row 1
    auto gateAreaCopy = gateArea;
    gateAreaCopy.removeFromTop(8); 
    auto gateTitleRow = gateAreaCopy.removeFromTop(24);
    gateTitleRow.removeFromLeft(8); 
    gateToggle.setBounds(gateTitleRow.removeFromLeft(90));
    gateAreaCopy.removeFromTop(8);

    auto expanderAreaCopy = expanderArea;
    expanderAreaCopy.removeFromTop(8); 
    auto expTitleRow = expanderAreaCopy.removeFromTop(24);
    expTitleRow.removeFromLeft(8); 
    expanderToggle.setBounds(expTitleRow.removeFromLeft(120));
    expanderAreaCopy.removeFromTop(8);

    auto compAreaCopy = compArea;
    compAreaCopy.removeFromTop(8);
    auto compTitleRow = compAreaCopy.removeFromTop(24);
    compTitleRow.removeFromLeft(8);
    compToggle.setBounds(compTitleRow.removeFromLeft(140));
    compAutoMakeupToggle.setBounds(compTitleRow.removeFromLeft(140));
    compAreaCopy.removeFromTop(8);

    // Process Titles for Row 2
    auto levelerAreaCopy = levelerArea;
    levelerAreaCopy.removeFromTop(8); 
    auto titleRow = levelerAreaCopy.removeFromTop(24);
    titleRow.removeFromLeft(8); 
    levelerToggle.setBounds(titleRow.removeFromLeft(110));
    levelerAreaCopy.removeFromTop(8);

    auto limiterAreaCopy = limiterArea;
    limiterAreaCopy.removeFromTop(8); 
    auto limTitleRow = limiterAreaCopy.removeFromTop(24);
    limTitleRow.removeFromLeft(8); 
    limiterToggle.setBounds(limTitleRow.removeFromLeft(130));
    limiterAreaCopy.removeFromTop(8);

    auto lookaheadAreaCopy = lookaheadArea;
    lookaheadAreaCopy.removeFromTop(8);
    auto lookTitleRow = lookaheadAreaCopy.removeFromTop(24);
    lookTitleRow.removeFromLeft(8);
    lookaheadToggle.setBounds(lookTitleRow.removeFromLeft(130));
    lookaheadAreaCopy.removeFromTop(8);

    // Calculate minimum uniform size for all knobs
    int minSlotW = std::min({
        gateAreaCopy.getWidth() / 3,
        expanderAreaCopy.getWidth() / 5,
        compAreaCopy.getWidth() / 5,
        levelerAreaCopy.getWidth() / 4,
        limiterAreaCopy.getWidth() / 1,
        lookaheadAreaCopy.getWidth() / 2
    });

    int minSlotH = std::min(gateAreaCopy.getHeight(), levelerAreaCopy.getHeight());

    int finalKnobW = minSlotW;
    int finalKnobH = minSlotH;

    auto placeKnob = [finalKnobW, finalKnobH](LabelledKnob& knob, juce::Rectangle<int> slot) {
        knob.setBounds(slot.withSizeKeepingCentre(finalKnobW, finalKnobH));
    };

    // Place Knobs
    const int gateKnobW = gateAreaCopy.getWidth() / 3;
    placeKnob(gateKnob,       gateAreaCopy.removeFromLeft(gateKnobW));
    placeKnob(gateAttackKnob, gateAreaCopy.removeFromLeft(gateKnobW));
    placeKnob(gateReleaseKnob,gateAreaCopy);

    const int expKnobW = expanderAreaCopy.getWidth() / 5;
    placeKnob(expThreshKnob,  expanderAreaCopy.removeFromLeft(expKnobW));
    placeKnob(expRatioKnob,   expanderAreaCopy.removeFromLeft(expKnobW));
    placeKnob(expAttackKnob,  expanderAreaCopy.removeFromLeft(expKnobW));
    placeKnob(expReleaseKnob, expanderAreaCopy.removeFromLeft(expKnobW));
    placeKnob(expKneeKnob,    expanderAreaCopy);

    const int compKnobW = compAreaCopy.getWidth() / 5;
    placeKnob(compThreshKnob,  compAreaCopy.removeFromLeft(compKnobW));
    placeKnob(compRatioKnob,   compAreaCopy.removeFromLeft(compKnobW));
    placeKnob(compAttackKnob,  compAreaCopy.removeFromLeft(compKnobW));
    placeKnob(compReleaseKnob, compAreaCopy.removeFromLeft(compKnobW));
    placeKnob(compMakeupKnob,  compAreaCopy);

    const int levKnobW = levelerAreaCopy.getWidth() / 4;
    placeKnob(targetKnob,  levelerAreaCopy.removeFromLeft(levKnobW));
    placeKnob(attackKnob,  levelerAreaCopy.removeFromLeft(levKnobW));
    placeKnob(releaseKnob, levelerAreaCopy.removeFromLeft(levKnobW));
    placeKnob(maxGainKnob, levelerAreaCopy);

    placeKnob(ceilingKnob, limiterAreaCopy);

    const int lookKnobW = lookaheadAreaCopy.getWidth() / 2;
    placeKnob(lookaheadKnob, lookaheadAreaCopy.removeFromLeft(lookKnobW));
    placeKnob(dryWetKnob,    lookaheadAreaCopy);
}

// ── paint ─────────────────────────────────────────────────────────────────────
void LufsNormalizerEditor::paint(juce::Graphics& g)
{
    // ── Deep background ────────────────────────────────────────────────────────
    g.fillAll(juce::Colour(0xff0d0e12));

    // Subtle top-to-bottom gradient overlay for depth
    {
        juce::ColourGradient bg(
            juce::Colour(0xff141620), 0.0f, 0.0f,
            juce::Colour(0xff0a0b0e), 0.0f, (float)getHeight(), false);
        g.setGradientFill(bg);
        g.fillRect(getLocalBounds());
    }

    // ── Glassmorphic section panels ───────────────────────────────────────────
    const juce::Colour panelCol(0xff14151c);

    drawSectionBackground(g, historyArea,  panelCol);
    drawSectionBackground(g, grArea,       panelCol);
    drawSectionBackground(g, gateArea,     panelCol);
    drawSectionBackground(g, expanderArea, panelCol);
    drawSectionBackground(g, compArea,     panelCol);
    drawSectionBackground(g, levelerArea,  panelCol);
    drawSectionBackground(g, limiterArea,  panelCol);
    drawSectionBackground(g, lookaheadArea,panelCol);

    // ── Header: Plugin title & Version pill badge (Dynamic Scaling) ───────────
    {
        // remainingArea starts after the reset button (x = 482)
        auto remainingArea = topBarArea.withTrimmedLeft(470);
        
        // Version pill badge on the far right
        auto pillArea = remainingArea.removeFromRight(120).reduced(0, 4);
        
        // Title area is the remaining space between the reset button and the pill
        auto titleArea = remainingArea;
        
        // Scale font size dynamically with width
        float fontHeight = 17.0f;
        if (remainingArea.getWidth() < 250)
            fontHeight = 13.0f;
        else if (remainingArea.getWidth() < 350)
            fontHeight = 15.0f;

        g.setFont(juce::Font(juce::FontOptions(fontHeight, juce::Font::bold)));
        g.setColour(juce::Colour(0xff00d4ff));
        g.drawText("L U F S   M A S T E R", titleArea,
                   juce::Justification::centred, false);

        // Draw pill
        const juce::String ver = "v1.3 | BS.1770-4";
        g.setColour(juce::Colour(0xff1a1d2a));
        g.fillRoundedRectangle(pillArea.toFloat(), 8.0f);
        g.setColour(juce::Colour(0xff2a2e3e));
        g.drawRoundedRectangle(pillArea.toFloat(), 8.0f, 0.8f);
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.setColour(juce::Colour(0xff606575));
        g.drawText(ver, pillArea, juce::Justification::centred, false);
    }
}

void LufsNormalizerEditor::drawSectionBackground(juce::Graphics& g,
    juce::Rectangle<int> area, juce::Colour colour) const
{
    auto r = area.toFloat();

    // ── Inner fill: subtle vertical gradient (lighter top, darker bottom) ────
    {
        juce::ColourGradient fill(colour.brighter(0.08f), r.getX(), r.getY(),
                                  colour.darker(0.05f),   r.getX(), r.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(r, 6.0f);
    }

    // ── Gradient border: top-left lighter, bottom-right darker ────────────────
    {
        juce::ColourGradient border(
            juce::Colour(0xff2e3248), r.getX(), r.getY(),
            juce::Colour(0xff181a24), r.getRight(), r.getBottom(), false);
        g.setGradientFill(border);
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }

    // ── Inner shadow at top edge ─────────────────────────────────────────────
    {
        juce::ColourGradient shadow(
            juce::Colour(0x0cffffff), r.getX() + r.getWidth() * 0.5f, r.getY(),
            juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.5f, r.getY() + 12.0f, false);
        g.setGradientFill(shadow);
        g.fillRoundedRectangle(r.withHeight(12.0f), 6.0f);
    }
}

// ── timerCallback ─────────────────────────────────────────────────────────────
void LufsNormalizerEditor::timerCallback()
{
    const float inMom  = processor.getInputMomentaryLUFS();
    const float inSt   = processor.getInputShortTermLUFS();
    const float inIntg = processor.getInputIntegratedLUFS();

    const float outMom  = processor.getOutputMomentaryLUFS();
    const float outSt   = processor.getOutputShortTermLUFS();
    const float outIntg = processor.getOutputIntegratedLUFS();

    const float targetLufs = processor.getAPVTS().getRawParameterValue(ParamID::TARGET_LUFS)->load();

    // Update input meter (raw audio)
    inputMeter.setPeakDb(processor.getInputPeakDb());
    inputMeter.setRmsDb(processor.getInputRmsDb());
    inputMeter.repaint();

    // Update output meter
    outputMeter.setPeakDb(processor.getOutputPeakDb());
    outputMeter.setRmsDb(processor.getOutputRmsDb());
    outputMeter.setTargetLUFS(targetLufs);
    outputMeter.repaint();

    // Update GR meter
    grMeter.setExpanderGR (processor.getExpanderGrDb());
    grMeter.setCompGR     (processor.getCompGrDb());
    grMeter.setLevelerGain(processor.getLevelerGainDb());
    grMeter.setLimiterGR  (processor.getLimiterGrDb());
    grMeter.repaint();

    // Update history (push every other tick -> ~10 Hz)
    if (++histTick >= 2)
    {
        histTick = 0;
        levelHistory.pushInputValue(processor.getInputRmsDb());
        levelHistory.pushOutputValue(processor.getOutputRmsDb());
        levelHistory.setTargetLUFS(targetLufs);
    }

    // Update readout labels
    auto fmt = [](float v) -> juce::String
    {
        if (v <= -140.0f) return "-inf";
        return juce::String(v, 1);
    };

    // Input readout labels
    momentaryLabel .setText("M: "  + fmt(inMom)  + " LUFS", juce::dontSendNotification);
    shortTermLabel .setText("ST: " + fmt(inSt)   + " LUFS", juce::dontSendNotification);
    integratedLabel.setText("I: "  + fmt(inIntg) + " LUFS", juce::dontSendNotification);

    // Output readout labels
    outMomentaryLabel .setText("M: "  + fmt(outMom)  + " LUFS", juce::dontSendNotification);
    outShortTermLabel .setText("ST: " + fmt(outSt)   + " LUFS", juce::dontSendNotification);
    outIntegratedLabel.setText("I: "  + fmt(outIntg) + " LUFS", juce::dontSendNotification);

    // Colour-code integrated labels (based on output vs target)
    const float diff = outIntg - targetLufs;
    juce::Colour intgColour = juce::Colours::lightgrey;
    if (outIntg > -140.0f)
    {
        if      (std::abs(diff) < 1.0f) intgColour = juce::Colour(0xff00ff88);
        else if (std::abs(diff) < 3.0f) intgColour = juce::Colour(0xffffcc00);
        else                            intgColour = juce::Colour(0xffff4444);
    }
    outIntegratedLabel.setColour(juce::Label::textColourId, intgColour);
    integratedLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}
