import SwiftUI

struct AboutView: View {
    @Environment(\.dismiss) private var dismiss

    private var version: String {
        let short = Bundle.main.object(
            forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "1.0.2"
        let build = Bundle.main.object(
            forInfoDictionaryKey: "CFBundleVersion") as? String ?? "3"
        return "\(short) (\(build))"
    }

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(alignment: .leading, spacing: 14) {
                    Text("AURELINE")
                        .font(.system(size: 28, weight: .black, design: .rounded))
                        .foregroundStyle(AurelineTheme.amber)
                    Text("8-VOICE ANALOG MODELING SYNTHESIZER")
                        .font(.system(size: 11, weight: .bold))
                        .foregroundStyle(AurelineTheme.gold)
                    Text("Version \(version)")
                        .font(.system(size: 12, design: .monospaced))

                    section("AUv3 INSTRUMENT") {
                        Text("Aureline includes an AUv3 Instrument for compatible hosts such as GarageBand.")
                        Text("GarageBand: create or open a song, add an Audio Unit Extension instrument, select Aureline, then play it from the host keyboard or a MIDI controller.")
                    }

                    section("PRIVACY") {
                        Text("Aureline does not collect, track, sell, or transmit personal data. Voice and library files remain on your device unless you choose to export them.")
                        Link("Privacy Policy",
                             destination: URL(string: "https://hidecade.github.io/Aureline-Support/privacy/")!)
                    }

                    section("SUPPORT") {
                        Link("Aureline Support",
                             destination: URL(string: "https://hidecade.github.io/Aureline-Support/support/")!)
                    }

                    Text("© 2026 Hideki Konishi / Hidecade Instruments")
                        .font(.system(size: 11))
                        .foregroundStyle(AurelineTheme.gold)
                }
                .frame(maxWidth: 620, alignment: .leading)
                .padding(24)
            }
            .background(AurelineTheme.background.ignoresSafeArea())
            .foregroundStyle(AurelineTheme.text)
            .navigationTitle("ABOUT")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("CLOSE") { dismiss() }
                }
            }
        }
        .preferredColorScheme(.dark)
    }

    private func section<Content: View>(
        _ title: String, @ViewBuilder content: () -> Content) -> some View {
        VStack(alignment: .leading, spacing: 7) {
            Text(title)
                .font(.system(size: 13, weight: .bold))
                .foregroundStyle(AurelineTheme.amber)
            content()
                .font(.system(size: 12))
        }
        .padding(12)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(AurelineTheme.panel)
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }
}
