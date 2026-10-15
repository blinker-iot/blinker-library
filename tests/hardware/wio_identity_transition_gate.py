#!/usr/bin/env python3
"""Finite Wio identity transition operator. No upload, reset or retry.

All raw material stays in ignored build directories. An immutable write intent
and a separate receipt distinguish a known result without erasing an unknown
one. Explicit successor admission is offline, append-only and evidence-bound;
it does not grant product release. Physical-epoch recovery and final firmware
restoration are external steps.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import struct
import time
import zlib

import wio_storage_gate as media

ROOT, SIZE, REGION = media.ROOT, media.SIZE, media.REGION
GateError = media.GateError
PHASES = ("prepare", "capture-current", "restore", "reconcile-readonly")
PROOF_FIELDS = {"fixture", "boardUid", "captureSourceId", "baselineSha256", "originalInstanceId",
                "originalMcuSha256", "originalMcuBytes", "maintenanceMode", "stableRawReads",
                "closedProofPath", "closedProofSha256"}
BIND_FIELDS = {"fixture", "boardUid", "sourceId", "originalRawPath", "originalMcuPath", "provenancePath",
               "originalRawSha256", "originalMcuSha256", "provenanceSha256", "originalInstanceId"}
AUTH_FIELDS = {"fixture", "boardUid", "previousPrivateDir", "previousClaimSha256", "previousBindingSha256",
               "closureAuditPath", "closureAuditSha256", "retiredLogicalDeviceId", "retirementLedgerPath",
               "retirementLedgerSha256", "normalDeleteProofPath", "normalDeleteProofSha256",
               "authorizedAtUtc", "authorizationId", "purpose"}


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")


def utc(value: str) -> datetime:
    if not isinstance(value, str) or not value.endswith("Z"):
        raise GateError("explicit UTC event time required")
    try:
        result = datetime.fromisoformat(value[:-1] + "+00:00")
    except ValueError as exc:
        raise GateError("invalid UTC event time") from exc
    if result.utcoffset().total_seconds() != 0:
        raise GateError("UTC time required")
    return result


def encoded(value: dict) -> bytes:
    return (json.dumps(value, sort_keys=True, allow_nan=False) + "\n").encode("ascii")


def read_json(path: Path) -> dict:
    if path.with_name(path.name + ".next").exists():
        raise GateError("incomplete protected transition; retain it without replay")
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise GateError("duplicate protected field")
            result[key] = value
        return result
    try:
        result = json.loads(path.read_bytes(), object_pairs_hook=unique)
    except (ValueError, UnicodeError, OSError) as exc:
        raise GateError("missing or invalid protected JSON") from exc
    if not isinstance(result, dict):
        raise GateError("protected JSON must be an object")
    return result


def private_path(value: Path) -> Path:
    result, build = value.resolve(), (ROOT / "build").resolve()
    if result == build or not result.is_relative_to(build):
        raise GateError("private artifacts must remain inside ignored build")
    return result


def fixed_hex(value: str, length: int) -> str:
    result = media.hex_id(value, length)
    if result != value or result == "0" * length:
        raise GateError("noncanonical or compile-only identifier")
    return result


def registry_path(uid: str) -> Path:
    return ROOT / "build" / "wio-identity-transition-targets" / (uid + ".json")


def checked_artifact(path: Path, digest: str) -> bytes:
    path = private_path(path)
    fixed_hex(digest, 64)
    if path.with_name(path.name + ".next").exists():
        raise GateError("incomplete referenced evidence")
    raw = path.read_bytes()
    if media.digest(raw) != digest:
        raise GateError("protected evidence changed")
    return raw


def successor_directory(uid: str) -> Path:
    return registry_path(uid).with_suffix(".successors")


def claim_valid(claim: dict, uid: str) -> None:
    if (set(claim) != {"boardUid", "privateDir", "bindingSha256"} or claim["boardUid"] != uid or
            str(private_path(Path(claim["privateDir"]))) != claim["privateDir"]):
        raise GateError("invalid physical session claim")
    fixed_hex(claim["bindingSha256"], 64)


def session_snapshot(folder: Path) -> dict:
    if any(folder.glob("*.next")) or any(p.is_dir() for p in folder.iterdir()):
        raise GateError("incomplete or unexpected identity session evidence")
    return {p.name: media.digest(p.read_bytes()) for p in sorted(folder.iterdir()) if p.is_file()}


def closed_predecessor(claim: dict, authorization: dict) -> None:
    """Verify restoration, not product acceptance. Task unknowns stay unknown."""
    uid, folder = claim["boardUid"], private_path(Path(claim["privateDir"]))
    binding, original = load_bound_session(folder, uid, claim)
    if unknown_actions(folder) or any(not (folder / (action + "-phase.json")).exists()
                                      for action in ("prepare", "restore", "reconcile-readonly")):
        raise GateError("predecessor identity transition is not fully confirmed")
    assert_original_restoration(folder, binding)
    audit_path = private_path(Path(authorization["closureAuditPath"]))
    checked_artifact(audit_path, authorization["closureAuditSha256"])
    audit = read_json(audit_path)
    if (audit.get("safeRestorationClosure") != "verified" or
            audit.get("originalUnknownOrFailedEvidenceRemoved") is not False or
            utc(audit["at"]) >= utc(authorization["authorizedAtUtc"])):
        raise GateError("predecessor closure or new authorization is not established")
    stages = audit.get("stages")
    if not isinstance(stages, list) or len({s.get("id") for s in stages}) != len(stages):
        raise GateError("invalid closure stages")
    by_id = {s.get("id"): s for s in stages}
    for name in ("normal-device-removal", "service-restoration", "quota-restoration",
                 "identity-restoration", "mcu-restoration", "inventory-and-app-finish"):
        if by_id.get(name, {}).get("state") != "passed":
            raise GateError("predecessor restoration closure is incomplete")
    evidence = {}
    for stage in stages + [audit.get("abortOrdinaryPropertyRegression", {})]:
        for item in stage.get("evidence", []):
            path = private_path(audit_path.parent / item["name"])
            if not path.is_relative_to(audit_path.parent) or type(item["bytes"]) is not int:
                raise GateError("closure evidence escapes its batch")
            raw = checked_artifact(path, item["sha256"])
            if len(raw) != item["bytes"]:
                raise GateError("closure evidence length changed")
            evidence[str(path)] = item
    mcu = Path(binding["originalMcuPath"]).read_bytes()
    restored = [Path(path) for path in evidence if Path(path).name.startswith("restored-full-")]
    if len(restored) != 1 or restored[0].read_bytes() != mcu or len(mcu) != 524288:
        raise GateError("full original MCU readback is not exact")
    mcu_evidence = by_id["mcu-restoration"]["evidence"]
    boot = [private_path(audit_path.parent / i["name"]).read_bytes() for i in mcu_evidence
            if Path(i["name"]).name.startswith("bootloader-readback-") and i["bytes"] == 16384]
    if not boot or any(raw != mcu[:16384] for raw in boot):
        raise GateError("original bootloader is not independently verified")
    ledger_path = private_path(Path(authorization["retirementLedgerPath"]))
    delete_path = private_path(Path(authorization["normalDeleteProofPath"]))
    for path, field in ((ledger_path, "retirementLedgerSha256"), (delete_path, "normalDeleteProofSha256")):
        if str(path) not in evidence:
            raise GateError("retirement proof is not sealed in the closure audit")
        checked_artifact(path, authorization[field])
    ledger, deletion = read_json(ledger_path), read_json(delete_path)
    logical = authorization["retiredLogicalDeviceId"]
    scopes = [row for row in ledger["v2_task_scopes"] if row.get("logical_device_id") == logical]
    temporary = read_json(folder / "prepare-phase.json")["result"]["temporaryInstanceId"]
    if (not isinstance(logical, str) or not logical.startswith("device_") or len(scopes) != 1 or
            scopes[0].get("state") != "retired" or
            scopes[0].get("device_instance_id") != {"type": "Buffer", "data": list(bytes.fromhex(temporary))}):
        raise GateError("previous temporary cloud scope is not retired")
    receipt = deletion.get("receipt", {})
    data = receipt.get("data", {})
    if (deletion.get("success") is not True or receipt.get("observedFromActualNetwork") is not True or
            receipt.get("httpStatus") not in (200, 202) or data.get("logicalDeviceId") != logical or
            data.get("state") != "deleted" or data.get("cloudAuthorization") != "revoked"):
        raise GateError("normal cloud deletion was not observed")
    if not any(row.get("method") == "DELETE" and row.get("actualNetworkRequest") is True and
               row.get("path") == "/api/v2/devices/" + logical and row.get("responseBodyObserved") is True
               for row in deletion.get("result", {}).get("actualTaskHttp", [])):
        raise GateError("normal deletion has no actual network request")
    # The audit hashes the entire ledger, including pending requests and unknown
    # attempts. Neither acceptance nor MCU retirement verification is required.
    if original != (folder / "independent-mode1.raw").read_bytes():
        raise GateError("mode-1 exact restoration changed")


def validate_authorization(path: Path, claim: dict, claim_bytes: bytes) -> dict:
    authorization = read_json(private_path(path))
    if (set(authorization) != AUTH_FIELDS or
            authorization["fixture"] != "wio-identity-successor-authorization-v1" or
            authorization["purpose"] != "new-limited-task-session-after-exact-restoration" or
            authorization["boardUid"] != claim["boardUid"] or
            authorization["previousPrivateDir"] != claim["privateDir"] or
            authorization["previousClaimSha256"] != media.digest(claim_bytes) or
            authorization["previousBindingSha256"] != claim["bindingSha256"] or
            utc(authorization["authorizedAtUtc"]) > datetime.now(timezone.utc)):
        raise GateError("new-batch authorization is not bound to the previous leaf")
    fixed_hex(authorization["authorizationId"], 32)
    closed_predecessor(claim, authorization)
    return authorization


def current_claim(uid: str) -> tuple[dict, bytes]:
    path = registry_path(uid)
    claim, claim_bytes = read_json(path), path.read_bytes()
    claim_valid(claim, uid)
    directory, visited = successor_directory(uid), set()
    entries = set(directory.iterdir()) if directory.exists() else set()
    if any(not p.is_file() or len(p.stem) != 64 or p.suffix != ".json" for p in entries):
        raise GateError("incomplete or unexpected successor registry entry")
    for entry in entries:
        fixed_hex(entry.stem, 64)
    while True:
        path = directory / (claim["bindingSha256"] + ".json")
        if path not in entries:
            break
        if path in visited:
            raise GateError("successor claim cycle")
        visited.add(path)
        record = read_json(path)
        if set(record) != {"fixture", "previousClaimSha256", "authorizationPath", "authorizationSha256",
                           "previousSessionSnapshot", "claim"} or record["fixture"] != "wio-identity-successor-v1":
            raise GateError("invalid successor claim")
        if record["previousClaimSha256"] != media.digest(claim_bytes):
            raise GateError("successor predecessor claim changed")
        auth_path = private_path(Path(record["authorizationPath"]))
        checked_artifact(auth_path, record["authorizationSha256"])
        validate_authorization(auth_path, claim, claim_bytes)
        if record["previousSessionSnapshot"] != session_snapshot(Path(claim["privateDir"])):
            raise GateError("sealed predecessor identity evidence changed")
        claim, claim_bytes = record["claim"], path.read_bytes()
        claim_valid(claim, uid)
    if entries != visited:
        raise GateError("orphan or forked successor registry entry")
    return claim, claim_bytes


def validate_provenance(path: Path, uid: str, original: bytes, mcu: bytes) -> dict:
    proof = read_json(path)
    if (set(proof) != PROOF_FIELDS or proof["fixture"] != "wio-identity-original-v1" or
            proof["boardUid"] != uid or proof["baselineSha256"] != media.digest(original) or
            proof["originalMcuSha256"] != media.digest(mcu) or len(mcu) != 524288 or
            type(proof["originalMcuBytes"]) is not int or proof["originalMcuBytes"] != len(mcu) or
            type(proof["maintenanceMode"]) is not int or proof["maintenanceMode"] != 1 or
            type(proof["stableRawReads"]) is not int or proof["stableRawReads"] != 2 or
            len(original) != SIZE or original[:REGION] != b"\xff" * REGION):
        raise GateError("original capture provenance is not fixed mode-1 blank-task material")
    for field, width in (("boardUid", 32), ("captureSourceId", 64), ("baselineSha256", 64),
                         ("originalInstanceId", 32), ("originalMcuSha256", 64), ("closedProofSha256", 64)):
        fixed_hex(proof[field], width)
    old_path = private_path(Path(proof["closedProofPath"]))
    old = read_json(old_path)
    if (media.digest(old_path.read_bytes()) != proof["closedProofSha256"] or
            old.get("boardUid") != uid or old.get("restored") is not True or old.get("unknown") is not False):
        raise GateError("previous physical session is not an intact closed proof")
    media.check_read_phases(old_path.parent, uid)
    return proof


def init_session(folder: Path, uid: str, source: str, raw_path: Path, mcu_path: Path,
                 proof_path: Path, predecessor_authorization: Path | None = None) -> dict:
    folder, raw_path, mcu_path, proof_path = map(private_path, (folder, raw_path, mcu_path, proof_path))
    fixed_hex(uid, 32)
    fixed_hex(source, 64)
    original, mcu = raw_path.read_bytes(), mcu_path.read_bytes()
    proof = validate_provenance(proof_path, uid, original, mcu)
    if folder.exists() and any(folder.iterdir()):
        raise GateError("session directory must be new and empty")
    claim_path = registry_path(uid)
    predecessor = None
    if predecessor_authorization is not None:
        predecessor, previous_bytes = current_claim(uid)
        validate_authorization(predecessor_authorization, predecessor, previous_bytes)
        if (folder == Path(predecessor["privateDir"]) or
                raw_path.read_bytes() != Path(read_json(Path(predecessor["privateDir"]) / "binding.json")["originalRawPath"]).read_bytes() or
                mcu_path.read_bytes() != Path(read_json(Path(predecessor["privateDir"]) / "binding.json")["originalMcuPath"]).read_bytes()):
            raise GateError("new session must preserve the exact original physical baseline")
    elif claim_path.exists() or claim_path.with_name(claim_path.name + ".next").exists() or successor_directory(uid).exists():
        raise GateError("physical UID already claimed; another directory cannot unlock it")
    folder.mkdir(parents=True, exist_ok=True)
    claim_path.parent.mkdir(parents=True, exist_ok=True)
    binding = {"fixture": "wio-identity-transition-v1", "boardUid": uid, "sourceId": source,
               "originalRawPath": str(raw_path), "originalMcuPath": str(mcu_path), "provenancePath": str(proof_path),
               "originalRawSha256": media.digest(original), "originalMcuSha256": media.digest(mcu),
               "provenanceSha256": media.digest(proof_path.read_bytes()), "originalInstanceId": proof["originalInstanceId"]}
    # Claim before any serial access or another mutable file. An interrupted
    # init remains blocked for manual reconciliation; it is not auto-resumed.
    claim = {"boardUid": uid, "privateDir": str(folder), "bindingSha256": media.digest(encoded(binding))}
    if predecessor is None:
        media.save_new(claim_path, encoded(claim))
    else:
        if claim["bindingSha256"] == predecessor["bindingSha256"]:
            raise GateError("successor binding must be distinct")
        # Append before the new binding, never replace a claim/head pointer.
        # Interrupted init blocks all access, including the previous session.
        successor_directory(uid).mkdir(exist_ok=True)
        record = {"fixture": "wio-identity-successor-v1", "previousClaimSha256": media.digest(previous_bytes),
                  "authorizationPath": str(private_path(predecessor_authorization)),
                  "authorizationSha256": media.digest(predecessor_authorization.read_bytes()),
                  "previousSessionSnapshot": session_snapshot(Path(predecessor["privateDir"])), "claim": claim}
        media.save_new(successor_directory(uid) / (predecessor["bindingSha256"] + ".json"), encoded(record))
    media.save_new(folder / "binding.json", encoded(binding))
    load_session(folder, uid)
    return binding


def load_session(folder: Path, uid: str) -> tuple[dict, bytes]:
    claim, _ = current_claim(uid)
    return load_bound_session(folder, uid, claim)


def load_bound_session(folder: Path, uid: str, claim: dict) -> tuple[dict, bytes]:
    """Validate one immutable binding without selecting a hardware owner."""
    folder = private_path(folder)
    fixed_hex(uid, 32)
    # A half-written seal may not be ignored by selecting a different action.
    if any(folder.glob("*.next")):
        raise GateError("incomplete session transition; no hardware access")
    binding_path = folder / "binding.json"
    binding = read_json(binding_path)
    if (set(binding) != BIND_FIELDS or binding["fixture"] != "wio-identity-transition-v1" or
            binding["boardUid"] != uid or set(claim) != {"boardUid", "privateDir", "bindingSha256"} or
            claim != {"boardUid": uid, "privateDir": str(folder), "bindingSha256": media.digest(binding_path.read_bytes())}):
        raise GateError("physical session claim/binding mismatch")
    fixed_hex(binding["sourceId"], 64)
    raw_path, mcu_path, proof_path = (private_path(Path(binding[key])) for key in
                                     ("originalRawPath", "originalMcuPath", "provenancePath"))
    original, mcu = raw_path.read_bytes(), mcu_path.read_bytes()
    proof = validate_provenance(proof_path, uid, original, mcu)
    if ((media.digest(original), media.digest(mcu), media.digest(proof_path.read_bytes()), proof["originalInstanceId"]) !=
            (binding["originalRawSha256"], binding["originalMcuSha256"], binding["provenanceSha256"], binding["originalInstanceId"])):
        raise GateError("protected backup or provenance changed")
    validate_records(folder, binding)
    if (folder / "recovery-authority.json").exists():
        recovery_authority(folder, binding)
    return binding, original


def validate_records(folder: Path, binding: dict) -> None:
    binding_sha = media.digest((folder / "binding.json").read_bytes())
    for action in PHASES:
        phase_path = folder / (action + "-phase.json")
        if phase_path.exists():
            value = read_json(phase_path)
            if (set(value) != {"action", "boardUid", "bindingSha256", "sourceId", "startedAtUtc", "outcome", "result"} or
                    value["action"] != action or value["boardUid"] != binding["boardUid"] or
                    value["bindingSha256"] != binding_sha or value["outcome"] not in ("pending", "confirmed") or
                    not isinstance(value["result"], dict)):
                raise GateError("invalid durable phase")
            utc(value["startedAtUtc"])
            fixed_hex(value["sourceId"], 64)
            if action != "reconcile-readonly" and value["sourceId"] != binding["sourceId"]:
                raise GateError("phase source differs from frozen candidate")
            if value["outcome"] == "pending" and value["result"]:
                raise GateError("pending phase cannot carry a success result")
            if value["outcome"] == "confirmed":
                validate_result(folder, binding, action, value["result"])
        for suffix in ("intent", "receipt") if action in ("prepare", "restore") else ():
            path = folder / (action + "-" + suffix + ".json")
            if not path.exists():
                continue
            value = read_json(path)
            fields = ({"action", "boardUid", "bindingSha256", "sourceId", "originalSha256", "outcome"}
                      if suffix == "intent" else {"action", "intentSha256", "result"})
            if set(value) != fields or value["action"] != action:
                raise GateError("invalid persistent mutation record")
            if not phase_path.exists():
                raise GateError("mutation record lost its preceding durable phase")
            if suffix == "intent":
                if value != {"action": action, "boardUid": binding["boardUid"], "bindingSha256": binding_sha,
                             "sourceId": binding["sourceId"], "originalSha256": binding["originalRawSha256"], "outcome": "pending"}:
                    raise GateError("mutation intent is no longer bound")
            elif (value["intentSha256"] != media.digest((folder / (action + "-intent.json")).read_bytes()) or
                  not isinstance(value["result"], dict)):
                raise GateError("mutation receipt not bound to immutable intent")
            elif phase_path.exists():
                phase = read_json(phase_path)
                if phase["outcome"] == "confirmed" and value["result"] != phase["result"]:
                    raise GateError("receipt and confirmed phase disagree")
        if (phase_path.exists() and read_json(phase_path)["outcome"] == "confirmed" and
                action in ("prepare", "restore") and not (folder / (action + "-receipt.json")).exists()):
            raise GateError("confirmed mutation has no immutable intent receipt")


def validate_result(folder: Path, binding: dict, action: str, result: dict) -> None:
    if action == "prepare":
        raw = (folder / "prepared-current.raw").read_bytes()
        if (set(result) != {"temporaryInstanceId", "currentSha256"} or len(raw) != SIZE or
                raw[:REGION] != b"\xff" * REGION or result["currentSha256"] != media.digest(raw) or
                result["temporaryInstanceId"] == binding["originalInstanceId"]):
            raise GateError("confirmed preparation proof changed")
        fixed_hex(result["temporaryInstanceId"], 32)
    elif action == "restore":
        if result != {"deviceVerifiedOriginalSha256": binding["originalRawSha256"],
                      "independentRawVerified": False, "originalFirmwareRestored": False}:
            raise GateError("restore ACK cannot assert independent or firmware restoration")
        if len((folder / "restore-before.raw").read_bytes()) != SIZE:
            raise GateError("restore current-media evidence missing")
    else:
        name = "capture-current.raw" if action == "capture-current" else "independent-mode1.raw"
        raw = (folder / name).read_bytes()
        fields = ({"currentSha256", "deviceState", "rawEqualsOriginal"} if action == "capture-current" else
                  {"currentSha256", "rawEqualsOriginal", "unknownIntentsPreserved", "originalFirmwareRestored"})
        if (set(result) != fields or len(raw) != SIZE or result["currentSha256"] != media.digest(raw) or
                type(result["rawEqualsOriginal"]) is not bool or
                result["rawEqualsOriginal"] != (media.digest(raw) == binding["originalRawSha256"])):
            raise GateError("confirmed read proof changed")
        if action == "capture-current":
            if type(result["deviceState"]) is not int or result["deviceState"] not in (0, 1):
                raise GateError("invalid captured device state")
        elif result["unknownIntentsPreserved"] is not True or result["originalFirmwareRestored"] is not False:
            raise GateError("read reconciliation must retain old intents and firmware boundary")


def unknown_actions(folder: Path) -> list[str]:
    return [action for action in PHASES if (folder / (action + "-phase.json")).exists() and
            read_json(folder / (action + "-phase.json"))["outcome"] != "confirmed"]


def recovery_binding(folder: Path, binding: dict, witness_path: Path) -> dict:
    """A NEW external physical observation, not an old power boolean unlock.

    This grants only the first exact-original restoration. It never confirms
    the old preparation, never permits t again, and never permits r replay.
    """
    prepare_path, intent_path = folder / "prepare-phase.json", folder / "prepare-intent.json"
    capture_path = folder / "capture-current-phase.json"
    before, capture = read_json(prepare_path), read_json(capture_path)
    if (before["outcome"] != "pending" or capture["outcome"] != "confirmed" or
            capture["result"].get("deviceState") != 0 or not intent_path.exists()):
        raise GateError("recovery needs existing unknown preparation and a fresh confirmed raw observation")
    witness_path = private_path(witness_path)
    witness = read_json(witness_path)
    fields = {"fixture", "boardUid", "bindingSha256", "preparePhaseSha256", "prepareIntentSha256",
              "observedAtUtc", "observationId", "wholeBoardPowerCycle"}
    expected = {"fixture": "wio-identity-power-cycle-v1", "boardUid": binding["boardUid"],
                "bindingSha256": media.digest((folder / "binding.json").read_bytes()),
                "preparePhaseSha256": media.digest(prepare_path.read_bytes()),
                "prepareIntentSha256": media.digest(intent_path.read_bytes())}
    if (set(witness) != fields or any(witness[key] != value for key, value in expected.items()) or
            witness["wholeBoardPowerCycle"] is not True or
            not (utc(before["startedAtUtc"]) < utc(witness["observedAtUtc"]) < utc(capture["startedAtUtc"]) <=
                 datetime.now(timezone.utc))):
        raise GateError("new physical epoch witness is not ordered after this unknown and before its raw observation")
    fixed_hex(witness["observationId"], 32)
    return {"fixture": "wio-identity-recovery-authority-v1", "boardUid": binding["boardUid"],
            "bindingSha256": expected["bindingSha256"], "originalSha256": binding["originalRawSha256"],
            "preparePhaseSha256": expected["preparePhaseSha256"], "prepareIntentSha256": expected["prepareIntentSha256"],
            "powerCycleProofPath": str(witness_path), "powerCycleProofSha256": media.digest(witness_path.read_bytes()),
            "capturePhaseSha256": media.digest(capture_path.read_bytes()),
            "currentSha256": media.digest((folder / "capture-current.raw").read_bytes())}


def recovery_authority(folder: Path, binding: dict) -> dict:
    value = read_json(folder / "recovery-authority.json")
    if not isinstance(value.get("powerCycleProofPath"), str):
        raise GateError("recovery authority has no protected physical proof")
    if value != recovery_binding(folder, binding, Path(value["powerCycleProofPath"])):
        raise GateError("recovery authority or its protected evidence changed")
    return value


def authorize_recovery(folder: Path, uid: str, witness_path: Path) -> dict:
    binding, _ = load_session(folder, uid)
    if any((folder / ("restore-" + suffix + ".json")).exists() for suffix in ("phase", "intent", "receipt")):
        raise GateError("restoration already consumed; no authority can allow replay")
    if unknown_actions(folder) != ["prepare"]:
        raise GateError("recovery authority is limited to the original unknown preparation")
    value = recovery_binding(folder, binding, witness_path)
    media.save_new(folder / "recovery-authority.json", encoded(value))
    return value


def begin_phase(folder: Path, binding: dict, action: str, source: str, readonly: bool = False) -> dict:
    load_session(folder, binding["boardUid"])
    unknown = unknown_actions(folder)
    recovery = (action == "restore" and unknown == ["prepare"] and
                (folder / "recovery-authority.json").exists())
    if recovery:
        recovery_authority(folder, binding)
    if action not in PHASES or (unknown and not readonly and not recovery):
        raise GateError("unknown earlier phase; no replay or mutation unlock")
    if action == "prepare" and any((folder / (name + "-phase.json")).exists() for name in PHASES):
        raise GateError("temporary preparation is legal only as the first hardware action")
    if action == "restore" and not recovery and not (folder / "prepare-receipt.json").exists():
        raise GateError("restore requires this session's consumed preparation")
    path = folder / (action + "-phase.json")
    value = {"action": action, "boardUid": binding["boardUid"],
             "bindingSha256": media.digest((folder / "binding.json").read_bytes()),
             "sourceId": source, "startedAtUtc": utc_now(), "outcome": "pending", "result": {}}
    media.save_new(path, encoded(value))
    return value


def active_phase(folder: Path, binding: dict, phase: dict, action: str) -> None:
    load_session(folder, binding["boardUid"])
    if (phase.get("action") != action or phase.get("outcome") != "pending" or
            (folder / (action + "-phase.json")).read_bytes() != encoded(phase)):
        raise GateError("exclusive durable phase required before first serial request")


def seal_phase(folder: Path, binding: dict, phase: dict, result: dict) -> None:
    load_session(folder, binding["boardUid"])
    path = folder / (phase["action"] + "-phase.json")
    if path.read_bytes() != encoded(phase):
        raise GateError("phase changed while hardware operation was active")
    media.save_state(path, {**phase, "outcome": "confirmed", "result": result})


def mutation_intent(folder: Path, binding: dict, action: str) -> None:
    load_session(folder, binding["boardUid"])
    value = {"action": action, "boardUid": binding["boardUid"],
             "bindingSha256": media.digest((folder / "binding.json").read_bytes()),
             "sourceId": binding["sourceId"], "originalSha256": binding["originalRawSha256"], "outcome": "pending"}
    media.save_new(folder / (action + "-intent.json"), encoded(value))


def mutation_receipt(folder: Path, action: str, result: dict) -> None:
    intent = folder / (action + "-intent.json")
    media.save_new(folder / (action + "-receipt.json"), encoded(
        {"action": action, "intentSha256": media.digest(intent.read_bytes()), "result": result}))


def profile(port, binding: dict, expected: tuple[int, ...]) -> int:
    media.send(port, b"p")
    raw = media.frame(port, b"BVI1", 96, time.monotonic() + 15)
    if (struct.unpack("<BIIIH", raw[:15]) != (1, 4194304, media.TASK_BASE, media.IDENTITY_BASE, REGION) or
            raw[15:31].hex() != binding["boardUid"] or raw[31:63].hex() != binding["sourceId"] or
            raw[63:95].hex() != binding["originalRawSha256"] or raw[95] not in expected):
        raise GateError("transition fixture identity/source/baseline/state mismatch")
    return raw[95]


def prepare(port, folder: Path, binding: dict, phase: dict) -> dict:
    active_phase(folder, binding, phase, "prepare")
    profile(port, binding, (0,))
    mutation_intent(folder, binding, "prepare")
    media.send(port, b"t")
    reply = media.frame(port, b"BVT1", 48, time.monotonic() + 180)
    temporary = reply[:16].hex()
    if temporary in (binding["originalInstanceId"], "0" * 32):
        raise GateError("temporary identity did not change")
    profile(port, binding, (1,))
    current = media.dump(port)  # exactly once: device internally double-verifies
    media.save_new(folder / "prepared-current.raw", current)
    if media.digest(current) != reply[16:].hex() or current[:REGION] != b"\xff" * REGION:
        raise GateError("temporary identity capture/hash/task baseline mismatch")
    profile(port, binding, (1,))
    result = {"temporaryInstanceId": temporary, "currentSha256": media.digest(current)}
    mutation_receipt(folder, "prepare", result)
    seal_phase(folder, binding, phase, result)
    return result


def capture_current(port, folder: Path, binding: dict, phase: dict) -> dict:
    active_phase(folder, binding, phase, "capture-current")
    state = profile(port, binding, (0, 1))
    current = media.dump(port)
    media.save_new(folder / "capture-current.raw", current)
    profile(port, binding, (state,))
    result = {"currentSha256": media.digest(current), "deviceState": state,
              "rawEqualsOriginal": media.digest(current) == binding["originalRawSha256"]}
    seal_phase(folder, binding, phase, result)
    return result


def restore(port, folder: Path, binding: dict, original: bytes, phase: dict) -> dict:
    active_phase(folder, binding, phase, "restore")
    if media.digest(original) != binding["originalRawSha256"]:
        raise GateError("restore payload changed")
    # The fresh fixture may have no valid BDN2 after an interrupted transition.
    # This read captures reality; it does not infer a successful old mutation.
    profile(port, binding, (0,))
    recovery = (folder / "recovery-authority.json").exists()
    if recovery:
        recovery_authority(folder, binding)
        # capture-current already consumed the controller's one d for this
        # physical epoch. restoreOriginal performs its own fresh native reads.
        current = (folder / "capture-current.raw").read_bytes()
    else:
        current = media.dump(port)
    media.save_new(folder / "restore-before.raw", current)
    if not recovery:
        profile(port, binding, (0,))
    load_session(folder, binding["boardUid"])
    mutation_intent(folder, binding, "restore")
    request = b"rBVR1" + struct.pack("<HI", SIZE, zlib.crc32(original)) + original
    media.send(port, request)
    reply = media.frame(port, b"BVA1", 32, time.monotonic() + 180)
    if reply.hex() != binding["originalRawSha256"]:
        raise GateError("restore receipt digest mismatch; retain unknown intent")
    profile(port, binding, (2,))
    result = {"deviceVerifiedOriginalSha256": reply.hex(), "independentRawVerified": False,
              "originalFirmwareRestored": False}
    mutation_receipt(folder, "restore", result)
    seal_phase(folder, binding, phase, result)
    return result


def reconcile_readonly(port, folder: Path, binding: dict, original: bytes,
                       phase: dict, maintenance_source: str) -> dict:
    active_phase(folder, binding, phase, "reconcile-readonly")
    media.maintenance_profile(port, binding["boardUid"], maintenance_source, 1)
    current = media.stable_dump(port)
    media.save_new(folder / "independent-mode1.raw", current)
    result = {"currentSha256": media.digest(current), "rawEqualsOriginal": current == original,
              "unknownIntentsPreserved": True, "originalFirmwareRestored": False}
    seal_phase(folder, binding, phase, result)
    return result


def assert_original_restoration(folder: Path, binding: dict) -> None:
    final = read_json(folder / "reconcile-readonly-phase.json")
    if (final["outcome"] != "confirmed" or final["result"].get("rawEqualsOriginal") is not True or
            final["result"].get("currentSha256") != binding["originalRawSha256"] or
            (folder / "independent-mode1.raw").read_bytes() != Path(binding["originalRawPath"]).read_bytes()):
        raise GateError("independent mode-1 restoration is not exact")


def assert_upload_allowed(folder: Path, uid: str, purpose: str) -> None:
    """External uploader guard; never uploads itself or accepts a power boolean.

    verify-original means mode-1 only, not ordinary runtime/original firmware.
    The uploader must additionally match its frozen image/profile and UID.
    """
    folder = private_path(folder)
    binding, _ = load_session(folder, uid)
    unknown = unknown_actions(folder)
    recovered = (unknown == ["prepare"] and (folder / "recovery-authority.json").exists() and
                 (folder / "restore-receipt.json").exists() and purpose in ("verify-original", "original"))
    if recovered:
        recovery_authority(folder, binding)
    if unknown and not recovered:
        raise GateError("pending media phase prohibits RAM reconstruction by upload")
    records = {action: read_json(folder / (action + "-phase.json")) if
               (folder / (action + "-phase.json")).exists() else None for action in PHASES}
    for action in ("prepare", "restore"):
        if (folder / (action + "-intent.json")).exists() and not (folder / (action + "-receipt.json")).exists() and not (
                recovered and action == "prepare"):
            raise GateError("unknown write result prohibits an upload unlock")
    if purpose == "transition":
        allowed = records["restore"] is None and records["reconcile-readonly"] is None
    elif purpose == "runtime":
        allowed = records["prepare"] is not None and records["restore"] is None and records["reconcile-readonly"] is None
    elif purpose == "verify-original":
        allowed = records["restore"] is not None and records["reconcile-readonly"] is None
    elif purpose == "original":
        final = records["reconcile-readonly"]
        allowed = (records["restore"] is not None and final is not None and
                   final["result"].get("rawEqualsOriginal") is True and
                   final["result"].get("currentSha256") == binding["originalRawSha256"] and
                   (folder / "independent-mode1.raw").read_bytes() == Path(binding["originalRawPath"]).read_bytes())
    else:
        raise GateError("unknown uploader purpose")
    if not allowed:
        raise GateError("requested upload is not legal at this confirmed stage")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("init", "init-successor", "status", "authorize-recovery", *PHASES))
    parser.add_argument("--private-dir", type=Path, required=True)
    parser.add_argument("--uid", required=True)
    parser.add_argument("--source-id")
    parser.add_argument("--port")
    parser.add_argument("--original-raw", type=Path)
    parser.add_argument("--original-mcu", type=Path)
    parser.add_argument("--provenance", type=Path)
    parser.add_argument("--maintenance-source-id")
    parser.add_argument("--power-cycle-proof", type=Path)
    parser.add_argument("--predecessor-authorization", type=Path)
    args = parser.parse_args(argv)
    folder, uid = private_path(args.private_dir), fixed_hex(args.uid, 32)
    if args.action in ("init", "init-successor"):
        if not all((args.source_id, args.original_raw, args.original_mcu, args.provenance)):
            raise GateError("init requires frozen source and real original backup provenance")
        if (args.action == "init-successor") != (args.predecessor_authorization is not None):
            raise GateError("new batch needs explicit predecessor authorization, not ordinary init")
        init_session(folder, uid, args.source_id, args.original_raw, args.original_mcu, args.provenance,
                     args.predecessor_authorization)
        print(json.dumps({"initialized": True, "hardwareAdmissionGranted": False}))
        return 0
    binding, original = load_session(folder, uid)
    if args.action == "authorize-recovery":
        if args.power_cycle_proof is None:
            raise GateError("new protected physical epoch proof required")
        authorize_recovery(folder, uid, args.power_cycle_proof)
        print(json.dumps({"exactOriginalRecoveryAuthorized": True, "unknownPreparationPreserved": True}))
        return 0
    if args.action == "status":
        print(json.dumps({"boardUid": uid, "unknownPhases": unknown_actions(folder),
                          "originalFirmwareRestored": False, "hardwareAdmissionGranted": False}))
        return 0
    if not args.port or args.source_id != binding["sourceId"]:
        raise GateError("explicit physical port and exact frozen fixture source required")
    if args.action == "reconcile-readonly":
        source = fixed_hex(args.maintenance_source_id, 64)
    else:
        source = binding["sourceId"]
    # Validate all state BEFORE importing serial/enumerating/opening hardware.
    # Persist even p/d access before Serial(): opening CDC is not a retry switch.
    phase = begin_phase(folder, binding, args.action, source,
                        readonly=args.action in ("capture-current", "reconcile-readonly"))
    import serial
    from serial.tools import list_ports
    matches = [p for p in list_ports.comports() if p.device.casefold() == args.port.casefold() and
               (p.serial_number or "").casefold() == uid and p.vid == 0x2886]
    if len(matches) != 1:
        raise GateError("physical Wio identity/port is not uniquely matched")
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=15) as port:
        if args.action == "prepare":
            result = prepare(port, folder, binding, phase)
        elif args.action == "capture-current":
            result = capture_current(port, folder, binding, phase)
        elif args.action == "restore":
            result = restore(port, folder, binding, original, phase)
        else:
            result = reconcile_readonly(port, folder, binding, original, phase, source)
    print(json.dumps({"result": result, "hardwareAdmissionGranted": False}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (GateError, OSError, ValueError, TypeError):
        print(json.dumps({"passed": False, "error": "gate rejected or result uncertain; retain private intents, do not replay"}))
        raise SystemExit(1)
