import Foundation
import Combine

enum SessionStage: String, Codable { case baseline, observing, complete }

struct SessionReading: Codable, Identifiable, Equatable {
    var id = UUID()
    let recordedAt: Date
    var deviceTimestampMs: Int64?
    let skinTemperatureC: Double?
    let ambientTemperatureC: Double?
    let sensorQuality: Double?

    init(id: UUID = UUID(), recordedAt: Date, deviceTimestampMs: Int64? = nil,
         skinTemperatureC: Double?, ambientTemperatureC: Double?, sensorQuality: Double?) {
        self.id = id
        self.recordedAt = recordedAt
        self.deviceTimestampMs = deviceTimestampMs
        self.skinTemperatureC = skinTemperatureC
        self.ambientTemperatureC = ambientTemperatureC
        self.sensorQuality = sensorQuality
    }
}

struct MeasurementSession: Codable, Identifiable {
    var id = UUID()
    let startedAt: Date
    var stage: SessionStage = .baseline
    var observationStartedAt: Date?
    var endedAt: Date?
    // Simulation provenance belongs to the record, not just the current screen.
    let isSimulated: Bool
    var note = ""
    var readings: [SessionReading] = []
    var healthSnapshots: [HealthContextSnapshot] = []

    var baseline: Double? {
        let values = readings.filter {
            guard let start = observationStartedAt else { return true }
            return $0.recordedAt < start
        }.compactMap { $0.skinTemperatureC }
        guard !values.isEmpty else { return nil }
        return values.reduce(0, +) / Double(values.count)
    }

    var latestDelta: Double? {
        guard let baseline, let skin = readings.last?.skinTemperatureC else { return nil }
        return skin - baseline
    }

    var title: String {
        let trimmedNote = note.trimmingCharacters(in: .whitespacesAndNewlines)
        if !trimmedNote.isEmpty { return trimmedNote }
        return isSimulated ? "Simulation check" : "Sensor check"
    }

    var sourceLabel: String { isSimulated ? "simulation" : "sensor" }

    @MainActor var featureSummary: SessionFeatureSummary {
        SessionFeatureSummary(session: self)
    }

    @MainActor var export: SessionExport {
        SessionExport(session: self)
    }

    @MainActor var exportJSONString: String {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        encoder.dateEncodingStrategy = .millisecondsSince1970
        guard let data = try? encoder.encode(export),
              let json = String(data: data, encoding: .utf8) else { return "{}" }
        return json
    }
}

struct SessionFeatureSummary: Codable, Equatable {
    let baselineSampleCount: Int
    let observationSampleCount: Int
    let baselineSkinTemperatureC: Double?
    let latestDeltaSkinTemperatureC: Double?
    let peakDeltaSkinTemperatureC: Double?
    let temperatureAreaCelsiusSeconds: Double?
    let timeToPeakSeconds: Double?
    let averageSensorQuality: Double?
    let durationSeconds: Double?

    @MainActor init(session: MeasurementSession) {
        let observationStart = session.observationStartedAt
        let baselineReadings = session.readings.filter { reading in
            guard let observationStart else { return true }
            return reading.recordedAt < observationStart
        }
        let observationReadings = session.readings.filter { reading in
            guard let observationStart else { return false }
            return reading.recordedAt >= observationStart
        }
        let baseline = session.baseline
        let deltas = observationReadings.compactMap { reading -> (Date, Double)? in
            guard let baseline, let skin = reading.skinTemperatureC else { return nil }
            return (reading.recordedAt, skin - baseline)
        }
        let peak = deltas.max { $0.1 < $1.1 }
        let qualities = session.readings.compactMap(\.sensorQuality)

        baselineSampleCount = baselineReadings.compactMap(\.skinTemperatureC).count
        observationSampleCount = observationReadings.compactMap(\.skinTemperatureC).count
        baselineSkinTemperatureC = baseline
        latestDeltaSkinTemperatureC = session.latestDelta
        peakDeltaSkinTemperatureC = peak?.1
        temperatureAreaCelsiusSeconds = Self.areaUnderCurve(deltas)
        timeToPeakSeconds = peak.flatMap { peak in observationStart.map { peak.0.timeIntervalSince($0) } }
        averageSensorQuality = qualities.isEmpty ? nil : qualities.reduce(0, +) / Double(qualities.count)
        durationSeconds = session.endedAt.map { $0.timeIntervalSince(session.startedAt) }
    }

