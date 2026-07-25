import AVFoundation
import Foundation

struct AurelineParameter: Identifiable {
    let id: String
    let name: String
    let range: ClosedRange<Double>
    let defaultValue: Double
    let logarithmic: Bool
}

final class MobileSynthModel: ObservableObject {
    static let waveMemoryNames = [
        "SOFT SINE", "HOLLOW", "BRIGHT 5", "REED", "ORGAN", "BELL", "METAL",
        "VOCAL A", "VOCAL O", "BASS STEP", "ARCADE 1", "ARCADE 2", "MAZE",
        "TOWER", "PULSE MIX", "NOISY EDGE"
    ]

    enum Screen: String, CaseIterable { case play = "PLAY", edit = "EDIT" }
    enum EditPage: String, CaseIterable {
        case oscillatorA = "OSC"
        case filter = "FILTER"
        case ampEnvelope = "AMP ENV"
        case modulation = "LFO"
        case performance = "PERF/ARP"
    }

    @Published var screen: Screen = .play
    @Published var editPage: EditPage = .oscillatorA
    @Published var values: [String: Double] = [:]
    @Published var octave = 0
    @Published var status = "Starting audio…"
    @Published var selectedPreset = "INIT ANALOG"
    @Published var pitchWheel = 0.0
    @Published var modWheel = 0.0
    @Published var waveformPitchNote = 60.0
    @Published var scopeSamples: [Float] = Array(repeating: 0, count: 128)
    @Published var waveformOutputLevel = 0.0
    @Published var externalActiveNotes: [Int: Int] = [:]
    @Published var factoryPresetNames: [String] = []
    @Published var canPasteVoice = false
    @Published var canPasteWave = false

    let bridge = AurelineMobileEngineBridge()
    private lazy var audio = MobileAudioEngine(bridge: bridge)
    private var midi: MobileMIDIInput?
    private var observers: [NSObjectProtocol] = []
    private var scopeTimer: Timer?
    private var copiedVoice: AurelineVoiceFile?
    private var copiedWave: [Int]?
    private var selectedFactoryPresetIndex: Int?
    private var factorySlotKeys: [String] = []
    private let lastVoiceDefaultsKey = "Aureline.lastSelectedVoiceSlot"

    private func voiceNameWithoutSlotPrefix(_ name: String) -> String {
        let trimmed = name.trimmingCharacters(in: .whitespacesAndNewlines)
        guard trimmed.count >= 3 else { return trimmed }
        let characters = Array(trimmed)
        if characters[0].isNumber, characters[1].isNumber,
           characters[2].isWhitespace {
            return String(characters.dropFirst(3))
                .trimmingCharacters(in: .whitespacesAndNewlines)
        }
        return trimmed
    }

    var selectedFactoryDisplayName: String? {
        guard let index = selectedFactoryPresetIndex,
              factoryPresetNames.indices.contains(index) else { return nil }
        return factoryPresetNames[index]
    }

