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

}
