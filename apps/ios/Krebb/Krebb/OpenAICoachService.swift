import Foundation

struct OpenAICoachResponse: Decodable, Equatable {
    let outputText: String?
    let output: [OutputItem]?

    enum CodingKeys: String, CodingKey {
        case outputText = "output_text"
        case output
    }

    struct OutputItem: Decodable, Equatable {
        let content: [ContentItem]?
    }

    struct ContentItem: Decodable, Equatable {
        let type: String?
        let text: String?
    }

    var text: String {
        if let outputText, !outputText.isEmpty { return outputText }
        let pieces = output?.flatMap { item in
            item.content?.compactMap { content in
                content.type == "output_text" ? content.text : nil
            } ?? []
        } ?? []
        return pieces.joined(separator: "\n").trimmingCharacters(in: .whitespacesAndNewlines)
    }
}

enum OpenAICoachService {
    nonisolated static let defaultModel = "gpt-5.6-luna"

    @MainActor static func prompt(for session: MeasurementSession) -> String {
        let insight = KrebbCoach.insight(for: session)
        let summary = session.featureSummary
        let knownCalories = insight.knownCalories.map { String($0) } ?? "unknown"
        let baselineSkin = string(summary.baselineSkinTemperatureC)
        let peakDelta = string(summary.peakDeltaSkinTemperatureC)
        let latestDelta = string(summary.latestDeltaSkinTemperatureC)
        let auc = string(summary.temperatureAreaCelsiusSeconds)
        let quality = string(summary.averageSensorQuality)
        return """
        You are Krebb Coach inside an iPhone health prototype.
        Write for a normal consumer, not an engineer.

        Output rules:
        - Plain text only. No Markdown, no asterisks, no bold, no headings with colons, no bullet symbols, and no hyphen bullets.
        - Write exactly 3 short numbered lines starting with "1)", "2)", and "3)".
        - Each line should be one sentence under 22 words.
        - Be specific enough to be useful, but avoid dumping raw metrics.
        - Mention the meal name naturally.
        - If known calories are present, mention that this is saved as a labeled example.
        - Do not give medical advice.
        - Do not claim calorie estimation.
        - Do not say "skin temperature is an indirect signal" unless you can make it sound natural.

        Meaning to convey:
        1) What changed after the meal.
        2) What the user should do next to make Krebb smarter.
        3) The limitation in friendly language.

        Session label: \(session.title)
        Known calories if present: \(knownCalories)
        Response class: \(insight.responseClass)
        Baseline skin C: \(baselineSkin)
        Peak delta C: \(peakDelta)
        Latest delta C: \(latestDelta)
        Temperature AUC C*s: \(auc)
        Average sensor quality: \(quality)
        Observation samples: \(summary.observationSampleCount)
        """
    }

    @MainActor static func generateInsight(for session: MeasurementSession, apiKey: String, model: String = defaultModel) async throws -> String {
        guard let url = URL(string: "https://api.openai.com/v1/responses") else { throw URLError(.badURL) }
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try JSONSerialization.data(withJSONObject: [
            "model": model,
            "store": false,
            "input": prompt(for: session)
        ])

        let (data, response) = try await URLSession.shared.data(for: request)
        guard let http = response as? HTTPURLResponse else { throw URLError(.badServerResponse) }
        guard (200..<300).contains(http.statusCode) else {
            let message = String(data: data, encoding: .utf8) ?? "OpenAI request failed."
            throw OpenAICoachError.requestFailed(status: http.statusCode, message: message)
        }
        let decoded = try JSONDecoder().decode(OpenAICoachResponse.self, from: data)
        let text = cleanForPhone(decoded.text)
        guard !text.isEmpty else { throw OpenAICoachError.emptyResponse }
        return text
    }

    static func cleanForPhone(_ text: String) -> String {
        let cleanedLines = text
            .replacingOccurrences(of: "**", with: "")
            .replacingOccurrences(of: "*", with: "")
            .split(separator: "\n", omittingEmptySubsequences: true)
            .map { rawLine in
                var line = rawLine.trimmingCharacters(in: .whitespacesAndNewlines)
                while line.hasPrefix("-") || line.hasPrefix("•") || line.hasPrefix("–") || line.hasPrefix("—") {
                    line.removeFirst()
                    line = line.trimmingCharacters(in: .whitespacesAndNewlines)
                }
                return line
            }
            .filter { !$0.isEmpty }
        return cleanedLines.joined(separator: "\n")
    }

    private static func string(_ value: Double?) -> String {
        guard let value else { return "unknown" }
        return String(value)
    }
}

enum OpenAICoachError: LocalizedError {
    case requestFailed(status: Int, message: String)
    case emptyResponse

    var errorDescription: String? {
        switch self {
        case let .requestFailed(status, message):
            return "OpenAI request failed (\(status)). \(message)"
        case .emptyResponse:
            return "OpenAI returned an empty coach response."
        }
    }
}