    static let oscillatorParameters = [
        AurelineParameter(id: "oscALevel", name: "VCO A", range: 0...1, defaultValue: 0.5, logarithmic: false),
        AurelineParameter(id: "oscBLevel", name: "VCO B", range: 0...1, defaultValue: 0.5, logarithmic: false),
        AurelineParameter(id: "oscBFine", name: "DETUNE", range: -100...100, defaultValue: 7, logarithmic: false),
        AurelineParameter(id: "pulseWidthA", name: "PW A", range: 0.02...0.98, defaultValue: 0.5, logarithmic: false),
        AurelineParameter(id: "pulseWidthB", name: "PW B", range: 0.02...0.98, defaultValue: 0.5, logarithmic: false),
        AurelineParameter(id: "noiseLevel", name: "NOISE", range: 0...1, defaultValue: 0, logarithmic: false)
    ]
    static let filterParameters = [
        AurelineParameter(id: "cutoff", name: "CUTOFF", range: 20...20_000, defaultValue: 8_000, logarithmic: true),
        AurelineParameter(id: "resonance", name: "RESONANCE", range: 0...1, defaultValue: 0.1, logarithmic: false),
        AurelineParameter(id: "filterEnvAmount", name: "FILTER ENV", range: -1...1, defaultValue: 0.25, logarithmic: false),
        AurelineParameter(id: "filterKeyTrack", name: "KEY TRACK", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "filterVelocity", name: "VELOCITY", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "filterAttack", name: "F ATTACK", range: 0.001...5, defaultValue: 0.01, logarithmic: true),
        AurelineParameter(id: "filterDecay", name: "F DECAY", range: 0.001...5, defaultValue: 0.3, logarithmic: true),
        AurelineParameter(id: "filterSustain", name: "F SUSTAIN", range: 0...1, defaultValue: 0.4, logarithmic: false),
        AurelineParameter(id: "filterRelease", name: "F RELEASE", range: 0.001...8, defaultValue: 0.5, logarithmic: true),
        AurelineParameter(id: "ampAttack", name: "A ATTACK", range: 0.001...5, defaultValue: 0.01, logarithmic: true),
        AurelineParameter(id: "ampDecay", name: "A DECAY", range: 0.001...5, defaultValue: 0.25, logarithmic: true),
        AurelineParameter(id: "ampSustain", name: "A SUSTAIN", range: 0...1, defaultValue: 0.75, logarithmic: false),
        AurelineParameter(id: "ampRelease", name: "A RELEASE", range: 0.001...8, defaultValue: 0.4, logarithmic: true)
    ]
    static let modulationParameters = [
        AurelineParameter(id: "lfoRate", name: "LFO RATE", range: 0.01...30, defaultValue: 5, logarithmic: true),
        AurelineParameter(id: "lfoAmount", name: "INITIAL AMT", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "lfoDelay", name: "LFO DELAY", range: 0...10, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "lfoFade", name: "LFO FADE", range: 0...10, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "polyModFilterEnv", name: "POLY F ENV", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "polyModOscB", name: "POLY OSC B", range: 0...1, defaultValue: 0, logarithmic: false)
    ]
    static let performanceParameters = [
        AurelineParameter(id: "spread", name: "SPREAD", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "vintage", name: "VINTAGE", range: 0...1, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "masterGain", name: "VOLUME", range: 0...1, defaultValue: 0.8, logarithmic: false),
        AurelineParameter(id: "glide", name: "GLIDE", range: 0...5, defaultValue: 0, logarithmic: true),
        AurelineParameter(id: "masterTune", name: "MASTER", range: -100...100, defaultValue: 0, logarithmic: false),
        AurelineParameter(id: "unisonDetune", name: "UNI DETUNE", range: 0...100, defaultValue: 14, logarithmic: false),
        AurelineParameter(id: "transpose", name: "TRANSPOSE", range: -24...24, defaultValue: 0, logarithmic: false)
    ]
    static let sequencerParameters = [
        AurelineParameter(id: "tempoBpm", name: "TEMPO", range: 40...240, defaultValue: 120, logarithmic: false),
        AurelineParameter(id: "arpGate", name: "GATE", range: 0.1...0.95, defaultValue: 0.75, logarithmic: false)
    ]
    static let pitchBendParameter = AurelineParameter(id: "pitchBendRange", name: "PITCH RANGE", range: 0...24, defaultValue: 2, logarithmic: false)

    init() {
        values = bridge.patchSnapshot() as? [String: Double] ?? [:]
        factoryPresetNames = bridge.factoryPresetNames()
        factorySlotKeys = factoryPresetNames
        ensureActiveLibrary()
        if let active = try? readActiveLibrary() {
            for voice in active where factoryPresetNames.indices.contains(voice.slot) {
                factoryPresetNames[voice.slot] = slotDisplayName(
                    voice.slot, voiceNameWithoutSlotPrefix(voice.name))
            }
        }
        createBuiltInLibraries()
        let lastSlot = min(49, max(0, UserDefaults.standard.object(
            forKey: lastVoiceDefaultsKey) == nil
            ? 0 : UserDefaults.standard.integer(forKey: lastVoiceDefaultsKey)))
        loadFactoryPreset(lastSlot)
        midi = MobileMIDIInput(synth: self)
        observeAudioSession()
        startScopeTimer()
        startAudio()
    }

    deinit { scopeTimer?.invalidate(); observers.forEach(NotificationCenter.default.removeObserver) }

