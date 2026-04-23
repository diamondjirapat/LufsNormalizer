#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

// ── Parameter layout ─────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
LufsNormalizerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ── LUFS Leveler ──────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::TARGET_LUFS, "Target LUFS",
        juce::NormalisableRange<float>(-36.0f, -6.0f, 0.1f), -16.0f,
        juce::AudioParameterFloatAttributes().withLabel("LUFS")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::LEVELER_ENABLED, "Leveler On", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::ATTACK_MS, "Leveler Attack",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.4f), 200.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::RELEASE_MS, "Leveler Release",
        juce::NormalisableRange<float>(50.0f, 5000.0f, 1.0f, 0.4f), 500.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::MAX_GAIN_DB, "Max Gain",
        juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f), 24.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // ── Expander ──────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::EXP_ENABLED, "Expander On", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::EXP_THRESHOLD, "Exp Threshold",
        juce::NormalisableRange<float>(-80.0f, -10.0f, 0.1f), -40.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::EXP_RATIO, "Exp Ratio",
        juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f), 2.0f,
        juce::AudioParameterFloatAttributes().withLabel(":1")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::EXP_ATTACK, "Exp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::EXP_RELEASE, "Exp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::EXP_KNEE, "Exp Knee",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // ── True-peak limiter ─────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::LIMITER_ENABLED, "Limiter On", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::LIMITER_CEILING, "TP Ceiling",
        juce::NormalisableRange<float>(-6.0f, 0.0f, 0.1f), -1.0f,
        juce::AudioParameterFloatAttributes().withLabel("dBTP")));

    // ── Lookahead ─────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::LOOKAHEAD_ENABLED, "Lookahead On", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::LOOKAHEAD_MS, "Lookahead",
        juce::NormalisableRange<float>(1.0f, 50.0f, 0.5f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    return { params.begin(), params.end() };
}

// ── Constructor ───────────────────────────────────────────────────────────────
LufsNormalizerProcessor::LufsNormalizerProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

// ── prepareToPlay ─────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int numCh = getTotalNumInputChannels();

    meter      .prepare(sampleRate, samplesPerBlock, numCh);
    expander   .prepare(sampleRate, samplesPerBlock, numCh);
    gainSmoother.prepare(sampleRate, samplesPerBlock, numCh);
    limiter    .prepare(sampleRate, samplesPerBlock, numCh);

    // Apply lookahead setting
    const bool  laEnabled = apvts.getRawParameterValue(ParamID::LOOKAHEAD_ENABLED)->load() > 0.5f;
    const float laMs      = apvts.getRawParameterValue(ParamID::LOOKAHEAD_MS)->load();
    gainSmoother.setLookaheadMs(laMs, laEnabled);

    // Report latency to host
    setLatencySamples(gainSmoother.getLatencySamples()
                    + limiter.getLatencySamples()); // limiter has 1 ms lookahead

    syncDspParameters();
}

void LufsNormalizerProcessor::releaseResources()
{
    meter.reset();
    expander.reset();
    gainSmoother.reset();
    limiter.reset();
}

// ── processBlock ─────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Silence any unused output channels
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // ── Sync parameters (cheap atomic reads) ─────────────────────────────────
    syncDspParameters();

    // ── 1. Expander ───────────────────────────────────────────────────────────
    expander.processBlock(buffer);

    // ── 2. LUFS measurement (on expanded signal) ──────────────────────────────
    meter.processBlock(buffer);

    // ── 3. Compute gain correction ────────────────────────────────────────────
    const bool levelerOn = apvts.getRawParameterValue(ParamID::LEVELER_ENABLED)->load() > 0.5f;

    if (levelerOn)
    {
        const float targetLufs  = apvts.getRawParameterValue(ParamID::TARGET_LUFS)->load();
        const float measuredLufs = meter.getShortTermLUFS(); // use short-term for stability

        // Guard against silence / unmeasured state
        if (measuredLufs > -140.0f)
        {
            const float errorDb = targetLufs - measuredLufs;
            gainSmoother.setTargetGainDb(errorDb);
        }
    }
    else
    {
        gainSmoother.setTargetGainDb(0.0f);
    }

    // ── 4. Apply smoothed gain ────────────────────────────────────────────────
    gainSmoother.processBlock(buffer);

    // ── 5. True-peak limiter ──────────────────────────────────────────────────
    limiter.processBlock(buffer);
}

