#!/usr/bin/env python3
"""Generate the 32-voice TR-808-inspired Aureline library."""

from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "Analog.aurelinelibrary.xml"
OUTPUT = ROOT / "assets" / "TR-808.aurelinelibrary.xml"


def voice(name: str, **values: object) -> tuple[str, dict[str, object]]:
    return name, values


VOICES = [
    voice("DEEP KICK", waveA=2, octaveA=-2, levelA=.92, cutoff=680, resonance=.16,
          filterEnv=.08, ampD=.72, ampR=.08, filterD=.18, polyEnv=.82,
          polyPitch=1, master=.98),
    voice("SHORT KICK", waveA=2, octaveA=-2, levelA=.94, cutoff=920, resonance=.12,
          filterEnv=.10, ampD=.24, ampR=.035, filterD=.075, polyEnv=.68,
          polyPitch=1, master=.98),
    voice("BOOM KICK", waveA=2, octaveA=-2, levelA=.96, cutoff=540, resonance=.20,
          filterEnv=.06, ampD=1.35, ampR=.12, filterD=.30, polyEnv=.90,
          polyPitch=1, master=.98),
    voice("TUNED KICK", waveA=2, octaveA=-1, levelA=.90, cutoff=1050, resonance=.14,
          filterEnv=.12, ampD=.58, ampR=.08, filterD=.14, polyEnv=.60,
          polyPitch=1, keyTrack=.18, master=.96),
    voice("CLICK KICK", waveA=2, waveB=4, octaveA=-2, octaveB=1, levelA=.88,
          levelB=.08, cutoff=1800, resonance=.12, filterEnv=.24, ampD=.32,
          ampR=.04, filterD=.045, polyEnv=.72, polyPitch=1, master=.98),
    voice("SUB DROP", waveA=2, octaveA=-2, levelA=.96, cutoff=520, resonance=.10,
          filterEnv=.04, ampD=1.80, ampR=.20, filterD=.55, polyEnv=1.0,
          polyPitch=1, master=.98),
    voice("808 SNARE", waveA=2, waveB=2, octaveA=-1, octaveB=-1, levelA=.35,
          levelB=.28, fine=38, noise=.64, cutoff=5200, resonance=.25,
          filterEnv=.34, ampD=.34, ampR=.06, filterD=.16, master=.94),
    voice("TIGHT SNARE", waveA=2, waveB=2, octaveA=0, octaveB=-1, levelA=.28,
          levelB=.22, fine=31, noise=.58, cutoff=6500, resonance=.20,
          filterEnv=.28, ampD=.18, ampR=.035, filterD=.09, master=.96),
    voice("NOISE SNARE", waveA=0, waveB=0, levelA=0, levelB=0, noise=.88,
          cutoff=4600, resonance=.38, filterEnv=.32, ampD=.28, ampR=.045,
          filterD=.12, master=.96),
    voice("RIM SHOT", waveA=4, waveB=4, octaveA=1, octaveB=2, levelA=.52,
          levelB=.34, fine=17, noise=.06, cutoff=6200, resonance=.62,
          filterEnv=.48, ampD=.075, ampR=.025, filterD=.05, polyB=.22,
          polyPitch=1, master=.92),
    voice("CLAVES", waveA=2, waveB=0, octaveA=2, levelA=.72, levelB=0,
          cutoff=7600, resonance=.52, filterEnv=.22, ampD=.09, ampR=.025,
          filterD=.055, master=.90),
    voice("HAND CLAP", waveA=0, waveB=0, levelA=0, levelB=0, noise=.94,
          cutoff=2100, resonance=.56, filterEnv=.62, ampD=.46, ampR=.10,
          filterD=.18, lfoRate=28, lfoAmount=.08, lfoFilter=1,
          lfoMask=4, lfoRetrig=1, master=.98),
    voice("MARACAS", waveA=0, waveB=0, levelA=0, levelB=0, noise=.84,
          cutoff=9200, resonance=.18, filterEnv=.18, ampD=.065, ampR=.02,
          filterD=.04, master=.92),
    voice("COWBELL", waveA=4, waveB=4, octaveA=1, octaveB=1, levelA=.58,
          levelB=.56, fine=37, cutoff=5400, resonance=.34, filterEnv=.24,
          ampD=.42, ampR=.08, filterD=.14, polyB=.38, polyPitch=1,
          pulseA=.5, pulseB=.5, master=.90),
    voice("LOW TOM", waveA=2, octaveA=-1, levelA=.88, cutoff=1500,
          resonance=.28, filterEnv=.34, ampD=.52, ampR=.07, filterD=.16,
          polyEnv=.48, polyPitch=1, master=.96),
    voice("MID TOM", waveA=2, octaveA=0, levelA=.86, cutoff=2300,
          resonance=.26, filterEnv=.32, ampD=.42, ampR=.06, filterD=.13,
          polyEnv=.44, polyPitch=1, master=.94),
    voice("HIGH TOM", waveA=2, octaveA=1, levelA=.84, cutoff=3600,
          resonance=.24, filterEnv=.30, ampD=.34, ampR=.05, filterD=.10,
          polyEnv=.40, polyPitch=1, master=.92),
    voice("LOW CONGA", waveA=2, octaveA=-1, levelA=.82, cutoff=1900,
          resonance=.34, filterEnv=.18, ampD=.30, ampR=.045, filterD=.10,
          polyEnv=.24, polyPitch=1, master=.94),
    voice("MID CONGA", waveA=2, octaveA=0, levelA=.80, cutoff=3000,
          resonance=.32, filterEnv=.16, ampD=.25, ampR=.04, filterD=.085,
          polyEnv=.22, polyPitch=1, master=.92),
    voice("HIGH CONGA", waveA=2, octaveA=1, levelA=.78, cutoff=4600,
          resonance=.30, filterEnv=.14, ampD=.21, ampR=.035, filterD=.07,
          polyEnv=.20, polyPitch=1, master=.90),
    voice("CLOSED HAT", waveA=4, waveB=4, octaveA=2, octaveB=3, levelA=.30,
          levelB=.28, fine=43, noise=.48, cutoff=10500, resonance=.18,
          filterEnv=.18, ampD=.07, ampR=.018, filterD=.035, polyB=.46,
          polyPitch=1, master=.88),
    voice("OPEN HAT", waveA=4, waveB=4, octaveA=2, octaveB=3, levelA=.28,
          levelB=.26, fine=43, noise=.52, cutoff=9800, resonance=.16,
          filterEnv=.16, ampD=.68, ampR=.16, filterD=.24, polyB=.44,
          polyPitch=1, master=.88),
    voice("METAL HAT", waveA=4, waveB=4, octaveA=2, octaveB=3, levelA=.38,
          levelB=.34, fine=29, noise=.28, cutoff=8600, resonance=.22,
          filterEnv=.20, ampD=.16, ampR=.035, filterD=.07, polyB=.68,
          polyPitch=1, master=.86),
    voice("808 CYMBAL", waveA=4, waveB=4, octaveA=1, octaveB=2, levelA=.34,
          levelB=.32, fine=47, noise=.44, cutoff=7800, resonance=.24,
          filterEnv=.20, ampD=1.45, ampR=.28, filterD=.50, polyB=.62,
          polyPitch=1, master=.88),
    voice("SHORT CYMBAL", waveA=4, waveB=4, octaveA=1, octaveB=2, levelA=.36,
          levelB=.34, fine=47, noise=.42, cutoff=8200, resonance=.22,
          filterEnv=.22, ampD=.48, ampR=.08, filterD=.18, polyB=.60,
          polyPitch=1, master=.90),
    voice("ACCENT KICK", waveA=2, waveB=4, octaveA=-2, octaveB=1, levelA=.98,
          levelB=.10, cutoff=1250, resonance=.18, filterEnv=.18, ampD=.82,
          ampR=.08, filterD=.12, polyEnv=.88, polyPitch=1, master=1.0),
    voice("ELECTRO SNARE", waveA=4, waveB=2, octaveA=0, octaveB=-1,
          levelA=.24, levelB=.34, fine=33, noise=.58, cutoff=5900,
          resonance=.30, filterEnv=.38, ampD=.40, ampR=.08, filterD=.14,
          polyB=.28, polyPitch=1, master=.94),
    voice("DISCO TOM", waveA=2, waveB=2, octaveA=-1, octaveB=0, levelA=.66,
          levelB=.28, fine=12, cutoff=2600, resonance=.30, filterEnv=.42,
          ampD=.72, ampR=.12, filterD=.18, polyEnv=.54, polyPitch=1,
          master=.94),
    voice("HAT PULSE", waveA=4, waveB=4, octaveA=2, octaveB=3, levelA=.26,
          levelB=.24, fine=41, noise=.44, cutoff=9800, resonance=.16,
          filterEnv=.14, ampD=.08, ampR=.02, filterD=.04, polyB=.48,
          polyPitch=1, arp=1, arpRate=2, arpGate=.28, master=.88),
    voice("COWBELL SEQ", waveA=4, waveB=4, octaveA=1, octaveB=1, levelA=.54,
          levelB=.52, fine=37, cutoff=5600, resonance=.32, filterEnv=.24,
          ampD=.30, ampR=.05, filterD=.11, polyB=.36, polyPitch=1,
          arp=1, arpRate=1, arpGate=.42, master=.90),
    voice("808 BASS", waveA=2, waveB=4, octaveA=-2, octaveB=-1, levelA=.78,
          levelB=.22, cutoff=920, resonance=.28, filterEnv=.46, ampD=.28,
          ampS=.72, ampR=.14, filterD=.22, filterS=.18, voiceMode=1,
          master=.96),
    voice("TRIGGER FX", waveA=4, waveB=4, octaveA=0, octaveB=2, levelA=.46,
          levelB=.38, fine=31, noise=.20, cutoff=3400, resonance=.58,
          filterEnv=.72, ampD=.86, ampR=.16, filterD=.32, polyEnv=.72,
          polyB=.58, polyPitch=1, sync=1, master=.92),
]