    func value(_ id: String, default fallback: Double = 0) -> Double { values[id] ?? fallback }
    func set(_ id: String, _ value: Double) { values[id] = value; bridge.setParameter(id, value: value) }
    func toggle(_ id: String) { set(id, value(id) >= 0.5 ? 0 : 1) }
    func noteOn(_ note: Int, velocity: Int = 100) {
        let soundingNote = max(0, min(127, note + octave * 12))
        waveformPitchNote = Double(soundingNote)
        bridge.noteOn(Int32(soundingNote), velocity: Int32(velocity))
    }
    func noteOff(_ note: Int) { bridge.noteOff(Int32(max(0, min(127, note + octave * 12)))) }
    func noteOnAbsolute(_ note: Int, velocity: Int = 100) {
        let soundingNote = max(0, min(127, note))
        waveformPitchNote = Double(soundingNote)
        bridge.noteOn(Int32(soundingNote), velocity: Int32(velocity))
    }
    func noteOffAbsolute(_ note: Int) { bridge.noteOff(Int32(max(0, min(127, note)))) }
    func setPitchWheel(_ value: Double) { pitchWheel = value; bridge.setPitchBend(value) }
    func setModWheel(_ value: Double) { modWheel = value; bridge.setModWheel(value) }
    func setSustain(_ down: Bool) { bridge.setSustainPedal(down) }
    func panic() { bridge.panic() }

    var displayedWaveformCycles: Double {
        let bend = pitchWheel * value("pitchBendRange", default: 2)
        let cycles = 2 * pow(2, (waveformPitchNote + bend - 60) / 12)
        return max(0.5, min(8, cycles))
    }

    func displayedWaveformCycles(oscillatorA: Bool) -> Double {
        let bend = pitchWheel * value("pitchBendRange", default: 2)
        let octave = value(oscillatorA ? "oscAOctave" : "oscBOctave")
        let fine = oscillatorA ? 0 : value("oscBFine") / 1200
        let cycles = 2 * pow(2, (waveformPitchNote + bend - 60) / 12
                             + octave + fine)
        return max(0.5, min(8, cycles))
    }

    func initializePatch() {
        let selectedName = selectedFactoryPresetIndex.flatMap {
            factoryPresetNames.indices.contains($0)
                ? voiceNameWithoutSlotPrefix(factoryPresetNames[$0]) : nil
        }
        bridge.resetPatch()
        values = bridge.patchSnapshot() as? [String: Double] ?? [:]
        selectedPreset = selectedName ?? "INIT ANALOG"
    }

    func waveMemoryData(oscillatorA: Bool) -> [Int] {
        let prefix = oscillatorA ? "waveMemoryStepA" : "waveMemoryStepB"
        return (0..<32).map { index in
            max(0, min(31, Int(value(String(format: "%@%02d", prefix, index)).rounded())))
        }
    }

    var synthesizedWaveformSamples: [Float] {
        let sampleCount = 128
        var result = Array(repeating: 0.0, count: sampleCount)
        for oscillatorA in [true, false] {
            let mask = Int(value(oscillatorA ? "waveformMaskA" : "waveformMaskB").rounded())
            let level = value(oscillatorA ? "oscALevel" : "oscBLevel")
            let octave = value(oscillatorA ? "oscAOctave" : "oscBOctave")
            let fineCents = oscillatorA ? 0 : value("oscBFine")
            let pulseWidth = value(oscillatorA ? "pulseWidthA" : "pulseWidthB")
            let character = Int(value(oscillatorA
                                      ? "waveMemoryCharacterA"
                                      : "waveMemoryCharacterB").rounded())
            let waveMemory = waveMemoryData(oscillatorA: oscillatorA)
            let enabledCount = [1, 2, 4, 8].reduce(0) { $0 + (mask & $1 == 0 ? 0 : 1) }
            guard enabledCount > 0, level > 0 else { continue }

            let frequencyRatio = pow(2, octave + fineCents / 1200)
            for index in 0..<sampleCount {
                let phase = (Double(index) / Double(sampleCount) * frequencyRatio)
                    .truncatingRemainder(dividingBy: 1)
                var sample = 0.0
                if mask & 1 != 0 { sample += phase * 2 - 1 }
                if mask & 2 != 0 { sample += 1 - 4 * abs(phase - 0.5) }
                if mask & 4 != 0 { sample += phase < pulseWidth ? 1 : -1 }
                if mask & 8 != 0 {
                    sample += renderedWaveMemorySample(phase: phase,
                                                       data: waveMemory,
                                                       character: character)
                }
                result[index] += level * sample / sqrt(Double(enabledCount))
            }
        }

        let peak = result.reduce(0) { max($0, abs($1)) }
        guard peak > 0 else { return result.map(Float.init) }
        return result.map { Float($0 / peak * waveformOutputLevel) }
    }

