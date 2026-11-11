"""Offline protocol/recovery regressions; never imports pyserial or opens USB."""
from __future__ import annotations

import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import time
import unittest
from unittest import mock
import zlib

import wio_storage_gate as gate

UID, SOURCE = "12" * 16, "34" * 32
ORIGINAL = b"\xff" * gate.REGION + b"i" * gate.REGION
CHANGED = b"t" * gate.REGION + ORIGINAL[gate.REGION:]


def framed(magic, raw):
    return magic + struct.pack("<HI", len(raw), zlib.crc32(raw)) + raw


def metadata(mode=1, uid=UID, source=SOURCE):
    return framed(b"BVP1", struct.pack("<BIIIH", mode, 4194304, gate.TASK_BASE,
                                     gate.IDENTITY_BASE, gate.REGION) + bytes.fromhex(uid + source))


def completion(command, state="Fresh", mask=0, code=0):
    return {"event": "done", "command": command, "code": code, "state": state, "attemptedMask": mask}


def profile(state="Fresh", mask=0, source=SOURCE):
    return [{"event": "profile", "fixture": "wio-task-storage-v1", "sourceId": source,
             "boardUid": UID, "baselineSha256": gate.digest(ORIGINAL), "capacity": 4,
             "payloadBytes": 644, "journalBytes": 664, "taskBase": gate.TASK_BASE,
             "identityBase": gate.IDENTITY_BASE, "regionBytes": gate.REGION, "regionReserved": 1},
            completion("PING", state, mask)]


def snapshot(identity=True):
    return {"event": "snapshot", "bytes": 644, "sha256": "56" * 32,
            "fixturePhase": 1, "identitySame": identity}


def events(values):
    return b"".join((json.dumps(value) + "\n").encode() for value in values)


class Wire:
    def __init__(self, exchanges=(), pending=b"", partial=False):
        self.exchanges = list(exchanges)
        self.pending = bytearray(pending)
        self.writes = []
        self.partial = partial

    @property
    def in_waiting(self):
        return len(self.pending)

    def read(self, count):
        if not self.pending:
            raise gate.GateError("test: response lost")
        part = self.pending[:count]
        del self.pending[:count]
        return bytes(part)

    def write(self, raw):
        self.writes.append(raw)
        if self.partial:
            return len(raw) - 1
        expected, response = self.exchanges.pop(0)
        if expected != raw:
            raise AssertionError("unexpected request; possible replay")
        self.pending.extend(response)
        return len(raw)


class WioStorageGateTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.path = Path(self.folder.name) / "state.json"
        self.state = {"boardUid": UID, "baselineSha256": gate.digest(ORIGINAL), "unknown": False,
                      "probeState": "Fresh", "attemptedMask": 0, "intents": []}

    def tearDown(self):
        self.folder.cleanup()

    def persisted(self):
        return json.loads(self.path.read_text())

    def test_exact_frame(self):
        self.assertEqual(gate.frame(Wire(pending=framed(b"BVS1", ORIGINAL)), b"BVS1", gate.SIZE,
                                    time.monotonic() + 1), ORIGINAL)

    def test_metadata_last_read_at_deadline_is_rejected(self):
        clock = [0.0]
        class LateWire(Wire):
            def read(wire, count):
                value = super().read(count)
                if not wire.pending:
                    clock[0] = 15.0
                return value
        port = LateWire(pending=metadata())
        with mock.patch.object(gate.time, "monotonic", side_effect=lambda: clock[0]), self.assertRaises(gate.GateError):
            gate.frame(port, b"BVP1", 63, 15.0)
        self.assertEqual(port.writes, [])
        self.assertEqual(port.in_waiting, 0)  # Fully delivered is not timely confirmation.

    def test_raw_last_read_after_deadline_is_rejected(self):
        clock = [0.0]
        class LateWire(Wire):
            def read(wire, count):
                value = super().read(count)
                if not wire.pending:
                    clock[0] = 15.001
                return value
        port = LateWire(pending=framed(b"BVS1", ORIGINAL))
        with mock.patch.object(gate.time, "monotonic", side_effect=lambda: clock[0]), self.assertRaises(gate.GateError):
            gate.frame(port, b"BVS1", gate.SIZE, 15.0)
        self.assertEqual(port.writes, [])
        self.assertEqual(port.in_waiting, 0)

    def test_wrong_prefix_no_magic_scan(self):
        for raw in (b"text" + framed(b"BVS1", ORIGINAL), framed(b"ABCD", ORIGINAL)):
            with self.subTest(prefix=raw[:4]), self.assertRaises(gate.GateError):
                gate.frame(Wire(pending=raw), b"BVS1", gate.SIZE, time.monotonic() + 1)

    def test_frame_length_crc_trailing_or_incomplete_rejected(self):
        valid = framed(b"BVS1", ORIGINAL)
        damaged = valid[:-1] + bytes([valid[-1] ^ 1])
        for raw in (framed(b"BVS1", ORIGINAL[:-1]), damaged, valid + b"x", valid[:-100]):
            with self.subTest(length=len(raw)), self.assertRaises(gate.GateError):
                gate.frame(Wire(pending=raw), b"BVS1", gate.SIZE, time.monotonic() + 1)

    def test_error_frame_does_not_echo_raw(self):
        with self.assertRaises(gate.GateError) as caught:
            gate.frame(Wire(pending=b"BVE1\x09PRIVATE"), b"BVS1", gate.SIZE, time.monotonic() + 1)
        self.assertNotIn("PRIVATE", str(caught.exception))

    def test_no_flush_or_resend(self):
        for port in (Wire(pending=b"unexpected"), Wire(partial=True)):
            with self.assertRaises(gate.GateError):
                gate.send(port, b"d")
            self.assertLessEqual(len(port.writes), 1)

    def test_maintenance_profile_exact_and_mismatch(self):
        gate.maintenance_profile(Wire([(b"p", metadata())]), UID, SOURCE, 1)
        for raw in (metadata(mode=2), metadata(uid="99" * 16), metadata(source="99" * 32)):
            port = Wire([(b"p", raw)])
            with self.assertRaises(gate.GateError):
                gate.maintenance_profile(port, UID, SOURCE, 1)
            self.assertEqual(port.writes, [b"p"])

    def test_independent_raw_reads(self):
        port = Wire([(b"d", framed(b"BVS1", ORIGINAL)), (b"d", framed(b"BVS1", CHANGED))])
        with self.assertRaises(gate.GateError):
            gate.stable_dump(port)
        self.assertEqual(port.writes, [b"d", b"d"])

    def test_strict_event_schema_types_and_duplicate_keys(self):
        for value in ([1], {**completion("STATUS"), "secret": "no"},
                      {**completion("STATUS"), "code": True}, {**completion("STATUS"), "command": 1},
                      {**completion("STATUS"), "state": "Unknown"},
                      {**snapshot(), "identitySame": 1}, {**snapshot(), "sha256": "xyz"},
                      {**completion("STATUS"), "event": []}):
            with self.subTest(value=value), self.assertRaises(gate.GateError):
                gate.receive_event(Wire(pending=events([value])), time.monotonic() + 1)
        with self.assertRaises(gate.GateError):
            gate.receive_event(Wire(pending=b'{"event":"done","event":"done"}\n'), time.monotonic() + 1)

    def test_oversized_or_nonjson_events(self):
        for raw in (b"x" * 2049 + b"\n", b"secret startup log\n"):
            with self.assertRaises(gate.GateError):
                gate.receive_event(Wire(pending=raw), time.monotonic() + 1)

    def test_exchange_wrong_completion_or_extra_bytes(self):
        for raw in (events([completion("OTHER")]), events([completion("STATUS")]) + b"x"):
            with self.assertRaises(gate.GateError):
                gate.exchange(Wire([(b"STATUS\n", raw)]), "STATUS")

    def test_profile_layout_source_baseline_and_order(self):
        good = profile()
        gate.probe_profile(good, UID, SOURCE, gate.digest(ORIGINAL))
        for key, value in (("capacity", 5), ("sourceId", "99" * 32),
                           ("baselineSha256", "99" * 32), ("boardUid", "99" * 16)):
            altered = copy.deepcopy(good)
            altered[0][key] = value
            with self.assertRaises(gate.GateError):
                gate.probe_profile(altered, UID, SOURCE, gate.digest(ORIGINAL))
        with self.assertRaises(gate.GateError):
            gate.probe_profile([good[0], good[0], good[1]], UID, SOURCE, gate.digest(ORIGINAL))

    def test_backup_state_length_uid_and_sha_binding(self):
        gate.bind_state(self.state, UID, ORIGINAL)
        for uid, raw in (("99" * 16, ORIGINAL), (UID, CHANGED), (UID, ORIGINAL[:-1])):
            with self.assertRaises(gate.GateError):
                gate.bind_state(self.state, uid, raw)

    def test_session_unknown_closed_source_and_restart(self):
        for fields in ({"unknown": True}, {"restoreStarted": True}, {"restored": True},
                       {"probeSourceId": "99" * 32}, {"probeState": "Written"}, {"attemptedMask": 1}):
            with self.subTest(fields=fields), self.assertRaises(gate.GateError):
                gate.authorize({**self.state, **fields}, "OPEN_CAPTURE", completion("PING"), SOURCE, False)

    def test_explicit_region_attestation_and_finite_commands(self):
        state = {**self.state, "probeState": "Captured"}
        with self.assertRaises(gate.GateError):
            gate.authorize(state, "REGION_ATTEST", completion("PING", "Captured"), SOURCE, False)
        gate.authorize(state, "REGION_ATTEST", completion("PING", "Captured"), SOURCE, True)
        for command in ("REBOOT", "REPLACE1", "READ"):
            with self.assertRaises(gate.GateError):
                gate.authorize(self.state, command, completion("PING"), SOURCE, True)

    def test_durable_intent_precedes_command_and_lost_reply(self):
        port = Wire([(b"PING\n", events(profile())), (b"OPEN_CAPTURE\n", b"")])
        with self.assertRaises(gate.GateError):
            gate.probe_command(port, self.path, self.state, "OPEN_CAPTURE", SOURCE, False)
        saved = self.persisted()
        self.assertTrue(saved["unknown"])
        self.assertEqual(saved["intents"][0]["outcome"], "pending")
        self.assertEqual(port.writes, [b"PING\n", b"OPEN_CAPTURE\n"])

    def test_successful_capture_advances_once(self):
        port = Wire([(b"PING\n", events(profile())),
                     (b"OPEN_CAPTURE\n", events([completion("OPEN_CAPTURE", "Captured")]))])
        gate.probe_command(port, self.path, self.state, "OPEN_CAPTURE", SOURCE, False)
        saved = self.persisted()
        self.assertFalse(saved["unknown"])
        self.assertEqual(saved["probeState"], "Captured")
        self.assertEqual(saved["intents"][0]["outcome"], "confirmed")

    def test_replace_requires_snapshot_and_operation(self):
        self.state["probeState"] = "Admitted"
        operation = {"event": "operation", "command": "REPLACE1", "code": 0, "us": 42}
        good = [operation, snapshot(), completion("REPLACE1", "Written", 1)]
        for result in (good[2:], [operation, snapshot(False), good[2]],
                       [{**operation, "code": 1}, good[1], good[2]], [good[1], good[0], good[2]]):
            with self.subTest(result=result):
                state = copy.deepcopy(self.state)
                port = Wire([(b"PING\n", events(profile("Admitted"))), (b"REPLACE1\n", events(result))])
                with self.assertRaises(gate.GateError):
                    gate.probe_command(port, self.path, state, "REPLACE1", SOURCE, False)
                self.assertTrue(self.persisted()["unknown"])
        port = Wire([(b"PING\n", events(profile("Admitted"))), (b"REPLACE1\n", events(good))])
        gate.probe_command(port, self.path, self.state, "REPLACE1", SOURCE, False)
        self.assertEqual(self.persisted()["attemptedMask"], 1)

    def test_actual_fixture_failure_keeps_intent(self):
        port = Wire([(b"PING\n", events(profile())),
                     (b"OPEN_CAPTURE\n", events([completion("OPEN_CAPTURE", "Failed", code=3)]))])
        with self.assertRaises(gate.GateError):
            gate.probe_command(port, self.path, self.state, "OPEN_CAPTURE", SOURCE, False)
        self.assertEqual(self.persisted()["intents"][0]["outcome"], "pending")

    def test_missing_read_is_not_failed_or_zero_value(self):
        self.state["probeState"] = "Admitted"
        port = Wire([(b"PING\n", events(profile("Admitted"))),
                     (b"READ\n", events([completion("READ", "Admitted", code=19)]))])
        returned = gate.probe_command(port, self.path, self.state, "READ", SOURCE, False)
        self.assertEqual(returned[-1]["code"], 19)
        self.assertFalse(self.persisted()["unknown"])

    def test_clear_requires_confirmed_tombstone_operation(self):
        self.state.update(probeState="IdentityEnded", attemptedMask=1)
        port = Wire([(b"PING\n", events(profile("IdentityEnded", 1))),
                     (b"CLEAR_FIXTURE\n", events([completion("CLEAR_FIXTURE", "Cleared", 3)]))])
        with self.assertRaises(gate.GateError):
            gate.probe_command(port, self.path, self.state, "CLEAR_FIXTURE", SOURCE, False)
        self.assertTrue(self.persisted()["unknown"])

    def restore_wire(self, reply=None, current=CHANGED, final=ORIGINAL):
        # Match the real CLI: immutable original raw and current state already
        # exist before a media phase is admitted. No fixture fabricates backup
        # state during the serial operation itself.
        if not (self.path.parent / "original-qspi.raw").exists():
            gate.save_new(self.path.parent / "original-qspi.raw", ORIGINAL)
        if not self.path.exists():
            gate.save_state(self.path, self.state)
        request = b"rBVR1" + struct.pack("<HII", gate.SIZE, zlib.crc32(current), zlib.crc32(ORIGINAL)) + ORIGINAL
        return Wire([(b"p", metadata(2)), (b"d", framed(b"BVS1", current)),
                     (b"d", framed(b"BVS1", current)),
                     (request, b"BVA1" + struct.pack("<I", zlib.crc32(ORIGINAL)) if reply is None else reply),
                     (b"d", framed(b"BVS1", final)), (b"d", framed(b"BVS1", final))])

    def test_restore_exact_original_and_consumed_intent(self):
        port = self.restore_wire()
        gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertTrue(self.persisted()["restored"])
        self.assertTrue(self.persisted()["restoreStarted"])
        self.assertEqual(sum(raw.startswith(b"rBVR1") for raw in port.writes), 1)
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(Wire(), self.path, self.state, ORIGINAL, True, True, SOURCE)

    def test_identity_difference_rejects_before_restore(self):
        current = CHANGED[:gate.REGION] + b"j" * gate.REGION
        port = self.restore_wire(current=current)
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, True, SOURCE)
        self.assertEqual(port.writes, [b"p", b"d", b"d"])

    def test_restore_unknown_or_nonexclusive_no_io(self):
        for exclusive, cold in ((False, True), (True, False)):
            self.state["unknown"] = True
            port = Wire()
            with self.assertRaises(gate.GateError):
                gate.restore_tasks(port, self.path, self.state, ORIGINAL, exclusive, cold, SOURCE)
            self.assertEqual(port.writes, [])

    def test_lost_restore_ack_is_never_replayed(self):
        port = self.restore_wire(reply=b"")
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        saved = self.persisted()
        self.assertTrue(saved["unknown"])
        self.assertTrue(saved["restoreStarted"])
        retry = Wire()
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(retry, self.path, saved, ORIGINAL, True, True, SOURCE)
        self.assertEqual(retry.writes, [])

    def test_reconcile_after_cold_is_read_only_and_keeps_consumed_intent(self):
        self.state.update(unknown=True, restoreStarted=True)
        no_cold = Wire()
        with self.assertRaises(gate.GateError):
            gate.reconcile(no_cold, self.path, self.state, ORIGINAL, False, SOURCE)
        self.assertEqual(no_cold.writes, [])
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", ORIGINAL)),
                     (b"d", framed(b"BVS1", ORIGINAL))])
        self.assertTrue(gate.reconcile(port, self.path, self.state, ORIGINAL, True, SOURCE))
        self.assertEqual(port.writes, [b"p", b"d", b"d"])
        self.assertTrue(self.persisted()["restoreStarted"])
        self.assertFalse(self.persisted()["unknown"])
        self.assertEqual((self.path.parent / "reconcile-000.raw").read_bytes(), ORIGINAL)

    def test_reconcile_different_raw_preserves_unknown(self):
        self.state["unknown"] = True
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", CHANGED)),
                     (b"d", framed(b"BVS1", CHANGED))])
        self.assertFalse(gate.reconcile(port, self.path, self.state, ORIGINAL, True, SOURCE))
        self.assertTrue(self.persisted()["unknown"])
        self.assertTrue(self.persisted()["reconciliations"][0]["identitySame"])

    def test_reconcile_identity_damage_is_recorded_not_repaired(self):
        current = b"j" * gate.SIZE
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", current)),
                     (b"d", framed(b"BVS1", current))])
        with self.assertRaises(gate.GateError):
            gate.reconcile(port, self.path, self.state, ORIGINAL, True, SOURCE)
        self.assertFalse(self.persisted()["reconciliations"][0]["identitySame"])
        self.assertEqual(port.writes, [b"p", b"d", b"d"])

    def test_original_and_pending_state_never_overwritten(self):
        path = self.path.parent / "original.raw"
        gate.save_new(path, ORIGINAL)
        with self.assertRaises(FileExistsError):
            gate.save_new(path, CHANGED)
        self.assertEqual(path.read_bytes(), ORIGINAL)
        gate.save_state(self.path, self.state)
        next_path = self.path.with_name("state.json.next")
        gate.save_new(next_path, b"pending")
        with self.assertRaises(FileExistsError):
            gate.save_state(self.path, {**self.state, "unknown": True})
        self.assertFalse(self.persisted()["unknown"])

    def test_capture_claim_sealed_only_after_exact_private_backup(self):
        folder = self.path.parent
        claim = folder / "claim.json"
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", ORIGINAL)),
                     (b"d", framed(b"BVS1", ORIGINAL))])
        self.assertEqual(gate.capture_session(port, folder, claim, UID, SOURCE, False), ORIGINAL)
        self.assertEqual(json.loads(claim.read_text())["phase"], "sealed")
        self.assertEqual((folder / "original-qspi.raw").read_bytes(), ORIGINAL)
        self.assertEqual(self.persisted()["baselineSha256"], gate.digest(ORIGINAL))
        with self.assertRaises(gate.GateError):
            gate.capture_session(Wire(), folder, claim, UID, SOURCE, True)

    def test_failed_capture_read_may_resume_only_same_empty_session_after_cold(self):
        folder = self.path.parent
        claim = folder / "claim.json"
        failed = Wire([(b"p", metadata()), (b"d", b"")])
        with self.assertRaises(gate.GateError):
            gate.capture_session(failed, folder, claim, UID, SOURCE, False)
        self.assertEqual(json.loads(claim.read_text())["phase"], "capture-pending")
        self.assertFalse((folder / "original-qspi.raw").exists())
        for uid, source, cold in ((UID, SOURCE, False), ("99" * 16, SOURCE, True), (UID, "99" * 32, True)):
            port = Wire()
            with self.assertRaises(gate.GateError):
                gate.capture_session(port, folder, claim, uid, source, cold)
            self.assertEqual(port.writes, [])
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", ORIGINAL)),
                     (b"d", framed(b"BVS1", ORIGINAL))])
        self.assertEqual(gate.capture_session(port, folder, claim, UID, SOURCE, True), ORIGINAL)
        self.assertEqual(port.writes, [b"p", b"d", b"d"])

    def test_partial_backup_or_next_file_does_not_unlock_capture(self):
        folder = self.path.parent
        claim = folder / "claim.json"
        gate.save_new(claim, json.dumps({"privateDir": str(folder), "boardUid": UID,
                                        "sourceId": SOURCE, "phase": "capture-pending"}).encode())
        path = folder / "original-qspi.raw"
        gate.save_new(path, b"partial")
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.capture_session(port, folder, claim, UID, SOURCE, True)
        self.assertEqual(path.read_bytes(), b"partial")
        self.assertEqual(port.writes, [])


class WioColdObservationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.folder = Path(self.temporary.name)
        self.state = {"boardUid": UID, "baselineSha256": gate.digest(ORIGINAL), "unknown": True,
                      "probeSourceId": SOURCE, "probeState": "Admitted", "attemptedMask": 0,
                      "intents": [{"command": "REPLACE1", "sourceId": SOURCE, "stateBefore": "Admitted",
                                   "outcome": "pending"}]}
        gate.save_new(self.folder / "original-qspi.raw", ORIGINAL)
        self.persist()
        self.original_state = (self.folder / "state.json").read_bytes()

    def tearDown(self):
        self.temporary.cleanup()

    def persist(self):
        gate.save_state(self.folder / "state.json", self.state)

    def prepare(self, current=CHANGED):
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", current)),
                     (b"d", framed(b"BVS1", current))])
        request = gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertEqual(port.writes, [b"p", b"d", b"d"])
        return request

    def assert_original_untouched(self):
        self.assertEqual((self.folder / "state.json").read_bytes(), self.original_state)
        self.assertEqual((self.folder / "original-qspi.raw").read_bytes(), ORIGINAL)

    def handshake(self, request):
        return [{"event": "cold-profile", "fixture": "wio-task-cold-v1", "sourceId": SOURCE,
                 "boardUid": UID, **{key: request[key] for key in
                    ("baselineSha256", "identitySha256", "currentSha256", "intentSha256")},
                 "capacity": 4, "payloadBytes": 644, "taskBase": gate.TASK_BASE,
                 "identityBase": gate.IDENTITY_BASE, "regionBytes": gate.REGION},
                {"event": "cold-done", "command": "PING", "code": 0, "state": "Fresh"}]

    def observation(self, missing=False):
        return [{"event": "cold-observation", "bytes": 0 if missing else 644,
                 "sha256": "" if missing else "56" * 32, "records": 0 if missing else 4,
                 "identitySame": True, "writeConfirmed": False},
                {"event": "cold-done", "command": "OBSERVE", "code": 19 if missing else 0,
                 "state": "ObservedNotFound" if missing else "ObservedSnapshot"}]

    def observe_wire(self, request, result=None, handshake=None):
        return Wire([(b"PING\n", events(self.handshake(request) if handshake is None else handshake)),
                     (b"OBSERVE\n", events(self.observation() if result is None else result))])

    def test_prepare_retains_original_unknown_and_binds_all_inputs(self):
        request = self.prepare()
        self.assertEqual(request["intentSha256"], gate.intent_digest(self.state["intents"][0]))
        self.assertEqual(request["stateSha256"], gate.digest(self.original_state))
        self.assertEqual(request["identitySha256"], gate.digest(ORIGINAL[gate.REGION:]))
        self.assertEqual(request["currentSha256"], gate.digest(CHANGED))
        self.assertEqual(gate.cold_request(self.folder, UID), request)
        self.assert_original_untouched()

    def test_unknown_write_needs_no_success_snapshot(self):
        self.assertNotIn("events", self.state["intents"][0])
        self.prepare()
        self.assert_original_untouched()

    def test_prepare_requires_explicit_cold_before_any_io(self):
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, False)
        self.assertEqual(port.writes, [])

    def test_second_mutation_clear_or_restore_intent_rejected(self):
        original = copy.deepcopy(self.state)
        for changes in ({"restoreStarted": True}, {"intents": original["intents"] * 2},
                        {"intents": original["intents"] + [{"command": "CLEAR_FIXTURE"}]}):
            self.state = {**copy.deepcopy(original), **changes}
            self.persist()
            port = Wire()
            with self.assertRaises(gate.GateError):
                gate.prepare_cold(port, self.folder, UID, SOURCE, True)
            self.assertEqual(port.writes, [])

    def test_intent_source_phase_or_missing_rejected(self):
        for intent in ({"command": "READ"}, {**self.state["intents"][0], "sourceId": "99" * 32},
                       {**self.state["intents"][0], "stateBefore": "Fresh"},
                       {**self.state["intents"][0], "outcome": "invented"}):
            state = {**self.state, "intents": [intent]}
            with self.assertRaises(gate.GateError):
                gate.cold_intent(state, ORIGINAL)

    def test_nonblank_original_task_region_rejected(self):
        state = {**self.state, "baselineSha256": gate.digest(CHANGED)}
        with self.assertRaises(gate.GateError):
            gate.cold_intent(state, CHANGED)

    def test_changed_identity_preserves_raw_evidence_not_proof(self):
        current = b"x" * gate.SIZE
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", current)),
                     (b"d", framed(b"BVS1", current))])
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertEqual((self.folder / "cold-current.raw").read_bytes(), current)
        self.assertFalse((self.folder / "cold-request.json").exists())
        self.assert_original_untouched()

    def test_proof_or_raw_or_state_change_rejected(self):
        request = self.prepare()
        state_file = self.folder / "state.json"
        gate.save_state(state_file, {**self.state, "unknown": False})
        with self.assertRaises(gate.GateError):
            gate.cold_request(self.folder, UID)
        gate.save_state(state_file, self.state)
        self.assertEqual(gate.cold_request(self.folder, UID), request)
        gate.save_state(self.folder / "cold-request.json", {**request, "intentSha256": "99" * 32})
        with self.assertRaises(gate.GateError):
            gate.cold_request(self.folder, UID)

    def test_pending_state_file_blocks_prepare(self):
        gate.save_new(self.folder / "state.json.next", b"pending transition")
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(Wire(), self.folder, UID, SOURCE, True)

    def test_state_changes_during_cold_reads_reject_proof(self):
        outer = self
        class ChangedStateWire(Wire):
            def write(self, raw):
                if raw == b"d":
                    gate.save_state(outer.folder / "state.json", {**outer.state, "unknown": False})
                return super().write(raw)
        port = ChangedStateWire([(b"p", metadata()), (b"d", framed(b"BVS1", CHANGED)),
                                 (b"d", framed(b"BVS1", CHANGED))])
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertFalse((self.folder / "cold-request.json").exists())
        self.assertFalse((self.folder / "cold-current.raw").exists())

    def test_cold_capture_unstable_media_never_creates_proof(self):
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", ORIGINAL)),
                     (b"d", framed(b"BVS1", CHANGED))])
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertFalse((self.folder / "cold-request.json").exists())
        self.assert_original_untouched()

    def test_intent_hash_is_canonical_but_value_sensitive(self):
        intent = self.state["intents"][0]
        self.assertEqual(gate.intent_digest(intent), gate.intent_digest(dict(reversed(list(intent.items())))))
        self.assertNotEqual(gate.intent_digest(intent), gate.intent_digest({**intent, "outcome": "confirmed"}))

    def test_original_raw_without_actual_restore_may_be_observed_missing(self):
        # Historical reconcile can close raw recovery without sending r. It
        # must neither prove the write nor prevent a strictly read-only look.
        self.state["restored"] = True
        self.persist()
        self.original_state = (self.folder / "state.json").read_bytes()
        request = self.prepare(ORIGINAL)
        gate.observe_cold(self.observe_wire(request, self.observation(True)), self.folder, UID, SOURCE)
        self.assert_original_untouched()

    def test_snapshot_observation_does_not_confirm_original_write(self):
        request = self.prepare()
        port = self.observe_wire(request)
        result = gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertIs(result["writeConfirmed"], False)
        self.assertEqual(port.writes, [b"PING\n", b"OBSERVE\n"])
        self.assert_original_untouched()
        self.assertEqual(json.loads((self.folder / "cold-attempt.json").read_bytes())["outcome"], "observed")

    def test_notfound_observation_is_not_zero_snapshot_or_write_success(self):
        request = self.prepare(ORIGINAL)
        result = gate.observe_cold(self.observe_wire(request, self.observation(True)), self.folder, UID, SOURCE)
        self.assertEqual((result["bytes"], result["sha256"], result["records"]), (0, "", 0))
        self.assertIs(result["writeConfirmed"], False)
        self.assert_original_untouched()

    def test_profile_mismatch_rejects_before_observe(self):
        request = self.prepare()
        for key, value in (("sourceId", "99" * 32), ("boardUid", "99" * 16),
                           ("currentSha256", "99" * 32), ("intentSha256", "99" * 32),
                           ("baselineSha256", "99" * 32), ("identitySha256", "99" * 32)):
            handshake = self.handshake(request)
            handshake[0][key] = value
            port = self.observe_wire(request, handshake=handshake)
            with self.subTest(key=key), self.assertRaises(gate.GateError):
                gate.observe_cold(port, self.folder, UID, SOURCE)
            self.assertEqual(port.writes, [b"PING\n"])
        self.assertFalse((self.folder / "cold-attempt.json").exists())

    def test_confirmed_phase_from_other_baseline_rejects_before_cold_handshake(self):
        self.prepare()
        path = self.folder / gate.READ_PHASE_FILES["prepare-cold"]
        changed = {**json.loads(path.read_bytes()), "baselineSha256": gate.digest(CHANGED)}
        gate.save_state(path, changed) # Models independent metadata corruption, not a recovery action.
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertEqual(port.writes, [])

    def test_lost_reply_remains_pending_and_never_retries(self):
        request = self.prepare()
        port = self.observe_wire(request, [])
        with self.assertRaises(gate.GateError):
            gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertEqual(json.loads((self.folder / "cold-attempt.json").read_bytes())["outcome"], "pending")
        retry = Wire()
        with self.assertRaises(gate.GateError):
            gate.observe_cold(retry, self.folder, UID, SOURCE)
        self.assertEqual(retry.writes, [])
        self.assert_original_untouched()

    def test_stage_change_during_profile_stops_before_observe(self):
        request = self.prepare()
        outer = self
        class StageChangedWire(Wire):
            def write(self, raw):
                if raw == b"PING\n":
                    gate.save_state(outer.folder / "state.json", {**outer.state, "restoreStarted": True})
                return super().write(raw)
        port = StageChangedWire([(b"PING\n", events(self.handshake(request)))])
        with self.assertRaises(gate.GateError):
            gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertEqual(port.writes, [b"PING\n"])
        self.assertFalse((self.folder / "cold-attempt.json").exists())

    def test_incomplete_transition_during_profile_stops_before_observe(self):
        request = self.prepare()
        outer = self
        class PendingTransitionWire(Wire):
            def write(self, raw):
                if raw == b"PING\n":
                    gate.save_new(outer.folder / "state.json.next", b"incomplete transition")
                return super().write(raw)
        port = PendingTransitionWire([(b"PING\n", events(self.handshake(request)))])
        with self.assertRaises(gate.GateError):
            gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertEqual(port.writes, [b"PING\n"])
        self.assertFalse((self.folder / "cold-attempt.json").exists())
        self.assert_original_untouched()

    def test_observation_cannot_claim_write_success(self):
        request = self.prepare()
        result = self.observation()
        result[0]["writeConfirmed"] = True
        with self.assertRaises(gate.GateError):
            gate.observe_cold(self.observe_wire(request, result), self.folder, UID, SOURCE)
        self.assert_original_untouched()

    def test_old_fixture_completion_not_accepted(self):
        request = self.prepare()
        with self.assertRaises(gate.GateError):
            gate.observe_cold(self.observe_wire(request, [completion("OBSERVE")]), self.folder, UID, SOURCE)
        self.assert_original_untouched()

    def test_corrupt_cold_private_raw_rejected_before_handshake(self):
        self.prepare()
        raw_path = self.folder / "cold-current.raw"
        # Test-only overwrite models independent corruption, not a gate action.
        with raw_path.open("r+b") as handle:
            handle.seek(0)
            handle.write(b"x")
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.observe_cold(port, self.folder, UID, SOURCE)
        self.assertEqual(port.writes, [])
        self.assert_original_untouched()


class WioReadPhaseTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.folder = self.root / "build" / "private"
        self.folder.mkdir(parents=True)
        self.path = self.folder / "state.json"
        self.state = {"boardUid": UID, "baselineSha256": gate.digest(ORIGINAL), "unknown": False,
                      "probeSourceId": SOURCE, "probeState": "IdentityEnded", "attemptedMask": 1,
                      "intents": [{"command": "REPLACE1", "sourceId": SOURCE, "stateBefore": "Admitted",
                                   "outcome": "confirmed"}]}
        gate.save_new(self.folder / "original-qspi.raw", ORIGINAL)
        gate.save_state(self.path, self.state)
        self.original_state = self.path.read_bytes()

    def tearDown(self):
        self.temporary.cleanup()

    def phase_path(self, phase):
        return self.folder / gate.READ_PHASE_FILES[phase]

    def phase(self, phase):
        return json.loads(self.phase_path(phase).read_bytes())

    def assert_original_untouched(self):
        self.assertEqual(self.path.read_bytes(), self.original_state)
        self.assertEqual((self.folder / "original-qspi.raw").read_bytes(), ORIGINAL)

    def prepare_wire(self, second=CHANGED):
        return Wire([(b"p", metadata()), (b"d", framed(b"BVS1", CHANGED)),
                     (b"d", framed(b"BVS1", second))])

    def restore_wire(self, current=CHANGED, final=ORIGINAL):
        packet = b"rBVR1" + struct.pack("<HII", gate.SIZE, zlib.crc32(current), zlib.crc32(ORIGINAL)) + ORIGINAL
        stages = [(b"p", metadata(2)), (b"d", framed(b"BVS1", current)), (b"d", framed(b"BVS1", current))]
        if current != ORIGINAL:
            stages.append((packet, b"BVA1" + struct.pack("<I", zlib.crc32(ORIGINAL))))
        stages.extend([(b"d", framed(b"BVS1", final)), (b"d", framed(b"BVS1", final))])
        return Wire(stages)

    def pending(self, phase="prepare-cold"):
        return gate.begin_read_phase(self.folder, phase, UID, SOURCE, ORIGINAL, self.original_state)

    def test_prepare_phase_is_durable_before_first_request(self):
        outer = self
        class CheckedWire(Wire):
            def write(self, raw):
                value = outer.phase("prepare-cold")
                outer.assertEqual(value["outcome"], "pending")
                outer.assertEqual(value["stateSha256"], gate.digest(outer.original_state))
                outer.assert_original_untouched()
                return super().write(raw)
        port = CheckedWire(self.prepare_wire().exchanges)
        gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertEqual(self.phase("prepare-cold")["outcome"], "confirmed")
        self.assert_original_untouched()

    def test_restore_phase_precedes_preflight_and_remains_until_final_reads(self):
        outer = self
        class CheckedWire(Wire):
            def write(self, raw):
                outer.assertEqual(outer.phase("restore-tasks")["outcome"], "pending")
                return super().write(raw)
        port = CheckedWire(self.restore_wire().exchanges)
        gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertEqual(self.phase("restore-tasks")["outcome"], "confirmed")
        self.assertEqual(json.loads(self.path.read_bytes())["intents"], json.loads(self.original_state)["intents"])
        self.assertTrue(self.state["restoreStarted"])
        self.assertTrue(self.state["restored"])
        self.assertEqual((self.folder / "original-qspi.raw").read_bytes(), ORIGINAL)

    def test_prepare_first_read_failure_is_not_hidden_by_old_unknown_false(self):
        port = Wire([(b"p", metadata()), (b"d", b"")])
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertEqual(self.phase("prepare-cold")["outcome"], "pending")
        self.assert_original_untouched()
        for cold in (False, True):
            retry = Wire()
            with self.assertRaises(gate.GateError):
                gate.prepare_cold(retry, self.folder, UID, SOURCE, cold)
            self.assertEqual(retry.writes, [])

    def test_prepare_independent_read_mismatch_keeps_phase_and_old_intent(self):
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(self.prepare_wire(ORIGINAL), self.folder, UID, SOURCE, True)
        self.assertEqual(self.phase("prepare-cold")["outcome"], "pending")
        self.assertFalse((self.folder / "cold-request.json").exists())
        self.assert_original_untouched()

    def test_restore_preflight_read_failure_preserves_state_and_sends_no_restore(self):
        port = Wire([(b"p", metadata(2)), (b"d", b"")])
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertEqual(port.writes, [b"p", b"d"])
        self.assertEqual(self.phase("restore-tasks")["outcome"], "pending")
        self.assert_original_untouched()
        retry = Wire()
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(retry, self.path, self.state, ORIGINAL, True, True, SOURCE)
        self.assertEqual(retry.writes, [])

    def test_restore_second_preflight_read_failure_is_also_pending(self):
        port = Wire([(b"p", metadata(2)), (b"d", framed(b"BVS1", CHANGED)), (b"d", b"")])
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertEqual(port.writes, [b"p", b"d", b"d"])
        self.assert_original_untouched()
        self.assertEqual(self.phase("restore-tasks")["outcome"], "pending")

    def test_noop_restore_final_read_failure_cannot_close_session(self):
        port = Wire([(b"p", metadata(2)), (b"d", framed(b"BVS1", ORIGINAL)),
                     (b"d", framed(b"BVS1", ORIGINAL)), (b"d", b"")])
        with self.assertRaises(gate.GateError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertFalse(any(raw.startswith(b"rBVR1") for raw in port.writes))
        self.assertEqual(self.phase("restore-tasks")["outcome"], "pending")
        self.assert_original_untouched()

    def test_phase_creation_failure_prevents_prepare_io(self):
        port = Wire()
        with mock.patch.object(gate, "save_new", side_effect=OSError("unavailable")), self.assertRaises(OSError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        self.assertEqual(port.writes, [])
        self.assert_original_untouched()

    def test_phase_creation_failure_prevents_restore_io(self):
        port = Wire()
        with mock.patch.object(gate, "save_new", side_effect=OSError("unavailable")), self.assertRaises(OSError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertEqual(port.writes, [])
        self.assert_original_untouched()

    def test_partial_phase_creation_is_preserved_and_not_overwritten(self):
        save_new = gate.save_new
        def partial(path, raw):
            save_new(path, raw[:5])
            raise OSError("partial private record")
        port = Wire()
        with mock.patch.object(gate, "save_new", side_effect=partial), self.assertRaises(OSError):
            gate.prepare_cold(port, self.folder, UID, SOURCE, True)
        retained = self.phase_path("prepare-cold").read_bytes()
        self.assertEqual(port.writes, [])
        with self.assertRaises(gate.GateError):
            gate.prepare_cold(Wire(), self.folder, UID, SOURCE, True)
        self.assertEqual(self.phase_path("prepare-cold").read_bytes(), retained)
        self.assert_original_untouched()

    def test_prepare_seal_failure_keeps_pending_even_with_complete_proof(self):
        with mock.patch.object(gate, "save_state", side_effect=OSError("cannot seal")), self.assertRaises(OSError):
            gate.prepare_cold(self.prepare_wire(), self.folder, UID, SOURCE, True)
        self.assertTrue((self.folder / "cold-request.json").exists())
        self.assertEqual(self.phase("prepare-cold")["outcome"], "pending")
        self.assert_original_untouched()
        with self.assertRaises(gate.GateError):
            gate.observe_cold(Wire(), self.folder, UID, SOURCE)

    def test_restore_seal_failure_does_not_reissue_consumed_restore(self):
        save_state = gate.save_state
        def fail_seal(path, value):
            if path == self.phase_path("restore-tasks"):
                raise OSError("cannot seal")
            save_state(path, value)
        port = self.restore_wire()
        with mock.patch.object(gate, "save_state", side_effect=fail_seal), self.assertRaises(OSError):
            gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, False, SOURCE)
        self.assertTrue(json.loads(self.path.read_bytes())["restored"])
        self.assertEqual(self.phase("restore-tasks")["outcome"], "pending")
        self.assertEqual(sum(raw.startswith(b"rBVR1") for raw in port.writes), 1)
        with self.assertRaises(gate.GateError):
            gate.check_read_phases(self.folder, UID, gate.digest(ORIGINAL))

    def test_pending_phase_blocks_all_direct_active_entries_before_handshake(self):
        self.pending()
        port = Wire()
        calls = (
            lambda: gate.prepare_cold(port, self.folder, UID, SOURCE, True),
            lambda: gate.restore_tasks(port, self.path, self.state, ORIGINAL, True, True, SOURCE),
            lambda: gate.probe_command(port, self.path, self.state, "READ", SOURCE, True),
            lambda: gate.observe_cold(port, self.folder, UID, SOURCE),
        )
        for call in calls:
            with self.assertRaises(gate.GateError):
                call()
        self.assertEqual(port.writes, [])
        self.assert_original_untouched()

    def test_incomplete_phase_transition_is_not_an_unlock(self):
        path, value = self.pending()
        gate.save_state(path, {**value, "outcome": "confirmed"})
        gate.save_new(path.with_name(path.name + ".next"), b"partial transition")
        with self.assertRaises(gate.GateError):
            gate.check_read_phases(self.folder, UID)

    def test_confirmed_phase_must_bind_the_same_board_and_backup(self):
        path, value = self.pending()
        gate.seal_read_phase(path, value)
        gate.check_read_phases(self.folder, UID, gate.digest(ORIGINAL))
        for uid, baseline in (("99" * 16, gate.digest(ORIGINAL)), (UID, gate.digest(CHANGED))):
            with self.assertRaises(gate.GateError):
                gate.check_read_phases(self.folder, uid, baseline)

    def test_corrupt_or_ambiguous_phase_never_becomes_confirmed(self):
        value = {"phase": "prepare-cold", "boardUid": UID, "baselineSha256": gate.digest(ORIGINAL),
                 "sourceId": SOURCE, "stateSha256": gate.digest(self.original_state), "outcome": "confirmed"}
        variants = (b"{", b"[]", json.dumps({"outcome": "confirmed"}).encode(),
                    json.dumps({**value, "extra": True}).encode(),
                    json.dumps({**value, "sourceId": "invalid"}).encode(),
                    json.dumps(value).replace('"outcome": "confirmed"',
                                             '"outcome": "pending", "outcome": "confirmed"').encode())
        for index, raw in enumerate(variants):
            folder = self.folder / str(index)
            folder.mkdir()
            path = folder / gate.READ_PHASE_FILES["prepare-cold"]
            gate.save_new(path, raw)
            with self.subTest(index=index), self.assertRaises(gate.GateError):
                gate.check_read_phases(folder, UID)
            self.assertEqual(path.read_bytes(), raw)

    def test_failed_phase_cannot_restart_backup_when_other_files_are_lost(self):
        folder = self.folder / "lost-inputs"
        folder.mkdir()
        gate.begin_read_phase(folder, "restore-tasks", UID, SOURCE, ORIGINAL, self.original_state)
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.capture_session(port, folder, folder / "claim.json", UID, SOURCE, True)
        self.assertEqual(port.writes, [])
        self.assertFalse((folder / "original-qspi.raw").exists())

    def test_cli_pending_phase_rejects_before_serial_import_or_open(self):
        self.pending()
        real_import = __import__
        def guarded_import(name, *args, **kwargs):
            if name == "serial" or name.startswith("serial."):
                raise AssertionError("pending CLI must not import/open hardware transport")
            return real_import(name, *args, **kwargs)
        for action in ("prepare-cold", "restore-tasks", "probe", "observe-cold", "reconcile"):
            argv = ["gate", action, "--private-dir", str(self.folder), "--uid", UID,
                    "--port", "COM6", "--source-id", SOURCE]
            if action != "reconcile":
                argv.append("--confirm-whole-board-power-cycle")
            with self.subTest(action=action), mock.patch.object(gate, "ROOT", self.root), \
                    mock.patch.object(sys, "argv", argv), mock.patch("builtins.__import__", side_effect=guarded_import), \
                    self.assertRaises(gate.GateError):
                gate.main()

    def test_cli_wrong_confirmed_baseline_rejects_before_serial_import(self):
        path, value = self.pending()
        gate.save_state(path, {**value, "outcome": "confirmed", "baselineSha256": gate.digest(CHANGED)})
        real_import = __import__
        def guarded_import(name, *args, **kwargs):
            if name == "serial" or name.startswith("serial."):
                raise AssertionError("mismatched private phase must not reach hardware transport")
            return real_import(name, *args, **kwargs)
        argv = ["gate", "restore-tasks", "--private-dir", str(self.folder), "--uid", UID,
                "--port", "COM6", "--source-id", SOURCE, "--confirm-whole-board-power-cycle"]
        with mock.patch.object(gate, "ROOT", self.root), mock.patch.object(sys, "argv", argv), \
                mock.patch("builtins.__import__", side_effect=guarded_import), self.assertRaises(gate.GateError):
            gate.main()

    def test_explicit_cold_reconcile_is_read_only_and_never_seals_phase(self):
        self.pending()
        phase_bytes = self.phase_path("prepare-cold").read_bytes()
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.reconcile(port, self.path, self.state, ORIGINAL, False, SOURCE)
        self.assertEqual(port.writes, [])
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", ORIGINAL)), (b"d", framed(b"BVS1", ORIGINAL))])
        self.assertTrue(gate.reconcile(port, self.path, self.state, ORIGINAL, True, SOURCE))
        self.assertEqual(port.writes, [b"p", b"d", b"d"])
        self.assertEqual(self.phase_path("prepare-cold").read_bytes(), phase_bytes)
        self.assertEqual(self.state["intents"], json.loads(self.original_state)["intents"])
        with self.assertRaises(gate.GateError):
            gate.check_read_phases(self.folder, UID)


if __name__ == "__main__":
    unittest.main()
