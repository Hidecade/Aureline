import SwiftUI
import UniformTypeIdentifiers

struct PlayView: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @State private var keyboardBaseNote = 48
    @State private var keyboardScrollWhiteIndex: CGFloat = 16
    @State private var normalKeyboardBaseNote = 48
    @State private var voiceError: String?
    @State private var showingVoicePicker = false
    @State private var importingVoice = false
    @State private var exportingVoice = false
    @State private var exportFileURL: URL?
    @State private var pendingLibraryURL: URL?
    @State private var confirmingLibraryLoad = false
    @State private var exportIsLibrary = false
    @State private var showingSaveName = false
    @State private var saveVoiceName = ""
    private let visibleWhiteKeys = 15

    var body: some View {
        GeometryReader { proxy in
            let contentHeight = max(1, proxy.size.height - 12)
            let keyboardHeight = max(118, contentHeight * 0.5 - 8)
            VStack(spacing: 2) {
                HStack(alignment: .top, spacing: 12) {
                    shiftButtons(up: false).frame(width: 58)
                    mainControlPanel
                    performanceKnobs.frame(width: 190)
                    shiftButtons(up: true).frame(width: 58)
                }
                .padding(8).frame(maxWidth: .infinity, maxHeight: .infinity).background(AurelineMetalPanel()).clipped()

                OpalineStyleKeyboardView(visibleWhiteKeyCount: visibleWhiteKeys, scrollWhiteIndex: keyboardScrollWhiteIndex)
                    .environmentObject(synth).frame(height: keyboardHeight).clipped()
            }.padding(.vertical, 6).background(AurelineTheme.background)
        }.background(AurelineTheme.background).preferredColorScheme(.dark)
            .onAppear {
                if Int(synth.value("voiceMode").rounded()) == 3 {
                    setBaseNote(36)
                }
            }
            .onChange(of: Int(synth.value("voiceMode").rounded())) { mode in
                synth.panic()
                if mode == 3 {
                    normalKeyboardBaseNote = keyboardBaseNote
                    setBaseNote(36)
                } else {
                    setBaseNote(normalKeyboardBaseNote)
                }
            }
            .overlay {
                if showingVoicePicker {
                    AurelineVoicePicker(
                        factoryNames: synth.factoryPresetNames,
                        bankNames: synth.bankNames,
                        selectedBank: synth.selectedBank,
                        selectedName: synth.selectedFactoryDisplayName,
                        onBankSelect: synth.selectBank,
                        onFactorySelect: { index in changeVoice(index); showingVoicePicker = false },
                        onClose: { showingVoicePicker = false }
                    )
                }
            }
            .alert("VOICE", isPresented: Binding(get: { voiceError != nil }, set: { if !$0 { voiceError = nil } })) {
                Button("OK") { voiceError = nil }
            } message: { Text(voiceError ?? "") }
            .alert("SAVE VOICE", isPresented: $showingSaveName) {
                TextField("Voice name", text: $saveVoiceName)
                Button("CANCEL", role: .cancel) {}
                Button("SAVE") { prepareVoiceExport(named: saveVoiceName) }
            } message: {
                Text("Enter a voice name (up to 16 characters).")
            }
            .alert("LOAD LIBRARY TO BANK", isPresented: $confirmingLibraryLoad) {
                Button("CANCEL", role: .cancel) { pendingLibraryURL = nil }
                ForEach(synth.bankNames.indices, id: \.self) { bank in
                    Button("BANK \(bank + 1)  \(synth.bankNames[bank])") {
                        loadPendingLibrary(into: bank)
                    }
                }
            } message: {
                Text("Choose the bank whose 32 voices will be replaced.")
            }
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
                        do { try synth.importPreset(from: url); resetKeyboard() }
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
                                ? "Current bank saved" : "Voice saved"
                        }
                    }
                }
            }
    }

    private func shiftButtons(up: Bool) -> some View {
        VStack(spacing: 4) {
            AurelinePanelButton(up ? "+1" : "-1", disabled: up ? keyboardBaseNote >= 82 : keyboardBaseNote <= 21) { shiftWhite(up ? 1 : -1) }.frame(height: 48)
            AurelinePanelButton(up ? "OCT+" : "OCT-", disabled: up ? keyboardBaseNote >= 82 : keyboardBaseNote <= 21) { shiftOctave(up ? 1 : -1) }.frame(height: 48)
            Spacer()
        }.padding(.top, 6)
    }

    private var wheelPanel: some View {
        HStack(spacing: 8) {
            AurelineWheelFader(title: "PITCH", value: Binding(get: { synth.pitchWheel }, set: { synth.setPitchWheel($0) }), range: -1...1, resets: true).frame(width: 59)
            AurelineWheelFader(title: "MODULATION", value: Binding(get: { synth.modWheel }, set: { synth.setModWheel($0) }), range: 0...1, resets: false).frame(width: 59)
        }.padding(.horizontal, 7).padding(.vertical, 6)
    }

    private var mainControlPanel: some View {
        GeometryReader { geometry in
            let contentWidth = max(0, geometry.size.width - 150)
            let lcdWidth = min(CGFloat(252), max(218, contentWidth * 0.62))
            let waveformWidth = max(0, contentWidth - lcdWidth - 6)
            VStack(alignment: .leading, spacing: 4) {
                HStack(spacing: 10) {
                    HStack(spacing: 2) {
                        playModeButton("PLAY", active: true) {}
                        playModeButton("EDIT", active: false) { synth.screen = .edit }
                    }
                    .frame(width: 98)
                    HStack(spacing: 4) {
                        playVoiceButton("LOAD") { importingVoice = true }
                        playVoiceButton("SAVE") {
                            saveVoiceName = String(synth.selectedPreset.prefix(16))
                            showingSaveName = true
                        }
                        playVoiceButton("COPY") { synth.copyCurrentVoice() }
                        playVoiceButton("PASTE", disabled: !synth.canPasteVoice) { synth.pasteCopiedVoice() }
                        playVoiceButton("INIT") { synth.initializePatch() }
                        playVoiceButton("STORE", store: true) {
                            do { try synth.storeCurrentVoice() }
                            catch { voiceError = error.localizedDescription }
                        }
                        playVoiceButton("SAVE BANK") { prepareLibraryExport() }
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.leading, 42)
                }
                .frame(maxWidth: .infinity, alignment: .leading)
                .frame(height: 28)
                .offset(y: -2)

                HStack(alignment: .center, spacing: 10) {
                    wheelPanel.frame(width: 140)
                    ZStack(alignment: .topLeading) {
                        VStack(alignment: .leading, spacing: 3) {
                            AurelineLCD(lines: lcdLines)
                                .frame(width: lcdWidth, height: 64)
                            HStack(spacing: 4) {
                                Button { showingVoicePicker = true } label: {
                                    AurelineDropdown(text: synth.selectedPreset)
                                }
                                .buttonStyle(.plain)
                                AurelinePanelButton("<") { stepVoice(-1) }.frame(width: 38)
                                AurelinePanelButton(">") { stepVoice(1) }.frame(width: 38)
                            }
                            .frame(width: lcdWidth, height: 28)
                            performanceSwitchRow
                                .frame(width: contentWidth, height: 48)
                        }

                        AurelineWaveform(
                            layout: .play,
                            samplesCombined: synth.scopeSamples,
                            samplesA: synth.synthesizedOscillatorWaveformSamples(
                                oscillatorA: true),
                            samplesB: synth.synthesizedOscillatorWaveformSamples(
                                oscillatorA: false),
                            cyclesCombined: 1,
                            cyclesA: synth.displayedWaveformCycles(oscillatorA: true),
                            cyclesB: synth.displayedWaveformCycles(oscillatorA: false))
                            .frame(width: waveformWidth, height: 95)
                            .offset(x: lcdWidth + 6)
                    }
                    .offset(y: -2)
                }
                .frame(maxHeight: .infinity)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    private func prepareVoiceExport(named name: String) {
        do {
            let trimmedName = name.trimmingCharacters(in: .whitespacesAndNewlines)
            if !trimmedName.isEmpty {
                synth.selectedPreset = String(trimmedName.prefix(16))
            }
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

    private func loadPendingLibrary(into bank: Int) {
        guard let url = pendingLibraryURL else { return }
        do {
            try synth.importLibrary(from: url, intoBank: bank)
            resetKeyboard()
        } catch {
            voiceError = error.localizedDescription
        }
        pendingLibraryURL = nil
    }

    private var performanceSwitchRow: some View {
        HStack(spacing: 1) {
            AurelineCompactRockerButton("POLY", active: Int(synth.value("voiceMode")) == 0) { synth.set("voiceMode", 0) }
            AurelineCompactRockerButton("MONO", active: Int(synth.value("voiceMode")) == 1) { synth.set("voiceMode", 1) }
            AurelineCompactRockerButton("UNISON", active: Int(synth.value("voiceMode")) == 2) { synth.set("voiceMode", 2) }
            AurelineCompactRockerButton("ARP", active: synth.value("arpEnabled") >= 0.5) { synth.toggle("arpEnabled") }
            AurelineCompactRockerButton("CHORD", active: synth.value("chordEnabled") >= 0.5) { synth.toggle("chordEnabled") }
            AurelineCompactRockerButton("HOLD", active: synth.value("arpHold") >= 0.5) { synth.toggle("arpHold") }
            AurelineCompactRockerButton("KIT", active: Int(synth.value("voiceMode")) == 3) {
                let kitIsActive = Int(synth.value("voiceMode").rounded()) == 3
                if !kitIsActive {
                    synth.set("arpEnabled", 0)
                    synth.set("chordEnabled", 0)
                }
                synth.set("voiceMode", kitIsActive ? 0 : 3)
            }
        }
    }

    private var performanceKnobs: some View {
        HStack(spacing: 4) {
            VStack(spacing: 2) {
                AurelineNumberedVolumeKnob(value: binding("masterGain"))
                    .frame(width: 62, height: 76)
                AurelineNumberedTransposeKnob(value: binding("transpose"))
                    .frame(width: 62, height: 76)
            }.frame(width: 62)
            VStack {
                ParameterKnob(parameter: MobileSynthModel.performanceParameters[3], value: binding("glide"))
                ParameterKnob(parameter: MobileSynthModel.sequencerParameters[0], value: binding("tempoBpm"))
            }
            VStack {
                ParameterKnob(parameter: MobileSynthModel.filterParameters[0], value: binding("cutoff"))
                ParameterKnob(parameter: MobileSynthModel.filterParameters[1], value: binding("resonance"))
            }
        }.frame(maxHeight: .infinity)
    }

    private var lcdLines: [String] {
        let mode = ["POLY", "MONO", "UNISON", "KIT"][min(3, max(0, Int(synth.value("voiceMode"))))]
        let macMode = mode == "UNISON" ? "UNI" : mode
        let tempo = Int(synth.value("tempoBpm").rounded())
        return ["\(macMode) TEMPO\(tempo)", synth.selectedPreset.uppercased()]
    }
    private func binding(_ id: String) -> Binding<Double> { Binding(get: { synth.value(id) }, set: { synth.set(id, $0) }) }
    private func changeVoice(_ index: Int) { synth.loadFactoryPreset(index); resetKeyboard() }
    private func stepVoice(_ delta: Int) {
        let current = synth.selectedFactoryDisplayName.flatMap {
            synth.factoryPresetNames.firstIndex(of: $0)
        } ?? 0
        changeVoice((current + delta + synth.factoryPresetNames.count) % max(1, synth.factoryPresetNames.count))
    }
    private func resetKeyboard() {
        let kitMode = Int(synth.value("voiceMode").rounded()) == 3
        normalKeyboardBaseNote = 48
        setBaseNote(kitMode ? 36 : 48)
    }
    private func shiftWhite(_ delta: Int) {
        var candidate = keyboardBaseNote + delta
        while candidate >= 21, candidate <= 82, [1, 3, 6, 8, 10].contains(candidate % 12) { candidate += delta }
        setBaseNote(min(82, max(21, candidate)))
    }
    private func shiftOctave(_ delta: Int) { setBaseNote(min(82, max(21, keyboardBaseNote + delta * 12))) }
    private func setBaseNote(_ note: Int) {
        withAnimation(.easeInOut(duration: 0.18)) {
            keyboardBaseNote = note
            keyboardScrollWhiteIndex = CGFloat((21..<note).filter { ![1, 3, 6, 8, 10].contains($0 % 12) }.count)
        }
    }
    private func playModeButton(_ title: String, active: Bool, action: @escaping () -> Void) -> some View {
        Button(action: action) { Text(title).font(.system(size: 11, weight: .bold)).frame(maxWidth: .infinity, maxHeight: .infinity).background(active ? AurelineTheme.amber : AurelineTheme.panelLight).foregroundStyle(active ? .black : AurelineTheme.text).clipShape(RoundedRectangle(cornerRadius: 3)) }
            .buttonStyle(.plain)
            .frame(width: 48, height: 24)
    }
    private func playVoiceButton(_ title: String, store: Bool = false, disabled: Bool = false,
                                 action: @escaping () -> Void) -> some View {
        Button(title, action: action)
            .buttonStyle(AurelinePlayVoiceButtonStyle(store: store))
            .frame(maxWidth: .infinity, minHeight: 24, maxHeight: 24)
            .disabled(disabled)
            .opacity(disabled ? 0.35 : 1)
    }
}

private struct AurelinePlayVoiceButtonStyle: ButtonStyle {
    let store: Bool
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 8, weight: .bold))
            .foregroundStyle(configuration.isPressed ? Color.white : Color(hexValue: 0xffad55))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(LinearGradient(colors: store
                ? [Color(hexValue: 0xa13b1d), Color(hexValue: 0x6f2412)]
                : [Color(hexValue: 0x30231a), Color(hexValue: 0x17110d)], startPoint: .top, endPoint: .bottom))
            .clipShape(RoundedRectangle(cornerRadius: 3))
            .overlay(RoundedRectangle(cornerRadius: 3).stroke(Color.black.opacity(0.72), lineWidth: 1))
            .opacity(configuration.isPressed ? 0.72 : 1)
    }
}

private struct AurelinePlayRockerButton: View {
    let title: String
    let active: Bool
    let action: () -> Void

    init(_ title: String, active: Bool = false, action: @escaping () -> Void) {
        self.title = title
        self.active = active
        self.action = action
    }

    var body: some View {
        Button(action: action) { Color.clear }
            .buttonStyle(AurelinePlayRockerStyle(title: title, active: active))
            .frame(maxWidth: .infinity)
            .accessibilityLabel(title)
            .accessibilityValue(active ? "On" : "Off")
    }
}

private struct AurelineCompactRockerButton: View {
    let title: String
    let active: Bool
    let action: () -> Void

    init(_ title: String, active: Bool = false, action: @escaping () -> Void) {
        self.title = title
        self.active = active
        self.action = action
    }

    var body: some View {
        Button(action: action) {
            VStack(spacing: 0) {
                Text(title)
                    .font(.system(size: 9, weight: .bold))
                    .foregroundStyle(Color(hexValue: 0xc7c9c8))
                    .lineLimit(1)
                    .minimumScaleFactor(0.85)
                ZStack(alignment: .top) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(LinearGradient(colors: [Color(hexValue: 0x303538), Color(hexValue: 0x080a0b)], startPoint: .top, endPoint: .bottom))
                        .overlay(RoundedRectangle(cornerRadius: 2).stroke(Color(hexValue: 0x020303), lineWidth: 1.2))
                    Circle().fill(Color.black.opacity(0.85)).frame(width: 9, height: 9).padding(.top, 2)
                    Circle().fill(active ? Color(hexValue: 0xff321c) : Color(hexValue: 0x35100c))
                        .frame(width: 6, height: 6).padding(.top, 3.5)
                    RoundedRectangle(cornerRadius: 1.5)
                        .fill(LinearGradient(colors: active
                            ? [Color(hexValue: 0x62686b), Color(hexValue: 0x111416)]
                            : [Color(hexValue: 0x4b5053), Color(hexValue: 0x111416)], startPoint: .top, endPoint: .bottom))
                        .frame(height: 15).padding(.horizontal, 3).padding(.top, 16)
                }
                .frame(maxWidth: .infinity)
                .frame(height: 33)
                .padding(.horizontal, 3)
            }
            .frame(maxWidth: .infinity)
            .frame(height: 48)
        }
        .buttonStyle(.plain)
        .frame(maxWidth: .infinity)
    }
}