// ── syncDspParameters ────────────────────────────────────────────────────────
void LufsNormalizerProcessor::syncDspParameters()
{
    // Expander
    expander.setEnabled    (apvts.getRawParameterValue(ParamID::EXP_ENABLED)  ->load() > 0.5f);
    expander.setThresholdDb(apvts.getRawParameterValue(ParamID::EXP_THRESHOLD)->load());
    expander.setRatio      (apvts.getRawParameterValue(ParamID::EXP_RATIO)    ->load());
    expander.setAttackMs   (apvts.getRawParameterValue(ParamID::EXP_ATTACK)   ->load());
    expander.setReleaseMs  (apvts.getRawParameterValue(ParamID::EXP_RELEASE)  ->load());
    expander.setKneeDb     (apvts.getRawParameterValue(ParamID::EXP_KNEE)     ->load());

    // Gain smoother
    gainSmoother.setAttackMs (apvts.getRawParameterValue(ParamID::ATTACK_MS) ->load());
    gainSmoother.setReleaseMs(apvts.getRawParameterValue(ParamID::RELEASE_MS)->load());
    gainSmoother.setMaxGainDb(apvts.getRawParameterValue(ParamID::MAX_GAIN_DB)->load());
    gainSmoother.setMinGainDb(-apvts.getRawParameterValue(ParamID::MAX_GAIN_DB)->load());

    // Limiter
    limiter.setEnabled   (apvts.getRawParameterValue(ParamID::LIMITER_ENABLED)->load() > 0.5f);
    limiter.setCeilingDb (apvts.getRawParameterValue(ParamID::LIMITER_CEILING)->load());
}

// ── Presets ───────────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::setCurrentProgram(int index)
{
    currentProgram = index;
    if (index == 0) return; // "Custom" – don't overwrite

    const int presetIdx = index - 1;
    if (presetIdx < 0 || presetIdx >= (int)std::size(kPresets)) return;

    const auto& p = kPresets[presetIdx];
    apvts.getParameter(ParamID::TARGET_LUFS)  ->setValueNotifyingHost(
        apvts.getParameterRange(ParamID::TARGET_LUFS).convertTo0to1(p.targetLufs));
    apvts.getParameter(ParamID::ATTACK_MS)    ->setValueNotifyingHost(
        apvts.getParameterRange(ParamID::ATTACK_MS).convertTo0to1(p.attackMs));
    apvts.getParameter(ParamID::RELEASE_MS)   ->setValueNotifyingHost(
        apvts.getParameterRange(ParamID::RELEASE_MS).convertTo0to1(p.releaseMs));
    apvts.getParameter(ParamID::EXP_THRESHOLD)->setValueNotifyingHost(
        apvts.getParameterRange(ParamID::EXP_THRESHOLD).convertTo0to1(p.expThreshold));
    apvts.getParameter(ParamID::EXP_RATIO)    ->setValueNotifyingHost(
        apvts.getParameterRange(ParamID::EXP_RATIO).convertTo0to1(p.expRatio));
}

const juce::String LufsNormalizerProcessor::getProgramName(int index)
{
    if (index == 0) return "Custom";
    const int pi = index - 1;
    if (pi >= 0 && pi < (int)std::size(kPresets))
        return kPresets[pi].name;
    return {};
}

// ── State save / load ─────────────────────────────────────────────────────────
void LufsNormalizerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LufsNormalizerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml(*xml);
        currentProgram = newState.getProperty("currentProgram", 0);
        apvts.replaceState(newState);
    }
}

// ── Editor ────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* LufsNormalizerProcessor::createEditor()
{
    return new LufsNormalizerEditor(*this);
}

// ── Factory function ──────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LufsNormalizerProcessor();
}