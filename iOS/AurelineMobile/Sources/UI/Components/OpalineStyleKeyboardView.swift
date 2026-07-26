import SwiftUI
import UIKit

private let aurelineWhiteNotes = (21...108).filter { !aurelineIsBlack($0) }
private let aurelineBlackNotes = (21...108).filter { aurelineIsBlack($0) }

private func aurelineIsBlack(_ note: Int) -> Bool { [1, 3, 6, 8, 10].contains(note % 12) }

struct OpalineStyleKeyboardView: View {
    @EnvironmentObject private var synth: MobileSynthModel
    @State private var touchedNotes: [Int: Int] = [:]
    let visibleWhiteKeyCount: Int
    let scrollWhiteIndex: CGFloat

    var body: some View {
        ZStack {
            Canvas { context, size in draw(context: &context, size: size) }
            AurelineKeyboardTouchSurface(synth: synth, visibleWhiteKeyCount: visibleWhiteKeyCount,
                scrollWhiteIndex: scrollWhiteIndex, activeNotes: $touchedNotes)
        }.accessibilityLabel("Aureline keyboard")
    }

    private var activeNotes: [Int: Int] { touchedNotes.merging(synth.externalActiveNotes) { max($0, $1) } }

    private func draw(context: inout GraphicsContext, size: CGSize) {
        guard size.width > 0, size.height > 0 else { return }
        let displayedNotes = activeNotes
        let baseArea = CGRect(origin: .zero, size: size)
        context.fill(Path(baseArea), with: .linearGradient(
            Gradient(colors: [Color(hexValue: 0x1b1a16), Color(hexValue: 0x050505)]),
            startPoint: CGPoint(x: baseArea.midX, y: baseArea.minY),
            endPoint: CGPoint(x: baseArea.midX, y: baseArea.maxY)))

        let keyArea = CGRect(x: 0, y: 0, width: max(1, size.width), height: max(1, size.height - 3))
        let whiteWidth = size.width / CGFloat(visibleWhiteKeyCount)
        let scrollX = -scrollWhiteIndex * whiteWidth
        let firstVisible = max(0, Int(floor(scrollWhiteIndex)) - 1)
        let lastVisible = min(aurelineWhiteNotes.count - 1, Int(ceil(scrollWhiteIndex)) + visibleWhiteKeyCount + 1)

        for index in firstVisible...lastVisible {
            let note = aurelineWhiteNotes[index]
            let velocity = displayedNotes[note] ?? 0
            let held = velocity > 0
            let rect = CGRect(x: scrollX + CGFloat(index) * whiteWidth + 0.45, y: 0,
                width: max(1, whiteWidth - 0.9), height: max(1, keyArea.height - 2))
            context.fill(Path(rect), with: .linearGradient(
                Gradient(colors: held
                    ? [Color(hexValue: 0xffffd47a), Color(hexValue: 0xd9992d)]
                    : [Color(hexValue: 0xf4eee1), Color(hexValue: 0xd8cdb7)]),
                startPoint: CGPoint(x: rect.midX, y: rect.minY), endPoint: CGPoint(x: rect.midX, y: rect.maxY)))
            context.stroke(Path(roundedRect: rect, cornerRadius: 1.7), with: .color(Color(hexValue: 0x5a5143).opacity(0.74)), lineWidth: 1)
            context.fill(Path(CGRect(x: rect.minX + 1.2, y: rect.minY, width: max(1, rect.width - 2.4), height: 5)), with: .color(Color.white.opacity(held ? 0.18 : 0.25)))
            context.fill(Path(CGRect(x: rect.minX, y: rect.maxY - 4, width: rect.width, height: 4)), with: .color(Color.black.opacity(0.20)))
            if held { drawVelocity(velocity, in: rect, context: &context, blackKey: false) }
            if note % 12 == 0 {
                drawCLabel(note: note, in: rect, context: &context, held: held)
            }
        }

        let blackWidth = whiteWidth * 0.58
        for note in aurelineBlackNotes {
            guard let before = aurelineWhiteNotes.firstIndex(where: { $0 >= note }) else { continue }
            guard before >= firstVisible - 1, before <= lastVisible + 1 else { continue }
            let velocity = displayedNotes[note] ?? 0
            let held = velocity > 0
            let x = scrollX + CGFloat(before) * whiteWidth - blackWidth * 0.5
            let rect = CGRect(x: x, y: -1, width: blackWidth, height: keyArea.height * 0.62).insetBy(dx: 1.2, dy: 0)
            let path = Path(roundedRect: rect, cornerRadius: 1)
            context.fill(Path(roundedRect: rect.offsetBy(dx: 0, dy: 3), cornerRadius: 1), with: .color(Color.black.opacity(0.28)))
            context.fill(path, with: .linearGradient(
                Gradient(colors: held
                    ? [Color(hexValue: 0x78551d), Color(hexValue: 0xe9782d)]
                    : [Color(hexValue: 0x20201c), Color(hexValue: 0x050504)]),
                startPoint: CGPoint(x: rect.midX, y: rect.minY), endPoint: CGPoint(x: rect.midX, y: rect.maxY)))
            let highlight = CGRect(x: rect.minX + 3, y: rect.minY + 3, width: max(1, rect.width - 6), height: rect.height * 0.18)
            context.fill(Path(roundedRect: highlight, cornerRadius: 1), with: .color(Color.white.opacity(held ? 0.16 : 0.08)))
            context.fill(Path(CGRect(x: rect.minX, y: rect.maxY - 5, width: rect.width, height: 5)), with: .color(Color.black.opacity(0.35)))
            context.stroke(path, with: .color(Color.black.opacity(0.95)), lineWidth: 1.2)
            if held { drawVelocity(velocity, in: rect, context: &context, blackKey: true) }
        }

        var outline = Path()
        outline.move(to: CGPoint(x: 0, y: 0)); outline.addLine(to: CGPoint(x: 0, y: size.height))
        outline.addLine(to: CGPoint(x: size.width, y: size.height)); outline.addLine(to: CGPoint(x: size.width, y: 0))
        context.stroke(outline, with: .color(Color.black.opacity(0.76)), lineWidth: 2)
    }

