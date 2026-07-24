import SwiftUI
import UniformTypeIdentifiers

struct EditView: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @State private var importingVoice = false
    @State private var exportingVoice = false
    @State private var exportFileURL: URL?
    @State private var voiceError: String?
    @State private var pendingLibraryURL: URL?
    @State private var confirmingLibraryLoad = false
    @State private var exportIsLibrary = false
    @State private var showingWaveEditor = false
    @State private var waveEditorOscillatorA = true
    @State private var showingOscillatorMemoryPicker = false
    @State private var showingOscillatorCharacterPicker = false
    @State private var pickerOscillatorA = true

    var body: some View {
        VStack(spacing: 5) {
            HStack(spacing: 7) {
                VStack(spacing: 2) {
                    ForEach(MobileSynthModel.EditPage.allCases, id: \.self) { page in
                        Button(page.rawValue) { synth.editPage = page }
                            .buttonStyle(EditSectionTabStyle(active: synth.editPage == page))
                            .frame(height: 22)
                    }
                }
                .frame(width: 62)

                currentSection
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .panelStyle()
            }
        }.padding(.vertical, 6)
            .sheet(isPresented: $importingVoice) {
                AurelineDocumentPicker(
                    mode: .importing([.aurelineVoice, .aurelineLibrary, .json, .xml]),
                    initialDirectoryURL: synth.aurelineDocumentsDirectoryURL
                ) { url in
                    importingVoice = false
                    guard let url else { return }
                    if synth.isLibraryURL(url) {
                        pendingLibraryURL = url
                        confirmingLibraryLoad = true
                    } else {
                        do { try synth.importPreset(from: url) }
                        catch { voiceError = error.localizedDescription }
                    }
                }
            }
            .sheet(isPresented: $exportingVoice) {
                if let exportFileURL {
                    AurelineDocumentPicker(
                        mode: .exporting(exportFileURL),
                        initialDirectoryURL: synth.aurelineDocumentsDirectoryURL
                    ) { savedURL in
                        try? FileManager.default.removeItem(at: exportFileURL)
                        self.exportFileURL = nil
                        exportingVoice = false
                        if savedURL != nil {
                            synth.status = exportIsLibrary
                                ? "All 50 voices saved" : "Voice saved"
                        }
                    }
                }
            }
            .alert("VOICE", isPresented: Binding(get: { voiceError != nil }, set: { if !$0 { voiceError = nil } })) {
                Button("OK") { voiceError = nil }
            } message: { Text(voiceError ?? "") }
            .alert("Replace all 50 voices?", isPresented: $confirmingLibraryLoad) {
                Button("CANCEL", role: .cancel) { pendingLibraryURL = nil }
                Button("REPLACE ALL", role: .destructive) {
                    guard let url = pendingLibraryURL else { return }
                    do { try synth.importLibrary(from: url) }
                    catch { voiceError = error.localizedDescription }
                    pendingLibraryURL = nil
                }
            } message: {
                Text("Loading this library will overwrite every numbered voice. This cannot be undone.")
            }
            .fullScreenCover(isPresented: $showingWaveEditor) {
                AurelineWaveMemoryEditor(oscillatorA: waveEditorOscillatorA)
                    .environmentObject(synth)
            }
            .overlay {
                if showingOscillatorMemoryPicker {
                    AurelineSelectionPicker(
                        title: "SELECT MEMORY",
                        sections: [AurelinePickerSection(
                            title: "FACTORY",
                            options: MobileSynthModel.waveMemoryNames)],
                        selected: oscillatorMemoryIsUser ? nil : oscillatorMemoryTitle,
                        onSelect: { _, index in
                            synth.selectWaveMemory(index, oscillatorA: pickerOscillatorA)
                            showingOscillatorMemoryPicker = false
                        },
                        onClose: { showingOscillatorMemoryPicker = false }
                    )
                } else if showingOscillatorCharacterPicker {
                    AurelineSelectionPicker(
                        title: "SELECT CHARACTER",
                        sections: [AurelinePickerSection(
                            title: "CHARACTER",
                            options: ["5-BIT", "4-BIT", "SMOOTH"])],
                        selected: oscillatorCharacterTitle,
                        onSelect: { _, index in
                            synth.set(pickerOscillatorA
                                      ? "waveMemoryCharacterA"
                                      : "waveMemoryCharacterB", Double(index))
                            showingOscillatorCharacterPicker = false
                        },
                        onClose: { showingOscillatorCharacterPicker = false }
                    )
                }
            }
    }

    private var voiceActions: some View {
        HStack(spacing: 5) {
            voiceButton("LOAD") { importingVoice = true }
            voiceButton("SAVE") { prepareVoiceExport() }
            voiceButton("COPY") { synth.copyCurrentVoice() }
            voiceButton("PASTE", disabled: !synth.canPasteVoice) { synth.pasteCopiedVoice() }
            voiceButton("INIT") { synth.initializePatch() }
            voiceButton("STORE", store: true) {
                do { try synth.storeCurrentVoice() } catch { voiceError = error.localizedDescription }
            }
            voiceButton("SAVE ALL") { prepareLibraryExport() }
        }.frame(maxWidth: 600, minHeight: 32, maxHeight: 32)
    }

    private func voiceButton(_ title: String, store: Bool = false, disabled: Bool = false,
                             action: @escaping () -> Void) -> some View {
        Button(title, action: action)
            .buttonStyle(AurelineMacVoiceButtonStyle(store: store))
            .disabled(disabled).opacity(disabled ? 0.35 : 1)
    }

    private func prepareVoiceExport() {
        do {
            exportIsLibrary = false
            exportFileURL = try synth.preparePresetExport()
            exportingVoice = true
        } catch {
            voiceError = error.localizedDescription
        }
    }

    private func prepareLibraryExport() {
        do {
            exportIsLibrary = true
            exportFileURL = try synth.prepareLibraryExport()
            exportingVoice = true
        } catch {
            voiceError = error.localizedDescription
        }
    }

    @ViewBuilder
    private var currentSection: some View {
        switch synth.editPage {
        case .oscillatorA: oscillatorCombinedSection
        case .filter: filterCombinedSection
        case .ampEnvelope: amplifierEnvelopeSection
        case .modulation: modulationSection
        case .performance: performanceArpeggiatorSection
        }
    }

    private var oscillatorCombinedSection: some View {
        VStack(spacing: 5) {
            HStack(spacing: 6) {
                AurelineEditGroup(title: "OSCILLATOR A") {
                    HStack(spacing: 5) {
                        AurelineEditKnob(parameter: octaveParameter("oscAOctave"),
                                         value: binding("oscAOctave"))
                        oscillatorShapeControls(id: "waveformMaskA")
                        AurelineEditKnob(parameter: renamedParameter(
                            MobileSynthModel.oscillatorParameters[3], "PW"),
                                         value: binding("pulseWidthA"))
                        waveMemoryControls(oscillatorA: true)
                    }
                }

                AurelineEditGroup(title: "MIXER") {
                    HStack(spacing: 1) {
                        AurelineEditKnob(parameter: MobileSynthModel.oscillatorParameters[0],
                                         value: binding("oscALevel"))
                        AurelineEditKnob(parameter: MobileSynthModel.oscillatorParameters[1],
                                         value: binding("oscBLevel"))
                        AurelineEditKnob(parameter: MobileSynthModel.oscillatorParameters[5],
                                         value: binding("noiseLevel"))
                    }
                }
                .frame(width: 215)
            }

            oscillatorBSection
        }
        .padding(4)
    }

    private var oscillatorBSection: some View {
        AurelineEditGroup(title: "OSCILLATOR B") {
            HStack(spacing: 5) {
                AurelineEditKnob(parameter: octaveParameter("oscBOctave"),
                                 value: binding("oscBOctave"))
                oscillatorShapeControls(id: "waveformMaskB")
                AurelineEditKnob(parameter: renamedParameter(
                    MobileSynthModel.oscillatorParameters[4], "PW"),
                                 value: binding("pulseWidthB"))
                waveMemoryControls(oscillatorA: false)
                AurelineEditKnob(parameter: MobileSynthModel.oscillatorParameters[2],
                                 value: binding("oscBFine"))
                HStack(spacing: -12) {
                    switchButton("SYNC", "oscSync")
                    switchButton("LF", "oscBLowFrequency")
                    switchButton("KB", "oscBKeyTrack")
                }
            }
        }
    }

    private var modulationSection: some View {
        VStack(spacing: 5) {
            AurelineEditGroup(title: "LFO") {
                HStack(spacing: 2) {
                    knobRow(Array(MobileSynthModel.modulationParameters.prefix(4)))
                    waveformControls(id: "lfoWaveMask")
                    switchButton("RETRIG", "lfoRetrigger").frame(width: 42)
                }
            }

            HStack(spacing: 6) {
                AurelineEditGroup(title: "LFO DEST") {
                    HStack(spacing: 3) {
                        switchButton("A FREQ", "lfoDestA")
                        switchButton("B FREQ", "lfoDestB")
                        switchButton("PW A", "lfoDestPWA")
                        switchButton("PW B", "lfoDestPWB")
                        switchButton("FILTER", "lfoDestFilter")
                    }
                    .frame(maxWidth: .infinity, alignment: .center)
                }

                AurelineEditGroup(title: "POLY MOD") {
                    HStack(spacing: 3) {
                        AurelineEditKnob(parameter: MobileSynthModel.modulationParameters[4],
                                         value: binding("polyModFilterEnv"))
                        AurelineEditKnob(parameter: MobileSynthModel.modulationParameters[5],
                                         value: binding("polyModOscB"))
                        switchButton("PITCH", "polyDestPitch")
                        switchButton("PW A", "polyDestPWA")
                        switchButton("FILTER", "polyDestFilter")
                    }
                    .frame(maxWidth: .infinity, alignment: .center)
                }
            }
        }
        .padding(4)
    }

    private var performanceSection: some View {
        let parameters = [
            MobileSynthModel.performanceParameters[6], MobileSynthModel.pitchBendParameter,
            MobileSynthModel.performanceParameters[4],
            MobileSynthModel.performanceParameters[3], MobileSynthModel.performanceParameters[5],
            MobileSynthModel.performanceParameters[0], MobileSynthModel.performanceParameters[1]
        ]
        return AurelineEditGroup(title: "PERFORMANCE") {
            knobRow(parameters)
                .frame(maxWidth: .infinity, alignment: .center)
        }
    }

    private var arpeggiatorSection: some View {
        AurelineEditGroup(title: "ARPEGGIO / PLAY MODE") {
            HStack(spacing: 8) {
                HStack(spacing: 3) {
                    AurelineEditKnob(parameter: discreteParameter("scaleRoot", "SCALE", 0...11, 0),
                                     value: binding("scaleRoot"))
                    AurelineEditKnob(parameter: discreteParameter("arpRate", "ARP RATE", 0...2, 1),
                                     value: binding("arpRate"))
                    AurelineEditKnob(parameter: discreteParameter("arpDirection", "DIRECTION", 0...3, 0),
                                     value: binding("arpDirection"))
                    AurelineEditKnob(parameter: MobileSynthModel.sequencerParameters[1],
                                     value: binding("arpGate"))
                }
                HStack(spacing: 3) {
                    switchButton("ARP", "arpEnabled")
                    switchButton("CHORD", "chordEnabled")
                    switchButton("HOLD", "arpHold")
                }
            }
            .frame(maxWidth: .infinity, alignment: .center)
        }
    }

    private var performanceArpeggiatorSection: some View {
        HStack(spacing: 6) {
            VStack(spacing: 5) {
                performanceSection
                arpeggiatorSection
            }
            AurelineEditGroup(title: "POLY MODE") {
                VStack(spacing: 3) {
                    HStack(spacing: -8) {
                        modeButton("POLY", 0)
                        modeButton("MONO", 1)
                    }
                    HStack(spacing: -8) {
                        modeButton("UNISON", 2)
                        switchButton("LEGATO", "glideLegato")
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
            }
            .frame(width: 145)
        }
        .padding(4)
    }

    private func knobRow(_ parameters: [AurelineParameter]) -> some View {
        HStack(spacing: 3) {
            ForEach(parameters) { parameter in
                AurelineEditKnob(parameter: parameter, value: binding(parameter.id))
            }
        }
    }

    private func envelopeSection(parameters: [AurelineParameter],
                                 ids: (String, String, String, String), title: String) -> some View {
        HStack(spacing: 18) {
            knobRow(parameters)
            AurelineEnvelopeGraph(
                title: title,
                attack: synth.value(ids.0), decay: synth.value(ids.1),
                sustain: synth.value(ids.2), release: synth.value(ids.3)
            )
            .frame(width: 250, height: 108)
        }
        .padding(.horizontal, 16)
    }

    private var filterCombinedSection: some View {
        VStack(spacing: 5) {
            AurelineEditGroup(title: "FILTER") {
                knobRow(Array(MobileSynthModel.filterParameters.prefix(5)))
                    .frame(maxWidth: .infinity, alignment: .center)
            }

            AurelineEditGroup(title: "FILTER ENV") {
                HStack(spacing: 10) {
                    knobRow(Array(MobileSynthModel.filterParameters[5...8]))
                    AurelineEnvelopeGraph(
                        title: "",
                        attack: synth.value("filterAttack"), decay: synth.value("filterDecay"),
                        sustain: synth.value("filterSustain"), release: synth.value("filterRelease")
                    )
                    .frame(maxWidth: .infinity, minHeight: 82, maxHeight: 92)
                }
            }
        }
        .padding(4)
    }

    private var amplifierEnvelopeSection: some View {
        AurelineEditGroup(title: "AMP ENV") {
            HStack(spacing: 14) {
                knobRow(Array(MobileSynthModel.filterParameters[9...12]))
                AurelineEnvelopeGraph(
                    title: "",
                    attack: synth.value("ampAttack"), decay: synth.value("ampDecay"),
                    sustain: synth.value("ampSustain"), release: synth.value("ampRelease")
                )
                .frame(maxWidth: .infinity, minHeight: 100, maxHeight: 130)
            }
        }
        .padding(4)
    }

    private func waveformControls(id: String) -> some View {
        VStack(spacing: 2) {
            if id == "lfoWaveMask" {
                HStack(spacing: 1) {
                    waveButton("SAW UP", id: id, bit: 1)
                    waveButton("TRI", id: id, bit: 2)
                    waveButton("SAW DOWN", id: id, bit: 8)
                    waveButton("SQUARE", id: id, bit: 4)
                    waveButton("S/H", id: id, bit: 16)
                }
            } else {
                HStack(alignment: .top, spacing: 4) {
                    VStack(spacing: 2) {
                        HStack(spacing: 2) {
                            waveButton("SAW", id: id, bit: 1)
                            waveButton("TRI", id: id, bit: 2)
                            waveButton("PULSE", id: id, bit: 4)
                            waveButton("MEM", id: id, bit: 8)
                        }
                        Text("− SHAPE −")
                            .font(.system(size: 9, weight: .bold))
                            .foregroundStyle(Color(hexValue: 0xc7c9c8))
                    }
                    VStack(spacing: 3) {
                        waveMemorySelector(oscillatorA: id == "waveformMaskA")
                        Button("EDIT WAVE") {
                            waveEditorOscillatorA = id == "waveformMaskA"
                            showingWaveEditor = true
                        }
                        .buttonStyle(EditMiniButtonStyle(active: false, fontSize: 12))
                        .frame(width: 92, height: 24)
                    }
                }
            }
            if id == "lfoWaveMask" {
                Text("− SHAPE −")
                    .font(.system(size: 9, weight: .bold))
                    .foregroundStyle(Color(hexValue: 0xc7c9c8))
            }
        }
    }

    private func oscillatorShapeControls(id: String) -> some View {
        VStack(spacing: 2) {
            HStack(spacing: 2) {
                waveButton("SAW", id: id, bit: 1)
                waveButton("TRI", id: id, bit: 2)
                waveButton("PULSE", id: id, bit: 4)
                waveButton("MEM", id: id, bit: 8)
            }
            Text("− SHAPE −")
                .font(.system(size: 9, weight: .bold))
                .foregroundStyle(Color(hexValue: 0xc7c9c8))
        }
    }

    private func waveMemoryControls(oscillatorA: Bool) -> some View {
        VStack(spacing: 3) {
            waveMemorySelector(oscillatorA: oscillatorA)
            Button("EDIT WAVE") {
                waveEditorOscillatorA = oscillatorA
                showingWaveEditor = true
            }
            .buttonStyle(EditMiniButtonStyle(active: false, fontSize: 12))
            .frame(width: 112, height: 24)
        }
    }

    private func waveMemorySelector(oscillatorA: Bool) -> some View {
        let indexID = oscillatorA ? "waveMemoryIndexA" : "waveMemoryIndexB"
        let characterID = oscillatorA ? "waveMemoryCharacterA" : "waveMemoryCharacterB"
        let userID = oscillatorA ? "waveMemoryUserA" : "waveMemoryUserB"
        let selected = max(0, min(15, Int(synth.value(indexID).rounded())))
        let character = max(0, min(2, Int(synth.value(characterID).rounded())))
        let characterNames = ["5-BIT", "4-BIT", "SMOOTH"]
        return VStack(spacing: 3) {
            Button {
                pickerOscillatorA = oscillatorA
                showingOscillatorMemoryPicker = true
            } label: {
                AurelineDropdown(text: synth.value(userID) >= 0.5
                                 ? "USER"
                                 : MobileSynthModel.waveMemoryNames[selected])
                    .frame(width: 112, height: 24)
            }
            .buttonStyle(.plain)

            Button {
                pickerOscillatorA = oscillatorA
                showingOscillatorCharacterPicker = true
            } label: {
                AurelineDropdown(text: characterNames[character])
                    .frame(width: 112, height: 24)
            }
            .buttonStyle(.plain)
        }
    }

    private var oscillatorMemoryIsUser: Bool {
        synth.value(pickerOscillatorA ? "waveMemoryUserA" : "waveMemoryUserB") >= 0.5
    }

    private var oscillatorMemoryTitle: String {
        let id = pickerOscillatorA ? "waveMemoryIndexA" : "waveMemoryIndexB"
        let index = max(0, min(15, Int(synth.value(id).rounded())))
        return MobileSynthModel.waveMemoryNames[index]
    }

    private var oscillatorCharacterTitle: String {
        let id = pickerOscillatorA ? "waveMemoryCharacterA" : "waveMemoryCharacterB"
        let names = ["5-BIT", "4-BIT", "SMOOTH"]
        return names[max(0, min(2, Int(synth.value(id).rounded())))]
    }

    private func waveButton(_ title: String, id: String, bit: Int) -> some View {
        let mask = Int(synth.value(id).rounded())
        return Button {
            let next = mask & bit == 0 ? mask | bit : mask & ~bit
            // Mac permits an empty LFO mask (no LFO waveform), while oscillator
            // waveform groups must always keep at least one waveform enabled.
            synth.set(id, Double(id == "lfoWaveMask" ? next : max(1, next)))
        } label: {
            AurelineMacWaveRocker(kind: title, active: mask & bit != 0)
        }
        .buttonStyle(.plain)
        .frame(width: 36, height: 68)
    }

    private func octaveParameter(_ id: String) -> AurelineParameter {
        AurelineParameter(id: id, name: "RANGE", range: -2...2, defaultValue: 0, logarithmic: false)
    }

    private func renamedParameter(_ parameter: AurelineParameter, _ name: String) -> AurelineParameter {
        AurelineParameter(id: parameter.id, name: name, range: parameter.range,
                          defaultValue: parameter.defaultValue, logarithmic: parameter.logarithmic)
    }

    private func discreteParameter(_ id: String, _ name: String, _ range: ClosedRange<Double>, _ defaultValue: Double) -> AurelineParameter {
        AurelineParameter(id: id, name: name, range: range, defaultValue: defaultValue, logarithmic: false)
    }

    private func modeButton(_ title: String, _ mode: Int) -> some View {
        let active = Int(synth.value("voiceMode").rounded()) == mode
        return Button { synth.set("voiceMode", Double(mode)) } label: {
            AurelineMacRockerLabel(title: title, active: active)
        }
        .buttonStyle(.plain)
        .frame(width: 58, height: 60)
    }

    private func steppedControl(_ title: String, id: String, values: [Double], labels: [String]) -> some View {
        let current = synth.value(id)
        let index = values.enumerated().min(by: { abs($0.element - current) < abs($1.element - current) })?.offset ?? 0
        return VStack(spacing: 3) {
            Text(title).font(.system(size: 8, weight: .bold)).foregroundStyle(Color(hexValue: 0xc7c9c8))
            HStack(spacing: 2) {
                Button("<") { synth.set(id, values[(index - 1 + values.count) % values.count]) }
                    .frame(width: 28, height: 28)
                Text(labels[index]).font(.system(size: 10, weight: .bold, design: .monospaced)).frame(width: 35)
                Button(">") { synth.set(id, values[(index + 1) % values.count]) }
                    .frame(width: 28, height: 28)
            }
            .buttonStyle(EditMiniButtonStyle(active: false))
            .frame(height: 28)
            .foregroundStyle(Color(hexValue: 0xf1f2ef))
        }
    }

    private func switchButton(_ title: String, _ id: String) -> some View {
        Button { synth.toggle(id) } label: {
            AurelineMacRockerLabel(title: title, active: synth.value(id) >= 0.5)
        }.buttonStyle(.plain)
    }
    private func binding(_ id: String) -> Binding<Double> { Binding(get: { synth.value(id) }, set: { synth.set(id, $0) }) }
}

private struct AurelineEditGroup<Content: View>: View {
    let title: String
    let content: Content

    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }

    var body: some View {
        content
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
            .padding(.horizontal, 7)
            .padding(.top, 8)
            .padding(.bottom, 2)
            .background(Color(hexValue: 0x151715))
            .clipShape(RoundedRectangle(cornerRadius: 5))
            .overlay {
                RoundedRectangle(cornerRadius: 5)
                    .stroke(Color(hexValue: 0x737779), lineWidth: 0.8)
            }
            .overlay(alignment: .topLeading) {
                Text(title)
                    .font(.system(size: 11, weight: .bold))
                    .foregroundStyle(Color(hexValue: 0xd7dcda))
                    .padding(.horizontal, 5)
                    .background(Color(hexValue: 0x151715))
                    .offset(x: 10, y: -6)
            }
    }
}