    private static func areaUnderCurve(_ points: [(Date, Double)]) -> Double? {
        guard points.count >= 2 else { return nil }
        return zip(points, points.dropFirst()).reduce(0) { total, pair in
            let seconds = pair.1.0.timeIntervalSince(pair.0.0)
            return total + ((pair.0.1 + pair.1.1) / 2) * seconds
        }
    }
}

struct SessionExport: Codable, Equatable {
    let sessionId: String
    let source: String
    let note: String
    let startedAtMs: Int64
    let observationStartedAtMs: Int64?
    let endedAtMs: Int64?
    let baselineSkinTemperatureC: Double?
    let deviceSamples: [DeviceSampleExport]
    let healthSamples: [HealthSampleExport]
    let features: SessionFeatureSummary

    @MainActor init(session: MeasurementSession) {
        sessionId = session.id.uuidString
        source = session.sourceLabel
        note = session.note
        startedAtMs = session.startedAt.millisecondsSince1970
        observationStartedAtMs = session.observationStartedAt?.millisecondsSince1970
        endedAtMs = session.endedAt?.millisecondsSince1970
        baselineSkinTemperatureC = session.baseline
        deviceSamples = session.readings.map { DeviceSampleExport(reading: $0, observationStartedAt: session.observationStartedAt) }
        healthSamples = session.healthSnapshots.map { HealthSampleExport(snapshot: $0) }
        features = session.featureSummary
    }
}

struct DeviceSampleExport: Codable, Equatable {
    let recordedAtMs: Int64
    let deviceTimestampMs: Int64?
    let phase: String
    let skinTemperatureC: Double?
    let ambientTemperatureC: Double?
    let sensorQuality: Double?

    init(reading: SessionReading, observationStartedAt: Date?) {
        recordedAtMs = reading.recordedAt.millisecondsSince1970
        deviceTimestampMs = reading.deviceTimestampMs
        phase = observationStartedAt.map { reading.recordedAt < $0 ? "baseline" : "observation" } ?? "baseline"
        skinTemperatureC = reading.skinTemperatureC
        ambientTemperatureC = reading.ambientTemperatureC
        sensorQuality = reading.sensorQuality
    }
}

struct HealthSampleExport: Codable, Equatable {
    let capturedAtMs: Int64
    let source: String
    let heartRateBpm: Double?
    let heartRateRecordedAtMs: Int64?
    let heartRateSource: String?
    let stepCount: Double?

    init(snapshot: HealthContextSnapshot) {
        capturedAtMs = snapshot.capturedAt.millisecondsSince1970
        source = snapshot.source
        heartRateBpm = snapshot.heartRate?.beatsPerMinute
        heartRateRecordedAtMs = snapshot.heartRate?.timestamp.millisecondsSince1970
        heartRateSource = snapshot.heartRate?.sourceName
        stepCount = snapshot.stepCount
    }
}

private extension Date {
    var millisecondsSince1970: Int64 {
        Int64((timeIntervalSince1970 * 1000).rounded())
    }
}

/// One atomic archive preserves both completed sessions and the in-progress draft.
/// Files stay inside the app container and are excluded from cloud device backup.
struct SessionArchive: Codable {
    var version = 1
    var draft: MeasurementSession?
    var completed: [MeasurementSession] = []
}

@MainActor
final class SessionStore: ObservableObject {
    @Published private(set) var archive = SessionArchive()
    @Published private(set) var errorMessage: String?
    private let fileURL: URL
    private var canWrite = true

    var active: MeasurementSession? { archive.draft }
    var completed: [MeasurementSession] { archive.completed }

    init(directory: URL? = nil) {
        var selectedDirectory = directory
        #if DEBUG
        if selectedDirectory == nil,
           let testID = ProcessInfo.processInfo.environment["KREBB_UI_TEST_STORE"], UUID(uuidString: testID) != nil {
            selectedDirectory = FileManager.default.temporaryDirectory.appendingPathComponent(testID)
        }
        #endif
        let root = selectedDirectory ?? FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("KrebbSessions", isDirectory: true)
        fileURL = root.appendingPathComponent("sessions.json")
        do {
            try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
            var excludedRoot = root
            var values = URLResourceValues()
            values.isExcludedFromBackup = true
            try excludedRoot.setResourceValues(values)
            if FileManager.default.fileExists(atPath: fileURL.path) {
                archive = try JSONDecoder().decode(SessionArchive.self, from: Data(contentsOf: fileURL))
                guard archive.version == 1 else { throw CocoaError(.fileReadCorruptFile) }
            }
        } catch {
            // Never overwrite an unreadable archive with an empty one.
            canWrite = false
            errorMessage = "Saved sessions could not be opened. Your file has been preserved. \(error.localizedDescription)"
        }
    }