private struct AurelinePlayRockerStyle: ButtonStyle {
    let title: String
    let active: Bool

    func makeBody(configuration: Configuration) -> some View {
        let illuminated = active || configuration.isPressed
        return VStack(spacing: 1) {
            Text(title)
                .font(.system(size: title.count > 5 ? 7 : 9, weight: .bold))
                .foregroundStyle(Color(hexValue: 0xc7c9c8))
                .lineLimit(1)
                .minimumScaleFactor(0.7)
            ZStack(alignment: .top) {
                RoundedRectangle(cornerRadius: 2)
                    .fill(LinearGradient(colors: [Color(hexValue: 0x303538), Color(hexValue: 0x080a0b)], startPoint: .top, endPoint: .bottom))
                    .overlay(RoundedRectangle(cornerRadius: 2).stroke(Color(hexValue: 0x020303), lineWidth: 1.2))
                Circle().fill(Color.black.opacity(0.85)).frame(width: 10, height: 10).padding(.top, 3)
                Circle().fill(illuminated ? Color(hexValue: 0xff321c) : Color(hexValue: 0x35100c))
                    .frame(width: 7, height: 7).padding(.top, 4.5)
                RoundedRectangle(cornerRadius: 1.5)
                    .fill(LinearGradient(colors: illuminated
                        ? [Color(hexValue: 0x62686b), Color(hexValue: 0x111416)]
                        : [Color(hexValue: 0x4b5053), Color(hexValue: 0x111416)], startPoint: .top, endPoint: .bottom))
                    .frame(height: 14).padding(.horizontal, 3).padding(.top, 17)
            }
            .frame(maxWidth: .infinity, minHeight: 34, maxHeight: 34)
            .padding(.horizontal, 4)
        }
        .frame(maxWidth: .infinity, minHeight: 51)
        .contentShape(Rectangle())
    }
}

