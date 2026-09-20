import SwiftUI
import Charts

struct CheckView: View {
    @ObservedObject var sessions: SessionStore
    @ObservedObject var healthContext: HealthContextStore
    @ObservedObject var sensorConnection: SensorConnectionStore
    @State private var showsSensors = false
    @State private var note = ""
    @State private var justFinished = false
    @State private var showsDiscardConfirmation = false
    @FocusState private var noteIsFocused: Bool

    private var title: String {
        guard let active = sessions.active else { return justFinished ? "A check, complete." : "A moment to settle." }
        return active.stage == .baseline ? "Finding your starting point." : "Follow the change."
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Label(activeModeLabel, systemImage: activeModeIcon)
                    .font(.subheadline).foregroundStyle(KrebbPalette.blush)
                Text(title).font(.largeTitle.weight(.semibold))
                    .accessibilityIdentifier("checkPhaseTitle")
                if let active = sessions.active {
                    Text(active.stage == .baseline
                         ? "Hold still while Krebb collects three steady readings. Then name what you are observing."
                         : "The curve compares readings with your saved baseline. Finish when you’re ready.")
                        .foregroundStyle(.secondary)
                    SessionStatusBadge(session: active)
                    SessionReadout(session: active)
                    if active.stage == .baseline {
                        BaselineGuidanceView(validSamples: validBaselineSamples(active),
                                             canBeginObservation: canBeginObservation)
                        VStack(alignment: .leading, spacing: 8) {
                            Text("What are you observing?")
                                .font(.headline)
                            LabelSuggestionChips(note: $note)
                            TextField("Breakfast, coffee, walk, no meal...", text: $note)
                                .textFieldStyle(.roundedBorder)
                                .focused($noteIsFocused)
                                .submitLabel(.done)
                                .onSubmit { beginObservation() }
                                .accessibilityIdentifier("sessionNote")
                            Text("This label becomes the saved Journal title.")
                                .font(.caption).foregroundStyle(.secondary)
                        }
                    }
                    Button(primaryActionTitle(active)) {
                        noteIsFocused = false
                        if active.stage == .baseline {
                            beginObservation()
                        } else {
                            sessions.attachHealth(healthContext.snapshot())
                            justFinished = sessions.finish()
                            if justFinished { note = "" }
                        }
                    }
                    .buttonStyle(.glassProminent)
                    .disabled(active.stage == .baseline ? !canBeginObservation : !hasObservation(active))
                    .accessibilityIdentifier("advanceCheck")
                    Button("Attach latest Health reading", systemImage: "heart") {
                        sessions.attachHealth(healthContext.snapshot())
                    }
                    .disabled(healthContext.latestHeartRate == nil && healthContext.latestStepCount == nil)
                    Button("Discard check", role: .destructive) {
                        noteIsFocused = false
                        showsDiscardConfirmation = true
                    }
                    if !active.isSimulated {
                        Text(sensorConnection.state.label)
                            .font(.footnote).foregroundStyle(.secondary)
                    } else if sensorConnection.packetCount > 0 {
                        Text("Live sensor packets are available. Discard this simulation if you want to start a live BLE check.")
                            .font(.footnote).foregroundStyle(KrebbPalette.blush)
                    }
                    Text("Health context is optional. Open Sensors to allow access or refresh readings. Attached values retain their original measurement times.")
                        .font(.footnote).foregroundStyle(.secondary)
                    Text(active.isSimulated
                         ? "This draft saves after each sample. Simulation runs while the app is active; time away leaves a gap in the recording."
                         : "This draft saves each sensor packet received by the phone. Device timestamp is retained, but the chart uses iPhone arrival time.")
                        .font(.footnote).foregroundStyle(.secondary)
                } else {
                    Text(justFinished
                         ? "Saved on this iPhone. Open Journal to review your check and any attached Health context."
                         : sensorConnection.packetCount > 0
                         ? "Live packets are ready. Start a sensor check, or use simulation only as a fallback."
                         : "Connect Krebb One when the ESP32 is powered, or run the simulation if hardware is still being checked.")
                        .foregroundStyle(.secondary)
                    sensorControls
                    simulationButton
                }
                if let error = sessions.errorMessage {
                    Text(error).foregroundStyle(.red).accessibilityIdentifier("sessionError")
                }
            }
            .padding(24)
        }
        .background(KrebbBackground())
        .navigationTitle("Check")
        .navigationBarTitleDisplayMode(.large)
        .toolbar { KrebbToolbar(showsSensors: $showsSensors) }
        .sheet(isPresented: $showsSensors) { SensorSheet(healthContext: healthContext, sensorConnection: sensorConnection) }
        .confirmationDialog("Discard this unfinished check?", isPresented: $showsDiscardConfirmation, titleVisibility: .visible) {
            Button("Discard check", role: .destructive) {
                if sessions.discardDraft() {
                    note = ""
                    justFinished = false
                }
            }
        } message: {
            Text("This removes the current note and readings. Your saved Journal checks are kept.")
        }
        .onAppear { syncNoteFromActiveSession() }
        .onChange(of: sessions.active?.id) { _, _ in syncNoteFromActiveSession() }
        .onChange(of: sessions.active?.stage) { _, _ in syncNoteFromActiveSession() }
        .onChange(of: note) { _, value in sessions.updateNote(value) }
    }

    private func syncNoteFromActiveSession() {
        guard let active = sessions.active else {
            note = ""
            noteIsFocused = false
            return
        }
        note = active.note
    }

    private var canBeginObservation: Bool {
        guard let active = sessions.active, active.stage == .baseline else { return false }
        return validBaselineSamples(active) >= 3
    }

    private func beginObservation() {
        noteIsFocused = false
        guard canBeginObservation else { return }
        sessions.beginObservation(note: note)
    }

    private func validBaselineSamples(_ session: MeasurementSession) -> Int {
        guard session.stage == .baseline else { return 0 }
        return session.readings.filter { $0.skinTemperatureC != nil }.count
    }

    private func primaryActionTitle(_ session: MeasurementSession) -> String {
        if session.stage == .observing { return "Finish and save" }
        let trimmedNote = note.trimmingCharacters(in: .whitespacesAndNewlines)
        return trimmedNote.isEmpty ? "Begin observation" : "Begin observing \(trimmedNote)"
    }

    private var activeModeLabel: String {
        guard let active = sessions.active else { return sensorConnection.state.isReceiving ? "Krebb One ready" : "Sensor or simulation" }
        return active.isSimulated ? "Simulated temperature" : "Krebb One sensor"
    }

    private var activeModeIcon: String {
        guard let active = sessions.active else { return sensorConnection.state.isReceiving ? "sensor.tag.radiowaves.forward.fill" : "sensor.tag.radiowaves.forward" }
        return active.isSimulated ? "play.circle" : "sensor.tag.radiowaves.forward.fill"
    }

    private var sensorControls: some View {
        VStack(alignment: .leading, spacing: 12) {
            LabeledContent("Krebb One", value: sensorConnection.state.label)
            if let sensor = sensorConnection.discoveredSensor {
                LabeledContent("Detected BLE device", value: "\(sensor.name) · \(sensor.rssi) dBm")
            }
            if sensorConnection.packetCount > 0 {
                LabeledContent("Packets received", value: "\(sensorConnection.packetCount)")
            } else if sensorConnection.state.isReceiving {
                Text("Connected, waiting for the first measurement packet.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            HStack {
                Button(sensorActionTitle, systemImage: sensorActionIcon) {
                    if sensorConnection.packetCount > 0 {
                        justFinished = false
                        note = ""
                        sessions.startSensorSession()
                        sessions.attachHealth(healthContext.snapshot())
                    } else if sensorConnection.state == .discovered {
                        sensorConnection.connectToDiscoveredSensor()
                    } else {
                        sensorConnection.start()
                    }
                }
                .buttonStyle(.bordered)
                .disabled(sensorActionDisabled)

                if !sensorConnection.state.isReceiving {
                    Button("Sensors", systemImage: "slider.horizontal.3") { showsSensors = true }
                        .buttonStyle(.bordered)
                }
            }
            if let error = sensorConnection.lastError {
                Text(error).font(.footnote).foregroundStyle(.red)
            }
        }
        .padding(16)
        .krebbPanel(cornerRadius: 22, hot: sensorConnection.packetCount > 0)
    }

    @ViewBuilder
    private var simulationButton: some View {
        let title = justFinished ? "Start another simulation" : "Start simulation"
        if sensorConnection.packetCount > 0 {
            Button(title, systemImage: "play.circle") { startSimulation() }
                .buttonStyle(.bordered)
                .accessibilityIdentifier("advanceCheck")
        } else {
            Button(title, systemImage: "play.circle") { startSimulation() }
                .buttonStyle(.glassProminent)
                .accessibilityIdentifier("advanceCheck")
        }
    }

    private func startSimulation() {
        justFinished = false
        note = ""
        sessions.startSimulation()
        sessions.simulateTick()
        sessions.attachHealth(healthContext.snapshot())
    }

    private var sensorActionTitle: String {
        if sensorConnection.packetCount > 0 { return "Start live sensor check" }
        if sensorConnection.state == .discovered { return "Connect to this Krebb One" }
        if sensorConnection.state == .connecting || sensorConnection.state == .connected || sensorConnection.state.isReceiving {
            return "Waiting for packets"
        }
        return "Scan for Krebb One"
    }

    private var sensorActionIcon: String {
        if sensorConnection.packetCount > 0 { return "record.circle" }
        if sensorConnection.state == .discovered { return "link" }
        return "antenna.radiowaves.left.and.right"
    }

    private var sensorActionDisabled: Bool {
        if sensorConnection.packetCount > 0 { return sessions.active != nil }
        if sensorConnection.state == .discovered { return false }
        return sensorConnection.state == .connecting || sensorConnection.state == .connected || sensorConnection.state.isReceiving
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
            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: 6) {
                    Text(session.stage == .baseline ? "Building baseline" : "Response trend")
                        .font(.headline)
                    Text(session.isSimulated ? "Simulation" : "Live sensor")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(KrebbPalette.blush)
                }
                Spacer()
                if let delta = session.latestDelta {
                    Text(delta, format: .number.sign(strategy: .always()).precision(.fractionLength(2)))
                        .font(.title2.weight(.semibold).monospacedDigit())
                        .foregroundStyle(KrebbPalette.coral)
                    Text("°C")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(.secondary)
                }
            }
            SessionChart(session: session).frame(height: 200)
            Grid(alignment: .leading, horizontalSpacing: 16, verticalSpacing: 10) {
                GridRow {
                    metric("Samples", "\(session.readings.count)")
                    metric("Skin", session.readings.last?.skinTemperatureC.map(temperature) ?? "--")
                }
                GridRow {
                    metric("Room", session.readings.last?.ambientTemperatureC.map(temperature) ?? "--")
                    metric(session.stage == .baseline ? "Baseline so far" : "Baseline", session.baseline.map(temperature) ?? "--")
                }
            }
            if let reading = session.healthSnapshots.last?.heartRate {
                LabeledContent("Attached heart rate", value: reading.displayValue)
                Text("Recorded \(reading.timestamp.formatted(date: .abbreviated, time: .standard)) · \(reading.sourceName ?? "HealthKit")")
                    .font(.caption).foregroundStyle(.secondary)
            }
            Text("\(session.healthSnapshots.count) Health snapshots attached")
                .font(.caption).foregroundStyle(.secondary)
        }
        .padding(18)
        .krebbPanel(cornerRadius: 26, hot: !session.isSimulated)
    }

    private func temperature(_ value: Double) -> String {
        "\(value.formatted(.number.precision(.fractionLength(2)))) °C"
    }

    private func metric(_ title: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(title)
                .font(.caption)
                .foregroundStyle(.secondary)
            Text(value)
                .font(.subheadline.weight(.medium).monospacedDigit())
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

struct SessionStatusBadge: View {
    let session: MeasurementSession

    var body: some View {
        Label(session.isSimulated ? "Simulation check" : "Live sensor check",
              systemImage: session.isSimulated ? "play.circle" : "sensor.tag.radiowaves.forward.fill")
            .font(.caption.weight(.semibold))
            .foregroundStyle(session.isSimulated ? .secondary : KrebbPalette.blush)
            .padding(.vertical, 6)
            .padding(.horizontal, 10)
            .background(KrebbPalette.surface, in: Capsule())
            .accessibilityIdentifier(session.isSimulated ? "simulationBadge" : "liveSensorBadge")
    }
}

struct BaselineGuidanceView: View {
    let validSamples: Int
    let canBeginObservation: Bool

    private var remaining: Int { max(0, 3 - validSamples) }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Label("Hold still for 3 readings", systemImage: canBeginObservation ? "checkmark.circle.fill" : "hand.raised")
                    .font(.headline)
                Spacer()
                Text("\(min(validSamples, 3))/3")
                    .font(.headline.monospacedDigit())
                    .foregroundStyle(canBeginObservation ? KrebbPalette.blush : .secondary)
            }
            ProgressView(value: min(Double(validSamples), 3), total: 3)
                .tint(KrebbPalette.coral)
            Text(canBeginObservation
                 ? "Baseline is ready. Name the food or event, then begin observation."
                 : "\(remaining) more steady \(remaining == 1 ? "reading" : "readings") needed before observation.")
                .font(.footnote).foregroundStyle(.secondary)
        }
        .padding(16)
        .krebbPanel(cornerRadius: 20, hot: canBeginObservation)
    }
}

