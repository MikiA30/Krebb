//
//  ContentView.swift
//  Krebb
//
//  Created by Mikiyas Asmamaw on 9/19/26.
//

import SwiftUI

struct ContentView: View {
    @State private var selectedTab: KrebbTab = .today

    var body: some View {
        ZStack(alignment: .bottom) {
            KrebbPalette.canvas
                .ignoresSafeArea()

            Group {
                switch selectedTab {
                case .today:
                    TodayView(onStartCheck: { selectedTab = .check })
                case .check:
                    CheckView()
                case .history:
                    HistoryView()
                }
            }

            KrebbTabBar(selectedTab: $selectedTab)
        }
        .preferredColorScheme(.dark)
    }
}

private enum KrebbTab: String, CaseIterable, Identifiable {
    case today = "Today"
    case check = "Check"
    case history = "History"

    var id: String { rawValue }

    var icon: String {
        switch self {
        case .today: "waveform.path.ecg"
        case .check: "circle.dotted.circle"
        case .history: "clock.arrow.circlepath"
        }
    }
}

enum KrebbPalette {
    static let canvas = Color(red: 0.035, green: 0.035, blue: 0.043)
    static let card = Color(red: 0.090, green: 0.090, blue: 0.102)
    static let cardRaised = Color(red: 0.125, green: 0.125, blue: 0.140)
    static let brand = Color(red: 0.80, green: 0.12, blue: 0.08)
    static let coral = Color(red: 0.96, green: 0.32, blue: 0.38)
    static let softPink = Color(red: 1.0, green: 0.58, blue: 0.62)
    static let mint = Color(red: 0.35, green: 0.90, blue: 0.70)
    static let primary = Color.white
    static let muted = Color(red: 0.62, green: 0.62, blue: 0.67)
    static let track = Color.white.opacity(0.12)
}

private struct TodayView: View {
    let onStartCheck: () -> Void

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                header
                responseCard
                metricGrid
                trendCard
                startCheckCard
            }
            .padding(.horizontal, 20)
            .padding(.top, 10)
            .padding(.bottom, 116)
        }
        .scrollIndicators(.hidden)
    }

    private var header: some View {
        HStack(spacing: 13) {
            Image("KrebbMark")
                .resizable()
                .scaledToFit()
                .frame(width: 46, height: 46)
                .clipShape(RoundedRectangle(cornerRadius: 14, style: .continuous))

            VStack(alignment: .leading, spacing: 3) {
                Text("KREBB")
                    .font(.caption.weight(.bold))
                    .tracking(1.7)
                    .foregroundStyle(KrebbPalette.coral)
                Text("Today’s response")
                    .font(.title2.weight(.bold))
                    .foregroundStyle(KrebbPalette.primary)
            }

            Spacer()

            Circle()
                .fill(KrebbPalette.cardRaised)
                .frame(width: 42, height: 42)
                .overlay {
                    Image(systemName: "bell")
                        .font(.body.weight(.semibold))
                        .foregroundStyle(KrebbPalette.primary)
                }
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel("Krebb, today's response")
    }

    private var responseCard: some View {
        VStack(alignment: .leading, spacing: 20) {
            HStack {
                Label("Live experiment", systemImage: "dot.radiowaves.left.and.right")
                    .font(.subheadline.weight(.semibold))
                    .foregroundStyle(KrebbPalette.softPink)
                Spacer()
                Text("DEMO")
                    .font(.caption2.weight(.bold))
                    .tracking(1)
                    .foregroundStyle(KrebbPalette.muted)
            }

            HStack(spacing: 20) {
                ResponseRings()
                    .frame(width: 154, height: 154)

                VStack(alignment: .leading, spacing: 7) {
                    Text("Response so far")
                        .font(.subheadline)
                        .foregroundStyle(KrebbPalette.muted)
                    Text("Moderate")
                        .font(.title2.weight(.bold))
                        .foregroundStyle(KrebbPalette.primary)
                    Text("+0.3°C from baseline")
                        .font(.subheadline.weight(.semibold))
                        .foregroundStyle(KrebbPalette.coral)
                    Divider()
                        .overlay(Color.white.opacity(0.1))
                    Text("12 min observed")
                        .font(.caption)
                        .foregroundStyle(KrebbPalette.muted)
                }
                Spacer(minLength: 0)
            }
        }
        .padding(20)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 28, style: .continuous))
    }

    private var metricGrid: some View {
        HStack(spacing: 12) {
            VitalCard(title: "Skin temp", value: "33.4°", detail: "+0.3° from baseline", icon: "thermometer.medium", tint: KrebbPalette.coral)
            VitalCard(title: "Ambient", value: "23.6°", detail: "Stable environment", icon: "sun.max", tint: KrebbPalette.softPink)
        }
    }

    private var trendCard: some View {
        VStack(alignment: .leading, spacing: 16) {
            HStack {
                VStack(alignment: .leading, spacing: 3) {
                    Text("Temperature trend")
                        .font(.headline)
                    Text("Skin temperature relative to baseline")
                        .font(.caption)
                        .foregroundStyle(KrebbPalette.muted)
                }
                Spacer()
                Text("+0.3°")
                    .font(.headline.monospacedDigit())
                    .foregroundStyle(KrebbPalette.coral)
            }

            TemperatureLineChart()
                .frame(height: 94)

            HStack {
                Text("BASELINE")
                Spacer()
                Text("NOW")
            }
            .font(.caption2.weight(.semibold))
            .tracking(0.8)
            .foregroundStyle(KrebbPalette.muted)
        }
        .padding(20)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 24, style: .continuous))
    }

    private var startCheckCard: some View {
        HStack(spacing: 15) {
            Image(systemName: "plus.circle.fill")
                .font(.title2)
                .foregroundStyle(KrebbPalette.coral)
            VStack(alignment: .leading, spacing: 4) {
                Text("Start a new check")
                    .font(.headline)
                Text("Capture a fresh baseline before a meal.")
                    .font(.caption)
                    .foregroundStyle(KrebbPalette.muted)
            }
            Spacer()
            Button(action: onStartCheck) {
                Image(systemName: "arrow.right")
                    .font(.subheadline.weight(.bold))
                    .frame(width: 36, height: 36)
                    .background(KrebbPalette.cardRaised, in: Circle())
            }
            .buttonStyle(.plain)
            .accessibilityLabel("Start a new check")
        }
        .padding(18)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 24, style: .continuous))
    }
}

