#!/usr/bin/env python3
"""Generate the 32-voice Aureline DRUM KIT bank."""

from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "Analog.aurelinelibrary.xml"
OUTPUT = ROOT / "assets" / "Circuit.aurelinelibrary.xml"


def voice(name: str, **values: object) -> tuple[str, dict[str, object]]:
    return name, values


VOICES = [
    voice("DEEP KICK", waveA=2, octaveA=-2, levelA=.92, cutoff=680, resonance=.16,
          filterEnv=.08, ampD=.72, ampR=.08, filterD=.18, polyEnv=.82,
          polyPitch=1, master=.88),
    voice("SHORT KICK", waveA=2, octaveA=-2, levelA=.94, cutoff=920, resonance=.12,
          filterEnv=.10, ampD=.24, ampR=.035, filterD=.075, polyEnv=.68,
          polyPitch=1, master=.90),
    voice("BOOM KICK", waveA=2, octaveA=-2, levelA=.96, cutoff=540, resonance=.20,
          filterEnv=.06, ampD=1.35, ampR=.12, filterD=.30, polyEnv=.90,
          polyPitch=1, master=.84),
    voice("TUNED KICK", waveA=2, octaveA=-1, levelA=.90, cutoff=1050, resonance=.14,
          filterEnv=.12, ampD=.58, ampR=.08, filterD=.14, polyEnv=.60,
          polyPitch=1, keyTrack=.18, master=.88),
    voice("CLICK KICK", waveA=2, waveB=4, octaveA=-2, octaveB=1, levelA=.88,
          levelB=.08, cutoff=1800, resonance=.12, filterEnv=.24, ampD=.32,
          ampR=.04, filterD=.045, polyEnv=.72, polyPitch=1, master=.90),
    voice("SUB DROP", waveA=2, octaveA=-2, levelA=.96, cutoff=520, resonance=.10,
          filterEnv=.04, ampD=1.80, ampR=.20, filterD=.55, polyEnv=1.0,
          polyPitch=1, master=.78),
    voice("CLASSIC SNARE", waveA=2, waveB=2, octaveA=-1, octaveB=-1, levelA=.35,
          levelB=.28, fine=38, noise=.64, cutoff=5200, resonance=.25,
          filterEnv=.34, ampD=.34, ampR=.06, filterD=.16, master=.91),
    voice("TIGHT SNARE", waveA=2, waveB=2, octaveA=0, octaveB=-1, levelA=.34,
          levelB=.25, fine=31, noise=.66, cutoff=8200, resonance=.16,
          filterEnv=.34, ampD=.20, ampR=.04, filterD=.075, master=.93),
    voice("NOISE SNARE", waveA=0, waveB=0, levelA=0, levelB=0, noise=.88,
          cutoff=4600, resonance=.38, filterEnv=.32, ampD=.28, ampR=.045,
          filterD=.12, master=.88),
    voice("RIM SHOT", waveA=4, waveB=4, octaveA=1, octaveB=2, levelA=.52,
          levelB=.34, fine=17, noise=.06, cutoff=6200, resonance=.62,
          filterEnv=.48, ampD=.075, ampR=.025, filterD=.05, polyB=.22,
          polyPitch=1, master=.86),
    voice("CLAVES", waveA=2, waveB=0, octaveA=2, levelA=.72, levelB=0,
          cutoff=7600, resonance=.52, filterEnv=.22, ampD=.09, ampR=.025,
          filterD=.055, master=.82),
    voice("HAND CLAP", waveA=0, waveB=0, levelA=0, levelB=0, noise=.94,
          cutoff=2100, resonance=.34, filterMode=1, filterEnv=.14,
          ampD=.34, ampR=.12, filterD=.11, lfoRate=31,
          lfoAmount=.62, lfoFilter=1, lfoMask=8, lfoRetrig=1,
          master=.90),
    voice("MARACAS", waveA=0, waveB=0, levelA=0, levelB=0, noise=.84,
          cutoff=9200, resonance=.18, filterEnv=.18, ampD=.065, ampR=.02,
          filterD=.04, master=.78),
    voice("COWBELL", waveA=4, waveB=4, octaveA=1, octaveB=1, levelA=.58,
          levelB=.54, fine=680, cutoff=2640, resonance=.56, filterEnv=.08,
          ampD=.22, ampR=.045, filterD=.09, pulseA=.5, pulseB=.5,
          filterMode=2, master=.39),
    voice("LOW TOM", waveA=2, octaveA=-1, transpose=-3, levelA=.90,
          noise=.025, cutoff=1250, resonance=.22, filterEnv=.18,
          ampD=.48, ampR=.065, filterD=.075, polyEnv=.30,
          polyPitch=1, master=.88),
    voice("MID TOM", waveA=2, octaveA=-1, transpose=4, levelA=.88,
          noise=.022, cutoff=1750, resonance=.20, filterEnv=.17,
          ampD=.40, ampR=.055, filterD=.065, polyEnv=.27,
          polyPitch=1, master=.87),
    voice("HIGH TOM", waveA=2, octaveA=-1, transpose=9, levelA=.86,
          noise=.02, cutoff=2350, resonance=.18, filterEnv=.16,
          ampD=.33, ampR=.045, filterD=.055, polyEnv=.24,
          polyPitch=1, master=.86),
    voice("LOW CONGA", waveA=2, octaveA=-1, levelA=.82, cutoff=1900,
          resonance=.34, filterEnv=.18, ampD=.30, ampR=.045, filterD=.10,
          polyEnv=.24, polyPitch=1, master=.86),
    voice("MID CONGA", waveA=2, octaveA=0, levelA=.80, cutoff=3000,
          resonance=.32, filterEnv=.16, ampD=.25, ampR=.04, filterD=.085,
          polyEnv=.22, polyPitch=1, master=.84),
    voice("HIGH CONGA", waveA=2, octaveA=1, levelA=.78, cutoff=4600,
          resonance=.30, filterEnv=.14, ampD=.21, ampR=.035, filterD=.07,
          polyEnv=.20, polyPitch=1, master=.82),
    voice("CLOSED HAT", waveA=12, waveB=12, octaveA=2, octaveB=3,
          waveIndexA=6, waveIndexB=14, waveCharacterA=2, waveCharacterB=2,
          levelA=.13, levelB=.11, fine=517, noise=.64, cutoff=10500,
          resonance=.14, filterEnv=.12, ampD=.07, ampR=.018,
          filterD=.035, polyB=.72,
          polyPitch=1, master=.76),
    voice("OPEN HAT", waveA=12, waveB=12, octaveA=2, octaveB=3,
          waveIndexA=6, waveIndexB=14, waveCharacterA=2, waveCharacterB=2,
          levelA=.12, levelB=.10, fine=517, noise=.66, cutoff=9800,
          resonance=.12, filterEnv=.10, ampD=.68, ampR=.16,
          filterD=.24, polyB=.68,
          polyPitch=1, master=.74),
    voice("METAL HAT", waveA=12, waveB=12, octaveA=2, octaveB=3,
          waveIndexA=6, waveIndexB=14, waveCharacterA=2, waveCharacterB=2,
          levelA=.17, levelB=.14, fine=517, noise=.54, cutoff=8600,
          resonance=.16, filterEnv=.12, ampD=.16, ampR=.035,
          filterD=.07, polyB=.78,
          polyPitch=1, master=.76),
    voice("METAL CYMBAL", waveA=12, waveB=12, octaveA=2, octaveB=2,
          waveIndexA=6, waveIndexB=14, waveCharacterA=2, waveCharacterB=2,
          levelA=.17, levelB=.14, fine=517, noise=.48, cutoff=3150,
          resonance=.18, filterMode=3, filterEnv=.15, ampA=.004,
          ampD=1.18, ampR=.24, filterD=.22, polyB=.76,
          polyPitch=1, master=.74),
    voice("SHORT CYMBAL", waveA=12, waveB=12, octaveA=2, octaveB=2,
          waveIndexA=6, waveIndexB=14, waveCharacterA=2, waveCharacterB=2,
          levelA=.18, levelB=.15, fine=517, noise=.46, cutoff=3400,
          resonance=.17, filterMode=3, filterEnv=.15, ampA=.003,
          ampD=.42, ampR=.07, filterD=.10, polyB=.72,
          polyPitch=1, master=.78),
    voice("ACCENT KICK", waveA=2, waveB=4, octaveA=-2, octaveB=1, levelA=.98,
          levelB=.10, cutoff=1250, resonance=.18, filterEnv=.18, ampD=.82,
          ampR=.08, filterD=.12, polyEnv=.88, polyPitch=1, master=.92),
    voice("ELECTRO SNARE", waveA=4, waveB=2, octaveA=0, octaveB=-1,
          levelA=.24, levelB=.34, fine=33, noise=.58, cutoff=5900,
          resonance=.30, filterEnv=.38, ampD=.40, ampR=.08, filterD=.14,
          polyB=.28, polyPitch=1, master=.90),
    voice("DISCO TOM", waveA=2, waveB=2, octaveA=-1, octaveB=0, levelA=.66,
          levelB=.28, fine=12, cutoff=2600, resonance=.30, filterEnv=.42,
          ampD=.72, ampR=.12, filterD=.18, polyEnv=.54, polyPitch=1,
          master=.84),
    voice("HAT PULSE", waveA=4, waveB=4, octaveA=2, octaveB=3, levelA=.26,
          levelB=.24, fine=41, noise=.44, cutoff=9800, resonance=.16,
          filterEnv=.14, ampD=.08, ampR=.02, filterD=.04, polyB=.48,
          polyPitch=1, arp=1, arpRate=2, arpGate=.28, master=.76),
    voice("MUTED COWBELL", waveA=4, waveB=4, octaveA=0, octaveB=1, levelA=.40,
          levelB=.32, fine=490, cutoff=1900, resonance=.42, filterEnv=.05,
          ampD=.09, ampR=.018, filterD=.045, filterMode=2, master=.62),
    voice("SUB BASS", waveA=2, waveB=4, octaveA=-2, octaveB=-1, levelA=.78,
          levelB=.22, cutoff=920, resonance=.28, filterEnv=.46, ampD=.28,
          ampS=.72, ampR=.14, filterD=.22, filterS=.18, voiceMode=1,
          master=.78),
    voice("TRIGGER FX", waveA=4, waveB=4, octaveA=0, octaveB=2, levelA=.46,
          levelB=.38, fine=31, noise=.20, cutoff=3400, resonance=.58,
          filterEnv=.72, ampD=.86, ampR=.16, filterD=.32, polyEnv=.72,
          polyB=.58, polyPitch=1, sync=1, master=.78),
]

