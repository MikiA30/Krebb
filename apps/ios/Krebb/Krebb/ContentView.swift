import SwiftUI

struct ContentView: View {
    @State private var selection = 0
    @Environment(\.scenePhase) private var scenePhase
    @StateObject private var sessions = SessionStore()
    @StateObject private var healthContext = HealthContextStore()
    @StateObject private var sensors = SensorConnectionStore()

    var body: some View {
        TabView(selection: $selection) {
            Tab("Today", systemImage: "waveform.path", value: 0) {
                NavigationStack { TodayView(healthContext: healthContext, sensorConnection: sensors, sessions: sessions) { selection = 1 } }
            }
            Tab("Check", systemImage: "sensor.tag.radiowaves.forward", value: 1) {
                NavigationStack { CheckView(sessions: sessions, healthContext: healthContext, sensorConnection: sensors) }
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
        .onChange(of: sensors.latestMeasurement) { _, measurement in
            guard let measurement, sessions.active?.isSimulated == false else { return }
            sessions.record(SessionReading(recordedAt: measurement.receivedAt,
                                           deviceTimestampMs: measurement.packet.timestampMs,
                                           skinTemperatureC: measurement.packet.skinTemperatureC,
                                           ambientTemperatureC: measurement.packet.ambientTemperatureC,
                                           sensorQuality: measurement.packet.sensorQuality))
        }
    }
}

#Preview { ContentView() }
