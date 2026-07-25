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

The JUCE Standalone app is built from the same plug-in processor as the VST3
instrument and macOS Audio Unit. All formats share the same audio/MIDI path and
1024 x 668 performance UI. All plug-in state is restored with the host project.

Build the three macOS installer packages into `dist/` with:

```sh
./scripts/build-macos-installers.sh
```

Set `AURELINE_APPLICATION_SIGN_IDENTITY` and `AURELINE_INSTALLER_SIGN_IDENTITY`
for distribution signing. Without them, the script creates ad-hoc signed local
development packages.

## Voice and Wave file extensions

Aureline reserves the following product-specific file extensions:

| Data | Extension | Contents | Status |
|---|---|---|---|
| Single voice | `.aurelinevoice` | Shared Mac/iPhone JSON containing one voice, including synth parameters, voice mode, and Wave Memory data | Implemented on Mac and iPhone |
| Wave Memory | `.aurelinewave` | One 32-step Wave Memory waveform, its character, and format version | Reserved; import/export not implemented |
| Complete voice library | `.aurelinelibrary.xml` | All 50 numbered voices in one XML library or backup file | Implemented on Mac |

The naming follows OpalineFM: `.opalinevoice` and `.opalinelibrary.xml`
correspond to Aureline's `.aurelinevoice` and `.aurelinelibrary.xml`.
The Wave Memory format uses the same product-plus-data-type rule:
`.aurelinewave`.

Mac and iPhone both use `.aurelinevoice` for single-voice files.

LOAD and PASTE temporarily apply their voice data to the currently selected
numbered voice. They do not modify that slot's saved data. STORE is the only
operation that overwrites the selected numbered voice; without STORE, selecting
another voice and returning restores the previously stored override, or the
factory voice when no override exists. SAVE exports the current sound as an
external `.aurelinevoice` file without modifying the selected slot.

On Mac, SAVE ALL LIBRARY exports the last stored contents of all 50 numbered
voices to one `.aurelinelibrary.xml` file. Opening that file with LOAD shows a
confirmation before all 50 numbered voices are replaced.

The Mac version keeps `factory.aurelinelibrary.xml` in the Aureline documents
folder as the factory-reset library. Loading it restores all 50 slots after
confirmation. SAVE ALL LIBRARY refuses to overwrite this reserved file name.

`RetroGame.aurelinelibrary.xml` is also provided in the same folder. It contains
50 original retro-game voices covering early 8-bit pulse/triangle sounds,
square-wave PSG tones, multi-channel 5-bit wavetable colours, compact 1980s
arcade-style wave-memory voices, and noise effects. Its built-in file name is
also protected from SAVE ALL LIBRARY.

Raw `.xml` and `.json` files may be accepted for debugging or compatibility
imports, but they are not the standard user-facing extensions.

## Build the engine tests

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

On macOS, launch the standalone build with:

```sh
open build/Aureline_Plugin_artefacts/Standalone/Aureline.app
```

During local development, CMake can reuse JUCE from the sibling OpalineFM
checkout. For independent builds, set `AURELINE_JUCE_DIR` or place JUCE at
`external/JUCE`.

See [docs/Aureline_Spec_ja.md](docs/Aureline_Spec_ja.md) for the product scope.
