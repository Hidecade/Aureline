import SwiftUI

struct RootView: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @State private var editKeyboardOctave = 3
    @State private var normalEditKeyboardOctave = 3
    @State private var showingAbout = false

    var body: some View {
        if synth.screen == .play {
            PlayView().ignoresSafeArea(.container, edges: .horizontal)
        } else {
          VStack(spacing: 0) {
            header
            Group {
                switch synth.screen {
                case .play: EmptyView()
                case .edit: EditView()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .zIndex(synth.screen == .edit ? 10 : 0)
            if synth.screen != .edit {
                performanceStrip
            }
            if synth.screen == .edit {
                editKeyboardAndWaveform.frame(height: 108)
            } else {
                MobileKeyboardView().frame(height: 116)
            }
        }
        .padding(.horizontal, 8)
        .background(AurelineTheme.background.ignoresSafeArea())
        .preferredColorScheme(.dark)
        .onAppear {
            if Int(synth.value("voiceMode").rounded()) == 3 {
                editKeyboardOctave = 2
            }
        }
        .onChange(of: Int(synth.value("voiceMode").rounded())) { mode in
            synth.panic()
            if mode == 3 {
                normalEditKeyboardOctave = editKeyboardOctave
                editKeyboardOctave = 2
            } else {
                editKeyboardOctave = normalEditKeyboardOctave
            }
        }
        .sheet(isPresented: $showingAbout) {
            AboutView()
        }
        }
    }

    private var header: some View {
        HStack(spacing: 12) {
            VStack(alignment: .leading, spacing: 0) {
                Text("AURELINE").font(.system(size: 22, weight: .black, design: .rounded)).foregroundStyle(AurelineTheme.amber)
                Text("8-VOICE ANALOG MODELING SYNTHESIZER").font(.system(size: 7, weight: .bold)).foregroundStyle(AurelineTheme.gold)
            }
            Text(synth.selectedPreset).font(.system(size: 12, weight: .semibold, design: .monospaced)).foregroundStyle(AurelineTheme.text)
            Spacer()
            if synth.screen == .edit {
                Button("ABOUT") { showingAbout = true }
                    .buttonStyle(HeaderVoiceButtonStyle(store: false))
                Button("INIT") { synth.initializePatch() }
                    .buttonStyle(HeaderVoiceButtonStyle(store: false))
                Button("STORE") {
                    do { try synth.storeCurrentVoice() }
                    catch { synth.status = "Store error: \(error.localizedDescription)" }
                }
                .buttonStyle(HeaderVoiceButtonStyle(store: true))
            }
            ForEach(MobileSynthModel.Screen.allCases, id: \.self) { screen in
                Button(screen.rawValue) { synth.screen = screen }
                    .buttonStyle(NavButtonStyle(selected: synth.screen == screen))
            }
        }
        .frame(height: 46)
    }

    private var performanceStrip: some View {
        HStack(spacing: 8) {
            Button("OCT −") { synth.octave = max(-2, synth.octave - 1); synth.panic() }
            Text("OCT \(synth.octave >= 0 ? "+" : "")\(synth.octave)").font(.system(size: 10, weight: .bold, design: .monospaced)).frame(width: 50)
            Button("OCT +") { synth.octave = min(2, synth.octave + 1); synth.panic() }
            Spacer()
            Text(synth.status).font(.system(size: 9)).foregroundStyle(AurelineTheme.gold).lineLimit(1)
        }
        .buttonStyle(NavButtonStyle(selected: false))
        .frame(height: 34)
    }

    private var editKeyboardAndWaveform: some View {
        GeometryReader { proxy in
            let leftWidth = max(240, proxy.size.width * 0.5)
            HStack(spacing: 8) {
                HStack(spacing: 5) {
                    VStack(spacing: 5) {
                        Button("OCT+") { shiftEditOctave(1) }
                            .buttonStyle(EditOctaveButtonStyle())
                        Button("OCT−") { shiftEditOctave(-1) }
                            .buttonStyle(EditOctaveButtonStyle())
                    }
                    .frame(width: 45)

                    OpalineStyleKeyboardView(
                        visibleWhiteKeyCount: 8,
                        scrollWhiteIndex: CGFloat(2 + (editKeyboardOctave - 1) * 7)
                    )
                    .environmentObject(synth)
                }
                .frame(width: leftWidth)

                AurelineWaveform(
                    layout: .edit,
                    samplesCombined: synth.scopeSamples,
                    samplesA: synth.synthesizedOscillatorWaveformSamples(
                        oscillatorA: true),
                    samplesB: synth.synthesizedOscillatorWaveformSamples(
                        oscillatorA: false),
                    cyclesCombined: 1,
                    cyclesA: synth.displayedWaveformCycles(oscillatorA: true),
                    cyclesB: synth.displayedWaveformCycles(oscillatorA: false))
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }

    private func shiftEditOctave(_ delta: Int) {
        editKeyboardOctave = min(7, max(1, editKeyboardOctave + delta))
        synth.panic()
    }
}

private struct EditOctaveButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 9, weight: .bold))
            .foregroundStyle(Color(hexValue: 0xd7dcda))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(LinearGradient(colors: [Color(hexValue: 0x3b3f41), Color(hexValue: 0x111315)], startPoint: .top, endPoint: .bottom))
            .clipShape(RoundedRectangle(cornerRadius: 3))
            .overlay(RoundedRectangle(cornerRadius: 3).stroke(Color.black.opacity(0.8), lineWidth: 1.2))
            .opacity(configuration.isPressed ? 0.7 : 1)
    }
}

private struct HeaderVoiceButtonStyle: ButtonStyle {
    let store: Bool
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 9, weight: .bold))
            .foregroundStyle(configuration.isPressed ? Color.white : Color(hexValue: 0xffad55))
            .frame(width: 48, height: 28)
            .background(LinearGradient(colors: store
                ? [Color(hexValue: 0xa13b1d), Color(hexValue: 0x6f2412)]
                : [Color(hexValue: 0x30231a), Color(hexValue: 0x17110d)], startPoint: .top, endPoint: .bottom))
            .clipShape(RoundedRectangle(cornerRadius: 3))
            .overlay(RoundedRectangle(cornerRadius: 3).stroke(Color.black.opacity(0.72), lineWidth: 1))
            .opacity(configuration.isPressed ? 0.72 : 1)
    }
}

struct NavButtonStyle: ButtonStyle {
    let selected: Bool
    func makeBody(configuration: Configuration) -> some View {
        configuration.label.font(.system(size: 10, weight: .bold)).padding(.horizontal, 10).padding(.vertical, 6)
            .foregroundStyle(selected ? Color.black : AurelineTheme.text)
            .background(selected ? AurelineTheme.amber : AurelineTheme.panelLight)
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .opacity(configuration.isPressed ? 0.7 : 1)
    }
}
