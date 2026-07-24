import CoreMIDI
import Foundation

final class MobileMIDIInput {
    private weak var synth: MobileSynthModel?
    private var client = MIDIClientRef()
    private var inputPort = MIDIPortRef()
    private var sources = Set<MIDIEndpointRef>()
    private var runningStatus: UInt8?

    init(synth: MobileSynthModel) {
        self.synth = synth
        MIDIClientCreateWithBlock("Aureline MIDI Client" as CFString, &client) { [weak self] _ in
            self?.reconnect()
        }
        MIDIInputPortCreateWithBlock(client, "Aureline MIDI Input" as CFString, &inputPort) { [weak self] list, _ in
            self?.parse(list.pointee)
        }
        reconnect()
    }

    deinit {
        if inputPort != 0 { MIDIPortDispose(inputPort) }
        if client != 0 { MIDIClientDispose(client) }
    }

    private func reconnect() {
        sources.forEach { MIDIPortDisconnectSource(inputPort, $0) }
        sources.removeAll()
        for index in 0..<MIDIGetNumberOfSources() {
            let source = MIDIGetSource(index)
            if source != 0, MIDIPortConnectSource(inputPort, source, nil) == noErr { sources.insert(source) }
        }
    }

    private func parse(_ list: MIDIPacketList) {
        var packet = list.packet
        for _ in 0..<list.numPackets {
            withUnsafePointer(to: packet.data) { pointer in
                pointer.withMemoryRebound(to: UInt8.self, capacity: Int(packet.length)) {
                    parseBytes(Array(UnsafeBufferPointer(start: $0, count: Int(packet.length))))
                }
            }
            packet = MIDIPacketNext(&packet).pointee
        }
    }

    private func parseBytes(_ bytes: [UInt8]) {
        var index = 0
        while index < bytes.count {
            let first = bytes[index]
            if first >= 0xf8 { index += 1; continue }
            let status: UInt8
            if first >= 0x80 {
                status = first
                runningStatus = status < 0xf0 ? status : nil
                index += 1
            } else if let runningStatus { status = runningStatus }
            else { index += 1; continue }
            let kind = status & 0xf0
            let length = [0x80, 0x90, 0xa0, 0xb0, 0xe0].contains(Int(kind)) ? 2 : 1
            guard index + length <= bytes.count else { return }
            let a = bytes[index], b = length == 2 ? bytes[index + 1] : 0
            index += length
            DispatchQueue.main.async { [weak self] in self?.handle(kind, a, b) }
        }
    }

    private func handle(_ kind: UInt8, _ a: UInt8, _ b: UInt8) {
        switch kind {
        case 0x80:
            synth?.externalActiveNotes.removeValue(forKey: Int(a)); synth?.noteOffAbsolute(Int(a))
        case 0x90:
            if b == 0 { synth?.externalActiveNotes.removeValue(forKey: Int(a)); synth?.noteOffAbsolute(Int(a)) }
            else { synth?.externalActiveNotes[Int(a)] = Int(b); synth?.noteOnAbsolute(Int(a), velocity: Int(b)) }
        case 0xb0:
            if a == 1 { synth?.setModWheel(Double(b) / 127) }
            if a == 64 { synth?.setSustain(b >= 64) }
            if a == 120 || a == 123 { synth?.panic() }
        case 0xe0:
            let raw = Int(a) | Int(b) << 7
            synth?.setPitchWheel(max(-1, min(1, (Double(raw) - 8192) / 8192)))
        default: break
        }
    }
}