    private func drawVelocity(_ velocity: Int, in rect: CGRect, context: inout GraphicsContext, blackKey: Bool) {
        let width = blackKey ? CGFloat(22) : min(CGFloat(34), max(18, rect.width - 8))
        let height = blackKey ? CGFloat(17) : CGFloat(18)
        let box = CGRect(x: rect.midX - width / 2, y: rect.maxY - (blackKey ? 24 : 28), width: width, height: height)
        context.fill(Path(roundedRect: box, cornerRadius: 2), with: .color(Color(hexValue: 0x050606).opacity(blackKey ? 0.84 : 0.82)))
        let text = context.resolve(Text("\(velocity)").font(.system(size: blackKey && velocity >= 100 ? 7.5 : 10.5, weight: .bold)).foregroundColor(Color(hexValue: 0xffffd52b)))
        context.draw(text, at: CGPoint(x: box.midX, y: box.midY), anchor: .center)
    }

    private func drawCLabel(note: Int, in rect: CGRect, context: inout GraphicsContext, held: Bool) {
        let text = context.resolve(Text("C\(note / 12 - 1)")
            .font(.system(size: min(13, max(8, rect.width * 0.28)), weight: .bold, design: .monospaced))
            .foregroundColor(held ? Color(hexValue: 0x3a2408).opacity(0.82) : Color(hexValue: 0x2c2922).opacity(0.72)))
        context.draw(text, at: CGPoint(x: rect.midX, y: rect.maxY - max(14, rect.height * 0.11)), anchor: .center)
    }
}

