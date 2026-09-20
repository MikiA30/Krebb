import SwiftUI
import Charts

struct CheckView: View {
    @ObservedObject var sessions: SessionStore
    @ObservedObject var healthContext: HealthContextStore
    @State private var showsSensors = false
    @State private var note = ""
    @State private var justFinished = false

    private var title: String {
        guard let active = sessions.active else { return justFinished ? "A check, complete." : "A moment to settle." }
        return active.stage == .baseline ? "Finding your starting point." : "Follow the change."
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Label("Simulated temperature", systemImage: "play.circle")
                    .font(.subheadline).foregroundStyle(KrebbPalette.blush)
                Text(title).font(.largeTitle.weight(.semibold))
                    .accessibilityIdentifier("checkPhaseTitle")
                if let active = sessions.active {
                    Text(active.stage == .baseline
                         ? "Collect a short simulated baseline, then label your observation. Three samples unlock the next step."
                         : "The curve compares simulated readings with your saved baseline. Finish when you’re ready.")
                        .foregroundStyle(.secondary)
                    SessionReadout(session: active)
                    if active.stage == .baseline {
                        TextField("Meal or observation note (optional)", text: $note, axis: .vertical)
                            .textFieldStyle(.roundedBorder)
                            .accessibilityIdentifier("sessionNote")
                    }
                    Button(active.stage == .baseline ? "Begin observation" : "Finish and save") {
                        if active.stage == .baseline {
                            sessions.beginObservation(note: note)
                        } else {
                            sessions.attachHealth(healthContext.snapshot())
                            justFinished = sessions.finish()
                        }
                    }
                    .buttonStyle(.glassProminent)
                    .disabled(active.stage == .baseline ? active.readings.count < 3 : !hasObservation(active))
                    .accessibilityIdentifier("advanceCheck")
                    Button("Attach latest Health reading", systemImage: "heart") {
                        sessions.attachHealth(healthContext.snapshot())
                    }
                    .disabled(healthContext.latestHeartRate == nil && healthContext.latestStepCount == nil)
                    Text("Health context is optional. Open Sensors to allow access or refresh readings. Attached values retain their original measurement times.")
                        .font(.footnote).foregroundStyle(.secondary)
                    Text("This draft saves after each sample. Simulation runs while the app is active; time away leaves a gap in the recording.")
                        .font(.footnote).foregroundStyle(.secondary)
                } else {
                    Text(justFinished
                         ? "Saved on this iPhone. Open Journal to review your simulation and any attached Health context."
                         : "Exercise the full recording flow while your sensor is being built. Temperature readings are generated; any Health readings you attach come from Apple Health.")
                        .foregroundStyle(.secondary)
                    Button(justFinished ? "Start another simulation" : "Start simulation") {
                        justFinished = false
                        note = ""
                        sessions.startSimulation()
                        sessions.simulateTick()
                        sessions.attachHealth(healthContext.snapshot())
                    }
                    .buttonStyle(.glassProminent)
                    .accessibilityIdentifier("advanceCheck")
                }
                if let error = sessions.errorMessage {
                    Text(error).foregroundStyle(.red).accessibilityIdentifier("sessionError")
                }
            }
            .padding(24)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle("Check")
        .navigationBarTitleDisplayMode(.large)
        .toolbar { KrebbToolbar(showsSensors: $showsSensors) }
        .sheet(isPresented: $showsSensors) { SensorSheet(healthContext: healthContext) }
        .onAppear { note = sessions.active?.note ?? "" }
        .onChange(of: note) { _, value in sessions.updateNote(value) }
    }

    private func hasObservation(_ session: MeasurementSession) -> Bool {
        guard let start = session.observationStartedAt else { return false }
        return session.readings.contains { $0.recordedAt >= start }
    }
}