    func synthesizedOscillatorWaveformSamples(oscillatorA: Bool) -> [Float] {
        let sampleCount = 128
        var result = Array(repeating: 0.0, count: sampleCount)
        let mask = Int(value(oscillatorA ? "waveformMaskA" : "waveformMaskB").rounded())
        let level = value(oscillatorA ? "oscALevel" : "oscBLevel")
        let pulseWidth = value(oscillatorA ? "pulseWidthA" : "pulseWidthB")
        let character = Int(value(oscillatorA
                                  ? "waveMemoryCharacterA"
                                  : "waveMemoryCharacterB").rounded())
        let waveMemory = waveMemoryData(oscillatorA: oscillatorA)
        let enabledCount = [1, 2, 4, 8].reduce(0) {
            $0 + (mask & $1 == 0 ? 0 : 1)
        }
        guard enabledCount > 0, level > 0 else {
            return result.map(Float.init)
        }
        for index in 0..<sampleCount {
            let phase = Double(index) / Double(sampleCount)
            var sample = 0.0
            if mask & 1 != 0 { sample += phase * 2 - 1 }
            if mask & 2 != 0 { sample += 1 - 4 * abs(phase - 0.5) }
            if mask & 4 != 0 { sample += phase < pulseWidth ? 1 : -1 }
            if mask & 8 != 0 {
                sample += renderedWaveMemorySample(
                    phase: phase, data: waveMemory, character: character)
            }
            result[index] = level * sample / sqrt(Double(enabledCount))
        }
        return result.map {
            Float(max(-1, min(1, $0 * waveformOutputLevel)))
        }
    }

    private func renderedWaveMemorySample(phase: Double, data: [Int],
                                          character: Int) -> Double {
        guard data.count == 32 else { return 0 }
        let position = phase * 32
        let index = Int(floor(position)) % 32
        let quantized: (Int) -> Double = { step in
            if character == 1 {
                let fourBit = Int((Double(step) / 31 * 15).rounded())
                return Double(fourBit) / 15 * 2 - 1
            }
            return Double(step) / 31 * 2 - 1
        }
        let current = quantized(data[index])
        guard character == 2 else { return current }
        let next = quantized(data[(index + 1) % 32])
        let fraction = position - floor(position)
        return current + (next - current) * fraction
    }

    func setWaveMemoryStep(oscillatorA: Bool, index: Int, value newValue: Int) {
        guard (0..<32).contains(index) else { return }
        enableWaveMemory(oscillatorA: oscillatorA)
        let prefix = oscillatorA ? "waveMemoryStepA" : "waveMemoryStepB"
        set(String(format: "%@%02d", prefix, index), Double(max(0, min(31, newValue))))
        values[oscillatorA ? "waveMemoryUserA" : "waveMemoryUserB"] = 1
    }

    func selectWaveMemory(_ index: Int, oscillatorA: Bool) {
        enableWaveMemory(oscillatorA: oscillatorA)
        set(oscillatorA ? "waveMemoryIndexA" : "waveMemoryIndexB",
            Double(max(0, min(15, index))))
        values = bridge.patchSnapshot() as? [String: Double] ?? values
    }

    func enableWaveMemory(oscillatorA: Bool) {
        let maskID = oscillatorA ? "waveformMaskA" : "waveformMaskB"
        let mask = Int(value(maskID).rounded())
        if mask & 8 == 0 { set(maskID, Double(mask | 8)) }
    }

    func copyWaveMemory(oscillatorA: Bool) {
        copiedWave = waveMemoryData(oscillatorA: oscillatorA)
        canPasteWave = true
    }

    func pasteWaveMemory(oscillatorA: Bool) {
        guard let copiedWave else { return }
        for (index, step) in copiedWave.enumerated() {
            setWaveMemoryStep(oscillatorA: oscillatorA, index: index, value: step)
        }
    }