private struct EditSectionTabStyle: ButtonStyle {
    let active: Bool
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 9, weight: .bold))
            .foregroundStyle(active ? Color.black : Color(hexValue: 0xd7dcda))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(active ? Color(hexValue: 0xff9138) : Color(hexValue: 0x28231d))
            .clipShape(RoundedRectangle(cornerRadius: 3))
            .overlay(RoundedRectangle(cornerRadius: 3).stroke(Color.black.opacity(0.7), lineWidth: 1))
            .opacity(configuration.isPressed ? 0.72 : 1)
    }
}

private struct EditMiniButtonStyle: ButtonStyle {
    let active: Bool
    var fontSize: CGFloat = 7
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: fontSize, weight: .bold))
            .foregroundStyle(active ? Color.black : Color(hexValue: 0xd7dcda))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(LinearGradient(
                colors: active
                    ? [Color(hexValue: 0x62686b), Color(hexValue: 0x232729)]
                    : [Color(hexValue: 0x3b3f41), Color(hexValue: 0x111315)],
                startPoint: .top, endPoint: .bottom
            ))
            .clipShape(RoundedRectangle(cornerRadius: 2))
            .overlay(RoundedRectangle(cornerRadius: 2).stroke(Color(hexValue: 0x020303), lineWidth: 1.2))
            .overlay(alignment: .top) { Rectangle().fill(Color.white.opacity(0.10)).frame(height: 1).padding(.horizontal, 2) }
            .opacity(configuration.isPressed ? 0.7 : 1)
    }
}