struct LabelSuggestionChips: View {
    @Binding var note: String

    private let suggestions = ["Breakfast", "Lunch", "Dinner", "Snack", "Coffee", "No meal"]

    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(suggestions, id: \.self) { suggestion in
                    Button(suggestion) { note = suggestion }
                        .font(.caption.weight(.semibold))
                        .buttonStyle(.bordered)
                        .tint(note == suggestion ? KrebbPalette.coral : .secondary)
                }
            }
        }
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
                        .interpolationMethod(.catmullRom)
                }
            }
            if let latest = session.readings.last, let skin = latest.skinTemperatureC {
                PointMark(x: .value("Latest time", latest.recordedAt), y: .value("Latest skin °C", skin))
                    .foregroundStyle(KrebbPalette.blush)
                    .symbolSize(45)
            }
            if let start = session.observationStartedAt {
                RuleMark(x: .value("Observation", start))
                    .foregroundStyle(KrebbPalette.blush)
                    .lineStyle(StrokeStyle(dash: [4]))
            }
        }
        .chartYScale(domain: .automatic(includesZero: false))
        .chartPlotStyle { plotArea in
            plotArea
                .background(.white.opacity(0.025), in: RoundedRectangle(cornerRadius: 14))
        }
        .accessibilityLabel(session.isSimulated ? "Recorded simulated skin temperatures" : "Recorded skin temperatures")
    }
}