ATTRIBUTES = {
    "waveA": "waveformMaskA", "waveB": "waveformMaskB",
    "octaveA": "oscillatorAOctave", "octaveB": "oscillatorBOctave",
    "levelA": "oscillatorALevel", "levelB": "oscillatorBLevel",
    "fine": "oscillatorBFine", "noise": "noiseLevel",
    "pulseA": "pulseWidthA", "pulseB": "pulseWidthB",
    "cutoff": "cutoff", "resonance": "resonance",
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
    "oscillatorBFine": 0, "pulseWidthA": .5, "pulseWidthB": .5,
    "noiseLevel": 0, "cutoff": 8000, "resonance": .1,
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
    if len(states) != 32 or len(VOICES) != 32:
        raise RuntimeError("Aureline libraries must contain exactly 32 voices")

    root.set("name", "TR-808 Inspired")
    for slot, (state, definition) in enumerate(zip(states, VOICES)):
        name, values = definition
        state.set("slot", str(slot))
        state.set("voiceName", name)
        for key, value in COMMON.items():
            state.set(key, number(value))
        for key, value in values.items():
            state.set(ATTRIBUTES[key], number(value))

    ET.indent(tree, space="  ")
    tree.write(OUTPUT, encoding="UTF-8", xml_declaration=True)
    with OUTPUT.open("a", encoding="UTF-8") as stream:
        stream.write("\n")
    print(f"Wrote {OUTPUT}")


if __name__ == "__main__":
    main()