private struct AurelineMacVoiceButtonStyle: ButtonStyle {
    let store: Bool
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 10, weight: .bold))
            .foregroundStyle(configuration.isPressed ? Color.white : Color(hexValue: 0xffad55))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(
                LinearGradient(
                    colors: configuration.isPressed
                        ? [Color(hexValue: 0xbf5728), Color(hexValue: 0x773016)]
                        : store
                            ? [Color(hexValue: 0xa13b1d), Color(hexValue: 0x6f2412)]
                            : [Color(hexValue: 0x30231a), Color(hexValue: 0x17110d)],
                    startPoint: .top, endPoint: .bottom
                )
            )
            .clipShape(RoundedRectangle(cornerRadius: 3))
            .overlay(RoundedRectangle(cornerRadius: 3).stroke(Color.black.opacity(0.72), lineWidth: 1.2))
            .overlay(alignment: .top) { Rectangle().fill(Color.white.opacity(0.07)).frame(height: 1).padding(.horizontal, 2) }
    }
}

private struct AurelineMacRockerLabel: View {
    let title: String
    let active: Bool
    var body: some View {
        VStack(spacing: 3) {
            Text(title).font(.system(size: title.count > 5 ? 7 : 9, weight: .bold))
                .foregroundStyle(Color(hexValue: 0xc7c9c8)).lineLimit(1)
            ZStack(alignment: .top) {
                RoundedRectangle(cornerRadius: 2)
                    .fill(LinearGradient(colors: [Color(hexValue: 0x303538), Color(hexValue: 0x080a0b)], startPoint: .top, endPoint: .bottom))
                    .overlay(RoundedRectangle(cornerRadius: 2).stroke(Color(hexValue: 0x020303), lineWidth: 1.2))
                Circle().fill(Color.black.opacity(0.85)).frame(width: 10, height: 10).padding(.top, 3)
                Circle().fill(active ? Color(hexValue: 0xff321c) : Color(hexValue: 0x35100c)).frame(width: 7, height: 7).padding(.top, 4.5)
                RoundedRectangle(cornerRadius: 1.5)
                    .fill(LinearGradient(colors: active
                        ? [Color(hexValue: 0x62686b), Color(hexValue: 0x111416)]
                        : [Color(hexValue: 0x4b5053), Color(hexValue: 0x111416)], startPoint: .top, endPoint: .bottom))
                    .frame(height: 20).padding(.horizontal, 3).padding(.top, 19)
            }.frame(width: 30, height: 43)
        }.frame(width: 58, height: 60)
    }
}