private struct AurelineMetalPanel: View {
    var body: some View { LinearGradient(colors: [Color(hexValue: 0x29251f), Color(hexValue: 0x11100d)], startPoint: .top, endPoint: .bottom).overlay(Rectangle().stroke(AurelineTheme.gold.opacity(0.35))) }
}

private struct AurelinePanelButton: View {
    let title: String; var active = false; var disabled = false; let action: () -> Void
    init(_ title: String, active: Bool = false, disabled: Bool = false, action: @escaping () -> Void) { self.title = title; self.active = active; self.disabled = disabled; self.action = action }
    var body: some View { Button(action: action) { Text(title).font(.system(size: 10, weight: .bold)).frame(maxWidth: .infinity, maxHeight: .infinity).foregroundStyle(active ? .black : AurelineTheme.text).background(LinearGradient(colors: active ? [AurelineTheme.amber, Color(hexValue: 0xb95718)] : [Color(hexValue: 0x3a3731), Color(hexValue: 0x171612)], startPoint: .top, endPoint: .bottom)).clipShape(RoundedRectangle(cornerRadius: 3)).overlay(RoundedRectangle(cornerRadius: 3).stroke(.black.opacity(0.6))) }.buttonStyle(.plain).disabled(disabled).opacity(disabled ? 0.35 : 1) }
}