struct JournalRow: View {
    let session: MeasurementSession

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            HStack {
                Label(session.isSimulated ? "Simulation" : "Live sensor",
                      systemImage: session.isSimulated ? "play.circle" : "sensor.tag.radiowaves.forward.fill")
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(KrebbPalette.blush)
                Spacer()
                Text(session.startedAt.formatted(date: .omitted, time: .shortened))
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Text(session.title)
                .font(.title2.weight(.semibold))
                .lineLimit(2)
            HStack(spacing: 14) {
                miniMetric("Samples", "\(session.readings.count)")
                if let delta = session.latestDelta {
                    miniMetric("Change", "\(delta.formatted(.number.sign(strategy: .always()).precision(.fractionLength(2)))) °C")
                }
                miniMetric("Health", "\(session.healthSnapshots.count)")
            }
        }
        .padding(18)
        .krebbPanel(cornerRadius: 24, hot: !session.isSimulated)
    }

    private func miniMetric(_ title: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(title)
                .font(.caption2)
                .foregroundStyle(.secondary)
            Text(value)
                .font(.caption.weight(.semibold).monospacedDigit())
        }
        .frame(maxWidth: .infinity, alignment: .leading)
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
                    JournalRow(session: session)
                }
                .accessibilityIdentifier("savedSession")
                .listRowBackground(Color.clear)
                .listRowSeparator(.hidden)
            }
            .onDelete { offsets in
                sessions.deleteCompletedSessions(at: offsets)
            }
        }
        .scrollContentBackground(.hidden)
        .background(KrebbBackground())
        .navigationTitle("Journal")
        .navigationBarTitleDisplayMode(.large)
        .toolbar {
            if !sessions.completed.isEmpty {
                EditButton()
            }
        }
    }
}

