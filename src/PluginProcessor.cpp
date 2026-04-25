#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

// ── Parameter layout ─────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
LufsNormalizerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ── Gate ──────────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::GATE_ENABLED, "Gate On", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::GATE_THRESHOLD, "Gate Thresh",
        juce::NormalisableRange<float>(-100.0f, 0.0f, 0.1f), -60.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::GATE_ATTACK, "Gate Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::GATE_RELEASE, "Gate Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

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

    // ── AutoGain ──────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamID::AUTOGAIN_ENABLED, "AutoGain On", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::AUTOGAIN_TARGET, "AutoGain Target",
        juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f), -18.0f,
        juce::AudioParameterFloatAttributes().withLabel("RMS")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::AUTOGAIN_SPEED, "AutoGain Speed",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 1.0f, 0.4f), 1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

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

    // ── Dry/Wet Mix ───────────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamID::DRY_WET, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

// ── Constructor ───────────────────────────────────────────────────────────────
LufsNormalizerProcessor::LufsNormalizerProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "Parameters", createParameterLayout())
{
    cacheParameterPointers();
}

// ── prepareToPlay ─────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int numCh = getTotalNumInputChannels();
    dryWetBuffer.setSize(numCh, samplesPerBlock, false, false, true);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numCh };
    gate       .prepare(spec);
    expander   .prepare(sampleRate, samplesPerBlock, numCh);
    autoGain   .prepare(sampleRate, samplesPerBlock, numCh);
    meter      .prepare(sampleRate, samplesPerBlock, numCh);
    gainSmoother.prepare(sampleRate, samplesPerBlock, numCh);
    limiter    .prepare(sampleRate, samplesPerBlock, numCh);

    // Apply lookahead setting
    const bool  laEnabled = pLookaheadEnabled->load() > 0.5f;
    const float laMs      = pLookaheadMs->load();
    gainSmoother.setLookaheadMs(laMs, laEnabled);

    // Report latency to host
    setLatencySamples(gainSmoother.getLatencySamples()
                    + limiter.getLatencySamples()); // limiter has 1 ms lookahead

    syncDspParameters();
}

void LufsNormalizerProcessor::releaseResources()
{
    dryWetBuffer.setSize(0, 0);
    gate.reset();
    expander.reset();
    autoGain.reset();
    meter.reset();
    gainSmoother.reset();
    limiter.reset();
}

// ── processBlock ─────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // ── Save dry signal for dry/wet mix ───────────────────────────────────────
    const float wetAmount = pDryWet->load() * 0.01f; // 0..1
    if (wetAmount < 0.999f)
    {
        const int numCh = std::min(buffer.getNumChannels(), dryWetBuffer.getNumChannels());
        const int numSamples = std::min(buffer.getNumSamples(), dryWetBuffer.getNumSamples());

        jassert(dryWetBuffer.getNumChannels() >= buffer.getNumChannels());
        jassert(dryWetBuffer.getNumSamples() >= buffer.getNumSamples());

        for (int ch = 0; ch < numCh; ++ch)
            dryWetBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // ── Sync parameters (cheap atomic reads via cached pointers) ─────────────
    syncDspParameters();

    // ── Sync lookahead at runtime ─────────────────────────────────────────────
    {
        const bool  laEnabled = pLookaheadEnabled->load() > 0.5f;
        const float laMs      = pLookaheadMs->load();
        gainSmoother.setLookaheadMs(laMs, laEnabled);

        const int newLatency = gainSmoother.getLatencySamples()
                             + limiter.getLatencySamples();
        if (newLatency != getLatencySamples())
            setLatencySamples(newLatency);
    }

    // ── 1. Gate ───────────────────────────────────────────────────────────────
    if (pGateEnabled->load() > 0.5f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        gate.process(context);
    }

    // ── 2. Expander ───────────────────────────────────────────────────────────
    expander.processBlock(buffer);

    // ── 3. AutoGain ───────────────────────────────────────────────────────────
    autoGain.processBlock(buffer);

    // ── 4. LUFS measurement (on signal after autogain) ────────────────────────
    meter.processBlock(buffer);

    // ── 5. Compute leveler gain correction ────────────────────────────────────
    const bool levelerOn = pLevelerEnabled->load() > 0.5f;

    if (levelerOn)
    {
        const float targetLufs   = pTargetLufs->load();
        const float measuredLufs = meter.getShortTermLUFS();
        const float momLufs      = meter.getMomentaryLUFS();
        const float gateThresh   = pGateThreshold->load();

        // Guard against silence / unmeasured state.
        // If the momentary signal is below the gate threshold, return gain to 0 dB
        // to prevent extreme gain build-up during silence.
        if (measuredLufs > -140.0f && momLufs > gateThresh)
        {
            const float errorDb = targetLufs - measuredLufs;
            gainSmoother.setTargetGainDb(errorDb);
        }
        else
        {
            gainSmoother.setTargetGainDb(0.0f);
        }
    }
    else
    {
        gainSmoother.setTargetGainDb(0.0f);
    }

    // ── 6. Apply smoothed gain ────────────────────────────────────────────────
    gainSmoother.processBlock(buffer);

    // ── 7. True-peak limiter ──────────────────────────────────────────────────
    limiter.processBlock(buffer);

    // ── 8. Dry/Wet mix ───────────────────────────────────────────────────────
    if (wetAmount < 0.999f)
    {
        const float dryAmount = 1.0f - wetAmount;
        const int numCh = std::min(buffer.getNumChannels(), dryWetBuffer.getNumChannels());
        const int numSamples = std::min(buffer.getNumSamples(), dryWetBuffer.getNumSamples());

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* wet = buffer.getWritePointer(ch);
            const float* dry = dryWetBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * dryAmount + wet[i] * wetAmount;
        }
    }
}

