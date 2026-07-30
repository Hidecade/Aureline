# Aureline Mobile

Aureline Mobile is the iPhone and AUv3 version of Aureline. It shares the C++ synthesis engine in `../../Source/Engine` and `../../Source/DSP`, while providing a native SwiftUI interface and Apple audio/MIDI integration.

## Current implementation

- iPhone landscape SwiftUI application
- AVAudioEngine stereo output
- Core MIDI note, velocity, pitch bend, modulation wheel, sustain, and all-notes-off input
- Play and Edit screens
- Shared 32-voice factory library used by desktop and iOS
- Cross-platform `.aurelinevoice` JSON import and export
- LOAD and PASTE temporarily apply a sound to the selected numbered voice
- STORE is the only action that permanently overwrites the selected numbered voice
- SAVE exports the current sound to an external file without modifying the selected slot
- Eight writable 32-slot banks; Circuit is bank 3, Retro bank 4, 8-Bit bank 5, and the dedicated DRUM KIT is bank 8
- SAVE BANK exports the selected bank as a Mac-compatible `.aurelinelibrary.xml`
- LOAD accepts both file types and asks which bank to replace for a library
- `Analog.aurelinelibrary.xml`, `Analog2.aurelinelibrary.xml`,
  `Circuit.aurelinelibrary.xml`, `Retro.aurelinelibrary.xml`, and
  `8-Bit.aurelinelibrary.xml` are bundled
  from `assets/` and installed as protected libraries in the Aureline documents
  folder
- first launch selects slot 01; later launches restore the last selected slot
- Oscillator, filter/envelope, modulation, and performance parameter editing
- On-screen keyboard and octave switching
- Shared arpeggiator, diatonic chord, hold, tempo, rate, direction, gate, and scale-root controls
- Objective-C++ bridge with a fixed-capacity real-time command queue
- AUv3 instrument with sample-accurate MIDI event dispatch, all synthesis/performance parameters, and state restoration
- Audio interruption, route change, and media-services-reset handling

Real-device performance profiling and host compatibility testing remain release follow-up work.

## Generate the project

```bash
cd iOS/AurelineMobile
xcodegen generate
```

## Simulator build

```bash
xcodebuild \
  -project AurelineMobile.xcodeproj \
  -scheme AurelineMobile \
  -sdk iphonesimulator \
  -destination 'generic/platform=iOS Simulator' \
  -derivedDataPath ../../build/ios-mobile \
  CODE_SIGNING_ALLOWED=NO \
  build
```

For a device build, select the `AurelineMobile` scheme in Xcode and configure signing for both the app and AUv3 extension targets.
