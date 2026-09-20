#!/usr/bin/env python3
"""Krebb One BLE monitor: connect, subscribe, validate, record.

Finds the device by its service UUID (falling back to the advertised name),
subscribes to the measurement characteristic, validates every notification
against the shared contract, and prints a summary on exit.

    python3 tools/ble_monitor.py
    python3 tools/ble_monitor.py --duration 60 --log-dir tools/recordings
    python3 tools/ble_monitor.py --reconnect-test 5

macOS note: the first run will ask for Bluetooth permission for whichever app
is running the script (Terminal, iTerm, VS Code). If no device is ever found,
check System Settings > Privacy & Security > Bluetooth. A Python built without
that entitlement will scan forever and find nothing.

Krebb One is a research prototype, not a medical device. Recordings are raw
sensor readings; keep them out of Git and do not add anything that identifies
a participant.
"""

from __future__ import annotations

import argparse
import asyncio
import csv
import json
import statistics
import sys
import time
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

try:
    from bleak import BleakClient, BleakScanner
except ImportError:  # pragma: no cover - dependency guidance
    print("bleak is not installed. Run:  pip install -r tools/requirements.txt", file=sys.stderr)
    raise SystemExit(1)

sys.path.insert(0, str(Path(__file__).resolve().parent))
from validate_packet import validate_packet  # noqa: E402

# Shared contract - see docs/sensor-protocol.md and docs/BLE_PROTOCOL.md.
SERVICE_UUID = "f000a001-0451-4000-b000-000000000000"
MEASUREMENT_CHAR_UUID = "f000a002-0451-4000-b000-000000000000"
DEVICE_NAME = "Krebb One"

DEFAULT_LOG_DIR = Path(__file__).resolve().parent / "recordings"


class Stats:
    """Accumulates packet statistics for the exit summary."""

    def __init__(self) -> None:
        self.total = 0
        self.valid = 0
        self.warned = 0
        self.errors: Counter[str] = Counter()
        self.intervals_ms: list[float] = []
        self.null_counts: Counter[str] = Counter()
        self.quality_histogram: Counter[str] = Counter()
        self._last_arrival: float | None = None
        self.last_timestamp_ms: int | None = None

    def record(self, result, arrival: float) -> None:
        self.total += 1

        if self._last_arrival is not None:
            self.intervals_ms.append((arrival - self._last_arrival) * 1000.0)
        self._last_arrival = arrival

        if result.warnings:
            self.warned += 1

        if not result.valid:
            for error in result.errors:
                # Collapse varying numbers so the summary groups causes.
                self.errors[error.split(":")[0].split("(")[0].strip()] += 1
            return

        self.valid += 1
        packet = result.packet
        self.last_timestamp_ms = packet["timestampMs"]

        for key in ("skinTemperatureC", "ambientTemperatureC"):
            if packet[key] is None:
                self.null_counts[key] += 1

        self.quality_histogram[quality_bucket(packet["sensorQuality"])] += 1

    def print_summary(self) -> None:
        print("\n" + "=" * 68)
        print("Krebb One BLE monitor summary")
        print("=" * 68)
        print(f"packets received : {self.total}")

        if self.total == 0:
            print("no packets received - was a client subscribed, and is the device advertising?")
            print("=" * 68)
            return

        pct = 100.0 * self.valid / self.total
        print(f"valid            : {self.valid} ({pct:.1f}%)")
        print(f"with warnings    : {self.warned}")

        if self.errors:
            print("errors by cause  :")
            for cause, count in self.errors.most_common():
                print(f"    {count:5d}  {cause}")

        if self.intervals_ms:
            mean = statistics.fmean(self.intervals_ms)
            print(
                f"interval (ms)    : mean {mean:.1f}  min {min(self.intervals_ms):.1f}  "
                f"max {max(self.intervals_ms):.1f}  (target 1000)"
            )
            if len(self.intervals_ms) > 1:
                print(f"                   stdev {statistics.stdev(self.intervals_ms):.1f}")

        print("null readings    :")
        for key in ("skinTemperatureC", "ambientTemperatureC"):
            count = self.null_counts[key]
            share = 100.0 * count / self.valid if self.valid else 0.0
            print(f"    {key:<22} {count:5d}  ({share:.1f}% of valid packets)")

        print("sensorQuality    :")
        for bucket in ("0.0 (invalid)", "0.0-0.3", "0.3-0.6 (stabilizing)", "0.6-0.7", "0.7-1.0 (stable)"):
            count = self.quality_histogram.get(bucket, 0)
            if count:
                bar = "#" * min(40, count)
                print(f"    {bucket:<22} {count:5d}  {bar}")
        print("=" * 68)


