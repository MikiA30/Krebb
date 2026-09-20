import SwiftUI

struct ContentView: View {
    @State private var selection = 0
    @State private var phase = CheckPhase.ready
    @StateObject private var healthContext = HealthContextStore()

    var body: some View {
        TabView(selection: $selection) {
            Tab("Today", systemImage: "waveform.path", value: 0) {
                NavigationStack { TodayView(healthContext: healthContext) { selection = 1 } }
            }
            Tab("Check", systemImage: "sensor.tag.radiowaves.forward", value: 1) {
                NavigationStack { CheckView(phase: $phase, healthContext: healthContext) }
            }
            Tab("Journal", systemImage: "book.closed", value: 2) {
                NavigationStack { JournalView() }
            }
        }
        .tint(KrebbPalette.coral)
        .preferredColorScheme(.dark)
    }
}

#Preview { ContentView() }
