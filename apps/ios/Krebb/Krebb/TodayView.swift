import SwiftUI
import Charts

struct TodayView: View {
    @ObservedObject var healthContext: HealthContextStore
    @ObservedObject var sensorConnection: SensorConnectionStore
    @ObservedObject var sessions: SessionStore
    let startCheck: () -> Void
    @State private var showsSensors = false
    @State private var selectedMinute: Int?

    private var selectedPoint: TemperaturePoint {
        TemperaturePoint.samples.min(by: {
            abs($0.minute - (selectedMinute ?? 12)) < abs($1.minute - (selectedMinute ?? 12))
        }) ?? TemperaturePoint.samples[12]
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 32) {
                HStack {
                    Text("YOUR THERMAL SIGNATURE")
                        .font(.caption2.weight(.semibold))
                        .tracking(1.6)
                        .foregroundStyle(.secondary)
                    Spacer()
                    if sessions.active == nil && sessions.completed.isEmpty { SampleLabel() }
                }

                if let session = sessions.active ?? sessions.completed.first {
                    TodaySessionHeader(session: session)
                    SessionReadout(session: session)
                } else {
                    signature
                }

                Button(action: startCheck) {
                    HStack {
                        Text(sessions.active == nil ? "Start a check" : "Continue check")
                        Spacer()
                        Image(systemName: "arrow.up.right")
                    }
                    .font(.headline)
                    .padding(.vertical, 10)
                    .padding(.horizontal, 8)
                }
                .buttonStyle(.glassProminent)
                .accessibilityIdentifier("startCheck")

                if sessions.active == nil && sessions.completed.isEmpty { temperatures }

                VStack(alignment: .leading, spacing: 18) {
                    Text("Make space for a baseline.")
                        .font(.title2.weight(.semibold))
                    Text("A quiet moment before your meal gives every reading that follows a point of comparison.")
                        .foregroundStyle(.secondary)
                    NavigationLink {
                        ResponseExplainer()
                    } label: {
                        HStack {
                            Text("What your temperature can tell you")
                            Spacer()
                            Image(systemName: "chevron.right").font(.caption.weight(.bold))
                        }
                        .font(.subheadline.weight(.medium))
                    }
                    .accessibilityIdentifier("responseExplainer")
                }
                .padding(.top, 6)

                Text("An experimental look at your response. Temperature trends aren’t a metabolic score.")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }
            .padding(.horizontal, 24)
            .padding(.top, 12)
            .padding(.bottom, 28)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle("Today")
        .navigationBarTitleDisplayMode(.large)
        .toolbar { KrebbToolbar(showsSensors: $showsSensors) }
        .sheet(isPresented: $showsSensors) { SensorSheet(healthContext: healthContext, sensorConnection: sensorConnection) }
    }

    private var signature: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                Text(selectedPoint.delta, format: .number.sign(strategy: .always()).precision(.fractionLength(1)))
                    .font(.system(size: 68, weight: .light, design: .rounded))
                    .contentTransition(.numericText())
                Text("°C")
                    .font(.title.weight(.light))
                    .foregroundStyle(KrebbPalette.blush)
            }
            .foregroundStyle(KrebbPalette.coral)
            .accessibilityElement(children: .combine)

            Text("Skin temperature · from baseline")
                .font(.subheadline)
                .foregroundStyle(.secondary)

            ThermalChart(selectedMinute: $selectedMinute)
                .frame(height: 205)
                .padding(.top, 20)

            HStack {
                Label("Meal logged", systemImage: "fork.knife")
                Spacer()
                Text(selectedMinute == nil ? "12 min later" : "Minute \(selectedPoint.minute)")
                    .monospacedDigit()
            }
            .font(.caption)
            .foregroundStyle(.secondary)
            .padding(.top, 8)
        }
    }

    private var temperatures: some View {
        VStack(alignment: .leading, spacing: 20) {
            HStack {
                Text("Behind the curve").font(.headline)
                Spacer()
                Text("EXAMPLE").font(.caption2.weight(.medium)).foregroundStyle(.secondary)
            }
            temperatureRow("Skin", icon: "thermometer.medium", value: "33.4", detail: "Contact sensor", color: KrebbPalette.coral)
            Divider()
            temperatureRow("Room", icon: "sun.max", value: "23.6", detail: "Ambient reference", color: KrebbPalette.blush)
        }
        .padding(22)
        .background(KrebbPalette.surface, in: RoundedRectangle(cornerRadius: 26))
    }

    private func temperatureRow(_ title: String, icon: String, value: String, detail: String, color: Color) -> some View {
        HStack(spacing: 12) {
            Image(systemName: icon).foregroundStyle(color).frame(width: 22)
            VStack(alignment: .leading, spacing: 4) {
                Text(title).font(.body.weight(.medium))
                Text(detail).font(.caption).foregroundStyle(.secondary)
            }
            Spacer()
            Text(value).font(.title2.weight(.medium).monospacedDigit())
            Text("°C").font(.subheadline).foregroundStyle(.secondary)
        }
    }
}

