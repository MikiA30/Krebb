import Foundation
import Combine
@preconcurrency import CoreBluetooth

struct SensorMeasurementPacket: Decodable, Equatable {
    let timestampMs: Int64
    let skinTemperatureC: Double?
    let ambientTemperatureC: Double?
    let sensorQuality: Double
}

struct SensorMeasurement: Equatable {
    let packet: SensorMeasurementPacket
    let receivedAt: Date
}

struct DiscoveredSensor: Equatable {
    let name: String
    let rssi: Int
}

enum SensorConnectionState: Equatable {
    case idle
    case bluetoothUnavailable(String)
    case scanning
    case discovered
    case connecting
    case connected
    case subscribed
    case disconnected(String?)

    var label: String {
        switch self {
        case .idle: "Ready to scan"
        case .bluetoothUnavailable(let message): message
        case .scanning: "Scanning for Krebb One"
        case .discovered: "Krebb One found"
        case .connecting: "Connecting"
        case .connected: "Connected"
        case .subscribed: "Receiving sensor packets"
        case .disconnected(let message): message ?? "Disconnected"
        }
    }

    var isReceiving: Bool {
        if case .subscribed = self { return true }
        return false
    }
}

@MainActor
final class SensorConnectionStore: NSObject, ObservableObject {
    static let serviceUUID = CBUUID(string: "F000A001-0451-4000-B000-000000000000")
    static let measurementUUID = CBUUID(string: "F000A002-0451-4000-B000-000000000000")

    @Published private(set) var state: SensorConnectionState = .idle
    @Published private(set) var latestMeasurement: SensorMeasurement?
    @Published private(set) var packetCount = 0
    @Published private(set) var lastError: String?
    @Published private(set) var discoveredSensor: DiscoveredSensor?

    private var central: CBCentralManager?
    private var peripheral: CBPeripheral?

    func start() {
        lastError = nil
        latestMeasurement = nil
        packetCount = 0
        discoveredSensor = nil
        if central == nil {
            central = CBCentralManager(delegate: self, queue: nil)
        } else {
            scanIfReady()
        }
    }

    func connectToDiscoveredSensor() {
        guard let central, let peripheral else { return }
        lastError = nil
        state = .connecting
        central.connect(peripheral)
    }

    func stop() {
        if let peripheral {
            central?.cancelPeripheralConnection(peripheral)
        }
        central?.stopScan()
        peripheral = nil
        discoveredSensor = nil
        latestMeasurement = nil
        packetCount = 0
        state = .idle
    }

    private func scanIfReady() {
        guard let central else { return }
        switch central.state {
        case .poweredOn:
            state = .scanning
            central.scanForPeripherals(withServices: [Self.serviceUUID],
                                       options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
        case .poweredOff:
            state = .bluetoothUnavailable("Bluetooth is off")
        case .unauthorized:
            state = .bluetoothUnavailable("Bluetooth permission is needed")
        case .unsupported:
            state = .bluetoothUnavailable("Bluetooth is not supported")
        default:
            state = .bluetoothUnavailable("Bluetooth is starting")
        }
    }
}

extension SensorConnectionStore: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        scanIfReady()
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let advertisedName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
        discoveredSensor = DiscoveredSensor(name: advertisedName ?? peripheral.name ?? "Krebb One",
                                            rssi: RSSI.intValue)
        self.peripheral = peripheral
        self.peripheral?.delegate = self
        central.stopScan()
        state = .discovered
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        state = .connected
        peripheral.discoverServices([Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        state = .disconnected(error?.localizedDescription)
        lastError = error?.localizedDescription
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        state = .disconnected(error?.localizedDescription)
        lastError = error?.localizedDescription
        self.peripheral = nil
    }
}

extension SensorConnectionStore: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error {
            lastError = error.localizedDescription
            return
        }
        peripheral.services?.filter { $0.uuid == Self.serviceUUID }.forEach {
            peripheral.discoverCharacteristics([Self.measurementUUID], for: $0)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let error {
            lastError = error.localizedDescription
            return
        }
        guard let characteristic = service.characteristics?.first(where: { $0.uuid == Self.measurementUUID }) else {
            lastError = "Measurement characteristic not found"
            return
        }
        peripheral.setNotifyValue(true, for: characteristic)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic,
                    error: Error?) {
        if let error {
            lastError = error.localizedDescription
        } else if characteristic.isNotifying {
            state = .subscribed
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        if let error {
            lastError = error.localizedDescription
            return
        }
        guard characteristic.uuid == Self.measurementUUID, let data = characteristic.value else { return }
        do {
            let packet = try JSONDecoder().decode(SensorMeasurementPacket.self, from: data)
            guard packet.sensorQuality.isFinite, (0...1).contains(packet.sensorQuality) else { return }
            latestMeasurement = SensorMeasurement(packet: packet, receivedAt: .now)
            packetCount += 1
            lastError = nil
        } catch {
            lastError = "Bad sensor packet: \(error.localizedDescription)"
        }
    }
}