# GM drum-map positions take priority from C2 through C5. Aureline-specific
# sounds occupy otherwise unused positions inside the same range.
MIDI_NOTES = [
    36, 53, 54, 55, 57, 58, 38, 40, 59, 37, 65, 39, 70, 56,
    41, 45, 48, 64, 63, 62, 42, 46, 44, 51, 49, 52, 47, 43,
    50, 67, 66, 72,
]

# LP keeps low-frequency weight, BP isolates a strike band, and LP+BP
# combines body with a brighter resonant component. Every voice stores its
# mode explicitly so loading a different voice never inherits the last mode.
FILTER_MODES = [
    1, 1, 1, 1, 3, 1, 3, 3,
    2, 2, 2, 2, 2, 2, 1, 1,
    1, 3, 3, 3, 2, 2, 3, 3,
    3, 3, 3, 3, 2, 2, 1, 3,
]

TRANSIENT_ACCENTS = [
    .65, .58, .70, .48, .76, .58, .58, .82,
    .48, .72, .68, .54, .38, .44, .58, .54,
    .50, .52, .48, .44, .34, .22, .38, .18,
    .30, .82, .60, .54, .38, .55, .16, .48,
]


ATTRIBUTES = {
    "waveA": "waveformMaskA", "waveB": "waveformMaskB",
    "octaveA": "oscillatorAOctave", "octaveB": "oscillatorBOctave",
    "transpose": "transpose",
    "levelA": "oscillatorALevel", "levelB": "oscillatorBLevel",
    "fine": "oscillatorBFine", "noise": "noiseLevel",
    "pulseA": "pulseWidthA", "pulseB": "pulseWidthB",
    "cutoff": "cutoff", "resonance": "resonance",
    "filterMode": "filterMode",
    "waveIndexA": "waveMemoryIndexA", "waveIndexB": "waveMemoryIndexB",
    "waveCharacterA": "waveMemoryCharacterA",
    "waveCharacterB": "waveMemoryCharacterB",
    "filterEnv": "filterEnvelope", "keyTrack": "filterKeyboardTracking",
    "ampA": "attack", "ampD": "decay", "ampS": "sustain", "ampR": "release",
    "filterA": "filterAttack", "filterD": "filterDecay",
    "filterS": "filterSustain", "filterR": "filterRelease",
    "polyEnv": "polyModFilterEnvelope", "polyB": "polyModOscillatorB",
    "polyPitch": "polyModToFrequencyA", "polyFilter": "polyModToFilter",
    "lfoRate": "lfoRate", "lfoAmount": "lfoAmount",
    "lfoFilter": "lfoDestination4", "lfoMask": "lfoWaveformMask",
    "lfoRetrig": "lfoRetrigger", "master": "master",
    "voiceMode": "voiceMode", "sync": "oscillatorSync",
    "arp": "arpEnabled", "arpRate": "arpRate", "arpGate": "arpGate",
}