private struct AurelineMacWaveRocker: View {
    let kind: String
    let active: Bool

    var body: some View {
        VStack(spacing: 1) {
            Canvas { context, size in
                let rect = CGRect(x: 4, y: 2, width: size.width - 8, height: size.height - 4)
                var path = Path()
                switch kind {
                case "MEM":
                    let steps: [CGFloat] = [0.72, 0.25, 0.48, 0.08, 0.38,
                                            0.82, 0.56, 0.18, 0.64]
                    let stepWidth = rect.width / CGFloat(steps.count)
                    path.move(to: CGPoint(x: rect.minX,
                                          y: rect.minY + steps[0] * rect.height))
                    for index in steps.indices {
                        let x = rect.minX + CGFloat(index) * stepWidth
                        let y = rect.minY + steps[index] * rect.height
                        if index > 0 { path.addLine(to: CGPoint(x: x, y: y)) }
                        path.addLine(to: CGPoint(x: x + stepWidth, y: y))
                    }
                case "TRI":
                    path.move(to: CGPoint(x: rect.minX, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.midX, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
                case "PULSE", "SQUARE":
                    path.move(to: CGPoint(x: rect.minX, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.minX, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.midX, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.midX, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
                case "SAW DOWN":
                    path.move(to: CGPoint(x: rect.minX, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.minY))
                case "S/H":
                    let third = rect.width / 3
                    path.move(to: CGPoint(x: rect.minX, y: rect.midY))
                    path.addLine(to: CGPoint(x: rect.minX + third, y: rect.midY))
                    path.addLine(to: CGPoint(x: rect.minX + third, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.minX + third * 2, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.minX + third * 2, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
                default:
                    path.move(to: CGPoint(x: rect.minX, y: rect.maxY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.minY))
                    path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
                }
                context.stroke(path, with: .color(active ? Color(hexValue: 0xeef1f0) : Color(hexValue: 0x777c7e)),
                               style: StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))
            }
            .frame(width: 38, height: 16)

            ZStack(alignment: .top) {
                RoundedRectangle(cornerRadius: 2)
                    .fill(LinearGradient(colors: [Color(hexValue: 0x303538), Color(hexValue: 0x080a0b)], startPoint: .top, endPoint: .bottom))
                    .overlay(RoundedRectangle(cornerRadius: 2).stroke(Color(hexValue: 0x020303), lineWidth: 1.2))
                Circle().fill(Color.black.opacity(0.85)).frame(width: 10, height: 10).padding(.top, 3)
                Circle().fill(active ? Color(hexValue: 0xff321c) : Color(hexValue: 0x35100c))
                    .frame(width: 7, height: 7).padding(.top, 4.5)
                RoundedRectangle(cornerRadius: 1.5)
                    .fill(LinearGradient(colors: active
                        ? [Color(hexValue: 0x62686b), Color(hexValue: 0x111416)]
                        : [Color(hexValue: 0x4b5053), Color(hexValue: 0x111416)], startPoint: .top, endPoint: .bottom))
                    .frame(height: 20).padding(.horizontal, 3).padding(.top, 19)
            }
            .frame(width: 30, height: 41)
        }
    }
}

private struct AurelineWaveMemoryEditor: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @Environment(\.dismiss) private var dismiss
    let oscillatorA: Bool

    @State private var steps = Array(repeating: 16, count: 32)
    @State private var lastDrawnStep: Int?
    @State private var lastDrawnValue: Int?
    @State private var showingMemoryPicker = false
    @State private var showingCharacterPicker = false
    @State private var auditioning = false
    @State private var auditionRestoreValues: [String: Double]?

    private var indexID: String { oscillatorA ? "waveMemoryIndexA" : "waveMemoryIndexB" }
    private var characterID: String { oscillatorA ? "waveMemoryCharacterA" : "waveMemoryCharacterB" }
    private var userID: String { oscillatorA ? "waveMemoryUserA" : "waveMemoryUserB" }
    private let characterNames = ["5-BIT", "4-BIT", "SMOOTH"]

    var body: some View {
        HStack(spacing: 14) {
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("WAVE MEMORY \(oscillatorA ? "A" : "B")")
                        .font(.system(size: 18, weight: .black, design: .rounded))
                        .foregroundStyle(AurelineTheme.amber)
                    Spacer()
                    Text(synth.value(userID) >= 0.5 ? "USER" : memoryTitle)
                        .font(.system(size: 11, weight: .bold, design: .monospaced))
                        .foregroundStyle(AurelineTheme.gold)
                }

                waveCanvas
                    .background(Color(hexValue: 0x070908))
                    .overlay(RoundedRectangle(cornerRadius: 5)
                        .stroke(Color(hexValue: 0x6d7170), lineWidth: 1))

                HStack {
                    Text("32 STEPS · \(characterNames[character])")
                    Spacer()
                    Text("DRAW")
                }
                .font(.system(size: 9, weight: .bold, design: .monospaced))
                .foregroundStyle(Color(hexValue: 0xaeb2b0))
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)

            VStack(spacing: 9) {
                Text("MEMORY").editorCaption()
                Button {
                    showingMemoryPicker = true
                } label: {
                    standardDropdownLabel(synth.value(userID) >= 0.5 ? "USER" : memoryTitle)
                }
                .buttonStyle(.plain)

                Text("CHARACTER").editorCaption()
                Button {
                    showingCharacterPicker = true
                } label: {
                    standardDropdownLabel(characterNames[character])
                }
                .buttonStyle(.plain)

                Spacer()
                auditionButton
                editorButton("COPY") { synth.copyWaveMemory(oscillatorA: oscillatorA) }
                editorButton("PASTE", disabled: !synth.canPasteWave) {
                    synth.pasteWaveMemory(oscillatorA: oscillatorA)
                    reloadSteps()
                }
                editorButton("INIT") {
                    synth.selectWaveMemory(0, oscillatorA: oscillatorA)
                    reloadSteps()
                }
                Spacer()
                editorButton("CLOSE", accent: true) { dismiss() }
            }
            .frame(width: 126)
        }
        .padding(18)
        .background(AurelineTheme.background.ignoresSafeArea())
        .preferredColorScheme(.dark)
        .onAppear {
            synth.enableWaveMemory(oscillatorA: oscillatorA)
            reloadSteps()
        }
        .onDisappear(perform: stopAudition)
        .overlay {
            if showingMemoryPicker {
                AurelineSelectionPicker(
                    title: "SELECT MEMORY",
                    sections: [AurelinePickerSection(
                        title: "FACTORY",
                        options: MobileSynthModel.waveMemoryNames)],
                    selected: synth.value(userID) >= 0.5 ? nil : memoryTitle,
                    onSelect: { _, index in
                        synth.selectWaveMemory(index, oscillatorA: oscillatorA)
                        reloadSteps()
                        showingMemoryPicker = false
                    },
                    onClose: { showingMemoryPicker = false }
                )
            } else if showingCharacterPicker {
                AurelineSelectionPicker(
                    title: "SELECT CHARACTER",
                    sections: [AurelinePickerSection(title: "CHARACTER", options: characterNames)],
                    selected: characterNames[character],
                    onSelect: { _, index in
                        synth.set(characterID, Double(index))
                        showingCharacterPicker = false
                    },
                    onClose: { showingCharacterPicker = false }
                )
            }
        }
    }

    private var character: Int {
        max(0, min(2, Int(synth.value(characterID).rounded())))
    }

    private var memoryTitle: String {
        let index = max(0, min(15, Int(synth.value(indexID).rounded())))
        return MobileSynthModel.waveMemoryNames[index]
    }

    private var waveCanvas: some View {
        GeometryReader { proxy in
            let width = max(1, proxy.size.width)
            let height = max(1, proxy.size.height)
            let columnWidth = width / 32
            ZStack(alignment: .bottomLeading) {
                Canvas { context, size in
                    for level in 0...4 {
                        let y = size.height * CGFloat(level) / 4
                        var line = Path()
                        line.move(to: CGPoint(x: 0, y: y))
                        line.addLine(to: CGPoint(x: size.width, y: y))
                        context.stroke(line, with: .color(.white.opacity(level == 2 ? 0.16 : 0.07)),
                                       lineWidth: 0.5)
                    }
                    for index in 0..<32 {
                        let x = CGFloat(index) * columnWidth
                        let normalized = CGFloat(steps[index]) / 31
                        let bar = CGRect(x: x + 1, y: (1 - normalized) * (height - 4) + 2,
                                         width: max(1, columnWidth - 2),
                                         height: max(2, normalized * (height - 4)))
                        context.fill(Path(roundedRect: bar, cornerRadius: 1),
                                     with: .linearGradient(
                                        Gradient(colors: [AurelineTheme.amber, Color(hexValue: 0xc35c23)]),
                                        startPoint: CGPoint(x: bar.midX, y: bar.minY),
                                        endPoint: CGPoint(x: bar.midX, y: bar.maxY)))
                    }
                }
                Color.clear
                    .contentShape(Rectangle())
                    .gesture(DragGesture(minimumDistance: 0)
                        .onChanged { draw(at: $0.location, size: proxy.size) }
                        .onEnded { _ in
                            lastDrawnStep = nil
                            lastDrawnValue = nil
                        })
            }
        }
    }

    private func draw(at location: CGPoint, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let step = max(0, min(31, Int(location.x / size.width * 32)))
        let raw = max(0, min(31, Int(((size.height - location.y) / size.height * 31).rounded())))
        let fourBitValue = Int((Double(raw) / 31 * 15).rounded())
        let value = character == 1
            ? Int((Double(fourBitValue) * 31 / 15).rounded())
            : raw

        if let previousStep = lastDrawnStep, let previousValue = lastDrawnValue,
           previousStep != step {
            let distance = abs(step - previousStep)
            for offset in 0...distance {
                let index = previousStep + (step > previousStep ? offset : -offset)
                let amount = Double(offset) / Double(distance)
                setStep(index, Int((Double(previousValue) + Double(value - previousValue) * amount).rounded()))
            }
        } else {
            setStep(step, value)
        }
        lastDrawnStep = step
        lastDrawnValue = value
    }

    private func setStep(_ index: Int, _ value: Int) {
        let clamped = max(0, min(31, value))
        steps[index] = clamped
        synth.setWaveMemoryStep(oscillatorA: oscillatorA, index: index, value: clamped)
    }

    private func reloadSteps() {
        steps = synth.waveMemoryData(oscillatorA: oscillatorA)
    }

    private func standardDropdownLabel(_ title: String) -> some View {
        AurelineDropdown(text: title)
        .frame(width: 126, height: 34)
    }

    private func editorButton(_ title: String, disabled: Bool = false, accent: Bool = false,
                              action: @escaping () -> Void) -> some View {
        Button(title, action: action)
            .font(.system(size: 11, weight: .black))
            .foregroundStyle(accent ? Color.black : Color(hexValue: 0xd6d8d7))
            .frame(maxWidth: .infinity, minHeight: 32)
            .background(accent ? AurelineTheme.amber : Color(hexValue: 0x252a2c))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4)
                .stroke(Color(hexValue: 0x050606), lineWidth: 1))
            .disabled(disabled)
            .opacity(disabled ? 0.35 : 1)
    }