    func copyCurrentVoice() {
        copiedVoice = makePreset()
        canPasteVoice = true
        status = "Voice copied: \(selectedPreset)"
    }

    func pasteCopiedVoice() {
        guard let copiedVoice else { return }
        let destinationName = voiceNameWithoutSlotPrefix(copiedVoice.name)
        try? applyPreset(copiedVoice)
        selectedPreset = destinationName
        status = "Voice pasted temporarily into: \(destinationName)"
    }

    func storeCurrentVoice() throws {
        guard let index = selectedFactoryPresetIndex,
              factorySlotKeys.indices.contains(index) else {
            throw CocoaError(.fileWriteInvalidFileName)
        }
        let slotName = factorySlotKeys[index]
        let voiceName = selectedPreset.trimmingCharacters(in: .whitespacesAndNewlines)
        let storedName = voiceName.isEmpty
            ? voiceNameWithoutSlotPrefix(slotName) : voiceName
        var voices = try currentLibraryVoices()
        let preset = makePreset(name: storedName)
        voices[index] = AurelineLibraryVoice(
            slot: index, name: storedName, patch: preset.patch)
        try writeActiveLibrary(voices)
        selectedPreset = storedName
        factoryPresetNames[index] = slotDisplayName(index, storedName)
        status = "Voice stored in slot \(index + 1): \(storedName)"
    }

    func loadFactoryPreset(_ index: Int) {
        let slotName = factorySlotKeys.indices.contains(index)
            ? factorySlotKeys[index] : "Factory"
        let name = voiceNameWithoutSlotPrefix(slotName)
        selectedFactoryPresetIndex = factorySlotKeys.indices.contains(index) ? index : nil
        if selectedFactoryPresetIndex != nil {
            UserDefaults.standard.set(index, forKey: lastVoiceDefaultsKey)
        }
        if let voices = try? readActiveLibrary(),
           voices.indices.contains(index) {
            let stored = voices[index]
            let voice = AurelineVoiceFile(
                format: "com.hidecade.aureline.voice", version: 1,
                name: stored.name, author: "", category: "User",
                patch: stored.patch,
                performance: [
                    "transpose": stored.patch["transpose"] ?? 0,
                    "pitchBendRange": stored.patch["pitchBendRange"] ?? 2
                ])
            if (try? applyPreset(voice)) != nil {
                factoryPresetNames[index] = slotDisplayName(index, selectedPreset)
                return
            }
        }
        bridge.loadFactoryPreset(Int32(index))
        values = bridge.patchSnapshot() as? [String: Double] ?? [:]
        selectedPreset = name
        if factoryPresetNames.indices.contains(index) {
            factoryPresetNames[index] = slotDisplayName(index, name)
        }
        panic()
    }

    func makePreset(name: String? = nil) -> AurelineVoiceFile {
        AurelineVoiceFile(format: "com.hidecade.aureline.voice", version: 1,
            name: name ?? selectedPreset, author: "", category: "User", patch: values,
            performance: ["transpose": value("transpose"), "pitchBendRange": value("pitchBendRange", default: 2)])
    }

    var aurelineDocumentsDirectoryURL: URL? {
        try? userPresetDirectory()
    }

    func preparePresetExport() throws -> URL {
        _ = try userPresetDirectory()
        let directory = FileManager.default.temporaryDirectory
            .appendingPathComponent("AurelineExports", isDirectory: true)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let url = directory.appendingPathComponent(safeFilename(selectedPreset))
            .appendingPathExtension("aurelinevoice")
        try writeVoice(makePreset(), to: url)
        return url
    }

    func importPreset(from url: URL) throws {
        let accessed = url.startAccessingSecurityScopedResource()
        defer { if accessed { url.stopAccessingSecurityScopedResource() } }
        let preset = try JSONDecoder().decode(AurelineVoiceFile.self, from: Data(contentsOf: url))
        try applyPreset(preset)
        status = "Voice loaded temporarily: \(selectedPreset)"
    }

    func isLibraryURL(_ url: URL) -> Bool {
        url.lastPathComponent.lowercased().hasSuffix(".aurelinelibrary.xml")
    }

