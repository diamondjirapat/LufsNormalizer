cmake_minimum_required(VERSION 3.22)

project(LufsNormalizer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────────────
# =============================================================================
# Clone JUCE into the project root or set JUCE_PATH to your local copy:
# JUCE
#   git clone https://github.com/juce-framework/JUCE.git JUCE
# =============================================================================
# Then configure with:

#   cmake -B build -DJUCE_PATH=./JUCE
# ---------------------------------------------------------------------------

if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    message(FATAL_ERROR
        "JUCE not found.\n"
        "Either:\n"
        "  1. Clone JUCE into ${CMAKE_CURRENT_SOURCE_DIR}/JUCE\n"
        "  2. Pass -DJUCE_PATH=<path> to cmake"
    )
endif()

# =============================================================================
# Plugin Target
# =============================================================================

juce_add_plugin(LufsNormalizer
    COMPANY_NAME                "Screammyzz"
    PLUGIN_MANUFACTURER_CODE    Yrcm
    PLUGIN_CODE                 Lnrm

    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "FS Master"

    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    VST3_CATEGORIES             Fx Dynamics

    COPY_PLUGIN_AFTER_BUILD     FALSE
)

juce_generate_juce_header(LufsNormalizer)

# =============================================================================
# Sources
# =============================================================================

target_sources(LufsNormalizer PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp

    src/dsp/LufsMeter.cpp
    src/dsp/Expander.cpp
    src/dsp/GainSmoother.cpp
    src/dsp/TruePeakLimiter.cpp

    src/ui/LufsDisplay.cpp
    src/ui/GainReductionMeter.cpp
    src/ui/LevelHistory.cpp
)

target_include_directories(LufsNormalizer PRIVATE
    src
)

# =============================================================================
# Compile Definitions
# =============================================================================

target_compile_definitions(LufsNormalizer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# =============================================================================
# Link Libraries
# =============================================================================

target_link_libraries(LufsNormalizer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra

        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# =============================================================================
# Testing
# =============================================================================

option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)

    enable_testing()

    # -------------------------------------------------------------------------
    # DSP Utils Tests
    # -------------------------------------------------------------------------

    add_executable(DspUtilsTests
        tests/test_dsp_utils.cpp
    )

    target_include_directories(DspUtilsTests PRIVATE
        src
    )

    add_test(NAME DspUtilsTests COMMAND DspUtilsTests)

    # -------------------------------------------------------------------------
    # Expander Tests
    # -------------------------------------------------------------------------

    add_executable(test_expander
        tests/test_expander.cpp
        src/dsp/Expander.cpp
    )

    target_include_directories(test_expander PRIVATE
        src
    )

    target_compile_definitions(test_expander PRIVATE
        UNIT_TEST=1
        JUCE_USE_CURL=0
    )

    target_link_libraries(test_expander PRIVATE
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME ExpanderTests COMMAND test_expander)

endif()