def quality_bucket(value: float) -> str:
    if value == 0.0:
        return "0.0 (invalid)"
    if value < 0.3:
        return "0.0-0.3"
    if value < 0.6:
        return "0.3-0.6 (stabilizing)"
    if value < 0.7:
        return "0.6-0.7"
    return "0.7-1.0 (stable)"


class Recorder:
    """Writes each notification to a JSONL and a CSV file."""

    CSV_FIELDS = (
        "receivedUnixMs",
        "timestampMs",
        "skinTemperatureC",
        "ambientTemperatureC",
        "sensorQuality",
        "valid",
        "note",
    )

    def __init__(self, log_dir: Path) -> None:
        log_dir.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        self.jsonl_path = log_dir / f"krebb-session-{stamp}.jsonl"
        self.csv_path = log_dir / f"krebb-session-{stamp}.csv"
        self._jsonl = self.jsonl_path.open("w", encoding="utf-8")
        self._csv_file = self.csv_path.open("w", newline="", encoding="utf-8")
        self._csv = csv.DictWriter(self._csv_file, fieldnames=self.CSV_FIELDS)
        self._csv.writeheader()

    def write(self, raw: bytes, result, arrival: float) -> None:
        received_ms = int(arrival * 1000)
        packet = result.packet or {}

        self._jsonl.write(
            json.dumps(
                {
                    "receivedUnixMs": received_ms,
                    "raw": raw.decode("utf-8", errors="replace"),
                    "valid": result.valid,
                    "errors": result.errors,
                    "warnings": result.warnings,
                },
                separators=(",", ":"),
            )
            + "\n"
        )
        self._jsonl.flush()

        self._csv.writerow(
            {
                "receivedUnixMs": received_ms,
                "timestampMs": packet.get("timestampMs", ""),
                "skinTemperatureC": "" if packet.get("skinTemperatureC") is None else packet["skinTemperatureC"],
                "ambientTemperatureC": "" if packet.get("ambientTemperatureC") is None else packet["ambientTemperatureC"],
                "sensorQuality": packet.get("sensorQuality", ""),
                "valid": int(result.valid),
                "note": result.summary if not result.valid or result.warnings else "",
            }
        )
        self._csv_file.flush()

    def close(self) -> None:
        self._jsonl.close()
        self._csv_file.close()
        print(f"\nrecorded to:\n  {self.jsonl_path}\n  {self.csv_path}")


async def find_device(scan_timeout: float):
    """Finds Krebb One by service UUID, falling back to the advertised name."""
    print(f"scanning for {scan_timeout:.0f}s (service {SERVICE_UUID})...")
    discovered = await BleakScanner.discover(timeout=scan_timeout, return_adv=True)

    for device, adv in discovered.values():
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        if SERVICE_UUID in uuids:
            print(f"found by service UUID: {device.address}  rssi={adv.rssi}")
            return device

    for device, adv in discovered.values():
        name = adv.local_name or device.name or ""
        if name == DEVICE_NAME:
            print(f"found by name '{DEVICE_NAME}': {device.address}  rssi={adv.rssi}")
            print("  (note: the service UUID was not in the advertisement)")
            return device

    print(f"no device advertising {SERVICE_UUID} or named '{DEVICE_NAME}' was found.")
    return None


