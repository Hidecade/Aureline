import SwiftUI

@main
struct AurelineMobileApp: App {
    @StateObject private var synth = MobileSynthModel()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            RootView()
                .environmentObject(synth)
                .onChange(of: scenePhase) { phase in
                    if phase == .active { synth.applicationDidBecomeActive() }
                    if phase == .background { synth.applicationDidEnterBackground() }
                }
        }
    }
}
