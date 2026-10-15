"""Offline protocol/recovery regressions; never imports pyserial or opens USB."""
from __future__ import annotations

import copy
import json
from pathlib import Path
import struct
import tempfile
import time
import unittest
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


if __name__ == "__main__":
    unittest.main()