// ── syncDspParameters ────────────────────────────────────────────────────────
void LufsNormalizerProcessor::syncDspParameters()
{
    // Gate
    gate.setThreshold(pGateThreshold->load());
    gate.setAttack   (pGateAttack->load());
    gate.setRelease  (pGateRelease->load());

    // Expander
    expander.setEnabled    (pExpEnabled->load()   > 0.5f);
    expander.setThresholdDb(pExpThreshold->load());
    expander.setRatio      (pExpRatio->load());
    expander.setAttackMs   (pExpAttack->load());
    expander.setReleaseMs  (pExpRelease->load());
    expander.setKneeDb     (pExpKnee->load());

    // AutoGain
    autoGain.setEnabled    (pAutoGainEnabled->load() > 0.5f);
    autoGain.setTargetRmsDb(pAutoGainTarget->load());
    autoGain.setSpeedMs    (pAutoGainSpeed->load());
    autoGain.setGateThresholdDb(pGateThreshold->load());

    // Gain smoother
    gainSmoother.setAttackMs (pAttackMs->load());
    gainSmoother.setReleaseMs(pReleaseMs->load());
    gainSmoother.setMaxGainDb(pMaxGainDb->load());
    gainSmoother.setMinGainDb(-pMaxGainDb->load());

    // Limiter
    limiter.setEnabled   (pLimiterEnabled->load() > 0.5f);
    limiter.setCeilingDb (pLimiterCeiling->load());
}

// ── cacheParameterPointers ───────────────────────────────────────────────────
void LufsNormalizerProcessor::cacheParameterPointers()
{
    pGateThreshold   = apvts.getRawParameterValue(ParamID::GATE_THRESHOLD);
    pGateAttack      = apvts.getRawParameterValue(ParamID::GATE_ATTACK);
    pGateRelease     = apvts.getRawParameterValue(ParamID::GATE_RELEASE);
    pGateEnabled     = apvts.getRawParameterValue(ParamID::GATE_ENABLED);
    pExpEnabled      = apvts.getRawParameterValue(ParamID::EXP_ENABLED);
    pExpThreshold    = apvts.getRawParameterValue(ParamID::EXP_THRESHOLD);
    pExpRatio        = apvts.getRawParameterValue(ParamID::EXP_RATIO);
    pExpAttack       = apvts.getRawParameterValue(ParamID::EXP_ATTACK);
    pExpRelease      = apvts.getRawParameterValue(ParamID::EXP_RELEASE);
    pExpKnee         = apvts.getRawParameterValue(ParamID::EXP_KNEE);
    pAutoGainEnabled = apvts.getRawParameterValue(ParamID::AUTOGAIN_ENABLED);
    pAutoGainTarget  = apvts.getRawParameterValue(ParamID::AUTOGAIN_TARGET);
    pAutoGainSpeed   = apvts.getRawParameterValue(ParamID::AUTOGAIN_SPEED);
    pAttackMs        = apvts.getRawParameterValue(ParamID::ATTACK_MS);
    pReleaseMs       = apvts.getRawParameterValue(ParamID::RELEASE_MS);
    pMaxGainDb       = apvts.getRawParameterValue(ParamID::MAX_GAIN_DB);
    pTargetLufs      = apvts.getRawParameterValue(ParamID::TARGET_LUFS);
    pLevelerEnabled  = apvts.getRawParameterValue(ParamID::LEVELER_ENABLED);
    pLimiterEnabled  = apvts.getRawParameterValue(ParamID::LIMITER_ENABLED);
    pLimiterCeiling  = apvts.getRawParameterValue(ParamID::LIMITER_CEILING);
    pLookaheadEnabled = apvts.getRawParameterValue(ParamID::LOOKAHEAD_ENABLED);
    pLookaheadMs     = apvts.getRawParameterValue(ParamID::LOOKAHEAD_MS);
    pDryWet          = apvts.getRawParameterValue(ParamID::DRY_WET);
}

// ── Presets ───────────────────────────────────────────────────────────────────
void LufsNormalizerProcessor::setCurrentProgram(int index)
{
    currentProgram = index;
    if (index == 0) return; // "Custom" – don't overwrite

    const int presetIdx = index - 1;
    if (presetIdx < 0 || presetIdx >= (int)std::size(kPresets)) return;

    const auto& p = kPresets[presetIdx];

    auto setParam = [&](const char* id, float value) {
        apvts.getParameter(id)->setValueNotifyingHost(
            apvts.getParameterRange(id).convertTo0to1(value));
    };

    // Leveler
    setParam(ParamID::TARGET_LUFS,   p.targetLufs);
    setParam(ParamID::ATTACK_MS,     p.attackMs);
    setParam(ParamID::RELEASE_MS,    p.releaseMs);
    setParam(ParamID::MAX_GAIN_DB,   p.maxGainDb);
    // Expander
    setParam(ParamID::EXP_THRESHOLD, p.expThreshold);
    setParam(ParamID::EXP_RATIO,     p.expRatio);
    setParam(ParamID::EXP_KNEE,      p.expKnee);
    // AutoGain
    setParam(ParamID::AUTOGAIN_TARGET, p.autoGainTarget);
    setParam(ParamID::AUTOGAIN_SPEED,  p.autoGainSpeed);
    // Gate
    setParam(ParamID::GATE_THRESHOLD, p.gateThreshold);
    // Limiter
    setParam(ParamID::LIMITER_CEILING, p.limiterCeiling);
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
