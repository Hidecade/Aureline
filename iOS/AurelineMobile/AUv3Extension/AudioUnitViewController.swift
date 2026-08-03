import AudioToolbox
import CoreAudioKit
import UIKit

public final class AudioUnitViewController: AUViewController, AUAudioUnitFactory {
    private var audioUnit: AurelineAUAudioUnit?
    private var voiceNames: [String] = []
    private var voiceIndex = 0
    private let voiceButton = UIButton(type: .system)
    private let previousButton = UIButton(type: .system)
    private let nextButton = UIButton(type: .system)
    private let mode = UISegmentedControl(items: ["POLY", "MONO", "UNISON"])
    private let portamentoButton = UIButton(type: .system)
    private let glideKnob = GlideKnob()
    private let statusLabel = UILabel()

    public override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = UIColor(red: 0.055, green: 0.047, blue: 0.038, alpha: 1)

        let title = UILabel()
        title.text = "AURELINE"
        title.textColor = UIColor(red: 1, green: 0.58, blue: 0.22, alpha: 1)
        title.font = .systemFont(ofSize: 22, weight: .black)
        title.textAlignment = .center

        configureButton(previousButton, title: "<")
        configureButton(voiceButton, title: "01 WARM BRASS")
        configureButton(nextButton, title: ">")
        previousButton.addTarget(self, action: #selector(previousVoice), for: .touchUpInside)
        nextButton.addTarget(self, action: #selector(nextVoice), for: .touchUpInside)
        voiceButton.showsMenuAsPrimaryAction = true

        mode.selectedSegmentIndex = 0
        mode.addTarget(self, action: #selector(modeChanged), for: .valueChanged)
        mode.selectedSegmentTintColor = UIColor(red: 1, green: 0.57, blue: 0.22, alpha: 1)
        mode.setTitleTextAttributes([.foregroundColor: UIColor.black,
                                     .font: UIFont.systemFont(ofSize: 12, weight: .bold)], for: .selected)
        mode.setTitleTextAttributes([.foregroundColor: UIColor(white: 0.84, alpha: 1),
                                     .font: UIFont.systemFont(ofSize: 12, weight: .bold)], for: .normal)

        configureButton(portamentoButton, title: "PORTA OFF")
        portamentoButton.addTarget(self, action: #selector(stepPortamento), for: .touchUpInside)
        glideKnob.addTarget(self, action: #selector(glideChanged), for: .valueChanged)

        statusLabel.textColor = UIColor(white: 0.72, alpha: 1)
        statusLabel.font = .systemFont(ofSize: 11, weight: .semibold)
        statusLabel.textAlignment = .center

        let voiceRow = UIStackView(arrangedSubviews: [previousButton, voiceButton, nextButton])
        voiceRow.axis = .horizontal
        voiceRow.spacing = 7
        let playRow = UIStackView(arrangedSubviews: [mode, portamentoButton, glideKnob])
        playRow.axis = .horizontal
        playRow.spacing = 8
        let stack = UIStackView(arrangedSubviews: [title, voiceRow, playRow, statusLabel])
        stack.translatesAutoresizingMaskIntoConstraints = false
        stack.axis = .vertical
        stack.alignment = .center
        stack.spacing = 10
        view.addSubview(stack)

        NSLayoutConstraint.activate([
            stack.centerXAnchor.constraint(equalTo: view.safeAreaLayoutGuide.centerXAnchor),
            stack.centerYAnchor.constraint(equalTo: view.safeAreaLayoutGuide.centerYAnchor),
            previousButton.widthAnchor.constraint(equalToConstant: 44),
            nextButton.widthAnchor.constraint(equalToConstant: 44),
            voiceButton.widthAnchor.constraint(equalToConstant: 240),
            voiceButton.heightAnchor.constraint(equalToConstant: 38),
            mode.widthAnchor.constraint(equalToConstant: 230),
            mode.heightAnchor.constraint(equalToConstant: 36),
            portamentoButton.widthAnchor.constraint(equalToConstant: 110),
            portamentoButton.heightAnchor.constraint(equalToConstant: 36),
            glideKnob.widthAnchor.constraint(equalToConstant: 70),
            glideKnob.heightAnchor.constraint(equalToConstant: 76)
        ])
        refreshFromAudioUnit()
    }

    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        let unit = try AurelineAUAudioUnit(componentDescription: componentDescription)
        audioUnit = unit
        DispatchQueue.main.async { [weak self] in self?.refreshFromAudioUnit() }
        return unit
    }

    @objc private func previousVoice() { stepVoice(-1) }
    @objc private func nextVoice() { stepVoice(1) }

    private func stepVoice(_ delta: Int) {
        guard !voiceNames.isEmpty else { return }
        selectVoice((voiceIndex + delta + voiceNames.count) % voiceNames.count)
    }

    private func selectVoice(_ index: Int) {
        guard voiceNames.indices.contains(index) else { return }
        voiceIndex = index
        parameter("factoryVoice")?.value = AUValue(index)
        updateLabels()
    }

    @objc private func modeChanged() {
        parameter("voiceMode")?.value = AUValue(mode.selectedSegmentIndex)
        updateLabels()
    }

    @objc private func stepPortamento() {
        let next = (currentPortamentoPreset() + 1) % 7
        let times: [Float] = [0, 0.08, 0.25, 0.65, 0.08, 0.25, 0.65]
        parameter("glide")?.value = times[next]
        parameter("glideLegato")?.value = next >= 4 ? 1 : 0
        updateLabels()
    }

    @objc private func glideChanged() {
        parameter("glide")?.value = AUValue(glideKnob.value)
        updateLabels()
    }

    private func currentPortamentoPreset() -> Int {
        let glide = parameter("glide")?.value ?? 0
        guard glide > 0.001 else { return 0 }
        let length = glide < 0.16 ? 1 : glide < 0.45 ? 2 : 3
        return (parameter("glideLegato")?.value ?? 0) >= 0.5 ? length + 3 : length
    }

    private func refreshFromAudioUnit() {
        guard isViewLoaded else { return }
        if let voiceParameter = parameter("factoryVoice") {
            voiceNames = voiceParameter.valueStrings ?? []
            voiceIndex = min(max(0, Int(voiceParameter.value.rounded())),
                             max(0, voiceNames.count - 1))
        }
        rebuildVoiceMenu()
        updateLabels()
    }

    private func rebuildVoiceMenu() {
        voiceButton.menu = UIMenu(title: "SELECT VOICE",
            children: voiceNames.enumerated().map { index, name in
                UIAction(title: name, state: index == voiceIndex ? .on : .off) {
                    [weak self] _ in self?.selectVoice(index)
                }
            })
    }

    private func updateLabels() {
        guard isViewLoaded else { return }
        let selectedMode = min(2, max(0, Int(parameter("voiceMode")?.value ?? 0)))
        mode.selectedSegmentIndex = selectedMode
        voiceButton.setTitle(voiceNames.indices.contains(voiceIndex)
            ? voiceNames[voiceIndex] : "--", for: .normal)
        portamentoButton.setTitle(portamentoTitle(currentPortamentoPreset()), for: .normal)
        let glide = parameter("glide")?.value ?? 0
        glideKnob.value = CGFloat(glide)
        statusLabel.text = "\(mode.titleForSegment(at: selectedMode) ?? "POLY") / \(portamentoTitle(currentPortamentoPreset())) / GLIDE \(formatGlide(glide))"
        rebuildVoiceMenu()
    }

    private func formatGlide(_ value: AUValue) -> String {
        value < 0.001 ? "OFF" : String(format: "%.3f s", value)
    }

    private func portamentoTitle(_ value: Int) -> String {
        switch value {
        case 1: return "FULL S"
        case 2: return "FULL M"
        case 3: return "FULL L"
        case 4: return "FINGER S"
        case 5: return "FINGER M"
        case 6: return "FINGER L"
        default: return "PORTA OFF"
        }
    }

    private func parameter(_ id: String) -> AUParameter? {
        audioUnit?.parameterTree?.allParameters.first { $0.identifier == id }
    }

    private func configureButton(_ button: UIButton, title: String) {
        button.translatesAutoresizingMaskIntoConstraints = false
        button.setTitle(title, for: .normal)
        button.setTitleColor(UIColor(red: 1, green: 0.68, blue: 0.34, alpha: 1), for: .normal)
        button.titleLabel?.font = .systemFont(ofSize: 12, weight: .bold)
        button.backgroundColor = UIColor(red: 0.14, green: 0.12, blue: 0.095, alpha: 1)
        button.layer.cornerRadius = 5
        button.layer.borderWidth = 1
        button.layer.borderColor = UIColor(red: 0.50, green: 0.39, blue: 0.20, alpha: 1).cgColor
    }
}

private final class GlideKnob: UIControl {
    var value: CGFloat = 0 {
        didSet {
            value = min(5, max(0, value))
            setNeedsDisplay()
            accessibilityValue = value < 0.001 ? "Off" : String(format: "%.3f seconds", value)
        }
    }

    private var dragStartY: CGFloat = 0
    private var dragStartPosition: CGFloat = 0

    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .clear
        isAccessibilityElement = true
        accessibilityLabel = "Glide time"
        accessibilityTraits = .adjustable
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
    }

    override func beginTracking(_ touch: UITouch, with event: UIEvent?) -> Bool {
        dragStartY = touch.location(in: self).y
        dragStartPosition = position(for: value)
        return true
    }

    override func continueTracking(_ touch: UITouch, with event: UIEvent?) -> Bool {
        let delta = (dragStartY - touch.location(in: self).y) / 120
        value = glideValue(for: min(1, max(0, dragStartPosition + delta)))
        sendActions(for: .valueChanged)
        return true
    }

    override func accessibilityIncrement() {
        value = glideValue(for: min(1, position(for: value) + 0.05))
        sendActions(for: .valueChanged)
    }

    override func accessibilityDecrement() {
        value = glideValue(for: max(0, position(for: value) - 0.05))
        sendActions(for: .valueChanged)
    }

    override func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }
        let center = CGPoint(x: rect.midX, y: 29)
        let radius: CGFloat = 19
        let start = CGFloat.pi * 0.75
        let sweep = CGFloat.pi * 1.5
        let end = start + sweep

        context.setLineWidth(4)
        context.setLineCap(.round)
        context.setStrokeColor(UIColor(red: 0.25, green: 0.21, blue: 0.16, alpha: 1).cgColor)
        context.addArc(center: center, radius: radius, startAngle: start, endAngle: end, clockwise: false)
        context.strokePath()

        let angle = start + sweep * position(for: value)
        context.setStrokeColor(UIColor(red: 1, green: 0.58, blue: 0.22, alpha: 1).cgColor)
        context.addArc(center: center, radius: radius, startAngle: start, endAngle: angle, clockwise: false)
        context.strokePath()

        context.setFillColor(UIColor(red: 0.14, green: 0.12, blue: 0.095, alpha: 1).cgColor)
        context.fillEllipse(in: CGRect(x: center.x - 15, y: center.y - 15, width: 30, height: 30))
        context.setStrokeColor(UIColor(red: 1, green: 0.68, blue: 0.34, alpha: 1).cgColor)
        context.setLineWidth(2)
        context.move(to: center)
        context.addLine(to: CGPoint(x: center.x + cos(angle) * 12,
                                    y: center.y + sin(angle) * 12))
        context.strokePath()

        let labelStyle: [NSAttributedString.Key: Any] = [
            .font: UIFont.systemFont(ofSize: 10, weight: .bold),
            .foregroundColor: UIColor(red: 1, green: 0.68, blue: 0.34, alpha: 1)
        ]
        let valueStyle: [NSAttributedString.Key: Any] = [
            .font: UIFont.monospacedDigitSystemFont(ofSize: 9, weight: .semibold),
            .foregroundColor: UIColor(white: 0.76, alpha: 1)
        ]
        drawCentered("GLIDE", y: 52, style: labelStyle)
        drawCentered(value < 0.001 ? "OFF" : String(format: "%.3fs", value), y: 64, style: valueStyle)
    }

    private func drawCentered(_ text: String, y: CGFloat,
                              style: [NSAttributedString.Key: Any]) {
        let size = text.size(withAttributes: style)
        text.draw(at: CGPoint(x: (bounds.width - size.width) * 0.5, y: y), withAttributes: style)
    }

    // The logarithmic response leaves useful travel for short synth glides while
    // retaining the full five-second parameter range.
    private func position(for glide: CGFloat) -> CGFloat {
        guard glide >= 0.005 else { return 0 }
        return min(1, max(0, log(glide / 0.005) / log(1000)))
    }

    private func glideValue(for position: CGFloat) -> CGFloat {
        guard position > 0.01 else { return 0 }
        return 0.005 * pow(1000, position)
    }
}