struct TodaySessionHeader: View {
    let session: MeasurementSession

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Label(session.isSimulated ? "Simulation" : "Live sensor", systemImage: session.isSimulated ? "play.circle" : "sensor.tag.radiowaves.forward.fill")
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(KrebbPalette.blush)
                Spacer()
                Text(session.startedAt.formatted(date: .omitted, time: .shortened))
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Text(session.title)
                .font(.largeTitle.weight(.semibold))
                .lineLimit(2)
            Text(session.stage == .complete ? "Saved response" : "Recording in progress")
                .font(.subheadline)
                .foregroundStyle(.secondary)
        }
        .padding(20)
        .background(
            LinearGradient(colors: [KrebbPalette.wine, KrebbPalette.surface],
                           startPoint: .topLeading,
                           endPoint: .bottomTrailing),
            in: RoundedRectangle(cornerRadius: 24)
        )
    }
}

struct TemperaturePoint: Identifiable {
    let minute: Int
    let delta: Double
    var id: Int { minute }

    static let samples = [0.0, 0.01, -0.01, 0.02, 0.04, 0.08, 0.07, 0.13, 0.18, 0.23, 0.22, 0.27, 0.30]
        .enumerated().map { TemperaturePoint(minute: $0.offset, delta: $0.element) }
}

struct ThermalChart: View {
    @Binding var selectedMinute: Int?

    var body: some View {
        Chart {
            ForEach(TemperaturePoint.samples) { point in
                AreaMark(x: .value("Minutes", point.minute), yStart: .value("Baseline", 0), yEnd: .value("Change", point.delta))
                    .interpolationMethod(.monotone)
                    .foregroundStyle(LinearGradient(colors: [KrebbPalette.coral.opacity(0.32), KrebbPalette.coral.opacity(0.01)], startPoint: .top, endPoint: .bottom))
                LineMark(x: .value("Minutes", point.minute), y: .value("Change", point.delta))
                    .interpolationMethod(.monotone)
                    .lineStyle(StrokeStyle(lineWidth: 3, lineCap: .round))
                    .foregroundStyle(KrebbPalette.coral)
            }
            RuleMark(y: .value("Baseline", 0))
                .lineStyle(StrokeStyle(lineWidth: 1, dash: [3, 5]))
                .foregroundStyle(.white.opacity(0.25))
            PointMark(x: .value("Minutes", 12), y: .value("Change", 0.3))
                .symbolSize(55)
                .foregroundStyle(KrebbPalette.blush)
            if let selectedMinute {
                RuleMark(x: .value("Selected minute", selectedMinute))
                    .foregroundStyle(.white.opacity(0.5))
                    .lineStyle(StrokeStyle(lineWidth: 1, dash: [3]))
            }
        }
        .chartXScale(domain: 0...12)
        .chartYScale(domain: -0.05...0.4)
        .chartXAxis {
            AxisMarks(values: [0, 4, 8, 12]) { value in
                AxisValueLabel {
                    if let minute = value.as(Int.self) {
                        Text(minute == 0 ? "Baseline" : "\(minute)m").font(.caption2)
                    }
                }
            }
        }
        .chartYAxis {
            AxisMarks(position: .leading, values: [0, 0.2, 0.4]) { value in
                AxisGridLine().foregroundStyle(.white.opacity(0.06))
                AxisValueLabel {
                    if let delta = value.as(Double.self) {
                        Text(delta, format: .number.precision(.fractionLength(1))).font(.caption2)
                    }
                }
            }
        }
        .chartXSelection(value: $selectedMinute)
        .accessibilityLabel("Sample skin temperature change in Celsius over twelve minutes")
    }
}

struct ResponseExplainer: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Image(systemName: "waveform.path").font(.largeTitle).foregroundStyle(KrebbPalette.coral)
                Text("A change starts with a reference.").font(.largeTitle.bold())
                Text("Krebb compares skin temperature with your own pre-meal baseline. The curve shows the change over time, in degrees Celsius.")
                Text("Room temperature helps put that change in context. Movement, sensor contact, and your surroundings can all affect a reading.")
                Text("This prototype explores patterns. Temperature alone doesn’t measure calories or establish a metabolic response.")
                    .foregroundStyle(.secondary)
            }
            .padding(24)
        }
        .background(KrebbPalette.canvas)
        .navigationTitle("About the response")
        .navigationBarTitleDisplayMode(.inline)
    }
}