    func prepareLibraryExport() throws -> URL {
        let voices = try currentLibraryVoices()
        let directory = FileManager.default.temporaryDirectory
            .appendingPathComponent("AurelineExports", isDirectory: true)
        try FileManager.default.createDirectory(at: directory,
                                                withIntermediateDirectories: true)
        let url = directory.appendingPathComponent(
            "Aureline Library.aurelinelibrary.xml")
        try AurelineLibraryCodec.encode(voices).write(to: url, options: .atomic)
        return url
    }

    func importLibrary(from url: URL) throws {
        let accessed = url.startAccessingSecurityScopedResource()
        defer { if accessed { url.stopAccessingSecurityScopedResource() } }
        let voices = try AurelineLibraryCodec.decode(Data(contentsOf: url))
        try writeActiveLibrary(voices)
        removeLegacyStoredVoices()
        for voice in voices {
            factoryPresetNames[voice.slot] = slotDisplayName(
                voice.slot, voiceNameWithoutSlotPrefix(voice.name))
        }
        if let selectedFactoryPresetIndex {
            loadFactoryPreset(selectedFactoryPresetIndex)
        }
        status = "All 50 voices replaced from: \(url.lastPathComponent)"
    }

    func applyPreset(_ preset: AurelineVoiceFile) throws {
        guard preset.format == "com.hidecade.aureline.voice", preset.version == 1 else { throw CocoaError(.fileReadCorruptFile) }
        let finitePatch = preset.patch.filter { $0.value.isFinite }
        bridge.applyPatchSnapshot(finitePatch.mapValues(NSNumber.init(value:)))
        values = bridge.patchSnapshot() as? [String: Double] ?? finitePatch
        selectedPreset = preset.name
        panic()
    }

    private func writeVoice(_ voice: AurelineVoiceFile, to url: URL) throws {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try encoder.encode(voice).write(to: url, options: .atomic)
    }

    private func applicationSupportDirectory() throws -> URL {
        let base = try FileManager.default.url(
            for: .applicationSupportDirectory, in: .userDomainMask,
            appropriateFor: nil, create: true)
        let directory = base.appendingPathComponent("Aureline", isDirectory: true)
        try FileManager.default.createDirectory(
            at: directory, withIntermediateDirectories: true)
        return directory
    }

    private func activeLibraryURL() throws -> URL {
        try applicationSupportDirectory().appendingPathComponent(
            "active-library.aurelinelibrary.xml")
    }

    private func readActiveLibrary() throws -> [AurelineLibraryVoice] {
        try AurelineLibraryCodec.decode(Data(contentsOf: activeLibraryURL()))
    }

    private func writeActiveLibrary(_ voices: [AurelineLibraryVoice]) throws {
        try AurelineLibraryCodec.encode(voices).write(
            to: activeLibraryURL(), options: .atomic)
    }

    private func ensureActiveLibrary() {
        if (try? readActiveLibrary()) != nil {
            removeLegacyStoredVoices()
            return
        }
        var voices = factoryLibraryVoices()
        guard let documents = try? userPresetDirectory() else { return }
        let decoder = JSONDecoder()
        for index in factorySlotKeys.indices {
            let candidates = [
                documents.appendingPathComponent(
                    String(format: "slot-%02d.aurelinevoice", index + 1)),
                documents.appendingPathComponent(
                    safeFilename(factorySlotKeys[index]))
                    .appendingPathExtension("aurelinevoice")
            ]
            guard let legacy = candidates.first(where: {
                      FileManager.default.fileExists(atPath: $0.path)
                  }),
                  let data = try? Data(contentsOf: legacy),
                  let voice = try? decoder.decode(AurelineVoiceFile.self, from: data)
            else { continue }
            voices[index] = AurelineLibraryVoice(
                slot: index, name: voiceNameWithoutSlotPrefix(voice.name),
                patch: voice.patch)
        }
        guard (try? writeActiveLibrary(voices)) != nil else { return }
        removeLegacyStoredVoices()
    }

