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
COLD_STATES = ("Fresh", "ObservedSnapshot", "ObservedNotFound", "Failed")
TRANSITIONS = {"OPEN_CAPTURE": ("Fresh", "Captured"), "REGION_ATTEST": ("Captured", "Admitted"),
               "REPLACE1": ("Admitted", "Written"), "IDENTITY_END": ("Written", "IdentityEnded"),
               "CLEAR_FIXTURE": ("IdentityEnded", "Cleared")}
READ_PHASE_FILES = {"prepare-cold": "prepare-cold-phase.json", "restore-tasks": "restore-tasks-phase.json"}


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
        # A final native read may return all remaining bytes only after its
        # deadline. Leaving the loop must not turn that late result into a
        # confirmed metadata/ACK/raw frame; retain the caller's pending intent.
        if time.monotonic() >= deadline:
            raise GateError("late response; stream must be closed")
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
        "cold-profile": {"event", "fixture", "sourceId", "boardUid", "baselineSha256", "identitySha256",
                         "currentSha256", "intentSha256", "capacity", "payloadBytes", "taskBase",
                         "identityBase", "regionBytes"},
        "cold-observation": {"event", "bytes", "sha256", "records", "identitySame", "writeConfirmed"},
        "cold-done": {"event", "command", "code", "state"},
    }
    if (not isinstance(value, dict) or not isinstance(value.get("event"), str) or
            set(value) != schemas.get(value.get("event"))):
        raise GateError("unexpected fixture fields; raw output not retained")
    text = {"event", "command", "state", "fixture", "sourceId", "boardUid", "baselineSha256", "sha256",
            "identitySha256", "currentSha256", "intentSha256"}
    if any(not isinstance(v, str) for k, v in value.items() if k in text):
        raise GateError("invalid textual fixture field")
    flags = {"identitySame", "writeConfirmed"}
    if any(type(v) is not int or v < 0 for k, v in value.items() if k not in text and k not in flags):
        raise GateError("invalid numeric fixture field")
    if any(type(v) is not bool for k, v in value.items() if k in flags):
        raise GateError("invalid comparison/confirmation flag")
    if value["event"] == "done" and (value["state"] not in STATES or value["attemptedMask"] > 3):
        raise GateError("invalid fixture state")
    if value["event"] == "cold-done" and value["state"] not in COLD_STATES:
        raise GateError("invalid cold fixture state")
    for key, width in (("sourceId", 64), ("boardUid", 32), ("baselineSha256", 64), ("sha256", 64),
                       ("identitySha256", 64), ("currentSha256", 64), ("intentSha256", 64)):
        if key in value:
            if key == "sha256" and value["event"] == "cold-observation" and value["sha256"] == "":
                continue # Explicit NotFound is validated by its consumer, never a zero snapshot.
            hex_id(value[key], width)
    return value


def exchange(port, command: str, done_event: str = "done") -> list[dict]:
    send(port, command.encode("ascii") + b"\n")
    events = []
    deadline = time.monotonic() + 180
    while len(events) < 12:
        event = receive_event(port, deadline)
        events.append(event)
        if event["event"] in ("done", "cold-done"):
            if event["event"] != done_event:
                raise GateError("wrong fixture protocol")
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


