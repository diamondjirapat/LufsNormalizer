# LUFS Normalizer — VST3 Plugin

A professional real-time loudness normalizer built with JUCE, implementing
EBU R128 / ITU-R BS.1770-4 measurement with an integrated downward expander,
smoothed gain compensation, true-peak limiting, and lookahead.

---

## Signal Chain

```
Input → Gate → Expander → Compressor → LUFS Meter → Gain Smoother → True-Peak Limiter → Dry/Wet Mix → Output
```

| Stage | Purpose |
|---|---|
| **Gate** | Attenuates audio below a configurable threshold |
| **Expander** | Downward expansion with soft knee — attenuates noise floor during quiet sections |
| **Compressor** | Controls dynamic range with manual or automatic makeup gain |
| **LUFS Meter** | EBU R128 K-weighted measurement: momentary (400 ms), short-term (3 s), integrated (gated) |
| **Gain Smoother** | Converts LUFS error to a smoothed gain correction with configurable attack/release |
| **True-Peak Limiter** | 4× FIR peak detection, stereo linked gain, and at least 1 ms lookahead; reserves 0.5 dB for output reconstruction |

The dry path uses the same delay as the processed path. Host bypass also preserves
the reported latency. Disabling the limiter bypasses gain reduction while retaining
its delay. At sample rates where the FIR detector requires more than 1 ms, its
lookahead extends to cover the detector latency. The ceiling applies to the wet
limiter output; mixing in an over-ceiling dry signal can exceed it.

---

## Building

### Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.22 |
| C++ compiler | C++17 (MSVC 2019+, GCC 9+, Clang 10+) |
| JUCE | 7.x or 8.x |

### Steps

```bash
# 1. Clone JUCE into the project folder
git clone https://github.com/juce-framework/JUCE.git LufsNormalizer/JUCE

# 2. Configure
cmake -B LufsNormalizer/build -S LufsNormalizer -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build LufsNormalizer/build --config Release --parallel

# 4. The VST3 is in:
#    LufsNormalizer/build/LufsNormalizer_artefacts/Release/VST3/
```

#### Windows (Visual Studio)
```bat
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

#### macOS (Xcode or Ninja)
```bash
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Linux
```bash
# Install JUCE dependencies first:
sudo apt install libasound2-dev libfreetype6-dev libx11-dev libxext-dev \
                 libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev \
                 libjack-jackd2-dev libcurl4-openssl-dev

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Alternate: JUCE in a custom location
```bash
cmake -B build -S . -DJUCE_PATH=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
```

### Running tests

Tests are enabled by default. After building, run:

```bash
ctest --test-dir build -C Release --output-on-failure
```

The regression suite uses real JUCE DSP and covers calibrated loudness, varying
block sizes, gated integration, expansion ratios, lookahead, dry/wet and host
bypass latency, state restoration, mono presets, and independent 8× FIR checks
of limiter output peaks at 8, 44.1, 48, 96, and 192 kHz. It also saves editor
snapshots at the minimum, default, and maximum window sizes in the build folder.
The existing assertion-based tests remain active in Release builds.

---

## Parameters

### LUFS Leveler
| Parameter | Range | Default | Description |
|---|---|---|---|
| Target LUFS | -36 … -6 LUFS | -16 | Target short-term loudness for gain correction |
| Attack | 10 … 2000 ms | 200 | How fast gain increases |
| Release | 50 … 5000 ms | 500 | How fast gain decreases |
| Max Gain | 0 … 36 dB | 24 | Maximum gain boost/cut |

### Expander
| Parameter | Range | Default | Description |
|---|---|---|---|
| Threshold | -80 … -10 dB | -40 | Level below which expansion begins |
| Ratio | 1 … 10 :1 | 2 | Expansion ratio |
| Attack | 0.1 … 100 ms | 10 | Expander attack time |
| Release | 10 … 1000 ms | 100 | Expander release time |
| Knee | 0 … 24 dB | 6 | Soft-knee width |

### True-Peak Limiter
| Parameter | Range | Default | Description |
|---|---|---|---|
| Ceiling | -6 … 0 dBTP | -1 | True-peak ceiling |

### Lookahead
| Parameter | Range | Default | Description |
|---|---|---|---|
| Lookahead | 1 … 50 ms | 5 | Delay for anticipatory gain changes |

---

## Presets

| Preset | Target | Notes |
|---|---|---|
| Streaming | -14 LUFS | Spotify / Apple Music / YouTube |
| Podcast | -16 LUFS | Podcast platforms |
| Broadcast | -23 LUFS | EBU R128 broadcast standard |

---

## Architecture Notes

- **Thread safety**: DSP parameters and published meter values use `std::atomic`. Meter reset requests are consumed on the audio thread; preset selection changes parameters without touching live DSP state.
- **Denormal protection**: `juce::ScopedNoDenormals` in `processBlock`.
- **Latency reporting**: Plugin reports lookahead + limiter latency to the host for PDC.
- **State persistence**: Full parameter state saved/loaded via APVTS XML serialization.
- **Loudness measurement**: Sample-based 400 ms and 3 s windows, with full 400 ms integration blocks advanced every 100 ms. Integrated gating uses a bounded 0.1 LU histogram with exact energy sums; relative-gate decisions are quantized to the histogram boundaries. Reset Integrated starts a fresh programme measurement and preserves the live filters and sliding windows.
- **Block sizes**: Blocks larger than the prepared maximum are processed in preallocated chunks.

---

## File Structure

```
LufsNormalizer/
├── CMakeLists.txt
├── README.md
└── src/
    ├── PluginProcessor.h/.cpp   — AudioProcessor, parameter layout, DSP orchestration
    ├── PluginEditor.h/.cpp      — JUCE GUI, meters, knobs, timer-driven refresh
    ├── dsp/
    │   ├── LufsMeter.h/.cpp     — EBU R128 K-weighting + gated integration
    │   ├── Expander.h/.cpp      — Downward expander with soft knee
    │   ├── GainSmoother.h/.cpp  — Smoothed gain compensation + lookahead delay
    │   └── TruePeakLimiter.h/.cpp — 4× oversampled brickwall limiter
    └── ui/
        ├── LufsDisplay.h/.cpp   — Vertical LUFS bar meter
        ├── GainReductionMeter.h/.cpp — Horizontal GR meter
        └── LevelHistory.h/.cpp  — Scrolling LUFS history graph
```
