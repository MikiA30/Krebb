import Foundation
import Combine
import HealthKit

struct HeartRateReading: Codable, Equatable, Sendable {
    let beatsPerMinute: Double
    let timestamp: Date
    let sourceName: String?

    var displayValue: String {
        "\(beatsPerMinute.formatted(.number.precision(.fractionLength(0)))) BPM"
    }
}

struct HealthContextSnapshot: Codable, Equatable, Sendable {
    let capturedAt: Date
    let heartRate: HeartRateReading?
    let stepCount: Double?

    /// Store this snapshot with a Krebb session; HealthKit remains the original source.
    var source: String = "HealthKit"
}

enum HealthContextStatus: Equatable {
    case notRequested
    case loading
    case available
    case unavailable
    case noRecentSamples
    case failed(String)

    var label: String {
        switch self {
        case .notRequested: "Not connected"
        case .loading: "Checking Health"
        case .available: "Available"
        case .unavailable: "Unavailable"
        case .noRecentSamples: "No recent sample"
        case .failed: "Could not load"
        }
    }

    var actionTitle: String {
        switch self {
        case .notRequested, .unavailable: "Allow Health access"
        case .loading: "Checking…"
        case .available, .noRecentSamples, .failed: "Refresh Apple Watch data"
        }
    }
}

@MainActor
final class HealthContextStore: ObservableObject {
    @Published private(set) var status: HealthContextStatus = .notRequested
    @Published private(set) var latestHeartRate: HeartRateReading?
    @Published private(set) var latestStepCount: Double?

    private let healthStore = HKHealthStore()
    private let recentSampleWindow: TimeInterval = 6 * 60 * 60

    var isLoading: Bool { status == .loading }

    func requestAccess() async {
        guard HKHealthStore.isHealthDataAvailable() else {
            status = .unavailable
            return
        }

        guard let heartRateType = HKQuantityType.quantityType(forIdentifier: .heartRate),
              let stepCountType = HKQuantityType.quantityType(forIdentifier: .stepCount) else {
            status = .unavailable
            return
        }

        do {
            try await healthStore.requestAuthorization(toShare: [], read: [heartRateType, stepCountType])
        } catch {
            latestHeartRate = nil
            latestStepCount = nil
            status = .failed(error.localizedDescription)
        }
    }

    func refresh() async {
        guard HKHealthStore.isHealthDataAvailable(),
              let heartRateType = HKQuantityType.quantityType(forIdentifier: .heartRate),
              let stepCountType = HKQuantityType.quantityType(forIdentifier: .stepCount) else {
            status = .unavailable
            return
        }

        status = .loading
        do {
            async let heartRateSample = latestSample(for: heartRateType)
            async let stepCountSample = latestSample(for: stepCountType)
            let (heartRate, steps) = try await (heartRateSample, stepCountSample)

            latestHeartRate = heartRate.map {
                HeartRateReading(
                    beatsPerMinute: $0.quantity.doubleValue(for: HKUnit.count().unitDivided(by: .minute())),
                    timestamp: $0.endDate,
                    sourceName: $0.sourceRevision.source.name
                )
            }
            latestStepCount = steps?.quantity.doubleValue(for: .count())
            status = latestHeartRate == nil && latestStepCount == nil ? .noRecentSamples : .available
        } catch {
            latestHeartRate = nil
            latestStepCount = nil
            status = .failed(error.localizedDescription)
        }
    }

    func snapshot() -> HealthContextSnapshot {
        HealthContextSnapshot(
            capturedAt: .now,
            heartRate: latestHeartRate.flatMap { Date.now.timeIntervalSince($0.timestamp) <= recentSampleWindow ? $0 : nil },
            stepCount: latestStepCount
        )
    }

    private func latestSample(for type: HKQuantityType) async throws -> HKQuantitySample? {
        try await withCheckedThrowingContinuation { continuation in
            let start = Date.now.addingTimeInterval(-recentSampleWindow)
            let query = HKSampleQuery(
                sampleType: type,
                predicate: HKQuery.predicateForSamples(withStart: start, end: .now),
                limit: 1,
                sortDescriptors: [NSSortDescriptor(key: HKSampleSortIdentifierEndDate, ascending: false)]
            ) { _, samples, error in
                if let error {
                    continuation.resume(throwing: error)
                } else {
                    continuation.resume(returning: samples?.first as? HKQuantitySample)
                }
            }
            healthStore.execute(query)
        }
    }
}