struct SessionDetailView: View {
    let session: MeasurementSession
    @State private var showsDeveloperExport = false
    @State private var showsExport = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Text(session.isSimulated ? "Simulated temperature · saved locally" : "Sensor data · saved locally")
                    .foregroundStyle(KrebbPalette.blush)
                Text(session.startedAt.formatted(date: .abbreviated, time: .standard))
                if !session.note.isEmpty { Text(session.note) }
                SessionReadout(session: session)
                FeatureSummaryView(summary: session.featureSummary)
#if DEBUG
                DisclosureGroup("Developer export", isExpanded: $showsDeveloperExport) {
                    VStack(alignment: .leading, spacing: 12) {
                        Text("This is the session package we can hand to the Python pipeline later. It stays local unless you share it.")
                            .font(.footnote).foregroundStyle(.secondary)
                        HStack {
                            ShareLink(item: session.exportJSONString,
                                      subject: Text("Krebb session \(session.title)")) {
                                Label("Share JSON", systemImage: "square.and.arrow.up")
                            }
                            .buttonStyle(.bordered)
                            Button(showsExport ? "Hide preview" : "Preview JSON", systemImage: "doc.text.magnifyingglass") {
                                showsExport.toggle()
                            }
                            .buttonStyle(.bordered)
                        }
                        if showsExport {
                            ScrollView(.horizontal) {
                                Text(session.exportJSONString)
                                    .font(.caption.monospaced())
                                    .textSelection(.enabled)
                                    .padding(12)
                            }
                            .background(KrebbPalette.surface, in: RoundedRectangle(cornerRadius: 12))
                        }
                    }
                }
                .font(.subheadline.weight(.medium))
#endif
                Text("Temperature trends are experimental observations, not a metabolic score.")
                    .font(.footnote).foregroundStyle(.secondary)
            }.padding(24)
        }
        .background(KrebbBackground())
        .navigationTitle(session.title)
        .navigationBarTitleDisplayMode(.inline)
    }
}

