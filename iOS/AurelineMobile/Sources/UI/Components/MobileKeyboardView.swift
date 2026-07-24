import SwiftUI

struct MobileKeyboardView: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @State private var held = Set<Int>()
    private let firstNote = 48
    private let whiteNotes = [0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24]
    private let blackNotes = [(1, 0.68), (3, 1.68), (6, 3.68), (8, 4.68), (10, 5.68), (13, 7.68), (15, 8.68), (18, 10.68), (20, 11.68), (22, 12.68)]

    var body: some View {
        GeometryReader { proxy in
            let width = proxy.size.width / CGFloat(whiteNotes.count)
            ZStack(alignment: .topLeading) {
                HStack(spacing: 1) {
                    ForEach(whiteNotes, id: \.self) { offset in key(note: firstNote + offset, black: false) }
                }
                ForEach(Array(blackNotes.enumerated()), id: \.offset) { _, item in
                    key(note: firstNote + item.0, black: true)
                        .frame(width: width * 0.62, height: proxy.size.height * 0.62)
                        .offset(x: width * item.1)
                }
            }
        }
    }

    private func key(note: Int, black: Bool) -> some View {
        let isHeld = held.contains(note)
        return RoundedRectangle(cornerRadius: 2)
            .fill(isHeld ? AurelineTheme.amber : (black ? Color.black : Color(white: 0.92)))
            .overlay(RoundedRectangle(cornerRadius: 2).stroke(.black.opacity(0.65), lineWidth: 1))
            .contentShape(Rectangle())
            .gesture(DragGesture(minimumDistance: 0)
                .onChanged { _ in if !held.contains(note) { held.insert(note); synth.noteOn(note) } }
                .onEnded { _ in held.remove(note); synth.noteOff(note) })
            .accessibilityLabel("MIDI note \(note)")
    }
}