    private var auditionButton: some View {
        Text("AUDITION C4")
            .font(.system(size: 11, weight: .black))
            .foregroundStyle(auditioning ? Color.black : AurelineTheme.amber)
            .frame(maxWidth: .infinity, minHeight: 36)
            .background(auditioning ? AurelineTheme.amber : Color(hexValue: 0x252a2c))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4)
                .stroke(Color(hexValue: 0x765d28), lineWidth: 1))
            .contentShape(Rectangle())
            .gesture(DragGesture(minimumDistance: 0)
                .onChanged { _ in
                    guard !auditioning else { return }
                    beginAudition()
                }
                .onEnded { _ in stopAudition() })
    }

    private func beginAudition() {
        let restoreIDs = [
            "waveformMaskA", "waveformMaskB",
            "oscALevel", "oscBLevel", "noiseLevel"
        ]
        auditionRestoreValues = Dictionary(uniqueKeysWithValues:
            restoreIDs.map { ($0, synth.value($0)) })

        synth.set("waveformMaskA", oscillatorA ? 8 : synth.value("waveformMaskA"))
        synth.set("waveformMaskB", oscillatorA ? synth.value("waveformMaskB") : 8)
        synth.set("oscALevel", oscillatorA ? 1 : 0)
        synth.set("oscBLevel", oscillatorA ? 0 : 1)
        synth.set("noiseLevel", 0)
        auditioning = true
        synth.noteOnAbsolute(60, velocity: 100)
    }

    private func stopAudition() {
        if auditioning {
            synth.noteOffAbsolute(60)
            auditioning = false
        }
        if let auditionRestoreValues {
            for (id, value) in auditionRestoreValues {
                synth.set(id, value)
            }
            self.auditionRestoreValues = nil
        }
    }
}

