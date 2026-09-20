import SwiftUI

enum KrebbPalette {
    static let canvas = Color(red: 0.035, green: 0.035, blue: 0.043)
    static let surface = Color(red: 0.085, green: 0.078, blue: 0.085)
    static let raisedSurface = Color(red: 0.115, green: 0.098, blue: 0.108)
    static let coral = Color(red: 1, green: 0.30, blue: 0.38)
    static let blush = Color(red: 1, green: 0.65, blue: 0.67)
    static let wine = Color(red: 0.34, green: 0.035, blue: 0.075)
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

    private var sensorButtonTitle: String {
        if sensorConnection.state.isReceiving { return "Stop sensor" }
        if sensorConnection.state == .discovered { return "Connect to this device" }
        if sensorConnection.state == .connecting || sensorConnection.state == .connected { return "Cancel connection" }
        return "Scan for Krebb One"
    }

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
                    if let sensor = sensorConnection.discoveredSensor {
                        LabeledContent("Detected device", value: "\(sensor.name) · \(sensor.rssi) dBm")
                    }
                    LabeledContent("Packets received", value: "\(sensorConnection.packetCount)")
                    Button(sensorButtonTitle) {
                        if sensorConnection.state.isReceiving ||
                            sensorConnection.state == .connecting ||
                            sensorConnection.state == .connected {
                            sensorConnection.stop()
                        } else if sensorConnection.state == .discovered {
                            sensorConnection.connectToDiscoveredSensor()
                        } else {
                            sensorConnection.start()
                        }
                    }
                    if let measurement = sensorConnection.latestMeasurement {
                        LabeledContent("Last packet", value: measurement.receivedAt.formatted(date: .omitted, time: .standard))
                        if let skin = measurement.packet.skinTemperatureC {
                            LabeledContent("Skin", value: "\(skin.formatted(.number.precision(.fractionLength(2)))) °C")
                        }
                        if let ambient = measurement.packet.ambientTemperatureC {
                            LabeledContent("Room", value: "\(ambient.formatted(.number.precision(.fractionLength(1)))) °C")
                        }
                        LabeledContent("Quality", value: measurement.packet.sensorQuality.formatted(.number.precision(.fractionLength(2))))
                    } else if sensorConnection.state.isReceiving {
                        Text("Connected, but no measurement packet has arrived yet.")
                            .foregroundStyle(.secondary)
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
                        Text("Krebb reads Health samples that your paired Apple Watch has synchronized to this iPhone. iOS may ask again after reinstalling the app or changing Health settings.")
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