    @discardableResult
    private func commit(_ next: SessionArchive) -> Bool {
        guard canWrite else { return false }
        do {
            let data = try JSONEncoder().encode(next)
            try data.write(to: fileURL, options: [.atomic, .completeFileProtection])
            archive = next
            errorMessage = nil
            return true
        } catch {
            errorMessage = "Could not save this change. Please try again. \(error.localizedDescription)"
            return false
        }
    }

    func startSimulation(at date: Date = .now) {
        guard active == nil else { return }
        var next = archive
        next.draft = MeasurementSession(startedAt: date, isSimulated: true)
        _ = commit(next)
    }

    func startSensorSession(at date: Date = .now) {
        guard active == nil else { return }
        var next = archive
        next.draft = MeasurementSession(startedAt: date, isSimulated: false)
        _ = commit(next)
    }

    func record(_ reading: SessionReading) {
        guard var draft = active, reading.recordedAt >= draft.startedAt,
              draft.readings.last.map({ reading.recordedAt > $0.recordedAt }) ?? true else { return }
        let numbers = [reading.skinTemperatureC, reading.ambientTemperatureC, reading.sensorQuality].compactMap { $0 }
        guard numbers.allSatisfy({ $0.isFinite }),
              reading.sensorQuality.map({ (0...1).contains($0) }) ?? true else { return }
        draft.readings.append(reading)
        var next = archive
        next.draft = draft
        _ = commit(next)
    }

    func simulateTick(at date: Date = .now) {
        guard let draft = active, draft.isSimulated else { return }
        let elapsed = date.timeIntervalSince(draft.startedAt)
        let observation = draft.observationStartedAt.map { max(0, date.timeIntervalSince($0)) } ?? 0
        record(SessionReading(recordedAt: date,
                              skinTemperatureC: 33.1 + 0.015 * sin(elapsed / 3) + 0.3 * (1 - exp(-observation / 25)),
                              ambientTemperatureC: 23.6 + 0.02 * sin(elapsed / 15), sensorQuality: 0.95))
    }

    func beginObservation(note: String, at date: Date = .now) {
        guard var draft = active, draft.stage == .baseline,
              draft.readings.filter({ $0.skinTemperatureC != nil }).count >= 3,
              let last = draft.readings.last, date > last.recordedAt else { return }
        draft.observationStartedAt = date
        draft.stage = .observing
        draft.note = note
        var next = archive
        next.draft = draft
        _ = commit(next)
    }

    @discardableResult
    func discardDraft() -> Bool {
        var next = archive
        next.draft = nil
        return commit(next)
    }

    func updateNote(_ note: String) {
        guard var draft = active, draft.note != note else { return }
        draft.note = note
        var next = archive
        next.draft = draft
        _ = commit(next)
    }

    func attachHealth(_ snapshot: HealthContextSnapshot) {
        guard var draft = active, snapshot.heartRate != nil || snapshot.stepCount != nil else { return }
        // Repeated refreshes of the same health values must not inflate the session.
        if let last = draft.healthSnapshots.last,
           last.heartRate == snapshot.heartRate, last.stepCount == snapshot.stepCount { return }
        draft.healthSnapshots.append(snapshot)
        var next = archive
        next.draft = draft
        _ = commit(next)
    }

    @discardableResult
    func finish(at date: Date = .now) -> Bool {
        guard var draft = active, draft.stage == .observing,
              let observationStart = draft.observationStartedAt,
              draft.readings.contains(where: { $0.recordedAt >= observationStart }),
              date >= (draft.readings.last?.recordedAt ?? draft.startedAt) else { return false }
        draft.endedAt = date
        draft.stage = .complete
        var next = archive
        next.completed.insert(draft, at: 0)
        next.draft = nil
        return commit(next)
    }
}