def check_read_phases(folder: Path, uid: str | None = None, baseline_sha: str | None = None) -> None:
    """Pending/partial media phases never unlock on an old power boolean.

    The uploader may reuse this same check. Explicit cold reconciliation may
    read for manual diagnosis, but never edits/seals/removes these phase files.
    """
    fields = {"phase", "boardUid", "baselineSha256", "sourceId", "stateSha256", "outcome"}
    def unique_phase(pairs):
        value = {}
        for key, item in pairs:
            if key in value:
                raise GateError("duplicate media phase field")
            value[key] = item
        return value
    for phase, name in READ_PHASE_FILES.items():
        path = folder / name
        if path.with_name(name + ".next").exists():
            raise GateError("incomplete media phase transition; preserve files for manual reconciliation")
        if not path.exists():
            continue
        try:
            value = json.loads(path.read_bytes(), object_pairs_hook=unique_phase)
            if (not isinstance(value, dict) or set(value) != fields or value["phase"] != phase or
                    value["outcome"] != "confirmed"):
                raise GateError("media read phase pending or invalid; no replay/unlock")
            for key, width in (("boardUid", 32), ("baselineSha256", 64), ("sourceId", 64), ("stateSha256", 64)):
                hex_id(value[key], width)
            if ((uid is not None and value["boardUid"] != uid) or
                    (baseline_sha is not None and value["baselineSha256"] != baseline_sha)):
                raise GateError("media read phase belongs to another protected session")
        except (OSError, ValueError, TypeError) as exc:
            raise GateError("media read phase pending, damaged or mismatched; preserve it, do not retry") from exc


def begin_read_phase(folder: Path, phase: str, uid: str, source: str,
                     original: bytes, state_bytes: bytes) -> tuple[Path, dict]:
    if phase not in READ_PHASE_FILES:
        raise GateError("unknown fixed media phase")
    check_read_phases(folder, uid, digest(original))
    path = folder / READ_PHASE_FILES[phase]
    if path.exists() or (folder / "state.json.next").exists():
        raise GateError("media phase already consumed or protected state incomplete")
    value = {"phase": phase, "boardUid": uid, "baselineSha256": digest(original),
             "sourceId": source, "stateSha256": digest(state_bytes), "outcome": "pending"}
    save_new(path, (json.dumps(value, sort_keys=True) + "\n").encode())
    return path, value


def seal_read_phase(path: Path, value: dict) -> None:
    if path.read_bytes() != (json.dumps(value, sort_keys=True) + "\n").encode():
        raise GateError("media phase changed during operation; preserve it, do not continue")
    save_state(path, {**value, "outcome": "confirmed"})


def capture_session(port, folder: Path, claim_path: Path, uid: str, source: str, cold_confirmed: bool) -> bytes:
    """A failed read may resume only its original empty, pending-capture session."""
    check_read_phases(folder, uid)
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
    check_read_phases(state_path.parent, state["boardUid"], state["baselineSha256"])
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
    check_read_phases(state_path.parent, state["boardUid"], state["baselineSha256"])
    state_bytes = state_path.read_bytes()
    if (json.loads(state_bytes) != state or (state_path.parent / "original-qspi.raw").read_bytes() != original):
        raise GateError("protected restore inputs changed before media access")
    phase_path, phase = begin_read_phase(state_path.parent, "restore-tasks", state["boardUid"],
                                         source, original, state_bytes)
    maintenance_profile(port, state["boardUid"], source, 2)
    current = stable_dump(port)
    if (state_path.read_bytes() != state_bytes or (state_path.parent / "original-qspi.raw").read_bytes() != original):
        raise GateError("protected restore inputs changed during preflight; do not write")
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
    if (state_path.parent / "original-qspi.raw").read_bytes() != original:
        raise GateError("protected original raw changed during restore")
    # restoreStarted is an irreversible consumed intent, not a retry switch.
    state.update(unknown=False, restored=True, restoreSourceId=source)
    save_state(state_path, state)
    seal_read_phase(phase_path, phase)


def reconcile(port, state_path: Path, state: dict, original: bytes, cold_confirmed: bool,
              source: str, mode: int = 1) -> bool:
    """Only fresh stable reads; a lost ACK can never cause another restore."""
    bind_state(state, state["boardUid"], original)
    if not cold_confirmed:
        check_read_phases(state_path.parent, state["boardUid"], state["baselineSha256"])
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


def intent_digest(intent: dict) -> str:
    return digest(json.dumps(intent, sort_keys=True, separators=(",", ":"),
                             ensure_ascii=True, allow_nan=False).encode("ascii"))