struct AurelineDropdown: View {
    let text: String
    var body: some View {
        HStack {
            Text(text)
                .font(.system(size: 12, weight: .bold))
                .lineLimit(1)
                .truncationMode(.tail)
            Spacer(minLength: 6)
            Image(systemName: "chevron.down")
                .font(.system(size: 13, weight: .bold))
        }
        .foregroundStyle(AurelineTheme.amber)
        .padding(.horizontal, 8)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(hexValue: 0x090b08))
        .clipShape(RoundedRectangle(cornerRadius: 3))
        .overlay(RoundedRectangle(cornerRadius: 3).stroke(AurelineTheme.gold.opacity(0.5)))
    }
}

private struct AurelineVoicePicker: View {
    let factoryNames: [String]
    let bankNames: [String]
    let selectedBank: Int
    let selectedName: String?
    let onBankSelect: (Int) -> Void
    let onFactorySelect: (Int) -> Void
    let onClose: () -> Void

    var body: some View {
        AurelineSelectionPicker(
            title: "SELECT VOICE · \(bankNames[selectedBank])",
            sections: [AurelinePickerSection(title: nil, options: factoryNames)],
            selected: selectedName,
            onSelect: { _, index in onFactorySelect(index) },
            onClose: onClose,
            bankNames: bankNames,
            selectedBank: selectedBank,
            onBankSelect: onBankSelect
        )
    }
}

struct AurelinePickerSection {
    let title: String?
    let options: [String]
}

struct AurelineSelectionPicker: View {
    let title: String
    let sections: [AurelinePickerSection]
    let selected: String?
    let onSelect: (Int, Int) -> Void
    let onClose: () -> Void
    var bankNames: [String] = []
    var selectedBank = 0
    var onBankSelect: ((Int) -> Void)?

    var body: some View {
        ZStack {
            Color.black.opacity(0.72).ignoresSafeArea().onTapGesture(perform: onClose)
            VStack(spacing: 6) {
                HStack {
                    Text(title).font(.system(size: 15, weight: .heavy, design: .monospaced))
                    Spacer()
                    Button("CLOSE", action: onClose).buttonStyle(.plain)
                }.foregroundStyle(AurelineTheme.text).padding(.horizontal, 10).frame(height: 28)
                if !bankNames.isEmpty {
                    HStack(spacing: 4) {
                        ForEach(bankNames.indices, id: \.self) { index in
                            Button("B\(index + 1)") { onBankSelect?(index) }
                                .buttonStyle(AurelineBankTabStyle(active: index == selectedBank))
                        }
                    }
                    .padding(.horizontal, 6)
                    .frame(height: 25)
                }
                ScrollViewReader { proxy in
                    List {
                        ForEach(Array(sections.enumerated()), id: \.offset) { sectionIndex, section in
                            if !section.options.isEmpty {
                                Section {
                                    ForEach(Array(section.options.enumerated()), id: \.offset) { optionIndex, option in
                                        optionButton(option) { onSelect(sectionIndex, optionIndex) }
                                            .id(option)
                                    }
                                } header: {
                                    if let title = section.title { pickerSection(title) }
                                }
                            }
                        }
                    }
                    .listStyle(.plain)
                    .scrollContentBackground(.hidden)
                    .environment(\.defaultMinListRowHeight, 27)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .onAppear {
                        if let selected { proxy.scrollTo(selected, anchor: .center) }
                    }
                }
            }
            .frame(width: 330, height: 300)
            .background(AurelineMetalPanel())
            .clipShape(RoundedRectangle(cornerRadius: 5))
            .overlay(RoundedRectangle(cornerRadius: 5).stroke(AurelineTheme.gold.opacity(0.7)))
        }
    }

    private func pickerSection(_ title: String) -> some View {
        Text(title).font(.system(size: 9, weight: .bold)).foregroundStyle(AurelineTheme.gold)
            .frame(maxWidth: .infinity, alignment: .leading)
            .textCase(nil)
            .listRowInsets(EdgeInsets(top: 4, leading: 6, bottom: 2, trailing: 6))
    }

    private func optionButton(_ option: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            Text(option).font(.system(size: 14, weight: .bold, design: .monospaced))
                .foregroundStyle(selected == option ? .black : AurelineTheme.text)
                .frame(maxWidth: .infinity, alignment: .leading).padding(.horizontal, 8).frame(height: 25)
                .background(selected == option ? AurelineTheme.amber : AurelineTheme.panelLight)
                .clipShape(RoundedRectangle(cornerRadius: 3))
        }
        .buttonStyle(.plain)
        .listRowInsets(EdgeInsets(top: 1, leading: 6, bottom: 1, trailing: 6))
        .listRowSeparator(.hidden)
        .listRowBackground(Color.clear)
    }
}

private struct AurelineBankTabStyle: ButtonStyle {
    let active: Bool

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 12, weight: .bold, design: .monospaced))
            .foregroundStyle(active ? Color.black : AurelineTheme.text)
            .frame(maxWidth: .infinity, minHeight: 28)
            .background(active ? AurelineTheme.amber : AurelineTheme.panelLight)
            .clipShape(RoundedRectangle(cornerRadius: 5))
            .opacity(configuration.isPressed ? 0.75 : 1)
    }
}

private struct AurelineWheelFader: View {
    let title: String; @Binding var value: Double; let range: ClosedRange<Double>; let resets: Bool
    var body: some View {
        VStack(spacing: 5) {
            GeometryReader { proxy in
                Canvas { context, size in drawWheel(context: &context, size: size) }
                    .contentShape(Rectangle()).gesture(DragGesture(minimumDistance: 0).onChanged { gesture in
                    let y = min(max(gesture.location.y, 12), proxy.size.height - 12)
                    let normalized = 1 - (y - 12) / max(1, proxy.size.height - 24)
                    value = range.lowerBound + normalized * (range.upperBound - range.lowerBound)
                }.onEnded { _ in if resets { value = 0 } })
            }
            Text(title).font(.system(size: 9, weight: .bold)).foregroundStyle(AurelineTheme.text)
                .lineLimit(1).minimumScaleFactor(0.72).frame(width: 64, height: 14)
        }
    }