    private func removeLegacyStoredVoices() {
        guard let documents = try? userPresetDirectory() else { return }
        for index in factorySlotKeys.indices {
            let files = [
                documents.appendingPathComponent(
                    String(format: "slot-%02d.aurelinevoice", index + 1)),
                documents.appendingPathComponent(
                    safeFilename(factorySlotKeys[index]))
                    .appendingPathExtension("aurelinevoice")
            ]
            for file in files where FileManager.default.fileExists(atPath: file.path) {
                try? FileManager.default.removeItem(at: file)
            }
        }
    }

    private func userPresetDirectory() throws -> URL {
        let base = try FileManager.default.url(for: .documentDirectory, in: .userDomainMask,
                                               appropriateFor: nil, create: true)
        let directory = base.appendingPathComponent("Aureline", isDirectory: true)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true, attributes: nil)
        return directory
    }

    private func safeFilename(_ name: String) -> String {
        let invalid = CharacterSet(charactersIn: "/:\\?%*|\"<>").union(.controlCharacters)
        return name.components(separatedBy: invalid).joined(separator: "_").prefix(80).description
    }

    private func slotDisplayName(_ index: Int, _ name: String) -> String {
        String(format: "%02d %@", index + 1, voiceNameWithoutSlotPrefix(name))
    }

    private func factoryLibraryVoices() -> [AurelineLibraryVoice] {
        let savedValues = values
        let savedName = selectedPreset
        var voices: [AurelineLibraryVoice] = []
        for index in factorySlotKeys.indices {
            bridge.loadFactoryPreset(Int32(index))
            let patch = bridge.patchSnapshot() as? [String: Double] ?? [:]
            voices.append(AurelineLibraryVoice(
                slot: index, name: voiceNameWithoutSlotPrefix(factorySlotKeys[index]),
                patch: patch))
        }
        bridge.applyPatchSnapshot(savedValues.mapValues(NSNumber.init(value:)))
        values = savedValues
        selectedPreset = savedName
        return voices
    }

    private func currentLibraryVoices() throws -> [AurelineLibraryVoice] {
        if let active = try? readActiveLibrary() { return active }
        let factory = factoryLibraryVoices()
        try writeActiveLibrary(factory)
        return factory
    }

    private func createBuiltInLibraries() {
        guard let directory = try? userPresetDirectory() else { return }
        let factory = factoryLibraryVoices()
        if let data = try? AurelineLibraryCodec.encode(factory) {
            try? data.write(to: directory.appendingPathComponent(
                "factory.aurelinelibrary.xml"), options: .atomic)
        }
        let retro = retroGameLibraryVoices(from: factory)
        if let data = try? AurelineLibraryCodec.encode(retro) {
            try? data.write(to: directory.appendingPathComponent(
                "RetroGame.aurelinelibrary.xml"), options: .atomic)
        }
    }

    private func retroGameLibraryVoices(
        from factory: [AurelineLibraryVoice]) -> [AurelineLibraryVoice] {
        let names = [
            "8BIT HERO", "8BIT QUEST", "PIXEL PLUCK", "BLOCKY BASS",
            "TRIANGLE CAVE", "COIN SPARK", "BOSS WARNING", "CHIP FANFARE",
            "DUNGEON STEP", "TINY DRUM", "CASTLE LEAD", "POWER UP",
            "SECRET DOOR", "NIGHT STAGE", "FINAL CASTLE", "PSG RACER",
            "PSG SKYLINE", "TONE CHANNEL", "NOISE RIDER", "ARCADE START",
            "SEGA BLUE", "RING PULSE", "GRID RUNNER", "SPACE PORT",
            "PSG VICTORY", "WAVE HERO", "TURBO WAVE", "CRYSTAL CHANNEL",
            "NEON BASS", "SIX VOICE PAD", "LASER HARBOR", "DIGITAL STEEL",
            "ORBIT LEAD", "WAVE RUNNER", "SUNSET CHIP", "MAZE CHOMP",
            "STAR SWARM", "HAPPY HOP", "FRUIT BONUS", "ALIEN MARCH",
            "DOT RUNNER", "GALAXY DIVE", "TOY PARADE", "ARCADE SIREN",
            "HIGH SCORE", "LASER ZAP", "NOISE BURST", "SHIP EXPLODE",
            "WARP FALL", "GAME OVER"
        ]
        let lfoSlots: Set<Int> = [
            0, 1, 6, 7, 10, 13, 14, 15, 16, 19, 20, 24,
            25, 27, 29, 30, 32, 34, 36, 40, 44
        ]
        return factory.enumerated().map { index, source in
            var patch = source.patch
            patch["lfoAmount"] = 0
            for destination in ["lfoDestA", "lfoDestB", "lfoDestPWA",
                                "lfoDestPWB", "lfoDestFilter"] {
                patch[destination] = 0
            }
            if index < 15 {
                patch["waveformMaskA"] = [4, 4, 4, 4, 2][index % 5]
                patch["pulseWidthA"] = [0.125, 0.25, 0.5, 0.75][index % 4]
            } else if index < 25 {
                patch["waveformMaskA"] = 4
                patch["waveformMaskB"] = 4
                patch["pulseWidthA"] = 0.5
            } else if index < 45 {
                patch["waveformMaskA"] = 8
                patch["waveMemoryUserA"] = 1
                for step in 0..<32 {
                    let phase = Double.pi * 2 * Double(step) / 32
                    let sample = sin(phase)
                        + (index < 35 ? 0.24 : 0.42) * sin(phase * 3)
                    patch[String(format: "waveMemoryStepA%02d", step)] =
                        Double(max(0, min(31, Int((15.5 + sample * 10).rounded()))))
                }
            } else {
                patch["noiseLevel"] = index == 47 ? 0.92 : 0.28
                patch["ampSustain"] = 0
            }
            if lfoSlots.contains(index) {
                patch["lfoRate"] = 4.5 + Double(index % 5) * 0.7
                patch["lfoAmount"] = 0.06 + Double(index % 4) * 0.02
                patch["lfoDestA"] = 1
            } else if index == 43 || index >= 45 {
                patch["lfoRate"] = index == 43 ? 6.5 : 8 + Double(index - 45) * 2
                patch["lfoAmount"] = index == 47 ? 0.22 : 0.16
                patch["lfoDestA"] = 1
            }
            return AurelineLibraryVoice(slot: index, name: names[index],
                                        patch: patch)
        }
    }

    func applicationDidBecomeActive() { if !audio.isRunning { startAudio() } }
    func applicationDidEnterBackground() { audio.suspend(); status = "Audio suspended" }

    private func startAudio() {
        do { try audio.start(); status = "Audio ready · MIDI all inputs" }
        catch { status = "Audio error: \(error.localizedDescription)" }
    }

    private func observeAudioSession() {
        let center = NotificationCenter.default
        observers.append(center.addObserver(forName: AVAudioSession.interruptionNotification, object: AVAudioSession.sharedInstance(), queue: .main) { [weak self] note in
            guard let self, let raw = note.userInfo?[AVAudioSessionInterruptionTypeKey] as? UInt,
                  let type = AVAudioSession.InterruptionType(rawValue: raw) else { return }
            if type == .began { self.audio.suspend(); self.status = "Audio interrupted" }
            else { self.startAudio() }
        })
        observers.append(center.addObserver(forName: AVAudioSession.mediaServicesWereResetNotification, object: AVAudioSession.sharedInstance(), queue: .main) { [weak self] _ in
            self?.audio.resetAfterMediaServicesReset(); self?.startAudio()
        })
        observers.append(center.addObserver(forName: AVAudioSession.routeChangeNotification, object: AVAudioSession.sharedInstance(), queue: .main) { [weak self] _ in
            self?.status = "Audio route changed"
        })
    }

    private func startScopeTimer() {
        scopeTimer = Timer.scheduledTimer(withTimeInterval: 1.0 / 20.0, repeats: true) { [weak self] _ in
            guard let self else { return }
            let samples: [Float] = self.bridge.scopeSnapshotData().withUnsafeBytes { bytes in
                Array(bytes.bindMemory(to: Float.self))
            }
            if samples.count == 128 {
                self.scopeSamples = samples
                let meanSquare = samples.reduce(0.0) {
                    $0 + Double($1) * Double($1)
                } / Double(samples.count)
                let measured = min(1, sqrt(meanSquare) * 5)
                let response = measured > self.waveformOutputLevel ? 0.68 : 0.22
                var next = self.waveformOutputLevel
                    + (measured - self.waveformOutputLevel) * response
                if measured < 0.0005, next < 0.002 { next = 0 }
                self.waveformOutputLevel = next
            }
        }
    }
}
