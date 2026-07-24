import Foundation

struct AurelineLibraryVoice {
    let slot: Int
    let name: String
    let patch: [String: Double]
}

enum AurelineLibraryCodec {
    static let format = "com.hidecade.aureline.library"

    private static let stateToVoice: [String: String] = [
        "oscillatorALevel": "oscALevel", "oscillatorBLevel": "oscBLevel",
        "oscillatorBFine": "oscBFine", "filterEnvelope": "filterEnvAmount",
        "filterKeyboardTracking": "filterKeyTrack", "attack": "ampAttack",
        "decay": "ampDecay", "sustain": "ampSustain", "release": "ampRelease",
        "lfoWaveformMask": "lfoWaveMask",
        "polyModFilterEnvelope": "polyModFilterEnv",
        "polyModOscillatorB": "polyModOscB",
        "polyModToFrequencyA": "polyDestPitch",
        "polyModToPulseWidthA": "polyDestPWA",
        "polyModToFilter": "polyDestFilter", "master": "masterGain",
        "glideLegatoOnly": "glideLegato", "oscillatorSync": "oscSync",
        "oscillatorAOctave": "oscAOctave", "oscillatorBOctave": "oscBOctave",
        "oscillatorBLowFrequency": "oscBLowFrequency",
        "oscillatorBKeyboardTracking": "oscBKeyTrack",
        "lfoDestination0": "lfoDestA", "lfoDestination1": "lfoDestB",
        "lfoDestination2": "lfoDestPWA", "lfoDestination3": "lfoDestPWB",
        "lfoDestination4": "lfoDestFilter"
    ]
    private static let voiceToState = Dictionary(
        uniqueKeysWithValues: stateToVoice.map { ($1, $0) })

    static func decode(_ data: Data) throws -> [AurelineLibraryVoice] {
        let delegate = LibraryParserDelegate()
        let parser = XMLParser(data: data)
        parser.delegate = delegate
        guard parser.parse(), delegate.validRoot, delegate.voices.count == 50 else {
            throw CocoaError(.fileReadCorruptFile)
        }
        let sorted = delegate.voices.sorted { $0.slot < $1.slot }
        guard sorted.enumerated().allSatisfy({ $0.offset == $0.element.slot }) else {
            throw CocoaError(.fileReadCorruptFile)
        }
        return sorted
    }

    static func encode(_ voices: [AurelineLibraryVoice]) throws -> Data {
        guard voices.count == 50 else { throw CocoaError(.fileWriteUnknown) }
        var xml = """
        <?xml version="1.0" encoding="UTF-8"?>

        <AurelineLibrary format="\(format)" version="1" voiceCount="50">

        """
        for voice in voices.sorted(by: { $0.slot < $1.slot }) {
            var attributes: [(String, String)] = [
                ("version", "2"), ("slot", "\(voice.slot)"),
                ("voiceName", escaped(voice.name))
            ]
            for (key, value) in voice.patch.sorted(by: { $0.key < $1.key }) {
                let stateKey = voiceToState[key] ?? key
                attributes.append((stateKey, String(value)))
            }
            xml += "  <AurelineState "
            xml += attributes.map { "\($0.0)=\"\($0.1)\"" }.joined(separator: " ")
            xml += "/>\n"
        }
        xml += "</AurelineLibrary>\n"
        guard let data = xml.data(using: .utf8) else {
            throw CocoaError(.fileWriteInapplicableStringEncoding)
        }
        return data
    }

    private static func escaped(_ text: String) -> String {
        text.replacingOccurrences(of: "&", with: "&amp;")
            .replacingOccurrences(of: "\"", with: "&quot;")
            .replacingOccurrences(of: "<", with: "&lt;")
            .replacingOccurrences(of: ">", with: "&gt;")
    }

    private final class LibraryParserDelegate: NSObject, XMLParserDelegate {
        var validRoot = false
        var voices: [AurelineLibraryVoice] = []

        func parser(_ parser: XMLParser, didStartElement elementName: String,
                    namespaceURI: String?, qualifiedName qName: String?,
                    attributes attributeDict: [String: String] = [:]) {
            if elementName == "AurelineLibrary" {
                validRoot = attributeDict["format"] == format
                    && attributeDict["version"] == "1"
                    && attributeDict["voiceCount"] == "50"
                return
            }
            guard validRoot, elementName == "AurelineState",
                  let slotText = attributeDict["slot"], let slot = Int(slotText),
                  let name = attributeDict["voiceName"] else { return }
            var patch: [String: Double] = [:]
            for (key, text) in attributeDict
                where key != "version" && key != "slot" && key != "voiceName" {
                guard let value = Double(text), value.isFinite else { continue }
                patch[stateToVoice[key] ?? key] = value
            }
            voices.append(AurelineLibraryVoice(slot: slot, name: name, patch: patch))
        }
    }
}