struct SessionReadout: View {
    let session: MeasurementSession

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            LabeledContent("Temperature samples", value: "\(session.readings.count)")
            if let skin = session.readings.last?.skinTemperatureC {
                LabeledContent("Skin", value: temperature(skin))
            }
            if let ambient = session.readings.last?.ambientTemperatureC {
                LabeledContent("Room", value: temperature(ambient))
            }
            if let baseline = session.baseline {
                LabeledContent(session.stage == .baseline ? "Baseline so far" : "Baseline", value: temperature(baseline))
            }
            if let delta = session.latestDelta {
                LabeledContent("Change from baseline", value: temperature(delta))
                    .foregroundStyle(KrebbPalette.blush)
            }
            SessionChart(session: session).frame(height: 200)
            if let reading = session.healthSnapshots.last?.heartRate {
                LabeledContent("Attached heart rate", value: reading.displayValue)
                Text("Recorded \(reading.timestamp.formatted(date: .abbreviated, time: .standard)) · \(reading.sourceName ?? "HealthKit")")
                    .font(.caption).foregroundStyle(.secondary)
            }
            Text("\(session.healthSnapshots.count) Health snapshots attached")
                .font(.caption).foregroundStyle(.secondary)
        }
    }

    private func temperature(_ value: Double) -> String {
        "\(value.formatted(.number.precision(.fractionLength(2)))) °C"
    }
}

struct SessionChart: View {
    let session: MeasurementSession

    private var points: [(reading: SessionReading, segment: Int)] {
        var segment = 0
        return session.readings.enumerated().map { index, reading in
            if index > 0 {
                let previous = session.readings[index - 1]
                if reading.recordedAt.timeIntervalSince(previous.recordedAt) > 3 || previous.skinTemperatureC == nil {
                    segment += 1
                }
            }
            return (reading, segment)
        }
    }

    var body: some View {
        Chart {
            ForEach(points, id: \.reading.id) { point in
                if let skin = point.reading.skinTemperatureC {
                    LineMark(x: .value("Time", point.reading.recordedAt), y: .value("Skin °C", skin),
                             series: .value("Segment", point.segment))
                        .foregroundStyle(KrebbPalette.coral)
                    PointMark(x: .value("Time", point.reading.recordedAt), y: .value("Skin °C", skin))
                        .foregroundStyle(KrebbPalette.coral).symbolSize(8)
                }
            }
            if let start = session.observationStartedAt {
                RuleMark(x: .value("Observation", start))
                    .foregroundStyle(KrebbPalette.blush)
                    .lineStyle(StrokeStyle(dash: [4]))
            }
        }
        .chartYScale(domain: .automatic(includesZero: false))
        .accessibilityLabel(session.isSimulated ? "Recorded simulated skin temperatures" : "Recorded skin temperatures")
    }
}

struct JournalView: View {
    @ObservedObject var sessions: SessionStore

    var body: some View {
        List {
            if sessions.completed.isEmpty {
                ContentUnavailableView("Your first check starts here", systemImage: "book.closed",
                                       description: Text("Finish a simulation in Check to save and review a session."))
            }
            if let error = sessions.errorMessage { Text(error).foregroundStyle(.red) }
            ForEach(sessions.completed) { session in
                NavigationLink {
                    SessionDetailView(session: session)
                } label: {
                    VStack(alignment: .leading, spacing: 6) {
                        Text(session.title).font(.headline)
                        Text(session.startedAt.formatted(date: .abbreviated, time: .shortened))
                            .font(.subheadline).foregroundStyle(.secondary)
                        Text("\(session.readings.count) temperature samples · \(session.healthSnapshots.count) Health snapshots")
                            .font(.caption).foregroundStyle(KrebbPalette.blush)
                    }
                    .padding(.vertical, 8)
                }
                .accessibilityIdentifier("savedSession")
                .listRowBackground(KrebbPalette.surface)
            }
        }
        .scrollContentBackground(.hidden)
        .background(KrebbPalette.canvas)
        .navigationTitle("Journal")
        .navigationBarTitleDisplayMode(.large)
    }
}

struct SessionDetailView: View {
    let session: MeasurementSession

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Text(session.isSimulated ? "Simulated temperature · saved locally" : "Sensor data · saved locally")
                    .foregroundStyle(KrebbPalette.blush)
                Text(session.startedAt.formatted(date: .abbreviated, time: .standard))
                if !session.note.isEmpty { Text(session.note) }
                SessionReadout(session: session)
                Text("Temperature trends are experimental observations, not a metabolic score.")
                    .font(.footnote).foregroundStyle(.secondary)
            }.padding(24)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle(session.title)
        .navigationBarTitleDisplayMode(.inline)
    }
}
