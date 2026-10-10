#!/usr/bin/env python3
"""Bounded serial driver for an ALREADY FLASHED Esp32TaskNvsGate fixture.

Never uploads, erases Flash, chooses another board, or treats reset as power loss.
Only recognized fixture events are retained; no arbitrary firmware output/secrets.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import time

COMMANDS = ("inspect", "fill4", "fill8", "fill10", "run", "idle", "edit", "replay", "full", "recover",
            "arm", "beforeclaim", "claim", "applied", "reset", "clear")
CUTS = {"arm": "armed", "beforeclaim": "before_claim", "claim": "claimed", "applied": "applied"}
FIELDS = {
    "open": ("us", "code"), "boot": ("code", "ready", "resetReason"),
    "write": ("bytes", "us", "code"), "apply": ("count", "value", "code"),
    "poll": ("us", "code", "progress", "invoked"), "cut": ("phase", "callbacks"),
    "done": ("command", "code"), "cleared": (),
    "status": ("label", "state", "count", "revision", "callbacks", "writes", "maxWriteUs",
               "heap", "minHeap", "stackFree", "nvsStatsCode", "nvsUsed", "nvsFree"),
    "record": ("index", "code", "id", "definition", "progress", "start", "enabled"),
}


def fixture_event(line: bytes) -> dict | None:
    if len(line) > 1024:
        return None
    try:
        value = json.loads(line)
    except (ValueError, UnicodeError):
        return None
    if not isinstance(value, dict) or value.get("event") not in FIELDS:
        return None
    fields = FIELDS[value["event"]]
    if set(value) != {"event", *fields}:
        return None
    text_fields = {"label", "phase", "command"}
    if any(type(value[key]) is not int for key in fields if key not in text_fields):
        return None
    for key in set(fields) & text_fields:
        allowed = set(COMMANDS) | set(CUTS.values()) | {"boot"}
        if not isinstance(value[key], str) or value[key] not in allowed:
            return None
    return value


def complete(command: str, events: list[dict]) -> bool:
    if command in CUTS:
        return any(e.get("event") == "cut" and e.get("phase") == CUTS[command] for e in events)
    if command in ("reset", "clear"):
        return any(e.get("event") == "status" and e.get("label") == "boot" for e in events)
    return any(e.get("event") == "done" and e.get("command") == command for e in events)


def validate(command: str, events: list[dict], count: int | None, progress: int | None,
             callbacks: int | None) -> None:
    if not complete(command, events):
        raise ValueError("fixture completion/cut marker not received")
    if command in ("reset", "clear") and not any(e["event"] == "boot" for e in events):
        raise ValueError("boot evidence is required after reset/clear")
    if command == "clear" and not any(e["event"] == "cleared" for e in events):
        raise ValueError("explicit fixture clear acknowledgment missing")
    for event in events:
        if event["event"] in ("write", "apply", "poll", "record", "done") and event.get("code") != 0:
            raise ValueError(f"fixture {event['event']} error {event.get('code')}")
        if event["event"] == "boot" and (event["ready"] != 1 or event["code"] not in (0, 19)):
            raise ValueError("fixture boot rejected storage/product state")
    statuses = [e for e in events if e["event"] == "status"]
    if statuses:
        last_status = max(i for i, e in enumerate(events) if e["event"] == "status")
        rows = [e for e in events[last_status + 1:] if e["event"] == "record"]
        if [e["index"] for e in rows] != list(range(statuses[-1]["count"])):
            raise ValueError("incomplete or unordered snapshot record evidence")
        if statuses[-1].get("nvsStatsCode", 0) != 0:
            raise ValueError("NVS statistics unavailable")
    if count is not None and (not statuses or statuses[-1]["count"] != count):
        raise ValueError("task count mismatch")
    if callbacks is not None and (not statuses or statuses[-1]["callbacks"] != callbacks):
        raise ValueError("callback count mismatch")
    if progress is not None:
        records = [e for e in events if e["event"] == "record" and e["index"] == 0]
        if not records or records[-1]["progress"] != progress:
            raise ValueError("first task progress mismatch")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--serial", required=True, help="expected USB serial/MAC, checked before every open")
    parser.add_argument("--command", choices=COMMANDS, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--expect-count", type=int)
    parser.add_argument("--expect-progress", type=int)
    parser.add_argument("--expect-callbacks", type=int)
    args = parser.parse_args()
    import serial
    from serial.tools import list_ports
    if args.port.upper() in ("COM11", "COM12"):
        parser.error("COM11/COM12 are protected witness boards for this gate")
    args.out_dir.mkdir(parents=True, exist_ok=True)
    events: list[dict] = []
    link = None
    sent = False
    deadline = time.monotonic() + 25
    error = None
    try:
        while time.monotonic() < deadline:
            if link is None:
                match = next((p for p in list_ports.comports() if p.device.upper() == args.port.upper()), None)
                if match is None:
                    time.sleep(.2)
                    continue
                if (match.serial_number or "").upper() != args.serial.upper():
                    raise ValueError("USB identity mismatch; refusing serial command")
                link = serial.Serial(port=None, baudrate=115200, timeout=.2, write_timeout=1)
                link.dtr = False
                link.rts = False
                link.port = args.port
                link.open()
                if not sent:
                    # Discard prior command/boot output before sending this command.
                    time.sleep(.15)
                    link.reset_input_buffer()
                    link.write((args.command + "\n").encode("ascii"))
                    link.flush()
                    sent = True
            try:
                event = fixture_event(link.readline(1025))
                if event is not None:
                    events.append(event)
                    print(json.dumps(event), flush=True)
                    if complete(args.command, events):
                        # Boot status precedes bounded per-record output.
                        if args.command in ("reset", "clear"):
                            until = time.monotonic() + .6
                            while time.monotonic() < until:
                                extra = fixture_event(link.readline(1025))
                                if extra is not None:
                                    events.append(extra)
                                    print(json.dumps(extra), flush=True)
                        break
            except serial.SerialException:
                link.close()
                link = None
                if args.command not in ("reset", "clear"):
                    raise
        validate(args.command, events, args.expect_count, args.expect_progress, args.expect_callbacks)
    except Exception as exc:
        error = str(exc)
        raise
    finally:
        if link is not None:
            link.close()
        result = {"command": args.command, "events": events, "error": error,
                  "scope": "isolated NVS fixture; reset is NOT physical power-loss evidence"}
        (args.out_dir / f"{args.command}-{time.time_ns()}.json").write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
