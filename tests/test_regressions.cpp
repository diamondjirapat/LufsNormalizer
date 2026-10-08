#include "PluginProcessor.h"
#include "dsp/DspUtils.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
constexpr double kTestSampleRate = 48000.0;
constexpr double pi = 3.14159265358979323846;

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void near(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance || !std::isfinite(actual))
    {
        std::cerr << message << ": actual=" << actual << " expected=" << expected << '\n';
        throw std::runtime_error(message);
    }
}

void setParameter(LufsNormalizerProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.getAPVTS().getParameter(id);
    require(parameter != nullptr, "Parameter missing");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void disableDynamics(LufsNormalizerProcessor& processor)
{
    for (const auto* id : { ParamID::GATE_ENABLED, ParamID::EXP_ENABLED,
                           ParamID::COMP_ENABLED, ParamID::LEVELER_ENABLED,
                           ParamID::LIMITER_ENABLED })
        setParameter(processor, id, 0.0f);
}

void testLoudness()
{
    LufsMeter meter;
    meter.prepare(kTestSampleRate, 512, 1);
    juce::AudioBuffer<float> tone(1, 192000);
    for (int i = 0; i < tone.getNumSamples(); ++i)
        tone.setSample(0, i, 0.1f * (float)std::sin(2.0 * pi * 997.0 * i / kTestSampleRate));
    meter.processBlock(tone);
    near(meter.getMomentaryLUFS(), -23.01f, 0.08f, "997 Hz calibration");
    near(meter.getIntegratedLUFS(), -23.01f, 0.08f, "Integrated calibration");
    const float momentary = meter.getMomentaryLUFS();
    meter.resetIntegrated();
    near(meter.getIntegratedLUFS(), -144.0f, 0.0f, "Integration reset");
    near(meter.getMomentaryLUFS(), momentary, 0.0f, "Integration reset must preserve sliding window");
    juce::AudioBuffer<float> afterReset(tone.getArrayOfWritePointers(), 1, 0, 19199);
    meter.processBlock(afterReset);
    near(meter.getIntegratedLUFS(), -144.0f, 0.0f, "Reset integration must wait for fresh 400 ms block");

    meter.reset();
    juce::AudioBuffer<float> shortTone(tone.getArrayOfWritePointers(), 1, 0, 19199);
    meter.processBlock(shortTone);
    near(meter.getIntegratedLUFS(), -144.0f, 0.0f, "Do not integrate incomplete 400 ms blocks");

    LufsMeter whole, partitioned;
    whole.prepare(kTestSampleRate, 512, 1);
    partitioned.prepare(kTestSampleRate, 512, 1);
    for (int i = 0; i < tone.getNumSamples(); ++i)
    {
        const float amplitude = i < 48000 ? 0.1f : i < 96000 ? 0.001f : i < 180000 ? 0.3f : 0.0f;
        tone.setSample(0, i, amplitude * (float)std::sin(2.0 * pi * 997.0 * i / kTestSampleRate));
    }
    whole.processBlock(tone);
    const int sizes[] = { 1, 17, 127, 512, 31, 2048, 3 };
    int block = 0;
    for (int start = 0; start < tone.getNumSamples(); )
    {
        const int length = std::min(sizes[block++ % 7], tone.getNumSamples() - start);
        juce::AudioBuffer<float> part(tone.getArrayOfWritePointers(), 1, start, length);
        partitioned.processBlock(part);
        start += length;
    }
    near(partitioned.getMomentaryLUFS(), whole.getMomentaryLUFS(), 0.0001f, "Momentary block independence");
    near(partitioned.getShortTermLUFS(), whole.getShortTermLUFS(), 0.0001f, "Short-term block independence");
    near(partitioned.getIntegratedLUFS(), whole.getIntegratedLUFS(), 0.0001f, "Integrated block independence");
    juce::AudioBuffer<float> silence(1, 19200);
    silence.clear();
    partitioned.processBlock(silence);
    require(partitioned.getMomentaryLUFS() < -100.0f, "Momentary window must expire after 400 ms");
}

void testExpanderRatio()
{
    Expander expander;
    expander.prepare(kTestSampleRate, 512, 2);
    expander.setThresholdDb(-10.0f);
    expander.setKneeDb(0.0f);
    expander.setAttackMs(0.0f);
    expander.setReleaseMs(0.0f);
    juce::AudioBuffer<float> buffer(2, 512);
    const float input = DspUtils::dbToGain(-30.0f);
    for (const float ratio : { 1.0f, 2.0f, 4.0f })
    {
        expander.setRatio(ratio);
        for (int block = 0; block < 100; ++block)
        {
            for (int i = 0; i < 512; ++i)
            {
                buffer.setSample(0, i, input);
                buffer.setSample(1, i, input * 0.1f);
            }
            expander.processBlock(buffer);
        }
        near(DspUtils::gainToDb(buffer.getSample(0, 511) / input),
             -20.0f * (ratio - 1.0f), 0.01f, "Downward expansion ratio");
        near(buffer.getSample(1, 511) / buffer.getSample(0, 511), 0.1f, 0.00001f, "Stereo linked gain");
    }
    expander.setRatio(2.0f);
    expander.setKneeDb(6.0f);
    const float threshold = DspUtils::dbToGain(-10.0f);
    for (int block = 0; block < 100; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            buffer.setSample(0, i, threshold);
            buffer.setSample(1, i, threshold);
        }
        expander.processBlock(buffer);
    }
    near(DspUtils::gainToDb(buffer.getSample(0, 511) / threshold), -0.75f, 0.01f, "Soft knee at threshold");
}