COMMON = {
    "oscillatorALevel": 0, "oscillatorBLevel": 0,
    "waveformMaskA": 0, "waveformMaskB": 0,
    "oscillatorAOctave": 0, "oscillatorBOctave": 0,
    "transpose": 0,
    "waveMemoryIndexA": 0, "waveMemoryIndexB": 0,
    "waveMemoryCharacterA": 0, "waveMemoryCharacterB": 0,
    "oscillatorBFine": 0, "pulseWidthA": .5, "pulseWidthB": .5,
    "noiseLevel": 0, "cutoff": 8000, "resonance": .1, "filterMode": 0,
    "filterEnvelope": 0, "filterKeyboardTracking": 0,
    "filterVelocity": 0, "attack": .002, "decay": .25,
    "sustain": 0, "release": .05, "filterAttack": .002,
    "filterDecay": .10, "filterSustain": 0, "filterRelease": .05,
    "polyModFilterEnvelope": 0, "polyModOscillatorB": 0,
    "polyModToFrequencyA": 0, "polyModToPulseWidthA": 0,
    "polyModToFilter": 0, "lfoRate": 5, "lfoAmount": 0,
    "lfoDelay": 0, "lfoFade": 0, "lfoRetrigger": 0,
    "lfoWaveformMask": 2,
    "lfoDestination0": 0, "lfoDestination1": 0,
    "lfoDestination2": 0, "lfoDestination3": 0,
    "lfoDestination4": 0, "spread": 0, "vintage": .08,
    "voiceMode": 1, "oscillatorSync": 0,
    "oscillatorBLowFrequency": 0, "oscillatorBKeyboardTracking": 1,
    "arpEnabled": 0, "arpRate": 1, "arpDirection": 0,
    "arpGate": .5, "arpHold": 0, "chordEnabled": 0,
    "waveMemoryUserA": 0, "waveMemoryUserB": 0,
    "master": .9,
}


