#!/usr/bin/env python3
"""Bounded serial probe for an already flashed Esp32WifiTaskLiveGate.

No upload, erase, auto-port selection or authorization reset. A reboot is NOT
power loss. Only structured fixture metrics are retained (never raw console).
"""
import argparse
import json
from pathlib import Path
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--command', choices=('inspect', 'measure', 'reboot'), default='inspect')
    args = parser.parse_args()
    import serial
    from serial.tools import list_ports
    board = next((p for p in list_ports.comports() if p.device.upper() == args.port.upper()), None)
    if args.port.upper() in ('COM11', 'COM12') or not board or (board.serial_number or '').upper() != args.serial.upper():
        parser.error('dedicated board identity mismatch; witness boards forbidden')
    destination = args.out_dir.resolve()
    root = Path(__file__).resolve().parents[2] / 'build'
    if not destination.is_relative_to(root) or destination == root:
        parser.error('use a dedicated library build evidence directory')
    fields = {'begun', 'wifi', 'power', 'callbacks', 'presses', 'loops', 'commits', 'snapshots',
              'maxCommitUs', 'maxSnapshotUs', 'maxLoopUs', 'heap', 'minHeap', 'stackFree', 'timeValid', 'utc', 'taskError'}
    record = None
    port = serial.Serial(port=None, baudrate=115200, timeout=0.2, write_timeout=1)
    port.dtr = False
    port.rts = False
    port.port = args.port
    try:
        port.open()
        # First require positive fixture identity, even before a counter reset or reboot.
        port.write(b'j')
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = port.read_until(b'\n', 1024)
            try:
                value = json.loads(line)
                if value.get('fixture') == 'task-product' and set(value) == fields | {'fixture'} and all(type(value[k]) is int for k in fields):
                    record = value
                    break
            except (ValueError, AttributeError):
                continue
        if record is None:
            raise RuntimeError('no valid task-product metrics; no modifying command sent')
        if args.command != 'inspect':
            port.write(b'm' if args.command == 'measure' else b'r')
            port.flush()
    finally:
        port.close()
    result = {'command': args.command, 'port': args.port, 'serial': args.serial, 'observedAt': time.time(),
              'metricsBeforeCommand': record}
    destination.mkdir(parents=True, exist_ok=True)
    with (destination / f'serial-{time.time_ns()}.json').open('x', encoding='utf-8') as target:
        json.dump(result, target, indent=2)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