void testLookahead()
{
    GainSmoother smoother;
    smoother.prepare(kTestSampleRate, 64, 1);
    smoother.setLookaheadMs(1.0f, true);
    juce::AudioBuffer<float> impulse(1, 128);
    impulse.clear();
    impulse.setSample(0, 0, 0.5f);
    smoother.processBlock(impulse);
    for (int i = 0; i < 128; ++i)
        near(impulse.getSample(0, i), i == 48 ? 0.5f : 0.0f, 0.0f, "Lookahead impulse latency");

    smoother.reset();
    smoother.setLookaheadMs(1.0f, false);
    impulse.clear();
    impulse.setSample(0, 127, 0.25f);
    smoother.processBlock(impulse);
    smoother.setLookaheadMs(1.0f, true);
    impulse.clear();
    smoother.processBlock(impulse);
    near(impulse.getSample(0, 47), 0.25f, 0.0f, "Enabling lookahead must use recent history");
}

void testLimiter()
{
    for (double rate : { 8000.0, 44100.0, 48000.0, 96000.0, 192000.0 })
    for (int channels : { 1, 2 })
    {
        TruePeakLimiter limiter;
        limiter.prepare(rate, 64, channels);
        limiter.setEnabled(false);
        juce::AudioBuffer<float> impulse(channels, 4096);
        impulse.clear();
        impulse.setSample(0, 0, 0.25f);
        limiter.processBlock(impulse);
        for (int i = 0; i < 4096; ++i)
            near(impulse.getSample(0, i), i == limiter.getLatencySamples() ? 0.25f : 0.0f,
                 0.0f, "Disabled limiter must preserve latency, including oversized blocks");

        for (double frequency : { 997.0, rate * 0.125, rate * 0.25, rate * 0.4, rate * 0.479 })
        {
            limiter.reset();
            limiter.setEnabled(true);
            limiter.setCeilingDb(-1.0f);
            juce::AudioBuffer<float> signal(channels, 12000);
            signal.clear();
            for (int i = 0; i < 10000; ++i)
            {
                // Repeated abrupt bursts, plus isolated over-ceiling transients.
                const float amplitude = i % 1400 < 700 ? 2.0f : 0.01f;
                const float value = amplitude * (float)std::sin(2.0 * pi * frequency * i / rate + pi / 4.0);
                for (int ch = 0; ch < channels; ++ch) signal.setSample(ch, i, value * (ch == 0 ? 1.0f : -0.5f));
            }
            signal.setSample(0, 11000, 4.0f);
            limiter.processBlock(signal);
            const float ceiling = DspUtils::dbToGain(-1.0f);
            require(signal.getMagnitude(0, signal.getNumSamples()) <= ceiling + 0.0001f, "Limiter transient sample ceiling");
            // Independent FIR reconstruction checks between output samples.
            juce::dsp::Oversampling<float> check((size_t)channels, 3,
                juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true);
            check.initProcessing((size_t)signal.getNumSamples());
            juce::dsp::AudioBlock<float> audio(signal);
            auto reconstructed = check.processSamplesUp(audio);
            float peak = 0.0f;
            for (size_t ch = 0; ch < reconstructed.getNumChannels(); ++ch)
                for (size_t i = 0; i < reconstructed.getNumSamples(); ++i)
                    peak = std::max(peak, std::abs(reconstructed.getChannelPointer(ch)[i]));
            if (peak > ceiling + 0.001f) std::cerr << "Frequency=" << frequency << " true peak=" << peak << '\n';
            require(peak <= ceiling + 0.001f, "Limiter reconstructed true-peak ceiling");
        }
    }
}

