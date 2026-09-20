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

    var body: some View {
        NavigationStack {
            List {
                Section {
                    Label("You’re exploring sample data", systemImage: "play.circle.fill")
                        .foregroundStyle(KrebbPalette.blush)
                    Text("Hardware connections will appear here when the sensor integration is ready.")
                        .foregroundStyle(.secondary)
                }
                Section("Your setup") {
                    LabeledContent("ESP32 + skin sensor", value: "Not connected")
                    LabeledContent("Ambient temperature", value: "Not connected")
                    LabeledContent("Apple Watch", value: "Optional")
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