async def monitor(duration: float | None, log_dir: Path | None, scan_timeout: float) -> int:
    device = await find_device(scan_timeout)
    if device is None:
        return 1

    stats = Stats()
    recorder = Recorder(log_dir) if log_dir else None
    stop = asyncio.Event()

    def handle(_characteristic, data: bytearray) -> None:
        arrival = time.time()
        result = validate_packet(bytes(data), previous_timestamp_ms=stats.last_timestamp_ms)
        stats.record(result, arrival)
        if recorder:
            recorder.write(bytes(data), result, arrival)

        marker = "ok " if result.valid else "BAD"
        if result.valid and result.warnings:
            marker = "warn"
        text = bytes(data).decode("utf-8", errors="replace")
        print(f"[{stats.total:5d}] {marker} {text}")
        for problem in result.errors + result.warnings:
            print(f"         -> {problem}")

    try:
        async with BleakClient(device) as client:
            print(f"connected to {device.address}")
            await client.start_notify(MEASUREMENT_CHAR_UUID, handle)
            print("subscribed - session active. Ctrl+C to stop.\n")

            if duration:
                try:
                    await asyncio.wait_for(stop.wait(), timeout=duration)
                except asyncio.TimeoutError:
                    pass
            else:
                await stop.wait()

            await client.stop_notify(MEASUREMENT_CHAR_UUID)
    except asyncio.CancelledError:
        pass
    except Exception as exc:  # noqa: BLE001 - surface any BLE stack error plainly
        print(f"\nBLE error: {exc}", file=sys.stderr)
        stats.print_summary()
        if recorder:
            recorder.close()
        return 1
    finally:
        if recorder:
            recorder.close()

    stats.print_summary()
    return 0 if stats.total and stats.valid == stats.total else 1


async def reconnect_test(cycles: int, listen_seconds: float, scan_timeout: float) -> int:
    """Connect, listen, disconnect, repeat - the demo-day reliability check."""
    print(f"reconnect test: {cycles} cycles of connect + {listen_seconds:.0f}s listen + disconnect\n")
    results: list[tuple[int, bool, int, str]] = []

    for cycle in range(1, cycles + 1):
        print(f"--- cycle {cycle}/{cycles} " + "-" * 40)
        device = await find_device(scan_timeout)
        if device is None:
            results.append((cycle, False, 0, "device not found"))
            continue

        received = 0
        valid = 0
        last_ts: int | None = None

        def handle(_characteristic, data: bytearray) -> None:
            nonlocal received, valid, last_ts
            received += 1
            result = validate_packet(bytes(data), previous_timestamp_ms=last_ts)
            if result.valid:
                valid += 1
                last_ts = result.packet["timestampMs"]

        try:
            async with BleakClient(device) as client:
                await client.start_notify(MEASUREMENT_CHAR_UUID, handle)
                await asyncio.sleep(listen_seconds)
                await client.stop_notify(MEASUREMENT_CHAR_UUID)
        except Exception as exc:  # noqa: BLE001
            results.append((cycle, False, received, f"error: {exc}"))
            print(f"    FAIL: {exc}")
            continue

        # ~1 Hz: allow for the connection setup inside the listen window.
        expected = max(1, int(listen_seconds) - 3)
        ok = received >= expected and valid == received
        results.append((cycle, ok, received, "" if ok else f"{received} packets, {valid} valid"))
        print(f"    {'PASS' if ok else 'FAIL'}: {received} packets, {valid} valid (expected >= {expected})")
        await asyncio.sleep(1.0)

    print("\n" + "=" * 68)
    print("reconnect test results")
    print("=" * 68)
    passed = sum(1 for _, ok, _, _ in results if ok)
    for cycle, ok, count, note in results:
        print(f"  cycle {cycle}: {'PASS' if ok else 'FAIL'}  {count:4d} packets  {note}")
    print(f"\n{passed}/{cycles} cycles passed")
    print("=" * 68)
    return 0 if passed == cycles else 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--duration", type=float, default=None, help="stop after N seconds")
    parser.add_argument(
        "--log-dir",
        type=Path,
        nargs="?",
        const=DEFAULT_LOG_DIR,
        default=None,
        help=f"record JSONL and CSV (default directory {DEFAULT_LOG_DIR}, git-ignored)",
    )
    parser.add_argument(
        "--reconnect-test",
        type=int,
        metavar="N",
        default=None,
        help="run N connect/listen/disconnect cycles and report pass/fail",
    )
    parser.add_argument("--scan-timeout", type=float, default=10.0, help="scan seconds (default 10)")
    parser.add_argument(
        "--listen-seconds", type=float, default=10.0, help="listen seconds per reconnect cycle"
    )
    args = parser.parse_args()

    try:
        if args.reconnect_test:
            return asyncio.run(
                reconnect_test(args.reconnect_test, args.listen_seconds, args.scan_timeout)
            )
        return asyncio.run(monitor(args.duration, args.log_dir, args.scan_timeout))
    except KeyboardInterrupt:
        print("\ninterrupted")
        return 130


if __name__ == "__main__":
    sys.exit(main())