def cold_intent(state: dict, original: bytes) -> tuple[int, dict]:
    """Bind an existing attempted write, never invent a successful write receipt."""
    bind_state(state, state["boardUid"], original)
    if state.get("restoreStarted") or original[:REGION] != b"\xff" * REGION:
        raise GateError("cold observation requires the original blank task baseline before any restore")
    intents = state.get("intents", [])
    matches = [(index, item) for index, item in enumerate(intents) if item.get("command") == "REPLACE1"]
    if len(matches) != 1 or any(item.get("command") == "CLEAR_FIXTURE" for item in intents):
        raise GateError("one existing replace intent without later clear is required")
    index, intent = matches[0]
    if (intent.get("sourceId") != state.get("probeSourceId") or
            intent.get("stateBefore") != "Admitted" or intent.get("outcome") not in ("pending", "confirmed")):
        raise GateError("write intent does not belong to the protected probe session")
    hex_id(intent.get("sourceId"), 64)
    return index, intent


def prepare_cold(port, folder: Path, uid: str, source: str, cold_confirmed: bool) -> dict:
    """Only p,d,d; leave original state bytes and unknown intent untouched."""
    if not cold_confirmed:
        raise GateError("explicit whole-board power-cycle confirmation required")
    check_read_phases(folder, uid)
    request_path, raw_path, state_path = folder / "cold-request.json", folder / "cold-current.raw", folder / "state.json"
    if any(path.exists() for path in (request_path, raw_path, folder / "cold-attempt.json", folder / "state.json.next")):
        raise GateError("cold preparation already exists; preserve files, do not replace proof")
    state_bytes = state_path.read_bytes()
    state, original = json.loads(state_bytes), (folder / "original-qspi.raw").read_bytes()
    bind_state(state, uid, original)
    index, intent = cold_intent(state, original)
    phase_path, phase = begin_read_phase(folder, "prepare-cold", uid, source, original, state_bytes)
    maintenance_profile(port, uid, source, 1)
    current = stable_dump(port)
    if state_path.read_bytes() != state_bytes or (folder / "original-qspi.raw").read_bytes() != original:
        raise GateError("protected input changed during cold capture")
    # Retain the actual read even if identity is different; never repair it.
    save_new(raw_path, current)
    if current[REGION:] != original[REGION:]:
        raise GateError("identity differs from original; no recovery write is permitted")
    request = {"fixture": "wio-task-cold-v1", "boardUid": uid,
               "baselineSha256": digest(original), "identitySha256": digest(original[REGION:]),
               "currentSha256": digest(current), "intentSha256": intent_digest(intent),
               "stateSha256": digest(state_bytes), "intentIndex": index,
               "probeSourceId": state["probeSourceId"], "captureSourceId": source,
               "wholeBoardPowerCycleConfirmed": True}
    save_new(request_path, (json.dumps(request, sort_keys=True) + "\n").encode())
    if raw_path.read_bytes() != current or cold_request(folder, uid) != request:
        raise GateError("cold proof private readback mismatch; preserve pending phase")
    seal_read_phase(phase_path, phase)
    return request


def cold_request(folder: Path, uid: str) -> dict:
    request = json.loads((folder / "cold-request.json").read_bytes())
    fields = {"fixture", "boardUid", "baselineSha256", "identitySha256", "currentSha256", "intentSha256",
              "stateSha256", "intentIndex", "probeSourceId", "captureSourceId", "wholeBoardPowerCycleConfirmed"}
    if not isinstance(request, dict) or set(request) != fields:
        raise GateError("invalid cold proof schema")
    state_bytes = (folder / "state.json").read_bytes()
    if (folder / "state.json.next").exists():
        raise GateError("protected state has an incomplete transition")
    state, original = json.loads(state_bytes), (folder / "original-qspi.raw").read_bytes()
    current = (folder / "cold-current.raw").read_bytes()
    bind_state(state, uid, original)
    index, intent = cold_intent(state, original)
    expected = {"fixture": "wio-task-cold-v1", "boardUid": uid,
                "baselineSha256": digest(original), "identitySha256": digest(original[REGION:]),
                "currentSha256": digest(current), "intentSha256": intent_digest(intent),
                "stateSha256": digest(state_bytes), "intentIndex": index,
                "probeSourceId": state["probeSourceId"], "captureSourceId": request["captureSourceId"],
                "wholeBoardPowerCycleConfirmed": True}
    hex_id(request["captureSourceId"], 64)
    if (request != expected or type(request["intentIndex"]) is not int or
            request["wholeBoardPowerCycleConfirmed"] is not True or
            len(current) != SIZE or current[REGION:] != original[REGION:]):
        raise GateError("cold proof no longer matches original raw/state/intent")
    return request