    private func drawWheel(context: inout GraphicsContext, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let slotWidth = min(size.width, 42)
        let slot = CGRect(x: (size.width - slotWidth) / 2, y: 0, width: slotWidth, height: size.height).insetBy(dx: 1, dy: 0)
        context.fill(Path(roundedRect: slot.offsetBy(dx: 0, dy: 2), cornerRadius: 3), with: .color(.black.opacity(0.48)))
        context.fill(Path(roundedRect: slot, cornerRadius: 3), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x1c2628), Color(hexValue: 0x050708)]), startPoint: CGPoint(x: slot.midX, y: slot.minY), endPoint: CGPoint(x: slot.midX, y: slot.maxY)))
        context.stroke(Path(roundedRect: slot, cornerRadius: 3), with: .color(Color(hexValue: 0x020405)), lineWidth: 1.8)
        context.stroke(Path(roundedRect: slot.insetBy(dx: 2, dy: 2), cornerRadius: 2), with: .color(.white.opacity(0.08)), lineWidth: 1)

        let wheel = slot.insetBy(dx: 7, dy: 6)
        context.fill(Path(roundedRect: wheel, cornerRadius: 2), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x1e2a2d), Color(hexValue: 0x050607)]), startPoint: CGPoint(x: wheel.midX, y: wheel.minY), endPoint: CGPoint(x: wheel.midX, y: wheel.maxY)))
        context.fill(Path(CGRect(x: wheel.minX, y: wheel.minY, width: 3, height: wheel.height)), with: .color(Color(hexValue: 0x07090a)))
        context.fill(Path(CGRect(x: wheel.maxX - 3, y: wheel.minY, width: 3, height: wheel.height)), with: .color(Color(hexValue: 0x07090a)))

        let normalized = CGFloat((value - range.lowerBound) / max(0.0001, range.upperBound - range.lowerBound))
        let spacing: CGFloat = 3.25
        let phase = ((1 - normalized) * 42).truncatingRemainder(dividingBy: spacing)
        for index in -2..<(Int(wheel.height / spacing) + 4) {
            let y = wheel.minY + 2 + phase + CGFloat(index) * spacing
            guard y >= wheel.minY + 2, y <= wheel.maxY - 2 else { continue }
            let curve = 1 - abs((y - wheel.midY) / max(1, wheel.height * 0.5))
            let inset = 3 + (1 - curve) * 2.2
            let line = Path { $0.move(to: CGPoint(x: wheel.minX + inset, y: y)); $0.addLine(to: CGPoint(x: wheel.maxX - inset, y: y)) }
            let shadow = Path { $0.move(to: CGPoint(x: wheel.minX + inset, y: y + 1)); $0.addLine(to: CGPoint(x: wheel.maxX - inset, y: y + 1)) }
            context.stroke(line, with: .color(Color(hexValue: 0x9aa7aa).opacity(0.22 + curve * 0.28)), lineWidth: 1)
            context.stroke(shadow, with: .color(Color(hexValue: 0x010202).opacity(0.62)), lineWidth: 1)
        }
        let topGlow = CGRect(x: wheel.minX, y: wheel.minY, width: wheel.width, height: wheel.height * 0.32)
        context.fill(Path(roundedRect: topGlow, cornerRadius: 2), with: .linearGradient(Gradient(colors: [.white.opacity(0.12), .white.opacity(0)]), startPoint: CGPoint(x: topGlow.midX, y: topGlow.minY), endPoint: CGPoint(x: topGlow.midX, y: topGlow.maxY)))
        let bottom = CGRect(x: wheel.minX, y: wheel.minY + wheel.height * 0.48, width: wheel.width, height: wheel.height * 0.52)
        context.fill(Path(roundedRect: bottom, cornerRadius: 2), with: .linearGradient(Gradient(colors: [.black.opacity(0), .black.opacity(0.46)]), startPoint: CGPoint(x: bottom.midX, y: bottom.minY), endPoint: CGPoint(x: bottom.midX, y: bottom.maxY)))
        context.fill(Path(roundedRect: CGRect(x: wheel.minX, y: wheel.minY + wheel.height * 0.55, width: wheel.width, height: wheel.height * 0.45), cornerRadius: 2), with: .color(.black.opacity(0.22)))
        context.stroke(Path(roundedRect: wheel, cornerRadius: 2), with: .color(.black), lineWidth: 1.4)

        let valueY = min(max(wheel.minY + 4, wheel.maxY - wheel.height * normalized), wheel.maxY - 4)
        let indicator = CGRect(x: wheel.minX + 3, y: valueY - 1.5, width: wheel.width - 6, height: 3)
        context.fill(Path(roundedRect: indicator.insetBy(dx: -1, dy: -1.5), cornerRadius: 1), with: .color(AurelineTheme.amber.opacity(0.28)))
        context.fill(Path(roundedRect: indicator, cornerRadius: 1), with: .color(AurelineTheme.amber))
    }
}

private struct AurelineNumberedVolumeKnob: View {
    @Binding var value: Double
    @State private var dragStartValue: Double?

    var body: some View {
        VStack(spacing: 0) {
            Text("VOLUME")
                .font(.system(size: 9, weight: .bold))
                .foregroundStyle(Color(hexValue: 0xc7cac9))
                .frame(height: 14)
            GeometryReader { proxy in
                Canvas { context, size in drawKnob(context: &context, size: size) }
                    .contentShape(Rectangle())
                    .gesture(DragGesture(minimumDistance: 0)
                        .onChanged { gesture in
                            if dragStartValue == nil { dragStartValue = value }
                            value = min(1, max(0, (dragStartValue ?? value) - Double(gesture.translation.height / 130)))
                        }
                        .onEnded { _ in dragStartValue = nil })
                    .onTapGesture(count: 2) { value = 0.8 }
            }
        }
        .accessibilityElement(children: .ignore)
        .accessibilityLabel("VOLUME")
        .accessibilityValue("\(Int((value * 10).rounded()))")
    }

    private func drawKnob(context: inout GraphicsContext, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let radius = min(size.width * 0.39, size.height * 0.34)
        let center = CGPoint(x: size.width * 0.5, y: size.height * 0.49)
        let activeIndex = Int((min(1, max(0, value)) * 10).rounded())
        let startAngle = -CGFloat.pi * 0.75
        let arcRange = CGFloat.pi * 1.5

        func point(radius: CGFloat, angle: CGFloat) -> CGPoint {
            CGPoint(x: center.x + sin(angle) * radius, y: center.y - cos(angle) * radius)
        }

        for tick in 0...10 {
            let angle = startAngle + CGFloat(tick) / 10 * arcRange
            let active = tick <= activeIndex
            let tickPath = Path { path in
                path.move(to: point(radius: radius - 8, angle: angle))
                path.addLine(to: point(radius: radius - 4.5, angle: angle))
            }
            context.stroke(tickPath, with: .color(active ? Color(hexValue: 0xc7cac9) : Color(hexValue: 0x34383a).opacity(0.82)), lineWidth: active ? 1.5 : 1)
            let number = context.resolve(Text("\(tick)").font(.system(size: 8, weight: .bold)).foregroundColor(active ? Color(hexValue: 0xc7cac9) : Color(hexValue: 0x34383a).opacity(0.82)))
            context.draw(number, at: point(radius: radius + 2.5, angle: angle), anchor: .center)
        }

        let outerRadius = radius * 0.73
        let outer = CGRect(x: center.x - outerRadius, y: center.y - outerRadius, width: outerRadius * 2, height: outerRadius * 2)
        context.fill(Path(ellipseIn: outer.offsetBy(dx: 0, dy: 2)), with: .color(.black.opacity(0.34)))
        context.fill(Path(ellipseIn: outer), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x424039), Color(hexValue: 0x070706)]), startPoint: CGPoint(x: outer.minX, y: outer.minY), endPoint: CGPoint(x: outer.maxX, y: outer.maxY)))
        context.stroke(Path(ellipseIn: outer), with: .color(Color(hexValue: 0x050505)), lineWidth: 2)

        let inner = outer.insetBy(dx: outer.width * 0.16, dy: outer.height * 0.16)
        context.fill(Path(ellipseIn: inner), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x22221f), Color(hexValue: 0x0a0a09)]), startPoint: CGPoint(x: inner.minX, y: inner.minY), endPoint: CGPoint(x: inner.maxX, y: inner.maxY)))
        context.stroke(Path(ellipseIn: inner), with: .color(.black.opacity(0.75)), lineWidth: 1.25)

        let angle = startAngle + CGFloat(min(1, max(0, value))) * arcRange
        let pointerEnd = point(radius: radius * 0.50, angle: angle)
        let pointer = Path { path in path.move(to: center); path.addLine(to: pointerEnd) }
        context.stroke(pointer, with: .linearGradient(Gradient(colors: [Color(hexValue: 0x777d80), Color(hexValue: 0xf0f2f1)]), startPoint: center, endPoint: pointerEnd), style: StrokeStyle(lineWidth: 4, lineCap: .round))
    }
}