private extension View {
    func editorCaption() -> some View {
        font(.system(size: 8, weight: .bold))
            .foregroundStyle(Color(hexValue: 0xaeb2b0))
            .frame(maxWidth: .infinity, alignment: .leading)
    }
}

private struct AurelineEnvelopeGraph: View {
    let title: String
    let attack: Double
    let decay: Double
    let sustain: Double
    let release: Double

    var body: some View {
        Canvas { context, size in
            let panel = CGRect(origin: .zero, size: size).insetBy(dx: 1, dy: 1)
            context.fill(Path(roundedRect: panel, cornerRadius: 4),
                         with: .linearGradient(Gradient(colors: [Color(hexValue: 0x171816), Color(hexValue: 0x080908)]),
                                               startPoint: CGPoint(x: panel.midX, y: panel.minY),
                                               endPoint: CGPoint(x: panel.midX, y: panel.maxY)))
            context.stroke(Path(roundedRect: panel, cornerRadius: 4),
                           with: .color(Color(hexValue: 0x737779)), lineWidth: 0.8)

            if !title.isEmpty {
                let caption = context.resolve(Text(title)
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(Color(hexValue: 0xd5d7d6)))
                context.draw(caption, at: CGPoint(x: 10, y: 8), anchor: .leading)
            }

            let graphTop: CGFloat = title.isEmpty ? 10 : 24
            let graph = CGRect(x: 10, y: graphTop, width: max(1, size.width - 20),
                               height: max(1, size.height - graphTop - 10))
            let total = max(0.1, attack + decay + release)
            let attackWidth = 1.5 + graph.width * 0.30 * CGFloat(min(1, max(0, attack / 5)))
            let decayWidth = graph.width * CGFloat(0.16 + 0.16 * decay / total)
            let releaseWidth = graph.width * CGFloat(0.16 + 0.16 * release / total)
            let sustainWidth = max(12, graph.width - attackWidth - decayWidth - releaseWidth)
            let baseY = graph.maxY - 2
            let peakY = graph.minY + 2
            let sustainY = baseY + (peakY - baseY) * CGFloat(min(1, max(0, sustain)))

            var envelope = Path()
            envelope.move(to: CGPoint(x: graph.minX, y: baseY))
            envelope.addLine(to: CGPoint(x: graph.minX + attackWidth, y: peakY))
            envelope.addLine(to: CGPoint(x: graph.minX + attackWidth + decayWidth, y: sustainY))
            envelope.addLine(to: CGPoint(x: graph.minX + attackWidth + decayWidth + sustainWidth, y: sustainY))
            envelope.addLine(to: CGPoint(x: graph.maxX, y: baseY))
            context.stroke(envelope, with: .color(Color(hexValue: 0xff7a28).opacity(0.14)),
                           style: StrokeStyle(lineWidth: 5, lineCap: .round, lineJoin: .round))
            context.stroke(envelope, with: .color(Color(hexValue: 0xff9a42)),
                           style: StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))
        }
        .accessibilityHidden(true)
    }
}

