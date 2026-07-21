# Aureline

Aureline is an eight-voice analog-modeling synthesizer by Hidecade Instruments.
It combines two band-limited VCOs per voice with a resonant four-pole low-pass
filter, two ADSR envelopes, LFO modulation, poly modulation, and unison.

The project is a separate product from Opaline FM. Product-specific engines,
patches, user interfaces, presets, and plug-in identifiers remain independent.
Only proven, product-neutral realtime utilities may later move into a shared
Hidecade Audio Core library.

## Current status

The repository contains the first headless DSP milestone:

- two oscillators per voice;
- saw, pulse, and triangle waveforms;
- amplifier and filter ADSR envelopes;
- resonant four-pole low-pass filter;
- eight-voice allocation and voice stealing;
- sustain pedal and configurable pitch bend;
- LFO, noise, modulation wheel, and per-voice poly modulation;
- oscillator hard sync and independent pulse-width modulation;
- poly, mono-legato, and eight-voice unison modes with glide;
- constant-power stereo voice spread;
- velocity and keyboard tracking for the filter;
- CMake-based engine tests.

The JUCE standalone app, VST3 instrument, and macOS Audio Unit are available
with the same 1024 x 668 performance UI. All plug-in state is restored with the
host project.

Build the three macOS installer packages into `dist/` with:

```sh
./scripts/build-macos-installers.sh
```

Set `AURELINE_APPLICATION_SIGN_IDENTITY` and `AURELINE_INSTALLER_SIGN_IDENTITY`
for distribution signing. Without them, the script creates ad-hoc signed local
development packages.

## Build the engine tests

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

On macOS, launch the standalone build with:

```sh
open build/Aureline_Standalone_artefacts/Aureline.app
```

During local development, CMake can reuse JUCE from the sibling OpalineFM
checkout. For independent builds, set `AURELINE_JUCE_DIR` or place JUCE at
`external/JUCE`.

See [docs/Aureline_Spec_ja.md](docs/Aureline_Spec_ja.md) for the product scope.