def observe_cold(port, folder: Path, uid: str, source: str) -> dict:
    check_read_phases(folder, uid)
    request = cold_request(folder, uid)
    check_read_phases(folder, uid, request["baselineSha256"])
    attempt_path = folder / "cold-attempt.json"
    if attempt_path.exists() or attempt_path.with_name("cold-attempt.json.next").exists():
        raise GateError("cold observation already attempted; never automatically resend")
    handshake = exchange(port, "PING", "cold-done")
    if ([item["event"] for item in handshake] != ["cold-profile", "cold-done"] or
            handshake[-1] != {"event": "cold-done", "command": "PING", "code": 0, "state": "Fresh"}):
        raise GateError("cold fixture profile handshake rejected")
    expected = {"event": "cold-profile", "fixture": request["fixture"], "sourceId": source, "boardUid": uid,
                **{key: request[key] for key in ("baselineSha256", "identitySha256", "currentSha256", "intentSha256")},
                "capacity": 4, "payloadBytes": 644, "taskBase": TASK_BASE,
                "identityBase": IDENTITY_BASE, "regionBytes": REGION}
    if handshake[0] != expected:
        raise GateError("cold fixture board/source/proof mismatch")
    if cold_request(folder, uid) != request:
        raise GateError("protected inputs changed during profile handshake; do not observe")
    attempt = {"command": "OBSERVE", "sourceId": source, "proofSha256": digest((folder / "cold-request.json").read_bytes()),
               "outcome": "pending"}
    save_new(attempt_path, (json.dumps(attempt, sort_keys=True) + "\n").encode())
    # No edit to state.json, original raw, old intents or unknown flags.
    if cold_request(folder, uid) != request:
        raise GateError("protected inputs changed before observation; keep pending read attempt")
    result = exchange(port, "OBSERVE", "cold-done")
    if [item["event"] for item in result] != ["cold-observation", "cold-done"]:
        raise GateError("cold observation incomplete; keep pending read attempt")
    observation, done = result
    present = done == {"event": "cold-done", "command": "OBSERVE", "code": 0, "state": "ObservedSnapshot"}
    missing = done == {"event": "cold-done", "command": "OBSERVE", "code": 19, "state": "ObservedNotFound"}
    if (not (present or missing) or observation["identitySame"] is not True or
            observation["writeConfirmed"] is not False or
            (observation["bytes"], observation["records"]) != ((644, 4) if present else (0, 0)) or
            (missing and observation["sha256"] != "")):
        raise GateError("cold observation invalid; original write remains unconfirmed")
    if present:
        hex_id(observation["sha256"], 64)
    if cold_request(folder, uid) != request:
        raise GateError("protected inputs changed during observation")
    attempt.update(outcome="observed", events=result)
    save_state(attempt_path, attempt)
    return observation


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("capture", "probe", "restore-tasks", "reconcile", "prepare-cold", "observe-cold", "status"))
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
    if args.action != "reconcile" or not args.confirm_whole_board_power_cycle:
        baseline_sha = digest(original_path.read_bytes()) if original_path.exists() else None
        check_read_phases(folder, uid, baseline_sha)
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
            elif args.action == "prepare-cold":
                request = prepare_cold(port, folder, uid, source, args.confirm_whole_board_power_cycle)
                print(json.dumps({"coldRequest": request, "hardwareAdmissionGranted": False}))
            elif args.action == "observe-cold":
                print(json.dumps({"observation": observe_cold(port, folder, uid, source),
                                  "hardwareAdmissionGranted": False}))
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
