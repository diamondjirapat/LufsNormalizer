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
    LufsDisplay        inputMeter;   // Left side – raw input (blue)
    LufsDisplay        outputMeter;  // Right side – output (green)
    GainReductionMeter grMeter;
    LevelHistory       levelHistory;

    // ── Gate section ──────────────────────────────────────────────────────────
    juce::ToggleButton gateToggle { "Gate" };
    LabelledKnob       gateKnob;
    LabelledKnob       gateAttackKnob;
    LabelledKnob       gateReleaseKnob;

    // ── LUFS Leveler section ──────────────────────────────────────────────────
    juce::ToggleButton levelerToggle { "Leveler" };
    LabelledKnob       targetKnob;
    LabelledKnob       attackKnob;
    LabelledKnob       releaseKnob;
    LabelledKnob       maxGainKnob;

    // ── Compressor section ──────────────────────────────────────────────────
    juce::ToggleButton compToggle { "Compressor" };
    LabelledKnob       compThreshKnob;
    LabelledKnob       compRatioKnob;
    LabelledKnob       compAttackKnob;
    LabelledKnob       compReleaseKnob;
    LabelledKnob       compMakeupKnob;
    juce::ToggleButton compAutoMakeupToggle { "Auto Makeup" };

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

    // ── Dry/Wet mix ───────────────────────────────────────────────────────
    LabelledKnob       dryWetKnob;

    // ── Preset combo ─────────────────────────────────────────────────────────
    juce::ComboBox presetCombo;
    juce::Label    presetLabel;

    // ── Reset button ─────────────────────────────────────────────────────────
    juce::TextButton resetButton { "Reset Integrated" };

    // ── History tick counter (per-instance, not static) ───────────────────────
    int histTick = 0;

    // ── LUFS readout labels ───────────────────────────────────────────────
    juce::Label momentaryLabel, shortTermLabel, integratedLabel;

    juce::Label outMomentaryLabel, outShortTermLabel, outIntegratedLabel;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> targetAtt, attackAtt, releaseAtt, maxGainAtt;
    std::unique_ptr<SliderAttachment> gateAtt, gateAttackAtt, gateReleaseAtt;
    std::unique_ptr<SliderAttachment> compThreshAtt, compRatioAtt, compAttackAtt, compReleaseAtt, compMakeupAtt;
    std::unique_ptr<ButtonAttachment> compAutoMakeupAtt;
    std::unique_ptr<SliderAttachment> expThreshAtt, expRatioAtt, expAttackAtt,
                                      expReleaseAtt, expKneeAtt;
    std::unique_ptr<SliderAttachment> ceilingAtt, lookaheadMsAtt, dryWetAtt;
    std::unique_ptr<ButtonAttachment> gateOnAtt, compOnAtt, levelerOnAtt, 
                                      expanderOnAtt, limiterOnAtt, lookaheadOnAtt;

    // ── Timer ─────────────────────────────────────────────────────────────────
    void timerCallback() override;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void buildAttachments();
    void layoutComponents();
    void drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> area,
                               juce::Colour colour) const;

    juce::Rectangle<int> topBarArea;
    juce::Rectangle<int> historyArea;
    juce::Rectangle<int> grArea;
    juce::Rectangle<int> gateArea;
    juce::Rectangle<int> expanderArea;
    juce::Rectangle<int> compArea;
    juce::Rectangle<int> levelerArea;
    juce::Rectangle<int> limiterArea;
    juce::Rectangle<int> lookaheadArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsNormalizerEditor)
};
