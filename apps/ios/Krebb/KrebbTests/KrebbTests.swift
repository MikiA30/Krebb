//
//  KrebbTests.swift
//  KrebbTests
//
//  Created by Mikiyas Asmamaw on 9/19/26.
//

import Foundation
import Testing
@testable import Krebb

struct KrebbTests {

    @Test @MainActor func sessionSurvivesReloadAndFreezesBaseline() throws {
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: directory) }
        let start = Date(timeIntervalSince1970: 1_760_000_000)
        let store = SessionStore(directory: directory)
        store.startSimulation(at: start)
        for index in 0..<3 {
            store.record(SessionReading(recordedAt: start.addingTimeInterval(Double(index)),
                                        skinTemperatureC: 33, ambientTemperatureC: 24, sensorQuality: 1))
        }
        let restored = SessionStore(directory: directory)
        #expect(restored.active?.readings.count == 3)
        restored.beginObservation(note: "No meal", at: start.addingTimeInterval(3))
        #expect(!restored.finish(at: start.addingTimeInterval(3)))
        restored.record(SessionReading(recordedAt: start.addingTimeInterval(4),
                                       skinTemperatureC: 34, ambientTemperatureC: 24, sensorQuality: 1))
        let heart = HeartRateReading(beatsPerMinute: 72, timestamp: start.addingTimeInterval(-60), sourceName: "Watch")
        let snapshot = HealthContextSnapshot(capturedAt: start, heartRate: heart, stepCount: nil)
        restored.attachHealth(snapshot)
        restored.attachHealth(snapshot)
        #expect(restored.finish(at: start.addingTimeInterval(5)))
        #expect(!restored.finish(at: start.addingTimeInterval(6)))
        let saved = SessionStore(directory: directory)
        #expect(saved.active == nil)
        #expect(saved.completed.count == 1)
        let session = try #require(saved.completed.first)
        #expect(session.baseline == 33)
        #expect(session.latestDelta == 1)
        #expect(session.note == "No meal")
        #expect(session.isSimulated)
        #expect(session.healthSnapshots.count == 1)
        #expect(session.healthSnapshots.first?.heartRate?.timestamp == heart.timestamp)
        saved.startSimulation(at: start.addingTimeInterval(10))
        saved.updateNote("Breakfast")
        #expect(saved.discardDraft())
        let afterDiscard = SessionStore(directory: directory)
        #expect(afterDiscard.active == nil)
        #expect(afterDiscard.completed.count == 1)
        afterDiscard.startSimulation(at: start.addingTimeInterval(11))
        #expect(afterDiscard.active?.note == "")
    }

    @Test @MainActor func missingAndInvalidSamplesDoNotBecomeBaseline() throws {
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: directory) }
        let start = Date.now
        let store = SessionStore(directory: directory)
        store.startSimulation(at: start)
        for index in 0..<3 {
            store.record(SessionReading(recordedAt: start.addingTimeInterval(Double(index)),
                                        skinTemperatureC: nil, ambientTemperatureC: 24, sensorQuality: nil))
        }
        store.record(SessionReading(recordedAt: start.addingTimeInterval(4),
                                    skinTemperatureC: .nan, ambientTemperatureC: 24, sensorQuality: 1))
        store.beginObservation(note: "", at: start.addingTimeInterval(5))
        #expect(store.active?.stage == .baseline)
        #expect(store.active?.baseline == nil)
        #expect(store.active?.readings.count == 3)
    }

    @Test @MainActor func unreadableArchiveIsPreserved() throws {
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: directory) }
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let file = directory.appendingPathComponent("sessions.json")
        let original = Data("unreadable archive".utf8)
        try original.write(to: file)
        let store = SessionStore(directory: directory)
        store.startSimulation()
        #expect(store.errorMessage != nil)
        #expect(store.active == nil)
        #expect(try Data(contentsOf: file) == original)
    }

    @Test @MainActor func failedSaveKeepsDraftInMemory() throws {
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: directory) }
        let store = SessionStore(directory: directory)
        store.startSimulation()
        let id = store.active?.id
        // Removing only this test's temporary directory forces the next atomic write to fail.
        try FileManager.default.removeItem(at: directory)
        store.simulateTick()
        #expect(store.errorMessage != nil)
        #expect(store.active?.id == id)
        #expect(store.active?.readings.isEmpty == true)
    }

    @Test func healthSnapshotPreservesMeasurementContext() {
        let timestamp = Date(timeIntervalSince1970: 1_760_000_000)
        let heartRate = HeartRateReading(
            beatsPerMinute: 72.4,
            timestamp: timestamp,
            sourceName: "Apple Watch"
        )
        let snapshot = HealthContextSnapshot(
            capturedAt: timestamp,
            heartRate: heartRate,
            stepCount: 18
        )

        #expect(snapshot.source == "HealthKit")
        #expect(snapshot.heartRate?.beatsPerMinute == 72.4)
        #expect(snapshot.heartRate?.timestamp == timestamp)
        #expect(snapshot.heartRate?.sourceName == "Apple Watch")
        #expect(snapshot.stepCount == 18)
    }

    @Test @MainActor func sensorPacketRecordsWithPhoneArrivalTime() throws {
        let json = Data("""
        {"timestampMs":1760000000000,"skinTemperatureC":33.42,"ambientTemperatureC":24.0,"sensorQuality":0.91}
        """.utf8)
        let packet = try JSONDecoder().decode(SensorMeasurementPacket.self, from: json)
        #expect(packet.timestampMs == 1_760_000_000_000)
        #expect(packet.skinTemperatureC == 33.42)
        #expect(packet.ambientTemperatureC == 24.0)
        #expect(packet.sensorQuality == 0.91)

        let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: directory) }
        let arrival = Date(timeIntervalSince1970: 1_760_000_123)
        let store = SessionStore(directory: directory)
        store.startSensorSession(at: arrival)
        store.record(SessionReading(recordedAt: arrival,
                                    deviceTimestampMs: packet.timestampMs,
                                    skinTemperatureC: packet.skinTemperatureC,
                                    ambientTemperatureC: packet.ambientTemperatureC,
                                    sensorQuality: packet.sensorQuality))
        #expect(store.active?.isSimulated == false)
        #expect(store.active?.readings.first?.recordedAt == arrival)
        #expect(store.active?.readings.first?.deviceTimestampMs == packet.timestampMs)
    }

}