void testProcessor()
{
    for (bool bypassed : { false, true })
    for (float mix : { 0.0f, 50.0f, 99.9f, 100.0f })
    {
        LufsNormalizerProcessor processor;
        if (!bypassed) disableDynamics(processor);
        setParameter(processor, ParamID::DRY_WET, mix);
        setParameter(processor, ParamID::LOOKAHEAD_ENABLED, 1.0f);
        setParameter(processor, ParamID::LOOKAHEAD_MS, 5.0f);
        processor.prepareToPlay(kTestSampleRate, 64);
        juce::AudioBuffer<float> impulse(2, 2048);
        impulse.clear();
        impulse.setSample(0, 0, 0.25f);
        juce::MidiBuffer midi;
        if (bypassed) processor.processBlockBypassed(impulse, midi);
        else processor.processBlock(impulse, midi);
        const int latency = processor.getLatencySamples();
        require(latency >= 288, "Host latency must include both lookahead stages");
        for (int i = 0; i < impulse.getNumSamples(); ++i)
            near(impulse.getSample(0, i), i == latency ? 0.25f : 0.0f, 0.00001f, "Dry/wet latency alignment");
    }

    LufsNormalizerProcessor processor;
    disableDynamics(processor);
    processor.prepareToPlay(kTestSampleRate, 512);
    juce::AudioBuffer<float> tone(2, 48000);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < tone.getNumSamples(); ++i)
            tone.setSample(ch, i, 0.1f * (float)std::sin(2.0 * pi * 997.0 * i / kTestSampleRate));
    juce::MidiBuffer midi;
    processor.processBlock(tone, midi);
    require(processor.getInputIntegratedLUFS() > -140.0f, "Processor integration");
    const float beforeReset = processor.getInputMomentaryLUFS();
    processor.resetIntegrated();
    require(processor.getInputIntegratedLUFS() > -140.0f, "Meter reset must be queued to audio thread");
    juce::AudioBuffer<float> next(2, 1);
    next.clear();
    processor.processBlock(next, midi);
    near(processor.getInputIntegratedLUFS(), -144.0f, 0.0f, "Queued integration reset");
    near(processor.getInputMomentaryLUFS(), beforeReset, 0.01f, "Reset must preserve meter filters");

    processor.setCurrentProgram(2);
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    LufsNormalizerProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    require(restored.getCurrentProgram() == 2, "Preset state round trip");
    near(restored.getAPVTS().getRawParameterValue(ParamID::TARGET_LUFS)->load(), -16.0f, 0.0f, "Parameter state round trip");
    auto invalid = restored.getAPVTS().copyState();
    invalid.setProperty("currentProgram", 999, nullptr);
    auto xml = invalid.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml, state);
    restored.setStateInformation(state.getData(), (int)state.getSize());
    require(restored.getCurrentProgram() < restored.getNumPrograms(), "Clamp invalid saved preset index");

    LufsNormalizerProcessor mono;
    auto layout = mono.getBusesLayout();
    layout.inputBuses.set(0, juce::AudioChannelSet::mono());
    layout.outputBuses.set(0, juce::AudioChannelSet::mono());
    require(mono.setBusesLayout(layout), "Mono layout must be supported");
    mono.prepareToPlay(kTestSampleRate, 127);
    juce::AudioBuffer<float> monoTone(1, 4096);
    for (int preset = 1; preset < mono.getNumPrograms(); ++preset)
    {
        mono.setCurrentProgram(preset);
        for (int i = 0; i < monoTone.getNumSamples(); ++i)
            monoTone.setSample(0, i, (float)std::sin(2.0 * pi * 997.0 * i / kTestSampleRate));
        mono.processBlock(monoTone, midi);
        for (int i = 0; i < monoTone.getNumSamples(); ++i)
            require(std::isfinite(monoTone.getSample(0, i)), "All presets must produce finite mono audio");
    }
    mono.releaseResources();
    mono.prepareToPlay(44100.0, 32);
    monoTone.clear();
    mono.processBlock(monoTone, midi);
    near(monoTone.getMagnitude(0, monoTone.getNumSamples()), 0.0f, 0.0f, "Reprepare must clear delay history");
}

void testEditor()
{
    LufsNormalizerProcessor processor;
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    for (const auto size : { juce::Point<int>(780, 460), juce::Point<int>(1020, 600), juce::Point<int>(1600, 960) })
    {
        editor->setSize(size.x, size.y);
        for (int i = 0; i < editor->getNumChildComponents(); ++i)
        {
            const auto* child = editor->getChildComponent(i);
            if (child->isVisible())
                require(editor->getLocalBounds().contains(child->getBounds()), "Editor control exceeds window bounds");
        }
        const auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
        const auto file = juce::File::getCurrentWorkingDirectory().getChildFile("editor-" + juce::String(size.x) + ".png");
        auto stream = file.createOutputStream();
        require(stream != nullptr, "Cannot save editor snapshot");
        stream->setPosition(0);
        stream->truncate();
        require(juce::PNGImageFormat().writeImageToStream(snapshot, *stream), "Cannot encode editor snapshot");
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        testLoudness();
        testExpanderRatio();
        testLookahead();
        testLimiter();
        testProcessor();
        testEditor();
        std::cout << "All DSP and processor regression checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