private struct AurelineEditKnob: View {
    let parameter: AurelineParameter
    @Binding var value: Double
    @State private var dragStartValue: Double?

    private var normalized: Double {
        if parameter.id == "lfoDelay" || parameter.id == "lfoFade" {
            let linear = (value - parameter.range.lowerBound)
                / (parameter.range.upperBound - parameter.range.lowerBound)
            // Matches JUCE Slider::setSkewFactor(0.5) used by the Mac version.
            return pow(min(1, max(0, linear)), 2)
        }
        if parameter.logarithmic, parameter.range.lowerBound > 0 {
            return log(max(parameter.range.lowerBound, value) / parameter.range.lowerBound)
                / log(parameter.range.upperBound / parameter.range.lowerBound)
        }
        return (value - parameter.range.lowerBound) / (parameter.range.upperBound - parameter.range.lowerBound)
    }

    private var controlWidth: CGFloat {
        parameter.id == "scaleRoot" ? 82 : 66
    }

    var body: some View {
        VStack(spacing: 1) {
            Text(parameter.name)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Color(hexValue: 0xd7dcda))
                .lineLimit(1).minimumScaleFactor(0.7)
                .frame(height: 12)
            Canvas { context, size in drawKnob(context: &context, size: size) }
                .frame(width: controlWidth - 4, height: 58)
                .contentShape(Rectangle())
                .gesture(DragGesture(minimumDistance: 0)
                    .onChanged { gesture in
                        if dragStartValue == nil { dragStartValue = normalized }
                        setNormalized(min(1, max(0, (dragStartValue ?? normalized) - Double(gesture.translation.height / 130))))
                    }
                    .onEnded { _ in dragStartValue = nil })
                .onTapGesture(count: 2) { value = parameter.defaultValue }
            Text(displayValue)
                .font(.system(size: 13, weight: .medium, design: .monospaced))
                .foregroundStyle(Color(hexValue: 0xf1f2ef))
                .frame(width: controlWidth - 8, height: 18)
                .background(Color(hexValue: 0x050505))
                .overlay(Rectangle().stroke(Color(hexValue: 0x67645e), lineWidth: 1))
        }
        .frame(width: controlWidth)
        .accessibilityElement(children: .ignore)
        .accessibilityLabel(parameter.name)
        .accessibilityValue(displayValue)
    }

    private func setNormalized(_ position: Double) {
        if ["oscAOctave", "oscBOctave", "scaleRoot", "arpRate", "arpDirection"].contains(parameter.id) {
            value = (parameter.range.lowerBound + position * (parameter.range.upperBound - parameter.range.lowerBound)).rounded()
            return
        }
        if parameter.id == "lfoDelay" || parameter.id == "lfoFade" {
            value = parameter.range.lowerBound
                + sqrt(position) * (parameter.range.upperBound - parameter.range.lowerBound)
            return
        }
        if parameter.logarithmic, parameter.range.lowerBound > 0 {
            value = parameter.range.lowerBound * pow(parameter.range.upperBound / parameter.range.lowerBound, position)
        } else {
            value = parameter.range.lowerBound + position * (parameter.range.upperBound - parameter.range.lowerBound)
        }
    }

    private var displayValue: String {
        if parameter.id == "oscBFine" { return String(format: "%.0f", value) }
        if parameter.id == "tempoBpm" { return String(format: "%.0f", value) }
        if parameter.id == "lfoRate" {
            if value < 1 { return String(format: "%.2f", value) }
            if value < 10 { return String(format: "%.1f", value) }
            return String(format: "%.0f", value)
        }
        if parameter.id == "lfoAmount" {
            return String(format: "%.1f", min(1, max(0, value)) * 10)
        }
        if parameter.id == "oscAOctave" || parameter.id == "oscBOctave" {
            let labels = ["32'", "16'", "8'", "4'", "2'"]
            return labels[max(0, min(4, Int(value.rounded()) + 2))]
        }
        if parameter.id == "scaleRoot" {
            let labels = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
            return labels[max(0, min(11, Int(value.rounded())))]
        }
        if parameter.id == "arpRate" {
            return ["1/8", "1/16", "1/32"][max(0, min(2, Int(value.rounded())))]
        }
        if parameter.id == "arpDirection" {
            return ["UP", "DOWN", "U/D", "RND"][max(0, min(3, Int(value.rounded())))]
        }
        return "\(Int((min(1, max(0, normalized)) * 10).rounded()))"
    }

    private func drawKnob(context: inout GraphicsContext, size: CGSize) {
        let position = CGFloat(min(1, max(0, normalized)))
        let isRange = parameter.id == "oscAOctave" || parameter.id == "oscBOctave"
        let isArpeggioChoice = ["scaleRoot", "arpRate", "arpDirection"].contains(parameter.id)
        let center = CGPoint(x: size.width * 0.5, y: size.height * 0.53)
        let radius = min(size.width, size.height) * (isRange || isArpeggioChoice ? 0.35 : 0.43)
        let startAngle = -CGFloat.pi * 0.75
        let arcRange = CGFloat.pi * 1.5
        let activeTick = Int((position * 22).rounded())

        func point(_ r: CGFloat, _ angle: CGFloat) -> CGPoint {
            CGPoint(x: center.x + sin(angle) * r, y: center.y - cos(angle) * r)
        }

        if isRange {
            let labels = ["32′", "16′", "8′", "4′", "2′"]
            let selectedIndex = max(0, min(4, Int(value.rounded()) + 2))
            for index in labels.indices {
                let angle = startAngle + CGFloat(index) / 4 * arcRange
                let labelPosition = point(radius + 4, angle)
                let label = context.resolve(Text(labels[index])
                    .font(.system(size: 9, weight: index == selectedIndex ? .bold : .medium))
                    .foregroundColor(index == selectedIndex
                        ? Color(hexValue: 0xe8eae8) : Color(hexValue: 0x777b7d)))
                context.draw(label, at: labelPosition, anchor: .center)
            }
        } else if isArpeggioChoice {
            let labels: [String]
            switch parameter.id {
            case "scaleRoot":
                labels = ["C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"]
            case "arpRate":
                labels = ["1/8", "1/16", "1/32"]
            default:
                labels = ["UP", "DOWN", "U/D", "RND"]
            }
            let selectedIndex = max(0, min(labels.count - 1, Int(value.rounded())))
            for index in labels.indices {
                let angle = startAngle + CGFloat(index) / CGFloat(max(1, labels.count - 1)) * arcRange
                let selected = index == selectedIndex
                let label = context.resolve(Text(labels[index])
                    .font(.system(size: parameter.id == "scaleRoot" ? 7 : 8,
                                  weight: selected ? .bold : .semibold))
                    .foregroundColor(selected ? Color(hexValue: 0xe8eae8)
                                              : Color(hexValue: 0x74787a)))
                context.draw(label, at: point(radius + (parameter.id == "scaleRoot" ? 7 : 8), angle),
                             anchor: .center)
            }
        } else {
            for tick in 0..<23 {
                let angle = startAngle + CGFloat(tick) / 22 * arcRange
                let major = tick == 0 || tick == 11 || tick == 22
                let selected = tick <= activeTick
                let color = selected ? Color(hexValue: 0xc7cac9) : Color(hexValue: 0x34383a).opacity(0.82)
                if major {
                    let mark = Path { path in
                        path.move(to: point(radius - 3.5, angle))
                        path.addLine(to: point(radius + 1, angle))
                    }
                    context.stroke(mark, with: .color(color), lineWidth: 1.8)
                } else {
                    let p = point(radius - 0.5, angle)
                    context.fill(Path(ellipseIn: CGRect(x: p.x - 1.35, y: p.y - 1.35,
                                                        width: 2.7, height: 2.7)),
                                 with: .color(color))
                }
            }
        }

        let outerRadius = radius * 0.69
        let outer = CGRect(x: center.x - outerRadius, y: center.y - outerRadius, width: outerRadius * 2, height: outerRadius * 2)
        context.fill(Path(ellipseIn: outer.offsetBy(dx: 0, dy: 2)), with: .color(.black.opacity(0.35)))
        context.fill(Path(ellipseIn: outer), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x424039), Color(hexValue: 0x070706)]), startPoint: CGPoint(x: outer.minX, y: outer.minY), endPoint: CGPoint(x: outer.maxX, y: outer.maxY)))
        context.stroke(Path(ellipseIn: outer), with: .color(Color(hexValue: 0x050505)), lineWidth: 2)
        let inner = outer.insetBy(dx: outer.width * 0.16, dy: outer.height * 0.16)
        context.fill(Path(ellipseIn: inner), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x22221f), Color(hexValue: 0x090909)]), startPoint: CGPoint(x: inner.minX, y: inner.minY), endPoint: CGPoint(x: inner.maxX, y: inner.maxY)))
        context.stroke(Path(ellipseIn: inner), with: .color(.black.opacity(0.8)), lineWidth: 1.2)

        let angle = startAngle + position * arcRange
        let end = point(radius * 0.48, angle)
        let pointer = Path { path in path.move(to: center); path.addLine(to: end) }
        context.stroke(pointer, with: .linearGradient(Gradient(colors: [Color(hexValue: 0x777d80), Color(hexValue: 0xe9edeb)]), startPoint: center, endPoint: end), style: StrokeStyle(lineWidth: 4, lineCap: .round))
    }
}