private struct ResponseRings: View {
    var body: some View {
        ZStack {
            ProgressRing(progress: 0.68, lineWidth: 15, color: KrebbPalette.brand)
            ProgressRing(progress: 0.91, lineWidth: 12, color: KrebbPalette.coral)
                .padding(22)
            ProgressRing(progress: 0.80, lineWidth: 10, color: KrebbPalette.softPink)
                .padding(42)
            VStack(spacing: 3) {
                Image(systemName: "waveform.path.ecg")
                    .font(.title3.weight(.bold))
                    .foregroundStyle(KrebbPalette.primary)
                Text("68%")
                    .font(.caption.weight(.bold).monospacedDigit())
                    .foregroundStyle(KrebbPalette.muted)
            }
        }
        .accessibilityElement(children: .ignore)
        .accessibilityLabel("Response 68 percent, signal quality 91 percent, context complete 80 percent")
    }
}

private struct ProgressRing: View {
    let progress: Double
    let lineWidth: CGFloat
    let color: Color

    var body: some View {
        Circle()
            .stroke(KrebbPalette.track, lineWidth: lineWidth)
            .overlay {
                Circle()
                    .trim(from: 0, to: progress)
                    .stroke(LinearGradient(colors: [color.opacity(0.72), color], startPoint: .topLeading, endPoint: .bottomTrailing), style: StrokeStyle(lineWidth: lineWidth, lineCap: .round))
                    .rotationEffect(.degrees(-90))
            }
    }
}

private struct VitalCard: View {
    let title: String
    let value: String
    let detail: String
    let icon: String
    let tint: Color

    var body: some View {
        VStack(alignment: .leading, spacing: 13) {
            Image(systemName: icon)
                .font(.subheadline.weight(.bold))
                .foregroundStyle(tint)
                .frame(width: 32, height: 32)
                .background(tint.opacity(0.14), in: Circle())
            Text(title)
                .font(.caption.weight(.semibold))
                .foregroundStyle(KrebbPalette.muted)
            Text(value)
                .font(.title.weight(.bold).monospacedDigit())
            Text(detail)
                .font(.caption2)
                .foregroundStyle(tint)
                .lineLimit(1)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(16)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 22, style: .continuous))
    }
}

private struct TemperatureLineChart: View {
    private let readings: [CGFloat] = [0.53, 0.49, 0.51, 0.44, 0.47, 0.39, 0.42, 0.31, 0.35, 0.26, 0.24, 0.18]

    var body: some View {
        GeometryReader { proxy in
            let width = proxy.size.width
            let height = proxy.size.height

            ZStack(alignment: .bottom) {
                VStack(spacing: 0) {
                    ForEach(0..<3, id: \.self) { _ in
                        Rectangle().fill(Color.white.opacity(0.08)).frame(height: 1)
                        Spacer()
                    }
                }

                Path { path in
                    for (index, reading) in readings.enumerated() {
                        let x = width * CGFloat(index) / CGFloat(readings.count - 1)
                        let y = height * reading
                        index == 0 ? path.move(to: CGPoint(x: x, y: y)) : path.addLine(to: CGPoint(x: x, y: y))
                    }
                }
                .stroke(KrebbPalette.coral, style: StrokeStyle(lineWidth: 3, lineCap: .round, lineJoin: .round))

                Circle()
                    .fill(KrebbPalette.softPink)
                    .frame(width: 9, height: 9)
                    .shadow(color: KrebbPalette.softPink.opacity(0.8), radius: 7)
                    .position(x: width, y: height * (readings.last ?? 0.5))
            }
        }
    }
}

