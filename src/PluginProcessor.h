#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/LufsMeter.h"
#include "dsp/Expander.h"
#include "dsp/AutoGain.h"
#include "dsp/GainSmoother.h"
#include "dsp/TruePeakLimiter.h"

// ── Parameter IDs ─────────────────────────────────────────────────────────────
namespace ParamID
{
    // LUFS leveler
    inline constexpr const char* TARGET_LUFS      = "targetLufs";
    inline constexpr const char* LEVELER_ENABLED  = "levelerEnabled";
    inline constexpr const char* ATTACK_MS        = "attackMs";
    inline constexpr const char* RELEASE_MS       = "releaseMs";
    inline constexpr const char* MAX_GAIN_DB      = "maxGainDb";

    // Gate
    inline constexpr const char* GATE_ENABLED     = "gateEnabled";
    inline constexpr const char* GATE_THRESHOLD   = "gateThreshold";
    inline constexpr const char* GATE_ATTACK      = "gateAttack";
    inline constexpr const char* GATE_RELEASE     = "gateRelease";

    // AutoGain
    inline constexpr const char* AUTOGAIN_ENABLED = "autoGainEnabled";
    inline constexpr const char* AUTOGAIN_TARGET  = "autoGainTarget";
    inline constexpr const char* AUTOGAIN_SPEED   = "autoGainSpeed";

    // Expander
    inline constexpr const char* EXP_ENABLED      = "expEnabled";
    inline constexpr const char* EXP_THRESHOLD    = "expThreshold";
    inline constexpr const char* EXP_RATIO        = "expRatio";
    inline constexpr const char* EXP_ATTACK       = "expAttack";
    inline constexpr const char* EXP_RELEASE      = "expRelease";
    inline constexpr const char* EXP_KNEE         = "expKnee";

    // True-peak limiter
    inline constexpr const char* LIMITER_ENABLED  = "limiterEnabled";
    inline constexpr const char* LIMITER_CEILING  = "limiterCeiling";

    // Lookahead
    inline constexpr const char* LOOKAHEAD_ENABLED = "lookaheadEnabled";
    inline constexpr const char* LOOKAHEAD_MS      = "lookaheadMs";
}

// ── Preset definitions ────────────────────────────────────────────────────────
struct Preset
{
    const char* name;
    float targetLufs;
    float attackMs;
    float releaseMs;
    float expThreshold;
    float expRatio;
};

inline constexpr Preset kPresets[] = {
    { "Streaming",  -14.0f, 300.0f, 600.0f, -45.0f, 2.0f },
    { "Podcast",    -16.0f, 200.0f, 500.0f, -40.0f, 2.5f },
    { "Broadcast",  -23.0f, 150.0f, 400.0f, -35.0f, 3.0f },
};

// ─────────────────────────────────────────────────────────────────────────────
class LufsNormalizerProcessor  : public juce::AudioProcessor
{
public:
    LufsNormalizerProcessor();
    ~LufsNormalizerProcessor() override = default;

    // ── AudioProcessor overrides ─────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LUFS Normalizer"; }

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()    override { return (int)std::size(kPresets) + 1; }
    int  getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── Public accessors for the editor ──────────────────────────────────────
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    float getMomentaryLUFS()  const noexcept { return meter.getMomentaryLUFS();  }
    float getShortTermLUFS()  const noexcept { return meter.getShortTermLUFS();  }
    float getIntegratedLUFS() const noexcept { return meter.getIntegratedLUFS(); }

    float getLevelerGainDb()  const noexcept { return gainSmoother.getCurrentGainDb(); }
    float getAutoGainDb()     const noexcept { return autoGain.getCurrentGainDb(); }
    float getExpanderGrDb()   const noexcept { return expander.getGainReductionDb();   }
    float getLimiterGrDb()    const noexcept { return limiter.getGainReductionDb();     }

    void resetIntegrated() { meter.reset(); }

private:
    // ── APVTS ─────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── DSP chain ─────────────────────────────────────────────────────────────
    juce::dsp::NoiseGate<float> gate;
    Expander       expander;
    AutoGain       autoGain;
    LufsMeter      meter;
    GainSmoother   gainSmoother;
    TruePeakLimiter limiter;

    // ── State ─────────────────────────────────────────────────────────────────
    int   currentProgram = 0;
    float lastTargetLufs = -16.0f;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void syncDspParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsNormalizerProcessor)
};