def number(value: object) -> str:
    if isinstance(value, int):
        return str(value)
    return f"{float(value):.6g}"


def main() -> None:
    tree = ET.parse(SOURCE)
    root = tree.getroot()
    states = root.findall("AurelineState")
    if (len(states) != 32 or len(VOICES) != 32
            or len(set(MIDI_NOTES)) != 32
            or min(MIDI_NOTES) != 36 or max(MIDI_NOTES) != 72):
        raise RuntimeError("Aureline libraries must contain exactly 32 voices")

    root.set("name", "DRUM KIT")
    source_order = sorted(range(len(VOICES)), key=lambda index: MIDI_NOTES[index])
    for slot, source_index in enumerate(source_order):
        state = states[slot]
        definition = VOICES[source_index]
        name, values = definition
        state.set("slot", str(slot))
        state.set("voiceName", name)
        for key, value in COMMON.items():
            state.set(key, number(value))
        for key, value in values.items():
            state.set(ATTRIBUTES[key], number(value))
        state.set("filterMode", str(FILTER_MODES[source_index]))
        state.set("transientAccent", number(TRANSIENT_ACCENTS[source_index]))

    ET.indent(tree, space="  ")
    tree.write(OUTPUT, encoding="UTF-8", xml_declaration=True)
    with OUTPUT.open("a", encoding="UTF-8") as stream:
        stream.write("\n")
    print(f"Wrote {OUTPUT}")


if __name__ == "__main__":
    main()
