import SwiftUI

enum CheckPhase {
    case ready, baseline, observing, complete

    var title: String {
        switch self {
        case .ready: "A moment to settle."
        case .baseline: "Finding your starting point."
        case .observing: "Follow the change."
        case .complete: "A check, complete."
        }
    }

    var detail: String {
        switch self {
        case .ready: "Place the contact sensor, get comfortable, and take a baseline before your meal."
        case .baseline: "In a real check, this stage collects a stable temperature reference. Preview the next step when you’re ready."
        case .observing: "The post-meal curve compares each new reading with your baseline and the room temperature."
        case .complete: "You’ve reached the end of the sample flow. No real measurements were collected or saved."
        }
    }

    var action: String {
        switch self {
        case .ready: "Try a sample check"
        case .baseline: "Preview meal observation"
        case .observing: "Finish sample check"
        case .complete: "Start again"
        }
    }

    var step: Int {
        switch self {
        case .ready: 0
        case .baseline: 1
        case .observing: 2
        case .complete: 3
        }
    }

    var next: CheckPhase {
        switch self {
        case .ready: .baseline
        case .baseline: .observing
        case .observing: .complete
        case .complete: .ready
        }
    }
}

struct CheckView: View {
    @Binding var phase: CheckPhase
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    @State private var showsSensors = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 32) {
                SampleLabel()
                Image("KrebbMark")
                    .resizable()
                    .scaledToFit()
                    .frame(width: 100, height: 100)
                    .clipShape(RoundedRectangle(cornerRadius: 26))
                    .padding(.top, 16)
                    .accessibilityHidden(true)
                VStack(alignment: .leading, spacing: 16) {
                    Text(phase.title).font(.largeTitle.weight(.semibold))
                        .contentTransition(.opacity)
                        .accessibilityIdentifier("checkPhaseTitle")
                    Text(phase.detail).foregroundStyle(.secondary)
                        .fixedSize(horizontal: false, vertical: true)
                }
                VStack(alignment: .leading, spacing: 24) {
                    stepRow("Settle & establish baseline", number: 1)
                    stepRow("Log meal & observe", number: 2)
                    stepRow("Review the change", number: 3)
                }
                .padding(.vertical, 12)
                Button {
                    withAnimation(reduceMotion ? nil : .smooth(duration: 0.35)) {
                        phase = phase.next
                    }
                } label: {
                    Text(phase.action).font(.headline)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 12)
                }
                .buttonStyle(.glassProminent)
                .accessibilityIdentifier("advanceCheck")
                Text("Sample mode · no sensor connection required")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            .padding(24)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle("Check")
        .navigationBarTitleDisplayMode(.large)
        .toolbar { KrebbToolbar(showsSensors: $showsSensors) }
        .sheet(isPresented: $showsSensors) { SensorSheet() }
    }

    private func stepRow(_ text: String, number: Int) -> some View {
        HStack(spacing: 16) {
            ZStack {
                Circle().fill(number <= phase.step ? KrebbPalette.coral : .white.opacity(0.08))
                if number < phase.step || phase == .complete {
                    Image(systemName: "checkmark").font(.caption.bold())
                } else {
                    Text("\(number)").font(.subheadline.monospacedDigit())
                }
            }
            .frame(width: 32, height: 32)
            Text(text).font(.subheadline.weight(number == phase.step ? .semibold : .regular))
                .foregroundStyle(number <= phase.step ? .primary : .secondary)
        }
        .accessibilityElement(children: .combine)
    }
}

struct JournalView: View {
    var body: some View {
        List {
            Section {
                VStack(alignment: .leading, spacing: 14) {
                    SampleLabel()
                    Text("Your patterns take time.").font(.title2.weight(.semibold))
                    Text("Completed sensor sessions will live here. Explore an example of what a check could look like.")
                        .foregroundStyle(.secondary)
                }
                .padding(.vertical, 12)
            }
            .listRowBackground(Color.clear)
            Section("Explore a sample") {
                NavigationLink {
                    SampleSessionView()
                } label: {
                    HStack(spacing: 16) {
                        Image(systemName: "waveform.path").font(.title2).foregroundStyle(KrebbPalette.coral)
                        VStack(alignment: .leading, spacing: 5) {
                            Text("After a meal").font(.headline)
                            Text("12-minute example").font(.caption).foregroundStyle(.secondary)
                        }
                        Spacer()
                        Text("+0.3°").font(.title3.monospacedDigit()).foregroundStyle(KrebbPalette.blush)
                    }
                    .padding(.vertical, 10)
                }
                .accessibilityIdentifier("sampleSession")
                .listRowBackground(KrebbPalette.surface)
            }
        }
        .scrollContentBackground(.hidden)
        .background(KrebbPalette.canvas)
        .navigationTitle("Journal")
        .navigationBarTitleDisplayMode(.large)
    }
}

private struct SampleSessionView: View {
    @State private var minute: Int?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                SampleLabel()
                Text("A small change, in context.").font(.largeTitle.weight(.semibold))
                ThermalChart(selectedMinute: $minute).frame(height: 240)
                LabeledContent("Baseline skin temperature", value: "33.1°C")
                LabeledContent("Final skin temperature", value: "33.4°C")
                LabeledContent("Ambient reference", value: "23.6°C")
                Text("Illustrative data only. This example is not a saved sensor session or a metabolic classification.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            .padding(24)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle("After a meal")
        .navigationBarTitleDisplayMode(.inline)
    }
}
