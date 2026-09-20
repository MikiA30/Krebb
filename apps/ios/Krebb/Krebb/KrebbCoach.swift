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
            claimBoundary: "Early prototype guidance. Not medical advice or a final calorie estimate.",
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
        switch responseClass {
        case "Control-like":
            return "Your body stayed steady during this check."
        case "Low confidence":
            return "Krebb needs a cleaner sensor fit for this check."
        case "Needs more data":
            return "Krebb needs a little more time to read this check."
        default:
            if let knownCalories {
                return "Your body showed a clear response after this ~\(knownCalories)-calorie meal."
            }
            return "Your body showed a clear response after this meal."
        }
    }

    private static func explanation(for responseClass: String, summary: SessionFeatureSummary) -> String {
        switch responseClass {
        case "Control-like":
            return "This looked like normal drift instead of a strong food response, which makes it useful as a comparison point."
        case "Low confidence":
            return "The reading changed, but contact quality was not steady enough to trust the pattern. Try again with the probe held more consistently."
        case "Needs more data":
            return "Start with a short quiet baseline, mark the first bite, then let the check run while you wait."
        case "Small early response":
            return "Krebb picked up a small shift from your starting point. A few more labeled meals will help separate food response from everyday noise."
        case "Moderate early response":
            return "Krebb picked up a noticeable shift from your starting point. This can become part of your personal meal-response profile."
        default:
            return "Krebb picked up a strong shift from your starting point. This is the kind of signal future versions can learn from once you have more labeled meals."
        }
    }

    private static func calibrationNote(for session: MeasurementSession, knownCalories: Int?) -> String {
        if let knownCalories {
            return "Saved as one labeled example for your personal calibration: \(knownCalories) calories."
        }
        if session.note.localizedCaseInsensitiveContains("no meal") {
            return "Saved as a no-meal comparison so Krebb can learn what steady looks like for you."
        }
        return "If you know the calories, add them to the title later so Krebb can use this as a labeled example."
    }

    private static func knownCalories(from note: String) -> Int? {
        let tokens = note.components(separatedBy: CharacterSet.decimalDigits.inverted)
        return tokens.compactMap(Int.init).first { (50...2000).contains($0) }
    }

}
