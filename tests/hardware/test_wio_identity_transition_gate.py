"""Offline Wio transition protocol/intention tests. Never opens USB or ADB."""
from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
import zlib

import wio_identity_transition_gate as gate
from test_wio_storage_gate import Wire, framed

UID, SOURCE, CAPTURE = "12" * 16, "34" * 32, "56" * 32
ORIGINAL_INSTANCE, TEMP_INSTANCE = "78" * 16, "9a" * 16
ORIGINAL = b"\xff" * gate.REGION + b"o" * gate.REGION
PREPARED = b"\xff" * gate.REGION + b"t" * gate.REGION
MCU = b"m" * 524288


def metadata(state=0, uid=UID, source=SOURCE, baseline=None):
    return framed(b"BVI1", struct.pack("<BIIIH", 1, 4194304, gate.media.TASK_BASE,
                                      gate.media.IDENTITY_BASE, gate.REGION) + bytes.fromhex(
        uid + source + (baseline or gate.media.digest(ORIGINAL))) + bytes([state]))


def prepared_reply(instance=TEMP_INSTANCE, raw=PREPARED):
    return framed(b"BVT1", bytes.fromhex(instance + gate.media.digest(raw)))


class TransitionGateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.patch = mock.patch.object(gate, "ROOT", self.root)
        self.patch.start()
        self.build = self.root / "build"
        self.build.mkdir()
        self.inputs = self.build / "original"
        self.inputs.mkdir()
        self.raw_path, self.mcu_path = self.inputs / "original.raw", self.inputs / "original.mcu"
        self.raw_path.write_bytes(ORIGINAL)
        self.mcu_path.write_bytes(MCU)
        self.old = self.inputs / "old-state.json"
        self.old.write_bytes(gate.encoded({"boardUid": UID, "restored": True, "unknown": False,
                                          "intents": [{"outcome": "pending"}]}))
        self.proof = self.inputs / "provenance.json"
        self.proof_value = {"fixture": "wio-identity-original-v1", "boardUid": UID,
                            "captureSourceId": CAPTURE, "baselineSha256": gate.media.digest(ORIGINAL),
                            "originalInstanceId": ORIGINAL_INSTANCE, "originalMcuSha256": gate.media.digest(MCU),
                            "originalMcuBytes": len(MCU), "maintenanceMode": 1, "stableRawReads": 2,
                            "closedProofPath": str(self.old), "closedProofSha256": gate.media.digest(self.old.read_bytes())}
        self.proof.write_bytes(gate.encoded(self.proof_value))
        self.folder = self.build / "session"
        self.binding = gate.init_session(self.folder, UID, SOURCE, self.raw_path, self.mcu_path, self.proof)

    def tearDown(self):
        self.patch.stop()
        self.temp.cleanup()

    def phase(self, action, readonly=False, source=SOURCE):
        return gate.begin_phase(self.folder, self.binding, action, source, readonly)

    def prepare_ok(self):
        phase = self.phase("prepare")
        port = Wire([(b"p", metadata()), (b"t", prepared_reply()), (b"p", metadata(1)),
                     (b"d", framed(b"BVS1", PREPARED)), (b"p", metadata(1))])
        result = gate.prepare(port, self.folder, self.binding, phase)
        return result, port

    def restore_ok(self):
        self.prepare_ok()
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", PREPARED)), (b"p", metadata()),
                     (request, framed(b"BVA1", bytes.fromhex(gate.media.digest(ORIGINAL)))), (b"p", metadata(2))])
        result = gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        return result, port

    def recovery_ready(self):
        phase = self.phase("prepare")
        gate.mutation_intent(self.folder, self.binding, "prepare")
        witness = {"fixture": "wio-identity-power-cycle-v1", "boardUid": UID,
                   "bindingSha256": gate.media.digest((self.folder / "binding.json").read_bytes()),
                   "preparePhaseSha256": gate.media.digest((self.folder / "prepare-phase.json").read_bytes()),
                   "prepareIntentSha256": gate.media.digest((self.folder / "prepare-intent.json").read_bytes()),
                   "observedAtUtc": gate.utc_now(), "observationId": "ab" * 16, "wholeBoardPowerCycle": True}
        witness_path = self.inputs / "new-physical-epoch.json"
        witness_path.write_bytes(gate.encoded(witness))
        capture = self.phase("capture-current", readonly=True)
        no_anchor = b"\xff" * gate.SIZE
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", no_anchor)), (b"p", metadata())])
        gate.capture_current(port, self.folder, self.binding, capture)
        return witness_path, phase

    def successor_ready(self):
        self.restore_ok()
        phase = self.phase("reconcile-readonly", readonly=True, source=CAPTURE)
        maintenance = framed(b"BVP1", struct.pack("<BIIIH", 1, 4194304, gate.media.TASK_BASE,
                                                  gate.media.IDENTITY_BASE, gate.REGION) + bytes.fromhex(UID + CAPTURE))
        port = Wire([(b"p", maintenance), (b"d", framed(b"BVS1", ORIGINAL)), (b"d", framed(b"BVS1", ORIGINAL))])
        gate.reconcile_readonly(port, self.folder, self.binding, ORIGINAL, phase, CAPTURE)
        artifacts = {
            "restored-full-test.bin": MCU,
            "bootloader-readback-test.bin": MCU[:16384],
            "retirement-ledger.json": gate.encoded({"v2_task_scopes": [{"logical_device_id": "device_previous",
                "state": "retired", "device_instance_id": {"type": "Buffer", "data": list(bytes.fromhex(TEMP_INSTANCE))},
                "retirement_verified_at": None}], "v2_task_requests": [{"state": "pending"}],
                "v2_task_attempts": [{"state": "unknown"}]}),
            "delete-proof.json": gate.encoded({"success": True, "receipt": {"httpStatus": 202,
                "observedFromActualNetwork": True, "data": {"logicalDeviceId": "device_previous",
                    "state": "deleted", "cloudAuthorization": "revoked", "localTasks": "not_confirmed"}},
                "result": {"actualTaskHttp": [{"method": "DELETE", "actualNetworkRequest": True,
                    "path": "/api/v2/devices/device_previous", "responseBodyObserved": True}]}}),
            "unknown-request.json": gate.encoded({"state": "unknown", "replayed": False}),
        }
        for name, raw in artifacts.items():
            (self.inputs / name).write_bytes(raw)
        evidence = [{"name": name, "bytes": len(raw), "sha256": gate.media.digest(raw)} for name, raw in artifacts.items()]
        audit = {"at": gate.utc_now(), "safeRestorationClosure": "verified", "passed": False,
                 "limitedProductAcceptance": "not-verified", "originalUnknownOrFailedEvidenceRemoved": False,
                 "stages": [{"id": name, "state": "passed", "evidence": evidence if name == "mcu-restoration" else []}
                            for name in ("normal-device-removal", "service-restoration", "quota-restoration",
                                         "identity-restoration", "mcu-restoration", "inventory-and-app-finish")]}
        audit_path = self.inputs / "closed-audit.json"
        audit_path.write_bytes(gate.encoded(audit))
        claim, claim_bytes = gate.current_claim(UID)
        self.authorization = self.inputs / "successor-authorization.json"
        self.authorization_value = {"fixture": "wio-identity-successor-authorization-v1", "boardUid": UID,
            "previousPrivateDir": str(self.folder), "previousClaimSha256": gate.media.digest(claim_bytes),
            "previousBindingSha256": claim["bindingSha256"], "closureAuditPath": str(audit_path),
            "closureAuditSha256": gate.media.digest(audit_path.read_bytes()), "retiredLogicalDeviceId": "device_previous",
            "retirementLedgerPath": str(self.inputs / "retirement-ledger.json"),
            "retirementLedgerSha256": gate.media.digest(artifacts["retirement-ledger.json"]),
            "normalDeleteProofPath": str(self.inputs / "delete-proof.json"),
            "normalDeleteProofSha256": gate.media.digest(artifacts["delete-proof.json"]),
            "authorizedAtUtc": gate.utc_now(), "authorizationId": "ab" * 16,
            "purpose": "new-limited-task-session-after-exact-restoration"}
        self.authorization.write_bytes(gate.encoded(self.authorization_value))
        self.fresh = self.build / "fresh-original"
        self.fresh.mkdir()
        for path in (self.raw_path, self.mcu_path, self.proof):
            (self.fresh / path.name).write_bytes(path.read_bytes())
        self.next_folder = self.build / "successor"
        return claim

    def successor_init(self):
        return gate.init_session(self.next_folder, UID, SOURCE, self.fresh / self.raw_path.name,
                                 self.fresh / self.mcu_path.name, self.fresh / self.proof.name, self.authorization)

    def test_successor_append_only_preserves_claim_identity_and_cloud_unknown(self):
        self.successor_ready()
        originals = {p: p.read_bytes() for p in self.folder.iterdir()}
        root_claim = gate.registry_path(UID).read_bytes()
        unknown = (self.inputs / "unknown-request.json").read_bytes()
        ledger = (self.inputs / "retirement-ledger.json").read_bytes()
        next_binding = self.successor_init()
        self.assertEqual(gate.registry_path(UID).read_bytes(), root_claim)
        self.assertEqual((self.inputs / "unknown-request.json").read_bytes(), unknown)
        self.assertEqual((self.inputs / "retirement-ledger.json").read_bytes(), ledger)
        self.assertTrue(all(p.read_bytes() == raw for p, raw in originals.items()))
        self.assertEqual(gate.load_session(self.next_folder, UID)[0], next_binding)
        gate.assert_upload_allowed(self.next_folder, UID, "transition")
        for purpose in ("transition", "runtime", "verify-original", "original"):
            with self.subTest(purpose=purpose), self.assertRaises(gate.GateError):
                gate.assert_upload_allowed(self.folder, UID, purpose)
        with self.assertRaises(gate.GateError):
            gate.init_session(self.build / "unauthorized", UID, SOURCE, self.raw_path, self.mcu_path, self.proof)

    def test_successor_rejects_pending_identity_phase_or_next_before_new_claim(self):
        self.successor_ready()
        phase_path = self.folder / "restore-phase.json"
        before = phase_path.read_bytes()
        phase = gate.read_json(phase_path)
        phase_path.write_bytes(gate.encoded({**phase, "outcome": "pending", "result": {}}))
        with self.assertRaises(gate.GateError):
            self.successor_init()
        phase_path.write_bytes(before)
        (self.folder / "restore-phase.json.next").write_bytes(b"partial")
        with self.assertRaises(gate.GateError):
            self.successor_init()
        self.assertFalse(gate.successor_directory(UID).exists())

    def test_successor_rejects_bool_only_audit_missing_readback_or_unretired_scope(self):
        self.successor_ready()
        for name in ("restored-full-test.bin", "bootloader-readback-test.bin", "retirement-ledger.json", "delete-proof.json"):
            path = self.inputs / name
            original = path.read_bytes()
            path.write_bytes(original + b"x")
            with self.subTest(name=name), self.assertRaises(gate.GateError):
                self.successor_init()
            path.write_bytes(original)
        # Matching a forged digest does not replace exact MCU or scope checks.
        ledger_path = self.inputs / "retirement-ledger.json"
        ledger = gate.read_json(ledger_path)
        ledger["v2_task_scopes"][0]["state"] = "active"
        ledger_path.write_bytes(gate.encoded(ledger))
        self.reseal_successor_evidence("retirement-ledger.json", "retirementLedgerSha256")
        with self.assertRaises(gate.GateError):
            self.successor_init()

    def reseal_successor_evidence(self, name, auth_field=None):
        audit_path = Path(self.authorization_value["closureAuditPath"])
        audit = gate.read_json(audit_path)
        raw = (self.inputs / name).read_bytes()
        for item in audit["stages"][4]["evidence"]:
            if item["name"] == name:
                item.update({"bytes": len(raw), "sha256": gate.media.digest(raw)})
        audit_path.write_bytes(gate.encoded(audit))
        if auth_field is not None:
            self.authorization_value[auth_field] = gate.media.digest(raw)
        self.authorization_value["closureAuditSha256"] = gate.media.digest(audit_path.read_bytes())
        self.authorization.write_bytes(gate.encoded(self.authorization_value))

    def test_successor_requires_new_explicit_bound_authorization(self):
        self.successor_ready()
        original = self.authorization.read_bytes()
        for field, value in (("authorizedAtUtc", "2000-01-01T00:00:00Z"),
                             ("authorizedAtUtc", "2999-01-01T00:00:00Z"),
                             ("authorizationId", "0" * 32), ("boardUid", "bc" * 16),
                             ("previousClaimSha256", "cd" * 32),
                             ("purpose", "resume-unknown-task")):
            self.authorization.write_bytes(gate.encoded({**self.authorization_value, field: value}))
            with self.subTest(field=field), self.assertRaises(gate.GateError):
                self.successor_init()
        self.authorization.write_bytes(original)
        gate.main(["init-successor", "--private-dir", str(self.next_folder), "--uid", UID,
                   "--source-id", SOURCE, "--original-raw", str(self.fresh / self.raw_path.name),
                   "--original-mcu", str(self.fresh / self.mcu_path.name),
                   "--provenance", str(self.fresh / self.proof.name),
                   "--predecessor-authorization", str(self.authorization)])

    def test_successor_predecessor_files_cannot_change_after_admission(self):
        self.successor_ready()
        self.successor_init()
        for path in (self.inputs / "unknown-request.json", self.folder / "prepare-intent.json", self.authorization):
            before = path.read_bytes()
            path.write_bytes(before + b" ")
            with self.subTest(path=path.name), self.assertRaises(gate.GateError):
                gate.load_session(self.next_folder, UID)
            path.write_bytes(before)
        (self.folder / "new-observation.json").write_bytes(b"{}")
        with self.assertRaises(gate.GateError):
            gate.load_session(self.next_folder, UID)

    def test_successor_interrupted_binding_or_registry_never_unlocks_previous(self):
        self.successor_ready()
        self.successor_init()
        (self.next_folder / "binding.json").unlink()
        for folder in (self.folder, self.next_folder):
            with self.subTest(folder=folder.name), self.assertRaises((gate.GateError, OSError)):
                gate.assert_upload_allowed(folder, UID, "transition")

    def test_successor_unreachable_registry_or_partial_claim_refuses_hardware(self):
        self.successor_ready()
        self.successor_init()
        directory = gate.successor_directory(UID)
        (directory / ("ab" * 32 + ".json")).write_bytes(b"{}")
        with self.assertRaises(gate.GateError):
            gate.load_session(self.next_folder, UID)
        (directory / ("ab" * 32 + ".json")).unlink()
        (directory / "pending.next").write_bytes(b"partial")
        with self.assertRaises(gate.GateError):
            gate.load_session(self.next_folder, UID)

    def test_successor_does_not_accept_changed_original(self):
        self.successor_ready()
        path = self.fresh / self.raw_path.name
        path.write_bytes(PREPARED)
        with self.assertRaises(gate.GateError):
            self.successor_init()
        self.assertFalse(gate.successor_directory(UID).exists())

    def test_successor_authorization_cannot_be_reused_for_second_successor(self):
        self.successor_ready()
        self.successor_init()
        with self.assertRaises(gate.GateError):
            gate.init_session(self.build / "reused", UID, SOURCE, self.fresh / self.raw_path.name,
                              self.fresh / self.mcu_path.name, self.fresh / self.proof.name, self.authorization)

    def test_successor_matching_audit_hash_cannot_replace_exact_mcu_readback(self):
        self.successor_ready()
        (self.inputs / "restored-full-test.bin").write_bytes(b"x" + MCU[1:])
        self.reseal_successor_evidence("restored-full-test.bin")
        with self.assertRaises(gate.GateError):
            self.successor_init()
        self.assertFalse(gate.successor_directory(UID).exists())

    def test_successor_matching_audit_hash_cannot_replace_normal_delete(self):
        self.successor_ready()
        path = self.inputs / "delete-proof.json"
        deletion = gate.read_json(path)
        deletion["result"]["actualTaskHttp"][0]["actualNetworkRequest"] = False
        path.write_bytes(gate.encoded(deletion))
        self.reseal_successor_evidence("delete-proof.json", "normalDeleteProofSha256")
        with self.assertRaises(gate.GateError):
            self.successor_init()

    def test_init_binds_all_real_material_and_old_closed_proof(self):
        binding, raw = gate.load_session(self.folder, UID)
        self.assertEqual(raw, ORIGINAL)
        self.assertEqual(binding["originalMcuSha256"], gate.media.digest(MCU))
        self.assertTrue(self.old.exists())
        self.assertEqual(json.loads(self.old.read_bytes())["intents"], [{"outcome": "pending"}])

    def test_new_directory_cannot_bypass_uid_claim(self):
        with self.assertRaises(gate.GateError):
            gate.init_session(self.build / "other", UID, SOURCE, self.raw_path, self.mcu_path, self.proof)

    def test_init_rejects_short_mcu_nonblank_task_and_compile_placeholder(self):
        for field, value in (("originalMcuBytes", True), ("maintenanceMode", True), ("stableRawReads", 1),
                             ("originalInstanceId", "0" * 32), ("captureSourceId", "0" * 64)):
            proof = {**self.proof_value, field: value}
            self.proof.write_bytes(gate.encoded(proof))
            with self.subTest(field=field), self.assertRaises(gate.GateError):
                gate.validate_provenance(self.proof, UID, ORIGINAL, MCU)
        self.proof.write_bytes(gate.encoded(self.proof_value))
        for raw, mcu in ((ORIGINAL[:-1], MCU), (b"x" + ORIGINAL[1:], MCU), (ORIGINAL, MCU[:-1])):
            with self.assertRaises(gate.GateError):
                gate.validate_provenance(self.proof, UID, raw, mcu)

    def test_outside_build_and_workspace_root_rejected(self):
        for path in (self.root, self.build, self.root / "private"):
            with self.assertRaises(gate.GateError):
                gate.private_path(path)

    def test_original_changes_refused_before_port_import(self):
        self.raw_path.write_bytes(PREPARED)
        with mock.patch.dict(sys.modules, {"serial": None}), self.assertRaises(gate.GateError):
            gate.main(["prepare", "--private-dir", str(self.folder), "--uid", UID,
                       "--source-id", SOURCE, "--port", "COM6"])
        self.assertFalse((self.folder / "prepare-phase.json").exists())

    def test_mcu_provenance_or_closed_proof_changes_are_rejected(self):
        for path in (self.mcu_path, self.proof, self.old):
            original = path.read_bytes()
            path.write_bytes(original + b"x")
            with self.subTest(path=path.name), self.assertRaises((gate.GateError, OSError)):
                gate.load_session(self.folder, UID)
            path.write_bytes(original)

    def test_duplicate_fields_invalid_schema_and_next_are_blocking(self):
        binding = self.folder / "binding.json"
        original = binding.read_bytes()
        for raw in (b'{"boardUid":"x","boardUid":"y"}', b'[]', b'{"unknown":true}', original + b"tail"):
            binding.write_bytes(raw)
            with self.subTest(raw=raw[:15]), self.assertRaises(gate.GateError):
                gate.load_session(self.folder, UID)
        binding.write_bytes(original)
        (self.folder / "unrelated.next").write_bytes(b"partial")
        with self.assertRaises(gate.GateError):
            gate.load_session(self.folder, UID)

    def test_closed_unknown_is_not_unlocked_by_power_boolean(self):
        old = {"boardUid": UID, "restored": True, "unknown": True, "wholeBoardPowerCycleConfirmed": True}
        self.old.write_bytes(gate.encoded(old))
        proof = {**self.proof_value, "closedProofSha256": gate.media.digest(self.old.read_bytes())}
        self.proof.write_bytes(gate.encoded(proof))
        with self.assertRaises(gate.GateError):
            gate.validate_provenance(self.proof, UID, ORIGINAL, MCU)

    def test_profile_state_uid_source_baseline_layout_are_exact(self):
        for response in (metadata(state=3), metadata(uid="aa" * 16), metadata(source="bb" * 32),
                         metadata(baseline="cc" * 32), framed(b"BVI1", b"x" * 95)):
            port = Wire([(b"p", response)])
            with self.assertRaises(gate.GateError):
                gate.profile(port, self.binding, (0,))
            self.assertEqual(port.writes, [b"p"])

    def test_direct_helper_requires_persisted_phase_before_first_p(self):
        port = Wire()
        with self.assertRaises((gate.GateError, OSError)):
            gate.prepare(port, self.folder, self.binding, {"action": "prepare", "outcome": "pending"})
        self.assertEqual(port.writes, [])

    def test_prepare_success_preserves_intent_and_separate_receipt(self):
        result, port = self.prepare_ok()
        self.assertEqual(result["temporaryInstanceId"], TEMP_INSTANCE)
        self.assertEqual(port.writes, [b"p", b"t", b"p", b"d", b"p"])
        intent = gate.read_json(self.folder / "prepare-intent.json")
        self.assertEqual(intent["outcome"], "pending")
        self.assertEqual(gate.unknown_actions(self.folder), [])
        gate.assert_upload_allowed(self.folder, UID, "runtime")
        self.assertEqual((self.folder / "prepared-current.raw").read_bytes(), PREPARED)

    def test_prepare_persists_phase_before_open_and_intent_before_t(self):
        phase = self.phase("prepare")
        seen = []
        class InspectedWire(Wire):
            def write(wire, data):
                seen.append(gate.read_json(self.folder / "prepare-phase.json")["outcome"])
                if data == b"t":
                    self.assertTrue((self.folder / "prepare-intent.json").exists())
                return super().write(data)
        port = InspectedWire([(b"p", metadata()), (b"t", prepared_reply()), (b"p", metadata(1)),
                              (b"d", framed(b"BVS1", PREPARED)), (b"p", metadata(1))])
        gate.prepare(port, self.folder, self.binding, phase)
        self.assertEqual(seen, ["pending"] * 5)

    def test_t_lost_partial_or_wrong_identity_stays_unknown_no_replay(self):
        phase = self.phase("prepare")
        port = Wire([(b"p", metadata()), (b"t", b"BVT1")])
        with self.assertRaises(gate.GateError):
            gate.prepare(port, self.folder, self.binding, phase)
        self.assertEqual(port.writes, [b"p", b"t"])
        self.assertTrue((self.folder / "prepare-intent.json").exists())
        for purpose in ("transition", "runtime", "verify-original", "original"):
            with self.subTest(purpose=purpose), self.assertRaises(gate.GateError):
                gate.assert_upload_allowed(self.folder, UID, purpose)
        with self.assertRaises(gate.GateError):
            self.phase("prepare")

    def test_prepare_same_identity_refused_before_d(self):
        phase = self.phase("prepare")
        port = Wire([(b"p", metadata()), (b"t", prepared_reply(ORIGINAL_INSTANCE))])
        with self.assertRaises(gate.GateError):
            gate.prepare(port, self.folder, self.binding, phase)
        self.assertEqual(port.writes, [b"p", b"t"])

    def test_prepare_capture_mismatch_retains_raw_and_pending_intent(self):
        phase = self.phase("prepare")
        port = Wire([(b"p", metadata()), (b"t", prepared_reply()), (b"p", metadata(1)),
                     (b"d", framed(b"BVS1", ORIGINAL))])
        with self.assertRaises(gate.GateError):
            gate.prepare(port, self.folder, self.binding, phase)
        self.assertEqual((self.folder / "prepared-current.raw").read_bytes(), ORIGINAL)
        self.assertFalse((self.folder / "prepare-receipt.json").exists())

    def test_explicit_readonly_capture_retains_old_unknown_and_no_mutation(self):
        old_phase = self.phase("prepare")
        gate.mutation_intent(self.folder, self.binding, "prepare")
        old_bytes = (self.folder / "prepare-phase.json").read_bytes()
        intent_bytes = (self.folder / "prepare-intent.json").read_bytes()
        phase = self.phase("capture-current", readonly=True)
        no_anchor = b"\xff" * gate.SIZE
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", no_anchor)), (b"p", metadata())])
        result = gate.capture_current(port, self.folder, self.binding, phase)
        self.assertFalse(result["rawEqualsOriginal"])
        self.assertEqual(old_bytes, gate.encoded(old_phase))
        self.assertEqual((self.folder / "prepare-phase.json").read_bytes(), old_bytes)
        self.assertEqual((self.folder / "prepare-intent.json").read_bytes(), intent_bytes)
        self.assertEqual(gate.unknown_actions(self.folder), ["prepare"])
        self.assertEqual(port.writes, [b"p", b"d", b"p"])
        with self.assertRaises(gate.GateError):
            self.phase("restore")

    def test_restore_requires_consumed_prepare(self):
        with self.assertRaises(gate.GateError):
            self.phase("restore")

    def test_restore_exact_frame_one_write_and_ack_boundary(self):
        result, port = self.restore_ok()
        self.assertFalse(result["independentRawVerified"])
        self.assertFalse(result["originalFirmwareRestored"])
        self.assertEqual(len(port.writes), 5)
        self.assertEqual(port.writes[-2][-gate.SIZE:], ORIGINAL)
        self.assertEqual((self.folder / "restore-before.raw").read_bytes(), PREPARED)
        gate.assert_upload_allowed(self.folder, UID, "verify-original")
        for purpose in ("transition", "runtime", "original"):
            with self.subTest(purpose=purpose), self.assertRaises(gate.GateError):
                gate.assert_upload_allowed(self.folder, UID, purpose)
        with self.assertRaises(FileExistsError):
            self.phase("restore")

    def test_restore_payload_change_refused_before_profile(self):
        self.prepare_ok()
        phase = self.phase("restore")
        port = Wire()
        with self.assertRaises(gate.GateError):
            gate.restore(port, self.folder, self.binding, PREPARED, phase)
        self.assertEqual(port.writes, [])

    def test_restore_lost_ack_never_replays_or_asserts_firmware_restored(self):
        self.prepare_ok()
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", PREPARED)), (b"p", metadata()), (request, b"BVA1")])
        with self.assertRaises(gate.GateError):
            gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        self.assertEqual(len(port.writes), 4)
        self.assertTrue((self.folder / "restore-intent.json").exists())
        self.assertFalse((self.folder / "restore-receipt.json").exists())
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "verify-original")

    def test_independent_mode1_two_reads_required_before_original_upload(self):
        self.restore_ok()
        phase = self.phase("reconcile-readonly", readonly=True, source=CAPTURE)
        maintenance = framed(b"BVP1", struct.pack("<BIIIH", 1, 4194304, gate.media.TASK_BASE,
                                                  gate.media.IDENTITY_BASE, gate.REGION) + bytes.fromhex(UID + CAPTURE))
        port = Wire([(b"p", maintenance), (b"d", framed(b"BVS1", ORIGINAL)), (b"d", framed(b"BVS1", ORIGINAL))])
        result = gate.reconcile_readonly(port, self.folder, self.binding, ORIGINAL, phase, CAPTURE)
        self.assertTrue(result["rawEqualsOriginal"])
        self.assertFalse(result["originalFirmwareRestored"])
        gate.assert_upload_allowed(self.folder, UID, "original")
        self.assertEqual(port.writes, [b"p", b"d", b"d"])

    def test_reconcile_different_reads_stays_pending(self):
        self.restore_ok()
        phase = self.phase("reconcile-readonly", readonly=True, source=CAPTURE)
        with mock.patch.object(gate.media, "maintenance_profile"), mock.patch.object(gate.media, "stable_dump",
                                                                                   side_effect=gate.GateError("different")):
            with self.assertRaises(gate.GateError):
                gate.reconcile_readonly(Wire(), self.folder, self.binding, ORIGINAL, phase, CAPTURE)
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "original")

    def test_tampered_receipt_phase_and_capture_block_upload(self):
        self.prepare_ok()
        receipt_path = self.folder / "prepare-receipt.json"
        before = receipt_path.read_bytes()
        receipt = gate.read_json(receipt_path)
        receipt["result"] = {"wrong": True}
        receipt_path.write_bytes(gate.encoded(receipt))
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "runtime")
        receipt_path.write_bytes(before)
        (self.folder / "prepared-current.raw").write_bytes(ORIGINAL)
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "runtime")

    def test_bad_phase_schema_or_duplicate_prevents_serial_open(self):
        path = self.folder / "prepare-phase.json"
        for raw in (b'{"action":"prepare","action":"prepare"}', b'{"outcome":"confirmed"}'):
            path.write_bytes(raw)
            with mock.patch.dict(sys.modules, {"serial": None}), self.assertRaises(gate.GateError):
                gate.main(["capture-current", "--private-dir", str(self.folder), "--uid", UID,
                           "--source-id", SOURCE, "--port", "COM6"])

    def test_missing_or_changed_source_refused_before_serial_import(self):
        for source in (None, "ab" * 32):
            args = ["prepare", "--private-dir", str(self.folder), "--uid", UID, "--port", "COM6"]
            if source:
                args += ["--source-id", source]
            with mock.patch.dict(sys.modules, {"serial": None}), self.assertRaises(gate.GateError):
                gate.main(args)
        self.assertFalse((self.folder / "prepare-phase.json").exists())

    def test_error_crc_trailing_partial_frames_do_not_scan_or_retry(self):
        for raw in (b"BVE1\x09", metadata() + b"secret", metadata()[:-1], b"junk" + metadata()):
            port = Wire([(b"p", raw)])
            with self.subTest(raw=raw[:4]), self.assertRaises(gate.GateError):
                gate.profile(port, self.binding, (0,))
            self.assertEqual(port.writes, [b"p"])

    def test_status_is_offline_and_never_claims_restored_firmware(self):
        with mock.patch.dict(sys.modules, {"serial": None}), mock.patch("builtins.print") as output:
            self.assertEqual(gate.main(["status", "--private-dir", str(self.folder), "--uid", UID]), 0)
        report = json.loads(output.call_args.args[0])
        self.assertFalse(report["originalFirmwareRestored"])
        self.assertFalse(report["hardwareAdmissionGranted"])

    def test_delivered_but_late_t_reply_cannot_seal_preparation(self):
        phase = self.phase("prepare")
        port = Wire([(b"p", metadata()), (b"t", prepared_reply()), (b"p", metadata(3))])
        with self.assertRaises(gate.GateError):
            gate.prepare(port, self.folder, self.binding, phase)
        self.assertEqual(port.writes, [b"p", b"t", b"p"])
        self.assertFalse((self.folder / "prepare-receipt.json").exists())

    def test_delivered_but_late_r_ack_cannot_seal_restoration(self):
        self.prepare_ok()
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (b"d", framed(b"BVS1", PREPARED)), (b"p", metadata()),
                     (request, framed(b"BVA1", bytes.fromhex(gate.media.digest(ORIGINAL)))), (b"p", metadata(3))])
        with self.assertRaises(gate.GateError):
            gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        self.assertFalse((self.folder / "restore-receipt.json").exists())
        self.assertEqual(gate.unknown_actions(self.folder), ["restore"])

    def test_no_anchor_recovery_first_exact_r_preserves_old_unknown(self):
        witness, old_phase = self.recovery_ready()
        unknown_bytes = (self.folder / "prepare-phase.json").read_bytes()
        gate.authorize_recovery(self.folder, UID, witness)
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (request, framed(b"BVA1", bytes.fromhex(gate.media.digest(ORIGINAL)))),
                     (b"p", metadata(2))])
        result = gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        self.assertFalse(result["originalFirmwareRestored"])
        self.assertEqual(port.writes, [b"p", request, b"p"])
        self.assertEqual((self.folder / "prepare-phase.json").read_bytes(), unknown_bytes)
        self.assertEqual(gate.read_json(self.folder / "prepare-phase.json"), old_phase)
        self.assertFalse((self.folder / "prepare-receipt.json").exists())
        gate.assert_upload_allowed(self.folder, UID, "verify-original")
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "original")
        with self.assertRaises(gate.GateError):
            gate.authorize_recovery(self.folder, UID, witness)

    def test_recovery_final_mode1_can_restore_program_but_never_seals_old_intent(self):
        witness, _ = self.recovery_ready()
        gate.authorize_recovery(self.folder, UID, witness)
        unknown_phase = (self.folder / "prepare-phase.json").read_bytes()
        unknown_intent = (self.folder / "prepare-intent.json").read_bytes()
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (request, framed(b"BVA1", bytes.fromhex(gate.media.digest(ORIGINAL)))),
                     (b"p", metadata(2))])
        gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        final = self.phase("reconcile-readonly", readonly=True, source=CAPTURE)
        maintenance = framed(b"BVP1", struct.pack("<BIIIH", 1, 4194304, gate.media.TASK_BASE,
                                                  gate.media.IDENTITY_BASE, gate.REGION) + bytes.fromhex(UID + CAPTURE))
        port = Wire([(b"p", maintenance), (b"d", framed(b"BVS1", ORIGINAL)), (b"d", framed(b"BVS1", ORIGINAL))])
        gate.reconcile_readonly(port, self.folder, self.binding, ORIGINAL, final, CAPTURE)
        gate.assert_upload_allowed(self.folder, UID, "original")
        self.assertEqual(gate.unknown_actions(self.folder), ["prepare"])
        self.assertEqual((self.folder / "prepare-phase.json").read_bytes(), unknown_phase)
        self.assertEqual((self.folder / "prepare-intent.json").read_bytes(), unknown_intent)
        self.assertFalse((self.folder / "prepare-receipt.json").exists())

    def test_old_boolean_wrong_uid_time_or_unbound_epoch_never_authorizes(self):
        witness_path, _ = self.recovery_ready()
        original = gate.read_json(witness_path)
        for field, value in (("wholeBoardPowerCycle", 1), ("boardUid", "aa" * 16),
                             ("prepareIntentSha256", "bb" * 32), ("bindingSha256", "cc" * 32),
                             ("observedAtUtc", "2020-01-01T00:00:00.000000Z"),
                             ("observedAtUtc", "2999-01-01T00:00:00.000000Z")):
            witness_path.write_bytes(gate.encoded({**original, field: value}))
            with self.subTest(field=field, value=value), self.assertRaises(gate.GateError):
                gate.authorize_recovery(self.folder, UID, witness_path)
        self.assertFalse((self.folder / "recovery-authority.json").exists())

    def test_recovery_proof_change_cannot_reconstruct_ram_or_unlock_r(self):
        witness, _ = self.recovery_ready()
        gate.authorize_recovery(self.folder, UID, witness)
        witness.write_bytes(witness.read_bytes() + b" ")
        with self.assertRaises(gate.GateError):
            self.phase("restore")
        with self.assertRaises(gate.GateError):
            gate.assert_upload_allowed(self.folder, UID, "transition")

    def test_recovery_r_unknown_can_never_be_reauthorized_or_replayed(self):
        witness, _ = self.recovery_ready()
        gate.authorize_recovery(self.folder, UID, witness)
        phase = self.phase("restore")
        request = b"rBVR1" + struct.pack("<HI", gate.SIZE, zlib.crc32(ORIGINAL)) + ORIGINAL
        port = Wire([(b"p", metadata()), (request, b"BVA1")])
        with self.assertRaises(gate.GateError):
            gate.restore(port, self.folder, self.binding, ORIGINAL, phase)
        for action in (lambda: self.phase("restore"), lambda: gate.authorize_recovery(self.folder, UID, witness),
                       lambda: gate.assert_upload_allowed(self.folder, UID, "verify-original")):
            with self.assertRaises(gate.GateError):
                action()

    def test_recovery_requires_capture_after_new_observation_not_just_old_power(self):
        phase = self.phase("prepare")
        gate.mutation_intent(self.folder, self.binding, "prepare")
        path = self.inputs / "not-a-witness.json"
        path.write_bytes(gate.encoded({"wholeBoardPowerCycleConfirmed": True}))
        with self.assertRaises((gate.GateError, OSError)):
            gate.authorize_recovery(self.folder, UID, path)


if __name__ == "__main__":
    unittest.main()
