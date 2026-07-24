import SwiftUI

struct ParameterKnob: View {
    let parameter: AurelineParameter
    @Binding var value: Double
    @State private var dragStartValue: Double?

    private var normalized: Double {
        if parameter.logarithmic, parameter.range.lowerBound > 0 {
            return log(value / parameter.range.lowerBound) / log(parameter.range.upperBound / parameter.range.lowerBound)
        }
        return (value - parameter.range.lowerBound) / (parameter.range.upperBound - parameter.range.lowerBound)
    }

    var body: some View {
        VStack(spacing: 1) {
            Text(parameter.name)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Color(hexValue: 0xd7dcda))
                .lineLimit(1).minimumScaleFactor(0.7)
                .frame(height: 12)
            Canvas { context, size in drawKnob(context: &context, size: size) }
            .frame(width: 58, height: 58)
            .contentShape(Rectangle())
            .gesture(DragGesture(minimumDistance: 0)
                .onChanged { gesture in
                    if dragStartValue == nil { dragStartValue = normalized }
                    setNormalized(min(1, max(0, (dragStartValue ?? normalized) - Double(gesture.translation.height / 130))))
                }
                .onEnded { _ in dragStartValue = nil })
            .onTapGesture(count: 2) { value = parameter.defaultValue }
        }
        .frame(width: 58)
        .accessibilityElement(children: .ignore)
        .accessibilityLabel(parameter.name)
        .accessibilityValue(displayValue)
        .accessibilityAdjustableAction { direction in
            let step = (parameter.range.upperBound - parameter.range.lowerBound) / 100
            value = max(parameter.range.lowerBound, min(parameter.range.upperBound, value + (direction == .increment ? step : -step)))
        }
    }

    private var displayValue: String {
        if parameter.id == "transpose" { return String(format: "%.0f", value) }
        if parameter.range.upperBound >= 1000 { return String(format: "%.0f", value) }
        if parameter.range.upperBound > 20 { return String(format: "%.1f", value) }
        return String(format: "%.2f", value)
    }

    private func setNormalized(_ position: Double) {
        if parameter.logarithmic, parameter.range.lowerBound > 0 {
            value = parameter.range.lowerBound * pow(parameter.range.upperBound / parameter.range.lowerBound, position)
        } else {
            value = parameter.range.lowerBound + position * (parameter.range.upperBound - parameter.range.lowerBound)
        }
    }

    private func drawKnob(context: inout GraphicsContext, size: CGSize) {
        let position = CGFloat(min(1, max(0, normalized)))
        let center = CGPoint(x: size.width * 0.5, y: size.height * 0.53)
        let radius = min(size.width, size.height) * 0.43
        let startAngle = -CGFloat.pi * 0.75
        let arcRange = CGFloat.pi * 1.5
        let activeTick = Int((position * 10).rounded())

        func point(_ r: CGFloat, _ angle: CGFloat) -> CGPoint {
            CGPoint(x: center.x + sin(angle) * r, y: center.y - cos(angle) * r)
        }

        for tick in 0...10 {
            let angle = startAngle + CGFloat(tick) / 10 * arcRange
            let selected = tick <= activeTick
            let color = selected ? Color(hexValue: 0xc7cac9) : Color(hexValue: 0x34383a).opacity(0.82)
            let mark = Path { path in
                path.move(to: point(radius - 5.5, angle))
                path.addLine(to: point(radius - 2, angle))
            }
            context.stroke(mark, with: .color(color), lineWidth: selected ? 1.5 : 1)
            let label = parameter.id == "tempoBpm" ? "\(40 + tick * 20)" : "\(tick)"
            let labelSize: CGFloat = parameter.id == "tempoBpm" ? 5.2 : 6.5
            let number = context.resolve(Text(label).font(.system(size: labelSize, weight: .bold)).foregroundColor(color))
            context.draw(number, at: point(radius + 2.5, angle), anchor: .center)
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
