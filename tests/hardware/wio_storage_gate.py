#!/usr/bin/env python3
"""Finite driver for already verified Wio storage fixtures; never uploads/resets.

Raw snapshots and durable intents are private. A partial/unknown response is
never resynchronized or replayed. A new directory is not a fault unlock.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import time
import zlib

ROOT = Path(__file__).resolve().parents[2]
SIZE, REGION, TASK_BASE, IDENTITY_BASE = 16384, 8192, 0x3FC000, 0x3FE000
STATES = ("Fresh", "Captured", "Admitted", "Written", "IdentityEnded", "Cleared", "Failed")
TRANSITIONS = {"OPEN_CAPTURE": ("Fresh", "Captured"), "REGION_ATTEST": ("Captured", "Admitted"),
               "REPLACE1": ("Admitted", "Written"), "IDENTITY_END": ("Written", "IdentityEnded"),
               "CLEAR_FIXTURE": ("IdentityEnded", "Cleared")}


class GateError(ValueError):
    pass


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def hex_id(value: str, length: int) -> str:
    if not isinstance(value, str) or not re.fullmatch(rf"[0-9a-fA-F]{{{length}}}", value):
        raise GateError("invalid fixed-width identifier")
    return value.lower()


def read_exact(port, count: int, deadline: float) -> bytes:
    result = bytearray()
    while len(result) < count:
        if time.monotonic() >= deadline:
            raise GateError("incomplete response; stream must be closed")
        chunk = port.read(count - len(result))
        if len(chunk) > count - len(result):
            raise GateError("invalid serial read length")
        result.extend(chunk)
    return bytes(result)


def frame(port, magic: bytes, size: int, deadline: float) -> bytes:
    prefix = read_exact(port, 4, deadline)
    if prefix == b"BVE1":
        code = read_exact(port, 1, deadline)[0]
        raise GateError(f"maintenance fixture rejected request ({code})")
    if prefix != magic:
        raise GateError("wrong frame prefix; no scanning/resynchronization")
    length, crc = struct.unpack("<HI", read_exact(port, 6, deadline))
    if length != size:
        raise GateError("wrong fixed-region response length")
    raw = read_exact(port, size, deadline)
    if zlib.crc32(raw) != crc or port.in_waiting:
        raise GateError("CRC or trailing-data rejection")
    return raw


def send(port, data: bytes) -> None:
    if port.in_waiting:
        raise GateError("unexpected bytes before request; do not flush/resynchronize")
    if port.write(data) != len(data):
        raise GateError("partial request; do not resend")


def maintenance_profile(port, uid: str, source: str, mode: int) -> None:
    send(port, b"p")
    raw = frame(port, b"BVP1", 63, time.monotonic() + 15)
    layout = struct.unpack("<BIIIH", raw[:15])
    if (layout != (mode, 4194304, TASK_BASE, IDENTITY_BASE, REGION) or
            raw[15:31].hex() != uid or raw[31:].hex() != source):
        raise GateError("maintenance board/source/mode/layout mismatch")


def dump(port) -> bytes:
    send(port, b"d")
    return frame(port, b"BVS1", SIZE, time.monotonic() + 180)


def stable_dump(port) -> bytes:
    first, second = dump(port), dump(port)
    if first != second:
        raise GateError("independent raw reads differ")
    return first


def receive_event(port, deadline: float) -> dict:
    line = bytearray()
    while not line.endswith(b"\n"):
        line.extend(read_exact(port, 1, deadline))
        if len(line) > 2048:
            raise GateError("oversized fixture event")
    def unique_object(pairs):
        value = {}
        for key, item in pairs:
            if key in value:
                raise GateError("duplicate fixture field")
            value[key] = item
        return value
    try:
        value = json.loads(line, object_pairs_hook=unique_object)
    except (ValueError, UnicodeError):
        raise GateError("noncontract fixture output; raw output not retained") from None
    schemas = {
        "done": {"event", "command", "code", "state", "attemptedMask"},
        "profile": {"event", "fixture", "sourceId", "boardUid", "baselineSha256", "capacity",
                    "payloadBytes", "journalBytes", "taskBase", "identityBase", "regionBytes", "regionReserved"},
        "operation": {"event", "command", "code", "us"},
        "snapshot": {"event", "bytes", "sha256", "fixturePhase", "identitySame"},
    }
    if (not isinstance(value, dict) or not isinstance(value.get("event"), str) or
            set(value) != schemas.get(value.get("event"))):
        raise GateError("unexpected fixture fields; raw output not retained")
    text = {"event", "command", "state", "fixture", "sourceId", "boardUid", "baselineSha256", "sha256"}
    if any(not isinstance(v, str) for k, v in value.items() if k in text):
        raise GateError("invalid textual fixture field")
    if any(type(v) is not int or v < 0 for k, v in value.items() if k not in text and k != "identitySame"):
        raise GateError("invalid numeric fixture field")
    if "identitySame" in value and type(value["identitySame"]) is not bool:
        raise GateError("invalid identity comparison flag")
    if value["event"] == "done" and (value["state"] not in STATES or value["attemptedMask"] > 3):
        raise GateError("invalid fixture state")
    for key, width in (("sourceId", 64), ("boardUid", 32), ("baselineSha256", 64), ("sha256", 64)):
        if key in value:
            hex_id(value[key], width)
    return value


def exchange(port, command: str) -> list[dict]:
    send(port, command.encode("ascii") + b"\n")
    events = []
    deadline = time.monotonic() + 180
    while len(events) < 12:
        event = receive_event(port, deadline)
        events.append(event)
        if event["event"] == "done":
            if event["command"] != command or port.in_waiting:
                raise GateError("completion/trailing-data mismatch")
            return events
    raise GateError("fixture event count exceeded")


def probe_profile(events: list[dict], uid: str, source: str, baseline: str) -> dict:
    profiles = [e for e in events if e["event"] == "profile"]
    done = events[-1]
    if ([e["event"] for e in events] != ["profile", "done"] or
            len(profiles) != 1 or done.get("command") != "PING" or done.get("code") != 0):
        raise GateError("profile handshake missing")
    p = profiles[0]
    if (p["fixture"] != "wio-task-storage-v1" or p["boardUid"] != uid or p["sourceId"] != source or
            p["baselineSha256"] != baseline or
            (p["capacity"], p["payloadBytes"], p["journalBytes"], p["taskBase"], p["identityBase"],
             p["regionBytes"], p["regionReserved"]) != (4, 644, 664, TASK_BASE, IDENTITY_BASE, REGION, 1)):
        raise GateError("probe board/source/baseline/layout mismatch")
    return done


def save_new(path: Path, raw: bytes) -> None:
    with path.open("xb") as handle:
        handle.write(raw)
        handle.flush()
        os.fsync(handle.fileno())


def save_state(path: Path, value: dict) -> None:
    # Unique next file avoids overwriting unrelated material; atomic replacement
    # is of this gate's own state only. Never replace the original raw backup.
    next_path = path.with_name(path.name + ".next")
    save_new(next_path, (json.dumps(value, sort_keys=True) + "\n").encode())
    os.replace(next_path, path)


def bind_state(state: dict, uid: str, baseline: bytes) -> None:
    if len(baseline) != SIZE or state.get("boardUid") != uid or state.get("baselineSha256") != digest(baseline):
        raise GateError("private backup/board/state mismatch")


def capture_session(port, folder: Path, claim_path: Path, uid: str, source: str, cold_confirmed: bool) -> bytes:
    """A failed read may resume only its original empty, pending-capture session."""
    state_path, original_path = folder / "state.json", folder / "original-qspi.raw"
    if any(path.exists() for path in (state_path, original_path, state_path.with_name("state.json.next"))):
        raise GateError("capture files already exist; preserve them for manual reconciliation")
    claim = {"privateDir": str(folder), "boardUid": uid, "sourceId": source, "phase": "capture-pending"}
    if claim_path.exists():
        if json.loads(claim_path.read_text()) != claim or not cold_confirmed:
            raise GateError("only same-session capture-pending reads may resume after whole-board power-cycle")
    maintenance_profile(port, uid, source, 1)
    if not claim_path.exists():
        save_new(claim_path, json.dumps(claim).encode())
    raw = stable_dump(port)
    save_new(original_path, raw)
    if original_path.read_bytes() != raw:
        raise GateError("private backup file readback mismatch")
    save_state(state_path, {"boardUid": uid, "baselineSha256": digest(raw), "captureSourceId": source,
                           "probeState": "Fresh", "attemptedMask": 0, "unknown": False, "intents": []})
    save_state(claim_path, {**claim, "phase": "sealed"})
    return raw


def authorize(state: dict, command: str, done: dict, source: str, exclusive: bool) -> None:
    if command not in (*TRANSITIONS, "READ", "STATUS"):
        raise GateError("unsupported probe command")
    if state.get("probeSourceId") not in (None, source):
        raise GateError("probe source changed within session")
    if state.get("restored"):
        raise GateError("session closed after restoration; no further probe commands")
    if state.get("unknown") or state.get("restoreStarted"):
        raise GateError("unknown/restore pending; only explicit recovery is allowed")
    if done["state"] != state.get("probeState", "Fresh") or done["attemptedMask"] != state.get("attemptedMask", 0):
        raise GateError("probe restarted or state diverged; reconcile without replay")
    if command == "REGION_ATTEST" and not exclusive:
        raise GateError("external task-region ownership confirmation required")
    if command in TRANSITIONS and done["state"] != TRANSITIONS[command][0]:
        raise GateError("command not legal in current stage")
    if command == "READ" and done["state"] not in ("Admitted", "Written", "IdentityEnded", "Cleared"):
        raise GateError("read not legal in current stage")


def probe_command(port, state_path: Path, state: dict, command: str, source: str,
                  exclusive: bool) -> list[dict]:
    done = probe_profile(exchange(port, "PING"), state["boardUid"], source, state["baselineSha256"])
    authorize(state, command, done, source, exclusive)
    state["probeSourceId"] = source
    # Persist BEFORE sending. Even a lost reply to a nonmutation that opens the
    # native owner requires reconciliation, not automatic command replay.
    intent = {"command": command, "sourceId": source, "stateBefore": done["state"], "outcome": "pending"}
    state.setdefault("intents", []).append(intent)
    state["unknown"] = True
    save_state(state_path, state)
    events = exchange(port, command)
    result = events[-1]
    missing = command == "READ" and result["code"] == 19 and result["state"] in ("Admitted", "Cleared")
    if (result["code"] != 0 and not missing) or result["state"] == "Failed":
        raise GateError("fixture rejected/failed; keep intent and do not replay")
    expected = TRANSITIONS.get(command, (done["state"], done["state"]))[1]
    mask = done["attemptedMask"] | ({"REPLACE1": 1, "CLEAR_FIXTURE": 2}.get(command, 0))
    if result["state"] != expected or result["attemptedMask"] != mask:
        raise GateError("unexpected completion; keep intent and do not replay")
    expected_events = (["operation", "snapshot", "done"] if command == "REPLACE1" else
                       ["operation", "done"] if command == "CLEAR_FIXTURE" else
                       ["snapshot", "done"] if command == "IDENTITY_END" or (command == "READ" and not missing) else
                       ["done"])
    if [e["event"] for e in events] != expected_events:
        raise GateError("unexpected event sequence; keep intent")
    for snapshot in (e for e in events if e["event"] == "snapshot"):
        if (snapshot["bytes"] != 644 or snapshot["fixturePhase"] != 1 or
                snapshot["identitySame"] is not True):
            raise GateError("fixture or identity verification incomplete; keep intent")
    if command == "REPLACE1":
        snapshots = [e for e in events if e["event"] == "snapshot"]
        operations = [e for e in events if e["event"] == "operation"]
        if (len(snapshots) != 1 or snapshots[0]["bytes"] != 644 or snapshots[0]["fixturePhase"] != 1 or
                snapshots[0]["identitySame"] is not True or len(operations) != 1 or
                operations[0]["command"] != command or operations[0]["code"] != 0):
            raise GateError("write verification incomplete; keep intent")
        hex_id(snapshots[0]["sha256"], 64)
    if command == "CLEAR_FIXTURE":
        operations = [e for e in events if e["event"] == "operation"]
        if (len(operations) != 1 or operations[0]["command"] != command or
                operations[0]["code"] != 0):
            raise GateError("clear verification incomplete; keep intent")
    intent["outcome"] = "confirmed"
    intent["events"] = events
    state.update(unknown=False, probeState=result["state"], attemptedMask=mask)
    save_state(state_path, state)
    return events


def restore_tasks(port, state_path: Path, state: dict, original: bytes, exclusive: bool,
                  cold_confirmed: bool, source: str) -> None:
    bind_state(state, state["boardUid"], original)
    if not exclusive or (state.get("unknown") and not cold_confirmed):
        raise GateError("exclusive region / whole-board power-cycle confirmation required")
    if state.get("restoreStarted"):
        raise GateError("restore already attempted; read-only reconciliation required")
    if state.get("restored"):
        raise GateError("session closed; use read-only reconciliation")
    maintenance_profile(port, state["boardUid"], source, 2)
    current = stable_dump(port)
    if current[REGION:] != original[REGION:]:
        raise GateError("identity raw differs; task-only restore must not touch identity")
    if current != original:
        state.update(restoreStarted=True, unknown=True, restoreSourceId=source)
        save_state(state_path, state)
        send(port, b"rBVR1" + struct.pack("<HII", SIZE, zlib.crc32(current), zlib.crc32(original)) + original)
        reply = read_exact(port, 8, time.monotonic() + 180)
        if reply != b"BVA1" + struct.pack("<I", zlib.crc32(original)) or port.in_waiting:
            raise GateError("restore reply uncertain; do not replay")
    if stable_dump(port) != original:
        raise GateError("original raw not independently restored")
    # restoreStarted is an irreversible consumed intent, not a retry switch.
    state.update(unknown=False, restored=True, restoreSourceId=source)
    save_state(state_path, state)


def reconcile(port, state_path: Path, state: dict, original: bytes, cold_confirmed: bool,
              source: str, mode: int = 1) -> bool:
    """Only fresh stable reads; a lost ACK can never cause another restore."""
    bind_state(state, state["boardUid"], original)
    if (state.get("unknown") or state.get("restoreStarted")) and not cold_confirmed:
        raise GateError("whole-board power-cycle required before unknown-result reconciliation")
    if mode not in (1, 2):
        raise GateError("fixed 16 KiB read-only reconciliation mode required")
    maintenance_profile(port, state["boardUid"], source, mode)
    current = stable_dump(port)
    record = {"sourceId": source, "sha256": digest(current),
              "identitySame": current[REGION:] == original[REGION:], "rawRestored": current == original,
              "wholeBoardPowerCycleConfirmed": cold_confirmed}
    index = len(state.get("reconciliations", []))
    save_new(state_path.with_name(f"reconcile-{index:03d}.raw"), current)
    state.setdefault("reconciliations", []).append(record)
    if record["rawRestored"]:
        state.update(unknown=False, restored=True)
    # Differing raw leaves the previous unknown intent intact. No replay or
    # implicit fixture restart is authorized by a successful read.
    save_state(state_path, state)
    if not record["identitySame"]:
        raise GateError("identity changed; task-only recovery must not write identity")
    return record["rawRestored"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("capture", "probe", "restore-tasks", "reconcile", "status"))
    parser.add_argument("--private-dir", type=Path, required=True)
    parser.add_argument("--uid", required=True)
    parser.add_argument("--port")
    parser.add_argument("--source-id")
    parser.add_argument("--command", choices=(*TRANSITIONS, "READ", "STATUS"))
    parser.add_argument("--confirm-exclusive-task-region", action="store_true")
    parser.add_argument("--confirm-whole-board-power-cycle", action="store_true")
    parser.add_argument("--maintenance-mode", type=int, choices=(1, 2), default=1,
                        help="reconcile only: expected already verified fixed-region maintenance mode")
    args = parser.parse_args()
    uid = hex_id(args.uid, 32)
    folder = args.private_dir.resolve()
    if not folder.is_relative_to((ROOT / "build").resolve()):
        raise GateError("private material must stay in the ignored library build directory")
    state_path, original_path = folder / "state.json", folder / "original-qspi.raw"
    if args.action == "status":
        state = json.loads(state_path.read_text())
        bind_state(state, uid, original_path.read_bytes())
        print(json.dumps({"boardUid": uid, "probeState": state.get("probeState"), "unknown": state["unknown"],
                          "restored": state.get("restored", False), "hardwareAdmissionGranted": False}))
        return 0
    if not args.port or not args.source_id:
        raise GateError("explicit port and frozen source identifier required")
    source = hex_id(args.source_id, 64)
    if source == "0" * 64:
        raise GateError("compile-only source placeholder cannot operate hardware")
    import serial
    from serial.tools import list_ports
    matches = [p for p in list_ports.comports() if p.device.casefold() == args.port.casefold() and
               (p.serial_number or "").casefold() == uid and p.vid == 0x2886]
    if len(matches) != 1:
        raise GateError("physical Wio USB identity/port not uniquely matched")
    registry = ROOT / "build" / "wio-storage-gate-targets"
    registry.mkdir(parents=True, exist_ok=True)
    claim = registry / f"{uid}.json"
    if args.action == "capture":
        folder.mkdir(parents=True, exist_ok=True)
    else:
        recorded = json.loads(claim.read_text())
        if (recorded.get("privateDir") != str(folder) or recorded.get("boardUid") != uid or
                recorded.get("phase") != "sealed"):
            raise GateError("target already belongs to another protected session")
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=15) as port:
        if args.action == "capture":
            raw = capture_session(port, folder, claim, uid, source, args.confirm_whole_board_power_cycle)
            print(json.dumps({"capturedBytes": len(raw), "sha256": digest(raw), "regionOwnershipVerified": False}))
        else:
            state = json.loads(state_path.read_text())
            original = original_path.read_bytes()
            bind_state(state, uid, original)
            if args.action == "restore-tasks":
                restore_tasks(port, state_path, state, original, args.confirm_exclusive_task_region,
                              args.confirm_whole_board_power_cycle, source)
                print(json.dumps({"rawRestored": True, "hardwareAdmissionGranted": False}))
            elif args.action == "reconcile":
                same = reconcile(port, state_path, state, original, args.confirm_whole_board_power_cycle,
                                 source, args.maintenance_mode)
                print(json.dumps({"rawRestored": same, "hardwareAdmissionGranted": False}))
            else:
                if args.command is None:
                    raise GateError("explicit finite probe command required")
                print(json.dumps({"events": probe_command(port, state_path, state, args.command, source,
                                                        args.confirm_exclusive_task_region),
                                  "hardwareAdmissionGranted": False}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (GateError, OSError, ValueError):
        # Do not echo malformed frames, arbitrary serial output or private data.
        print(json.dumps({"passed": False, "error": "gate rejected or outcome uncertain; inspect private state, do not replay"}))
        raise SystemExit(1)
