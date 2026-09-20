import Foundation

struct KrebbCoachInsight: Equatable {
    let responseClass: String
    let headline: String
    let explanation: String
    let calibrationNote: String
    let claimBoundary: String
    let knownCalories: Int?
}

enum KrebbCoach {
    static func insight(for session: MeasurementSession) -> KrebbCoachInsight {
        let summary = session.featureSummary
        let knownCalories = knownCalories(from: session.note)
        let responseClass = classify(summary: summary)
        let headline = headline(for: responseClass, knownCalories: knownCalories)
        let explanation = explanation(for: responseClass, summary: summary)
        let calibrationNote = calibrationNote(for: session, knownCalories: knownCalories)

        return KrebbCoachInsight(
            responseClass: responseClass,
            headline: headline,
            explanation: explanation,
            calibrationNote: calibrationNote,
            claimBoundary: "Experimental response guidance only. Krebb is not yet a validated calorie estimator or medical device.",
            knownCalories: knownCalories
        )
    }

    private static func classify(summary: SessionFeatureSummary) -> String {
        guard let peak = summary.peakDeltaSkinTemperatureC else { return "Needs more data" }
        if let quality = summary.averageSensorQuality, quality < 0.45 { return "Low confidence" }
        if peak < 0.35, (summary.temperatureAreaCelsiusSeconds ?? 0) < 200 { return "Control-like" }
        if peak < 0.8 { return "Small early response" }
        if peak < 1.5 { return "Moderate early response" }
        return "High early response"
    }

    private static func headline(for responseClass: String, knownCalories: Int?) -> String {
        if let knownCalories {
            return "Krebb saw a \(responseClass.lowercased()) after ~\(knownCalories) calories."
        }
        return "Krebb saw a \(responseClass.lowercased())."
    }

    private static func explanation(for responseClass: String, summary: SessionFeatureSummary) -> String {
        let peak = signedTemperature(summary.peakDeltaSkinTemperatureC)
        let latest = signedTemperature(summary.latestDeltaSkinTemperatureC)
        let samples = summary.observationSampleCount
        let quality = summary.averageSensorQuality.map { $0.formatted(.number.precision(.fractionLength(2))) } ?? "unknown"

        switch responseClass {
        case "Control-like":
            return "The curve stayed close to baseline across \(samples) observation samples. Peak change was \(peak), with latest change at \(latest)."
        case "Low confidence":
            return "The signal moved, but sensor quality averaged \(quality). Treat this as a contact-quality check before using it as calibration data."
        case "Needs more data":
            return "This session does not have enough usable observation samples yet. Record a baseline, mark first bite, then keep the phone session running."
        default:
            return "The curve rose above baseline across \(samples) observation samples. Peak change was \(peak), latest change was \(latest), and average sensor quality was \(quality)."
        }
    }

    private static func calibrationNote(for session: MeasurementSession, knownCalories: Int?) -> String {
        if let knownCalories {
            return "Use this as one personal calibration point at \(knownCalories) calories. A real calorie model needs more labeled meals from this same person."
        }
        if session.note.localizedCaseInsensitiveContains("no meal") {
            return "This is useful as a control point: it teaches the model what your no-food baseline drift looks like."
        }
        return "Add the known calories to the label when available, like ‘sandwich 450 cal’, so future versions can learn your personal response curve."
    }

    private static func knownCalories(from note: String) -> Int? {
        let tokens = note.components(separatedBy: CharacterSet.decimalDigits.inverted)
        return tokens.compactMap(Int.init).first { (50...2000).contains($0) }
    }

    private static func signedTemperature(_ value: Double?) -> String {
        guard let value else { return "—" }
        return "\(value.formatted(.number.sign(strategy: .always()).precision(.fractionLength(2)))) °C"
    }
}
