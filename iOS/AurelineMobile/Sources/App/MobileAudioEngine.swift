import AVFoundation

final class MobileAudioEngine {
    private var audioEngine = AVAudioEngine()
    private let bridge: AurelineMobileEngineBridge
    private var sourceNode: AVAudioSourceNode?

    init(bridge: AurelineMobileEngineBridge) { self.bridge = bridge }

    var isRunning: Bool { audioEngine.isRunning }

    func start() throws {
        stopGraph()
        let session = AVAudioSession.sharedInstance()
        try session.setCategory(.playback, mode: .default, options: [.mixWithOthers])
        try session.setPreferredSampleRate(44_100)
        try session.setPreferredIOBufferDuration(0.005)
        try session.setActive(true)
        let rate = session.sampleRate > 0 ? session.sampleRate : 44_100
        bridge.prepare(sampleRate: rate)
        let format = AVAudioFormat(standardFormatWithSampleRate: rate, channels: 2)!
        let node = AVAudioSourceNode { [weak self] _, _, frameCount, buffers in
            self?.bridge.render(to: buffers, frames: Int32(frameCount))
            return noErr
        }
        audioEngine.attach(node)
        audioEngine.connect(node, to: audioEngine.mainMixerNode, format: format)
        sourceNode = node
        try audioEngine.start()
    }

    func suspend() { bridge.panic(); audioEngine.pause() }

    func stop() {
        bridge.panic()
        stopGraph()
        try? AVAudioSession.sharedInstance().setActive(false, options: [.notifyOthersOnDeactivation])
    }

    func resetAfterMediaServicesReset() {
        sourceNode = nil
        audioEngine = AVAudioEngine()
    }

    private func stopGraph() {
        audioEngine.stop()
        if let sourceNode {
            audioEngine.disconnectNodeOutput(sourceNode)
            audioEngine.detach(sourceNode)
            self.sourceNode = nil
        }
        audioEngine.reset()
    }
}
