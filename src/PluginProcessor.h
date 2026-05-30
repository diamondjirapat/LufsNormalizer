#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

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
    inline constexpr const char* AUTOGAIN_ATTACK  = "autoGainAttack";
    inline constexpr const char* AUTOGAIN_RELEASE = "autoGainRelease";
    inline constexpr const char* AUTOGAIN_MAXGAIN = "autoGainMaxGain";
    inline constexpr const char* AUTOGAIN_REDUCE  = "autoGainReduce";

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

    // Mix
    inline constexpr const char* DRY_WET           = "dryWet";
}

// ── Preset definitions ────────────────────────────────────────────────────────
struct Preset
{
    const char* name;
    // Leveler
    float targetLufs;
    float attackMs;
    float releaseMs;
    float maxGainDb;
    // Expander
    float expThreshold;
    float expRatio;
    float expKnee;
    // AutoGain
    float autoGainTarget;
    float autoGainAttack;
    float autoGainRelease;
    float autoGainMaxGain;
    // Gate
    float gateThreshold;
    // Limiter
    float limiterCeiling;
};

inline constexpr Preset kPresets[] = {
    //  name            tgt     atk     rel    max     eT     eR    eK     agT    agAtk    agRel   agMax    gT     lC
    { "Streaming",    -14.0f, 300.0f, 600.0f, 24.0f, -45.0f, 2.0f, 6.0f, -18.0f, 500.0f, 1000.0f, 12.0f, -60.0f, -1.0f },
    { "Podcast",      -16.0f, 200.0f, 500.0f, 18.0f, -40.0f, 2.5f, 6.0f, -18.0f, 400.0f,  800.0f, 12.0f, -55.0f, -1.0f },
    { "Broadcast",    -23.0f, 150.0f, 400.0f, 12.0f, -35.0f, 3.0f, 4.0f, -20.0f, 300.0f,  600.0f, 12.0f, -50.0f, -2.0f },
    { "Film / TV",    -24.0f, 200.0f, 800.0f, 12.0f, -50.0f, 1.5f, 8.0f, -24.0f, 800.0f, 2000.0f, 12.0f, -60.0f, -1.0f },
    { "Music Master", -14.0f, 400.0f,1000.0f, 12.0f, -60.0f, 1.5f, 6.0f, -16.0f, 600.0f, 1500.0f, 12.0f, -70.0f, -1.0f },
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
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;

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

    float getInputMomentaryLUFS()   const noexcept { return inputMeter.getMomentaryLUFS();  }
    float getInputShortTermLUFS()   const noexcept { return inputMeter.getShortTermLUFS();  }
    float getInputIntegratedLUFS()  const noexcept { return inputMeter.getIntegratedLUFS(); }

    float getOutputMomentaryLUFS()  const noexcept { return outputMeter.getMomentaryLUFS();  }
    float getOutputShortTermLUFS()  const noexcept { return outputMeter.getShortTermLUFS();  }
    float getOutputIntegratedLUFS() const noexcept { return outputMeter.getIntegratedLUFS(); }

    float getAnalysisShortTermLUFS() const noexcept { return analysisMeter.getShortTermLUFS(); }
    float getAnalysisMomentaryLUFS() const noexcept { return analysisMeter.getMomentaryLUFS(); }

    float getLevelerGainDb()  const noexcept { return gainSmoother.getCurrentGainDb(); }
    float getAutoGainDb()     const noexcept { return autoGain.getCurrentGainDb(); }
    float getExpanderGrDb()   const noexcept { return expander.getGainReductionDb();   }
    float getLimiterGrDb()    const noexcept { return limiter.getGainReductionDb();     }

    /** Input peak level in dBFS (before any processing). */
    float getInputPeakDb()   const noexcept { return inputPeakDb.load();  }
    /** Output peak level in dBFS (after full processing chain). */
    float getOutputPeakDb()  const noexcept { return outputPeakDb.load(); }
    /** Input RMS level in dBFS. */
    float getInputRmsDb()    const noexcept { return inputRmsDb.load();   }
    /** Output RMS level in dBFS. */
    float getOutputRmsDb()   const noexcept { return outputRmsDb.load();  }

    void resetIntegrated()
    {
        inputMeter.reset();
        analysisMeter.reset();
        outputMeter.reset();
    }

private:
    // ── APVTS ─────────────────────────────────────────────────────────────────
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── DSP chain ─────────────────────────────────────────────────────────────
    juce::dsp::NoiseGate<float> gate;
    Expander       expander;
    AutoGain       autoGain;
    LufsMeter      inputMeter;
    LufsMeter      analysisMeter;
    LufsMeter      outputMeter;
    GainSmoother   gainSmoother;
    TruePeakLimiter limiter;

    // ── State ─────────────────────────────────────────────────────────────────
    int   currentProgram = 0;
    float lastTargetLufs = -16.0f;
    juce::AudioBuffer<float> dryWetBuffer;

    // ── Cached parameter pointers (avoids string lookups per block) ───────────
    std::atomic<float>* pGateThreshold  = nullptr;
    std::atomic<float>* pGateAttack     = nullptr;
    std::atomic<float>* pGateRelease    = nullptr;
    std::atomic<float>* pGateEnabled    = nullptr;
    std::atomic<float>* pExpEnabled     = nullptr;
    std::atomic<float>* pExpThreshold   = nullptr;
    std::atomic<float>* pExpRatio       = nullptr;
    std::atomic<float>* pExpAttack      = nullptr;
    std::atomic<float>* pExpRelease     = nullptr;
    std::atomic<float>* pExpKnee        = nullptr;
    std::atomic<float>* pAutoGainEnabled = nullptr;
    std::atomic<float>* pAutoGainTarget = nullptr;
    std::atomic<float>* pAutoGainAttack = nullptr;
    std::atomic<float>* pAutoGainRelease = nullptr;
    std::atomic<float>* pAutoGainMaxGain = nullptr;
    std::atomic<float>* pAutoGainReduce = nullptr;
    std::atomic<float>* pAttackMs       = nullptr;
    std::atomic<float>* pReleaseMs      = nullptr;
    std::atomic<float>* pMaxGainDb      = nullptr;
    std::atomic<float>* pTargetLufs     = nullptr;
    std::atomic<float>* pLevelerEnabled = nullptr;
    std::atomic<float>* pLimiterEnabled = nullptr;
    std::atomic<float>* pLimiterCeiling = nullptr;
    std::atomic<float>* pLookaheadEnabled = nullptr;
    std::atomic<float>* pLookaheadMs    = nullptr;
    std::atomic<float>* pDryWet         = nullptr;

    // ── Metering state ────────────────────────────────────────────────────────
    std::atomic<float> inputPeakDb  { -100.0f };
    std::atomic<float> outputPeakDb { -100.0f };
    std::atomic<float> inputRmsDb   { -100.0f };
    std::atomic<float> outputRmsDb  { -100.0f };

    // ── Helpers ───────────────────────────────────────────────────────────────
    void syncDspParameters();
    void cacheParameterPointers();
    void measureLevels(const juce::AudioBuffer<float>& buffer,
                       std::atomic<float>& peakOut,
                       std::atomic<float>& rmsOut);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsNormalizerProcessor)
};
