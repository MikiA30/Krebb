import SwiftUI

struct ContentView: View {
    @State private var selection = 0
    @Environment(\.scenePhase) private var scenePhase
    @StateObject private var sessions = SessionStore()
    @StateObject private var healthContext = HealthContextStore()

    var body: some View {
        TabView(selection: $selection) {
            Tab("Today", systemImage: "waveform.path", value: 0) {
                NavigationStack { TodayView(healthContext: healthContext, sessions: sessions) { selection = 1 } }
            }
            Tab("Check", systemImage: "sensor.tag.radiowaves.forward", value: 1) {
                NavigationStack { CheckView(sessions: sessions, healthContext: healthContext) }
            }
            Tab("Journal", systemImage: "book.closed", value: 2) {
                NavigationStack { JournalView(sessions: sessions) }
            }
        }
        .tint(KrebbPalette.coral)
        .preferredColorScheme(.dark)
        .task(id: scenePhase) {
            guard scenePhase == .active else { return }
            while !Task.isCancelled {
                do { try await Task.sleep(for: .seconds(1)) } catch { return }
                guard !Task.isCancelled else { return }
                sessions.simulateTick()
            }
        }
    }
}

#Preview { ContentView() }