private struct CheckView: View {
    @State private var isRunning = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                Text("New check")
                    .font(.largeTitle.weight(.bold))
                    .padding(.top, 14)

                VStack(alignment: .leading, spacing: 10) {
                    Text(isRunning ? "Baseline in progress" : "Ready when you are")
                        .font(.title2.weight(.bold))
                    Text(isRunning ? "Stay still for a few minutes while Krebb learns your baseline." : "Use the sensor before a meal or experiment to establish your comparison point.")
                        .font(.body)
                        .foregroundStyle(KrebbPalette.muted)
                }
                .padding(22)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 28, style: .continuous))

                VStack(spacing: 14) {
                    DeviceRow(title: "Krebb sensor", detail: isRunning ? "Streaming at 1 Hz" : "Connected", icon: "sensor.tag.radiowaves.forward", tint: KrebbPalette.coral)
                    DeviceRow(title: "Ambient sensor", detail: "Stable · 23.6°C", icon: "sun.max", tint: KrebbPalette.softPink)
                    DeviceRow(title: "Apple Watch", detail: "Optional context", icon: "applewatch", tint: KrebbPalette.mint)
                }

                Button { isRunning.toggle() } label: {
                    Label(isRunning ? "Finish baseline" : "Begin baseline", systemImage: isRunning ? "checkmark" : "play.fill")
                        .font(.headline)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 18)
                        .background(isRunning ? KrebbPalette.cardRaised : KrebbPalette.brand, in: RoundedRectangle(cornerRadius: 18, style: .continuous))
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 116)
        }
    }
}

private struct DeviceRow: View {
    let title: String
    let detail: String
    let icon: String
    let tint: Color

    var body: some View {
        HStack(spacing: 14) {
            Image(systemName: icon)
                .foregroundStyle(tint)
                .frame(width: 38, height: 38)
                .background(tint.opacity(0.14), in: RoundedRectangle(cornerRadius: 12, style: .continuous))
            VStack(alignment: .leading, spacing: 3) {
                Text(title).font(.headline)
                Text(detail).font(.caption).foregroundStyle(KrebbPalette.muted)
            }
            Spacer()
            Circle().fill(tint).frame(width: 8, height: 8)
        }
        .padding(15)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 20, style: .continuous))
    }
}

private struct HistoryView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 22) {
                Text("History")
                    .font(.largeTitle.weight(.bold))
                    .padding(.top, 14)
                Text("Your experiments stay on this device while Krebb is in prototype mode.")
                    .foregroundStyle(KrebbPalette.muted)
                HistoryEntry(title: "Breakfast response", time: "Today · 8:42 AM", response: "Moderate", tint: KrebbPalette.coral)
                HistoryEntry(title: "Morning baseline", time: "Today · 7:58 AM", response: "Complete", tint: KrebbPalette.softPink)
                HistoryEntry(title: "Evening check", time: "Yesterday · 7:16 PM", response: "Small", tint: KrebbPalette.mint)
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 116)
        }
    }
}

private struct HistoryEntry: View {
    let title: String
    let time: String
    let response: String
    let tint: Color

    var body: some View {
        HStack(spacing: 15) {
            Circle()
                .stroke(tint, lineWidth: 5)
                .frame(width: 44, height: 44)
                .overlay(Image(systemName: "waveform.path.ecg").font(.caption).foregroundStyle(tint))
            VStack(alignment: .leading, spacing: 4) {
                Text(title).font(.headline)
                Text(time).font(.caption).foregroundStyle(KrebbPalette.muted)
            }
            Spacer()
            Text(response)
                .font(.subheadline.weight(.semibold))
                .foregroundStyle(tint)
        }
        .padding(16)
        .background(KrebbPalette.card, in: RoundedRectangle(cornerRadius: 20, style: .continuous))
    }
}

private struct KrebbTabBar: View {
    @Binding var selectedTab: KrebbTab

    var body: some View {
        HStack(spacing: 8) {
            ForEach(KrebbTab.allCases) { tab in
                Button { selectedTab = tab } label: {
                    VStack(spacing: 5) {
                        Image(systemName: tab.icon).font(.subheadline.weight(.semibold))
                        Text(tab.rawValue).font(.caption2.weight(.semibold))
                    }
                    .foregroundStyle(selectedTab == tab ? KrebbPalette.primary : KrebbPalette.muted)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 11)
                    .background(selectedTab == tab ? KrebbPalette.cardRaised : .clear, in: RoundedRectangle(cornerRadius: 16, style: .continuous))
                }
                .buttonStyle(.plain)
            }
        }
        .padding(8)
        .background(.ultraThinMaterial, in: Capsule())
        .overlay(Capsule().stroke(Color.white.opacity(0.14), lineWidth: 1))
        .padding(.horizontal, 20)
        .padding(.bottom, 10)
    }
}

#Preview {
    ContentView()
}
