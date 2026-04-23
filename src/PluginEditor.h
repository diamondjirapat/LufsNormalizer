#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/LufsDisplay.h"
#include "ui/GainReductionMeter.h"
#include "ui/LevelHistory.h"

// ── Custom LookAndFeel ────────────────────────────────────────────────────────
class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont(juce::Label&) override;
};

// ── Labelled knob helper ──────────────────────────────────────────────────────
struct LabelledKnob
{
    juce::Slider slider { juce::Slider::RotaryVerticalDrag,
                          juce::Slider::TextBoxBelow };
    juce::Label  label;

    void setup(juce::Component* parent, const juce::String& labelText);
    void setBounds(juce::Rectangle<int> area);
};

// ── Editor ────────────────────────────────────────────────────────────────────
class LufsNormalizerEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit LufsNormalizerEditor(LufsNormalizerProcessor&);
    ~LufsNormalizerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    LufsNormalizerProcessor& processor;

    DarkLookAndFeel laf;

    // ── Meters ────────────────────────────────────────────────────────────────
    LufsDisplay        lufsDisplay;
    GainReductionMeter grMeter;
    LevelHistory       levelHistory;

    // ── LUFS Leveler section ──────────────────────────────────────────────────
    juce::ToggleButton levelerToggle { "Leveler" };
    LabelledKnob       targetKnob;
    LabelledKnob       attackKnob;
    LabelledKnob       releaseKnob;
    LabelledKnob       maxGainKnob;
    LabelledKnob       gateKnob;

    // ── Expander section ──────────────────────────────────────────────────────
    juce::ToggleButton expanderToggle { "Expander" };
    LabelledKnob       expThreshKnob;
    LabelledKnob       expRatioKnob;
    LabelledKnob       expAttackKnob;
    LabelledKnob       expReleaseKnob;
    LabelledKnob       expKneeKnob;

    // ── Limiter section ───────────────────────────────────────────────────────
    juce::ToggleButton limiterToggle  { "True Peak" };
    LabelledKnob       ceilingKnob;

    // ── Lookahead section ─────────────────────────────────────────────────────
    juce::ToggleButton lookaheadToggle { "Lookahead" };
    LabelledKnob       lookaheadKnob;

    // ── Preset combo ─────────────────────────────────────────────────────────
    juce::ComboBox presetCombo;
    juce::Label    presetLabel;

    // ── Reset button ─────────────────────────────────────────────────────────
    juce::TextButton resetButton { "Reset Integrated" };

    // ── LUFS readout labels ───────────────────────────────────────────────────
    juce::Label momentaryLabel, shortTermLabel, integratedLabel;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> targetAtt, attackAtt, releaseAtt, maxGainAtt, gateAtt;
    std::unique_ptr<SliderAttachment> expThreshAtt, expRatioAtt, expAttackAtt,
                                      expReleaseAtt, expKneeAtt;
    std::unique_ptr<SliderAttachment> ceilingAtt, lookaheadMsAtt;
    std::unique_ptr<ButtonAttachment> levelerOnAtt, expanderOnAtt,
                                      limiterOnAtt, lookaheadOnAtt;

    // ── Timer ─────────────────────────────────────────────────────────────────
    void timerCallback() override;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void buildAttachments();
    void layoutComponents();
    void drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> area,
                               const juce::String& title,
                               juce::Colour colour) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsNormalizerEditor)
};
