import SwiftUI

enum KrebbPalette {
    static let canvas = Color(red: 0.035, green: 0.035, blue: 0.043)
    static let surface = Color(red: 0.085, green: 0.078, blue: 0.085)
    static let coral = Color(red: 1, green: 0.30, blue: 0.38)
    static let blush = Color(red: 1, green: 0.65, blue: 0.67)
}

struct SampleLabel: View {
    var body: some View {
        Label("Sample data", systemImage: "play.circle")
            .font(.caption.weight(.medium))
            .foregroundStyle(KrebbPalette.blush)
    }
}

struct KrebbToolbar: ToolbarContent {
    @Binding var showsSensors: Bool

    var body: some ToolbarContent {
        ToolbarItem(placement: .topBarLeading) {
            Image("KrebbMark")
                .resizable()
                .scaledToFit()
                .frame(width: 34, height: 34)
                .clipShape(RoundedRectangle(cornerRadius: 9))
                .accessibilityLabel("Krebb")
        }
        .sharedBackgroundVisibility(.hidden)
        ToolbarItem(placement: .topBarTrailing) {
            Button("Sensors", systemImage: "sensor.tag.radiowaves.forward") {
                showsSensors = true
            }
        }
    }
}

struct SensorSheet: View {
    @Environment(\.dismiss) private var dismiss
    @ObservedObject var healthContext: HealthContextStore
    @ObservedObject var sensorConnection: SensorConnectionStore

    var body: some View {
        NavigationStack {
            List {
                Section {
                    Label(sensorConnection.state.isReceiving ? "Krebb One is streaming" : "Sensor setup",
                          systemImage: sensorConnection.state.isReceiving ? "sensor.tag.radiowaves.forward.fill" : "sensor.tag.radiowaves.forward")
                        .foregroundStyle(KrebbPalette.blush)
                    Text("The app scans for the firmware service UUID, subscribes to the measurement characteristic, and records packets when a sensor check is active.")
                        .foregroundStyle(.secondary)
                }
                Section("Your setup") {
                    LabeledContent("Krebb One", value: sensorConnection.state.label)
                    LabeledContent("Packets received", value: "\(sensorConnection.packetCount)")
                    Button(sensorConnection.state.isReceiving ? "Stop sensor" : "Scan for Krebb One") {
                        sensorConnection.state.isReceiving ? sensorConnection.stop() : sensorConnection.start()
                    }
                    if let measurement = sensorConnection.latestMeasurement {
                        if let skin = measurement.packet.skinTemperatureC {
                            LabeledContent("Skin", value: "\(skin.formatted(.number.precision(.fractionLength(2)))) °C")
                        }
                        if let ambient = measurement.packet.ambientTemperatureC {
                            LabeledContent("Room", value: "\(ambient.formatted(.number.precision(.fractionLength(1)))) °C")
                        }
                        LabeledContent("Quality", value: measurement.packet.sensorQuality.formatted(.number.precision(.fractionLength(2))))
                    }
                    if let error = sensorConnection.lastError {
                        Text(error).foregroundStyle(.red)
                    }
                }
                Section("Apple Watch context") {
                    LabeledContent("Status", value: healthContext.status.label)
                    if let reading = healthContext.latestHeartRate {
                        LabeledContent("Latest heart rate", value: reading.displayValue)
                        LabeledContent("Recorded", value: reading.timestamp.formatted(date: .omitted, time: .shortened))
                        if let sourceName = reading.sourceName {
                            LabeledContent("Source", value: sourceName)
                        }
                        if let stepCount = healthContext.latestStepCount {
                            LabeledContent(
                                "Latest step sample",
                                value: stepCount.formatted(.number.precision(.fractionLength(0)))
                            )
                        }
                    } else {
                        Text("Krebb will read available Health samples that your paired Apple Watch has already synchronized to this iPhone.")
                            .foregroundStyle(.secondary)
                    }
                    Button(healthContext.status.actionTitle) {
                        Task {
                            if healthContext.status == .notRequested || healthContext.status == .unavailable {
                                await healthContext.requestAccess()
                            }
                            await healthContext.refresh()
                        }
                    }
                    .disabled(healthContext.isLoading)
                }
            }
            .navigationTitle("Sensors")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done", systemImage: "checkmark") { dismiss() }
                }
            }
        }
        .presentationDetents([.medium, .large])
        .presentationDragIndicator(.visible)
    }
}