private struct AurelineNumberedTransposeKnob: View {
    @Binding var value: Double
    @State private var dragStartValue: Double?

    private var normalized: Double { (min(24, max(-24, value)) + 24) / 48 }

    var body: some View {
        VStack(spacing: 0) {
            Text("TRANSPOSE")
                .font(.system(size: 8, weight: .bold))
                .foregroundStyle(Color(hexValue: 0xc7cac9))
                .frame(height: 14)
            GeometryReader { _ in
                Canvas { context, size in drawKnob(context: &context, size: size) }
                    .contentShape(Rectangle())
                    .gesture(DragGesture(minimumDistance: 0)
                        .onChanged { gesture in
                            if dragStartValue == nil { dragStartValue = value }
                            let next = (dragStartValue ?? value) - Double(gesture.translation.height / 3.2)
                            value = min(24, max(-24, next.rounded()))
                        }
                        .onEnded { _ in dragStartValue = nil })
                    .onTapGesture(count: 2) { value = 0 }
            }
        }
        .accessibilityElement(children: .ignore)
        .accessibilityLabel("TRANSPOSE")
        .accessibilityValue("\(Int(value.rounded()))")
    }

    private func drawKnob(context: inout GraphicsContext, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let radius = min(size.width * 0.39, size.height * 0.34)
        let center = CGPoint(x: size.width * 0.5, y: size.height * 0.49)
        let activeIndex = Int((normalized * 8).rounded())
        let startAngle = -CGFloat.pi * 0.75
        let arcRange = CGFloat.pi * 1.5

        func point(radius: CGFloat, angle: CGFloat) -> CGPoint {
            CGPoint(x: center.x + sin(angle) * radius, y: center.y - cos(angle) * radius)
        }

        for tick in 0...8 {
            let angle = startAngle + CGFloat(tick) / 8 * arcRange
            let active = tick <= activeIndex
            let tickPath = Path { path in
                path.move(to: point(radius: radius - 8, angle: angle))
                path.addLine(to: point(radius: radius - 4.5, angle: angle))
            }
            let color = active ? Color(hexValue: 0xc7cac9) : Color(hexValue: 0x34383a).opacity(0.82)
            context.stroke(tickPath, with: .color(color), lineWidth: active ? 1.5 : 1)
            let label = -24 + tick * 6
            let number = context.resolve(Text("\(label)").font(.system(size: 6.5, weight: .bold)).foregroundColor(color))
            context.draw(number, at: point(radius: radius + 3, angle: angle), anchor: .center)
        }

        let outerRadius = radius * 0.73
        let outer = CGRect(x: center.x - outerRadius, y: center.y - outerRadius, width: outerRadius * 2, height: outerRadius * 2)
        context.fill(Path(ellipseIn: outer.offsetBy(dx: 0, dy: 2)), with: .color(.black.opacity(0.34)))
        context.fill(Path(ellipseIn: outer), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x424039), Color(hexValue: 0x070706)]), startPoint: CGPoint(x: outer.minX, y: outer.minY), endPoint: CGPoint(x: outer.maxX, y: outer.maxY)))
        context.stroke(Path(ellipseIn: outer), with: .color(Color(hexValue: 0x050505)), lineWidth: 2)
        let inner = outer.insetBy(dx: outer.width * 0.16, dy: outer.height * 0.16)
        context.fill(Path(ellipseIn: inner), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x22221f), Color(hexValue: 0x0a0a09)]), startPoint: CGPoint(x: inner.minX, y: inner.minY), endPoint: CGPoint(x: inner.maxX, y: inner.maxY)))
        context.stroke(Path(ellipseIn: inner), with: .color(.black.opacity(0.75)), lineWidth: 1.25)

        let angle = startAngle + CGFloat(normalized) * arcRange
        let pointerEnd = point(radius: radius * 0.50, angle: angle)
        let pointer = Path { path in path.move(to: center); path.addLine(to: pointerEnd) }
        context.stroke(pointer, with: .linearGradient(Gradient(colors: [Color(hexValue: 0x777d80), Color(hexValue: 0xf0f2f1)]), startPoint: center, endPoint: pointerEnd), style: StrokeStyle(lineWidth: 4, lineCap: .round))
    }
}

private struct AurelineVolumeFader: View {
    @Binding var value: Double
    var body: some View {
        VStack(spacing: 5) {
            GeometryReader { proxy in
                Canvas { context, size in drawFader(context: &context, size: size) }
                    .contentShape(Rectangle()).gesture(DragGesture(minimumDistance: 0).onChanged { gesture in
                        let y = min(max(gesture.location.y, 23), proxy.size.height - 23)
                        value = Double(1 - (y - 23) / max(1, proxy.size.height - 46))
                    })
            }
            Text("VOLUME").font(.system(size: 9, weight: .bold)).foregroundStyle(AurelineTheme.text).frame(width: 54, height: 14)
        }
    }

