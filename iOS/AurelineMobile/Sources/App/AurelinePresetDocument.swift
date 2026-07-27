import SwiftUI
import UniformTypeIdentifiers
import UIKit

extension UTType {
    static let aurelineVoice = UTType(exportedAs: "com.hidecade.aureline.voice", conformingTo: .json)
    static let aurelineLibrary = UTType(exportedAs: "com.hidecade.aureline.library",
                                        conformingTo: .xml)
    static let aurelineWave = UTType(exportedAs: "com.hidecade.aureline.wave",
                                     conformingTo: .xml)
}

struct AurelineWaveDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.aurelineWave, .xml] }
    var steps: [Int]
    var character: Int

    init(steps: [Int], character: Int) {
        self.steps = Array(steps.prefix(32)).map { max(0, min(31, $0)) }
        while self.steps.count < 32 { self.steps.append(0) }
        self.character = max(0, min(2, character))
    }

    init(data: Data) throws {
        guard let text = String(data: data, encoding: .utf8),
              text.contains("<AURELINE_WAVE") else {
            throw CocoaError(.fileReadCorruptFile)
        }
        func attribute(_ name: String) -> String? {
            let pattern = "\\b\(NSRegularExpression.escapedPattern(for: name))=\"([^\"]*)\""
            guard let expression = try? NSRegularExpression(pattern: pattern),
                  let match = expression.firstMatch(
                    in: text, range: NSRange(text.startIndex..., in: text)),
                  let range = Range(match.range(at: 1), in: text) else { return nil }
            return String(text[range])
        }
        let values = (attribute("steps") ?? "")
            .split(separator: ",").compactMap { Int($0) }
        guard values.count == 32 else { throw CocoaError(.fileReadCorruptFile) }
        self.init(steps: values, character: Int(attribute("character") ?? "0") ?? 0)
    }

    init(configuration: ReadConfiguration) throws {
        guard let data = configuration.file.regularFileContents else {
            throw CocoaError(.fileReadCorruptFile)
        }
        try self.init(data: data)
    }

    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        let values = steps.map(String.init).joined(separator: ",")
        let xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            + "<AURELINE_WAVE version=\"1\" character=\"\(character)\" "
            + "steps=\"\(values)\"/>\n"
        return FileWrapper(regularFileWithContents: Data(xml.utf8))
    }
}

struct AurelineVoiceFile: Codable {
    let format: String
    let version: Int
    var name: String
    var author: String
    var category: String
    var patch: [String: Double]
    var performance: [String: Double]
}

struct AurelineVoiceDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.aurelineVoice, .json] }
    var voice: AurelineVoiceFile

    init(voice: AurelineVoiceFile) { self.voice = voice }

    init(configuration: ReadConfiguration) throws {
        guard let data = configuration.file.regularFileContents else { throw CocoaError(.fileReadCorruptFile) }
        voice = try JSONDecoder().decode(AurelineVoiceFile.self, from: data)
    }

    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        let encoder = JSONEncoder(); encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        return FileWrapper(regularFileWithContents: try encoder.encode(voice))
    }
}

struct AurelineDocumentPicker: UIViewControllerRepresentable {
    enum Mode {
        case importing([UTType])
        case exporting(URL)
    }

    let mode: Mode
    let initialDirectoryURL: URL?
    let completion: (URL?) -> Void

    func makeCoordinator() -> Coordinator { Coordinator(completion: completion) }

    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        let picker: UIDocumentPickerViewController
        switch mode {
        case let .importing(types):
            picker = UIDocumentPickerViewController(forOpeningContentTypes: types, asCopy: true)
        case let .exporting(url):
            picker = UIDocumentPickerViewController(forExporting: [url], asCopy: true)
        }
        picker.delegate = context.coordinator
        picker.allowsMultipleSelection = false
        picker.directoryURL = initialDirectoryURL
        return picker
    }

    func updateUIViewController(_ uiViewController: UIDocumentPickerViewController,
                                context: Context) {}

    final class Coordinator: NSObject, UIDocumentPickerDelegate {
        let completion: (URL?) -> Void
        init(completion: @escaping (URL?) -> Void) { self.completion = completion }
        func documentPicker(_ controller: UIDocumentPickerViewController,
                            didPickDocumentsAt urls: [URL]) { completion(urls.first) }
        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
            completion(nil)
        }
    }
}