struct FeatureSummaryView: View {
    let summary: SessionFeatureSummary

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Response features").font(.headline)
            Grid(alignment: .leading, horizontalSpacing: 18, verticalSpacing: 12) {
                GridRow {
                    feature("Baseline samples", "\(summary.baselineSampleCount)")
                    feature("Observation samples", "\(summary.observationSampleCount)")
                }
                GridRow {
                    feature("Baseline skin", temperature(summary.baselineSkinTemperatureC))
                    feature("Latest change", signedTemperature(summary.latestDeltaSkinTemperatureC))
                }
                GridRow {
                    feature("Peak change", signedTemperature(summary.peakDeltaSkinTemperatureC))
                    feature("Time to peak", seconds(summary.timeToPeakSeconds))
                }
                GridRow {
                    feature("Temp AUC", auc(summary.temperatureAreaCelsiusSeconds))
                    feature("Avg quality", quality(summary.averageSensorQuality))
                }
            }
        }
        .padding(16)
        .background(KrebbPalette.surface, in: RoundedRectangle(cornerRadius: 16))
    }

    private func feature(_ title: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(title).font(.caption).foregroundStyle(.secondary)
            Text(value).font(.body.weight(.medium).monospacedDigit())
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func temperature(_ value: Double?) -> String {
        guard let value else { return "—" }
        return "\(value.formatted(.number.precision(.fractionLength(2)))) °C"
    }

    private func signedTemperature(_ value: Double?) -> String {
        guard let value else { return "—" }
        return "\(value.formatted(.number.sign(strategy: .always()).precision(.fractionLength(2)))) °C"
    }

    private func seconds(_ value: Double?) -> String {
        guard let value else { return "—" }
        return "\(value.formatted(.number.precision(.fractionLength(0)))) s"
    }

    private func auc(_ value: Double?) -> String {
        guard let value else { return "—" }
        return "\(value.formatted(.number.precision(.fractionLength(1)))) °C·s"
    }

    private func quality(_ value: Double?) -> String {
        guard let value else { return "—" }
        return value.formatted(.number.precision(.fractionLength(2)))
    }
}