    private func drawFader(context: inout GraphicsContext, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let panel = CGRect(x: (size.width - min(size.width, 44)) / 2, y: 0, width: min(size.width, 44), height: size.height).insetBy(dx: 1, dy: 0)
        context.fill(Path(roundedRect: panel.offsetBy(dx: 0, dy: 2), cornerRadius: 1.5), with: .color(.black.opacity(0.5)))
        context.fill(Path(roundedRect: panel, cornerRadius: 1.5), with: .linearGradient(Gradient(colors: [Color(hexValue: 0x211f1a), Color(hexValue: 0x0a0907)]), startPoint: CGPoint(x: panel.midX, y: panel.minY), endPoint: CGPoint(x: panel.midX, y: panel.maxY)))
        context.stroke(Path(roundedRect: panel, cornerRadius: 1.5), with: .color(.black.opacity(0.95)), lineWidth: 1.6)
        context.stroke(Path(roundedRect: panel.insetBy(dx: 2, dy: 2), cornerRadius: 1), with: .color(Color(hexValue: 0x4a463c)), lineWidth: 1)
        let maxText = context.resolve(Text("MAX").font(.system(size: 8, weight: .heavy)).foregroundColor(.white))
        let minText = context.resolve(Text("MIN").font(.system(size: 8, weight: .heavy)).foregroundColor(.white))
        context.draw(maxText, at: CGPoint(x: panel.maxX - 12, y: panel.minY + 9), anchor: .center)
        context.draw(minText, at: CGPoint(x: panel.maxX - 12, y: panel.maxY - 9), anchor: .center)
        let track = CGRect(x: panel.minX + 14, y: panel.minY + 23, width: 8, height: max(1, panel.height - 46))
        context.fill(Path(roundedRect: track, cornerRadius: 3), with: .color(Color(hexValue: 0x050504)))
        context.stroke(Path(roundedRect: track, cornerRadius: 3), with: .color(Color(hexValue: 0x161b1d)), lineWidth: 1)
        for index in 0...8 {
            let ratio = CGFloat(index) / 8
            let y = track.maxY - ratio * track.height
            let length: CGFloat = index % 4 == 0 ? 9 : 6
            let tick = Path { $0.move(to: CGPoint(x: panel.minX + 27, y: y)); $0.addLine(to: CGPoint(x: panel.minX + 27 + length, y: y)) }
            context.stroke(tick, with: .color(Color(hexValue: 0x77776f)), lineWidth: 1)
        }
        let y = track.maxY - CGFloat(max(0, min(1, value))) * track.height
        let handle = CGRect(x: panel.minX + 4, y: y - 5, width: 31, height: 10)
        context.fill(Path(roundedRect: handle.offsetBy(dx: 0, dy: 1.5), cornerRadius: 1.2), with: .color(.black.opacity(0.52)))
        context.fill(Path(roundedRect: handle, cornerRadius: 1.2), with: .linearGradient(Gradient(colors: [Color(hexValue: 0xd7dfde), Color(hexValue: 0x657071)]), startPoint: CGPoint(x: handle.midX, y: handle.minY), endPoint: CGPoint(x: handle.midX, y: handle.maxY)))
        context.stroke(Path(roundedRect: handle, cornerRadius: 1.2), with: .color(Color(hexValue: 0x273033)), lineWidth: 1)
        context.stroke(Path(CGRect(x: handle.minX + 2, y: handle.midY, width: handle.width - 4, height: 1)), with: .color(.white.opacity(0.55)), lineWidth: 1)
    }
}

private struct AurelineLCD: View {
    let lines: [String]
    var body: some View {
        Canvas { context, size in drawLCD(context: &context, size: size) }
            .background(LinearGradient(colors: [Color(hexValue: 0x28170d), Color(hexValue: 0x0b0704)], startPoint: .top, endPoint: .bottom))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color(hexValue: 0x594235)))
    }

    private func drawLCD(context: inout GraphicsContext, size: CGSize) {
        let characterCount = 16
        let displayLines = (0..<2).map { index -> String in
            let source = index < lines.count ? lines[index].uppercased() : ""
            return String(source.prefix(characterCount)).padding(toLength: characterCount, withPad: " ", startingAt: 0)
        }

        let content = CGRect(origin: .zero, size: size).insetBy(dx: 5, dy: 4)
        let rowsPerCharacter = 8
        let lineGap = max(1, content.height * 0.035)
        let dotPitchY = (content.height - lineGap) / CGFloat(rowsPerCharacter * 2)
        let lineHeight = dotPitchY * CGFloat(rowsPerCharacter)
        let verticalOffset = max(1, dotPitchY)
        let firstY = content.midY - lineHeight - lineGap / 2 + verticalOffset
        let secondY = content.midY + lineGap / 2 + verticalOffset

        drawLine(displayLines[0], context: &context,
                 rect: CGRect(x: content.minX, y: firstY, width: content.width, height: lineHeight))
        drawLine(displayLines[1], context: &context,
                 rect: CGRect(x: content.minX, y: secondY, width: content.width, height: lineHeight))

        let highlight = CGRect(x: 4, y: 4, width: max(0, size.width - 8), height: 11)
        context.fill(Path(roundedRect: highlight, cornerRadius: 3), with: .color(.white.opacity(0.10)))
        let bottomShade = CGRect(x: 2, y: max(0, size.height - 10), width: max(0, size.width - 4), height: 8)
        context.fill(Path(roundedRect: bottomShade, cornerRadius: 2), with: .color(.black.opacity(0.10)))
    }

    private func drawLine(_ text: String, context: inout GraphicsContext, rect: CGRect) {
        let characters = Array(text.prefix(16))
        let cellColumns = max(1, characters.count * 6 - 1)
        let pitchX = rect.width / CGFloat(cellColumns)
        let pitchY = rect.height / 8
        let pitch = max(1, min(pitchX, pitchY))
        let dotSize = max(1, floor(pitch * 0.94))
        let textWidth = pitch * CGFloat(cellColumns)
        let origin = CGPoint(x: rect.midX - textWidth / 2, y: rect.minY)

        for (characterIndex, character) in characters.enumerated() {
            let glyph = Self.glyph(character)
            let xBase = origin.x + CGFloat(characterIndex * 6) * pitch
            for column in 0..<5 {
                for row in 0..<8 {
                    let enabled = glyph[column] & (1 << row) != 0
                    let dot = CGRect(x: xBase + CGFloat(column) * pitch,
                                     y: origin.y + CGFloat(row) * pitch,
                                     width: dotSize, height: dotSize)
                    context.fill(Path(dot), with: .color(enabled
                        ? Color(hexValue: 0xffa04a).opacity(0.96)
                        : Color(hexValue: 0x603119).opacity(0.34)))
                }
            }
        }
    }

    private static func glyph(_ character: Character) -> [UInt8] {
        let glyphs: [Character: [UInt8]] = [
            "0":[0x3e,0x51,0x49,0x45,0x3e], "1":[0,0x42,0x7f,0x40,0], "2":[0x42,0x61,0x51,0x49,0x46], "3":[0x21,0x41,0x45,0x4b,0x31],
            "4":[0x18,0x14,0x12,0x7f,0x10], "5":[0x27,0x45,0x45,0x45,0x39], "6":[0x3c,0x4a,0x49,0x49,0x30], "7":[1,0x71,9,5,3],
            "8":[0x36,0x49,0x49,0x49,0x36], "9":[6,0x49,0x49,0x29,0x1e], "A":[0x7e,0x11,0x11,0x11,0x7e], "B":[0x7f,0x49,0x49,0x49,0x36],
            "C":[0x3e,0x41,0x41,0x41,0x22], "D":[0x7f,0x41,0x41,0x22,0x1c], "E":[0x7f,0x49,0x49,0x49,0x41], "F":[0x7f,9,9,9,1],
            "G":[0x3e,0x41,0x49,0x49,0x7a], "H":[0x7f,8,8,8,0x7f], "I":[0,0x41,0x7f,0x41,0], "J":[0x20,0x40,0x41,0x3f,1],
            "K":[0x7f,8,0x14,0x22,0x41], "L":[0x7f,0x40,0x40,0x40,0x40], "M":[0x7f,2,0x0c,2,0x7f], "N":[0x7f,4,8,0x10,0x7f],
            "O":[0x3e,0x41,0x41,0x41,0x3e], "P":[0x7f,9,9,9,6], "Q":[0x3e,0x41,0x51,0x21,0x5e], "R":[0x7f,9,0x19,0x29,0x46],
            "S":[0x46,0x49,0x49,0x49,0x31], "T":[1,1,0x7f,1,1], "U":[0x3f,0x40,0x40,0x40,0x3f], "V":[0x1f,0x20,0x40,0x20,0x1f],
            "W":[0x3f,0x40,0x38,0x40,0x3f], "X":[0x63,0x14,8,0x14,0x63], "Y":[7,8,0x70,8,7], "Z":[0x61,0x51,0x49,0x45,0x43],
            "-":[8,8,8,8,8]
        ]
        return glyphs[character] ?? [0, 0, 0, 0, 0]
    }
}