private struct AurelineKeyboardTouchSurface: UIViewRepresentable {
    let synth: MobileSynthModel
    let visibleWhiteKeyCount: Int
    let scrollWhiteIndex: CGFloat
    @Binding var activeNotes: [Int: Int]

    func makeUIView(context: Context) -> AurelineKeyboardTouchView { makeOrUpdate(AurelineKeyboardTouchView()) }
    func updateUIView(_ view: AurelineKeyboardTouchView, context: Context) { _ = makeOrUpdate(view) }
    private func makeOrUpdate(_ view: AurelineKeyboardTouchView) -> AurelineKeyboardTouchView {
        view.synth = synth; view.visibleWhiteKeyCount = visibleWhiteKeyCount; view.scrollWhiteIndex = scrollWhiteIndex
        view.onChange = { activeNotes = $0 }; return view
    }
}

private final class AurelineKeyboardTouchView: UIView {
    weak var synth: MobileSynthModel?
    var visibleWhiteKeyCount = 15
    var scrollWhiteIndex: CGFloat = 0
    var onChange: (([Int: Int]) -> Void)?
    private var touchesByNote: [UITouch: Int] = [:]
    private var noteCounts: [Int: Int] = [:]
    private var velocities: [Int: Int] = [:]

    override init(frame: CGRect) { super.init(frame: frame); isMultipleTouchEnabled = true; backgroundColor = .clear }
    required init?(coder: NSCoder) { super.init(coder: coder); isMultipleTouchEnabled = true; backgroundColor = .clear }
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) { move(touches) }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) { move(touches) }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) { finish(touches) }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) { finish(touches) }

    private func move(_ touches: Set<UITouch>) {
        for touch in touches {
            let next = note(at: touch.location(in: self))
            if let old = touchesByNote[touch], old != next { stop(old); touchesByNote.removeValue(forKey: touch) }
            if let next, touchesByNote[touch] == nil {
                let velocity = max(64, min(110, 64 + Int(touch.location(in: self).y / max(1, bounds.height) * 46)))
                touchesByNote[touch] = next; start(next, velocity: velocity)
            }
        }
        onChange?(velocities)
    }
    private func finish(_ touches: Set<UITouch>) {
        for touch in touches { if let note = touchesByNote.removeValue(forKey: touch) { stop(note) } }
        onChange?(velocities)
    }
    private func start(_ note: Int, velocity: Int) {
        let count = noteCounts[note, default: 0]; noteCounts[note] = count + 1; velocities[note] = velocity
        if count == 0 { synth?.noteOnAbsolute(note, velocity: velocity) }
    }
    private func stop(_ note: Int) {
        let count = max(0, noteCounts[note, default: 0] - 1)
        if count == 0 { noteCounts.removeValue(forKey: note); velocities.removeValue(forKey: note); synth?.noteOffAbsolute(note) }
        else { noteCounts[note] = count }
    }
    private func note(at point: CGPoint) -> Int? {
        guard bounds.contains(point), bounds.width > 0 else { return nil }
        let whiteWidth = bounds.width / CGFloat(visibleWhiteKeyCount)
        let contentX = point.x + scrollWhiteIndex * whiteWidth
        if point.y < bounds.height * 0.62 {
            let blackWidth = whiteWidth * 0.58
            for note in aurelineBlackNotes {
                guard let before = aurelineWhiteNotes.firstIndex(where: { $0 >= note }) else { continue }
                if abs(CGFloat(before) * whiteWidth - contentX) <= blackWidth / 2 { return note }
            }
        }
        return aurelineWhiteNotes[min(max(0, Int(contentX / whiteWidth)), aurelineWhiteNotes.count - 1)]
    }
}

extension Color {
    init(hexValue: UInt32) {
        self.init(red: Double((hexValue >> 16) & 255) / 255, green: Double((hexValue >> 8) & 255) / 255, blue: Double(hexValue & 255) / 255)
    }
}
