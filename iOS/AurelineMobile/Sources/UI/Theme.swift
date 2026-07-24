import SwiftUI

enum AurelineTheme {
    static let background = Color(red: 0.055, green: 0.047, blue: 0.038)
    static let panel = Color(red: 0.12, green: 0.105, blue: 0.085)
    static let panelLight = Color(red: 0.20, green: 0.17, blue: 0.13)
    static let amber = Color(red: 1.0, green: 0.58, blue: 0.22)
    static let gold = Color(red: 0.84, green: 0.68, blue: 0.40)
    static let text = Color(red: 0.89, green: 0.87, blue: 0.82)
}

extension View {
    func panelStyle() -> some View {
        background(AurelineTheme.panel).clipShape(RoundedRectangle(cornerRadius: 8))
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(AurelineTheme.gold.opacity(0.3)))
    }

    func sectionTitle() -> some View {
        font(.system(size: 11, weight: .black)).foregroundStyle(AurelineTheme.amber)
    }
}