struct AurelineWaveform: View {
    enum Layout {
        case play
        case edit
    }
    let layout: Layout
    let samplesCombined: [Float]
    let samplesA: [Float]
    let samplesB: [Float]
    let cyclesCombined: Double
    let cyclesA: Double
    let cyclesB: Double
    var body: some View {
        Group {
            if layout == .play {
                VStack(spacing: 4) {
                    combinedPanel
                    HStack(spacing: 4) {
                        oscillatorAPanel
                        oscillatorBPanel
                    }
                }
            } else {
                HStack(spacing: 4) {
                    VStack(spacing: 4) {
                        oscillatorAPanel
                        oscillatorBPanel
                    }
                    combinedPanel
                }
            }
        }
    }

    private var combinedPanel: some View {
        AurelineOscillatorWaveformPanel(
            title: "FINAL MIX",
            samples: samplesCombined,
            cycles: cyclesCombined)
    }

    private var oscillatorAPanel: some View {
        AurelineOscillatorWaveformPanel(
            title: "OSC A", samples: samplesA, cycles: cyclesA)
    }

    private var oscillatorBPanel: some View {
        AurelineOscillatorWaveformPanel(
            title: "OSC B", samples: samplesB, cycles: cyclesB)
    }
}

private struct AurelineOscillatorWaveformPanel: View {
    private static let displayGain: Float = 2
    let title: String
    let samples: [Float]
    let cycles: Double

    var body: some View {
        Canvas { context, size in
            let scope = CGRect(x: 4, y: 3, width: max(1, size.width - 8),
                               height: max(1, size.height - 6))
            guard samples.count > 1 else { return }
            let pointCount = max(256, samples.count * 2)
            let startPhase = 0.5 - cycles / 2
            let waveformPath: (ClosedRange<Int>) -> Path = { range in
                var path = Path()
                var lastY: CGFloat?
                for point in range {
                    let phase = startPhase
                        + Double(point) / Double(pointCount) * cycles
                    let wrappedPhase = phase - floor(phase)
                    let sourceIndex = min(samples.count - 1,
                                          Int(wrappedPhase * Double(samples.count)))
                    let x = scope.minX + scope.width
                        * CGFloat(point) / CGFloat(pointCount)
                    let sample = CGFloat(max(
                        -1, min(1, samples[sourceIndex] * Self.displayGain)))
                    let y = scope.midY - sample * scope.height * 0.47
                    if point == range.lowerBound {
                        path.move(to: CGPoint(x: x, y: y))
                    } else {
                        if let previousY = lastY,
                           abs(y - previousY) > scope.height * 0.18 {
                            path.addLine(to: CGPoint(x: x, y: previousY))
                        }
                        path.addLine(to: CGPoint(x: x, y: y))
                    }
                    lastY = y
                }
                return path
            }
            let halfCentreCycle = min(0.5, 0.5 / max(0.0001, cycles))
            let centreStart = max(0, min(pointCount,
                Int((0.5 - halfCentreCycle) * Double(pointCount))))
            let centreEnd = max(centreStart, min(pointCount,
                Int((0.5 + halfCentreCycle) * Double(pointCount))))
            if centreStart > 0 {
                context.stroke(waveformPath(0...centreStart),
                               with: .color(Color(hexValue: 0xaeb2b0).opacity(0.78)),
                               style: StrokeStyle(lineWidth: 1))
            }
            if centreEnd < pointCount {
                context.stroke(waveformPath(centreEnd...pointCount),
                               with: .color(Color(hexValue: 0xaeb2b0).opacity(0.78)),
                               style: StrokeStyle(lineWidth: 1))
            }
            context.stroke(waveformPath(centreStart...centreEnd),
                           with: .color(Color(hexValue: 0xff9a42)),
                           style: StrokeStyle(lineWidth: 1.3))
        }
        .background(LinearGradient(colors: [Color(hexValue: 0x101719), Color(hexValue: 0x030708)], startPoint: .top, endPoint: .bottom))
        .clipShape(RoundedRectangle(cornerRadius: 4))
        .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color(hexValue: 0x51443c), lineWidth: 1.2))
        .overlay(alignment: .topLeading) {
            ZStack(alignment: .topLeading) {
                Color(hexValue: 0x101719)
                    .frame(height: 3)
                    .offset(y: -1.5)
                Text(title)
                    .font(.system(size: 7, weight: .bold))
                    .foregroundColor(Color(hexValue: 0xc7c9c8))
                    .padding(.horizontal, 4)
                    .frame(height: 10)
                    .offset(y: -5)
            }
            .fixedSize(horizontal: true, vertical: false)
            .offset(x: 7)
        }
    }
}
