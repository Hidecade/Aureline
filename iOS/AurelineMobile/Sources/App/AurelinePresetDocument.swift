import SwiftUI
import UniformTypeIdentifiers
import UIKit

extension UTType {
    static let aurelineVoice = UTType(exportedAs: "com.hidecade.aureline.voice", conformingTo: .json)
    static let aurelineLibrary = UTType(exportedAs: "com.hidecade.aureline.library",
                                        conformingTo: .xml)
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
