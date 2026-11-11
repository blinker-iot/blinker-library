"""Finite offline arithmetic checks; no compiler, SDK, allocation or hardware."""

from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import r4_allocation_workset as workset


class RestrictedAllocationWorkset(unittest.TestCase):
    def test_string_boundaries_include_old_and_new_only_when_heap_backed(self):
        cases = (
            (0, 15, 0, 0, 0), (15, 15, 0, 0, 0),
            (16, 30, 31, 31, 1), (31, 60, 61, 92, 2),
            (32, 60, 61, 92, 2), (1023, 1920, 1921, 2882, 7),
            (1024, 1920, 1921, 2882, 7),
        )
        for maximum, capacity, retained, peak, steps in cases:
            with self.subTest(maximum=maximum):
                self.assertEqual(workset.string_growth(maximum), {
                    "maximumCharacters": maximum,
                    "capacityCharacters": capacity,
                    "retainedRequestedBytes": retained,
                    "growthPeakRequestedBytes": peak,
                    "growthSteps": steps,
                })

    def test_integer_validation_rejects_bool_float_negative_and_overflow(self):
        for invalid in (True, False, 1.0, "32", None, -1,
                        workset.STRING_MAX_SIZE + 1, workset.UINT32_MAX + 1):
            with self.subTest(value=invalid), self.assertRaises(ValueError):
                workset.string_growth(invalid)

    def test_large_valid_bound_is_logarithmic_and_clamps_to_abi_maximum(self):
        result = workset.string_growth(workset.STRING_MAX_SIZE)
        self.assertEqual(result["capacityCharacters"], workset.STRING_MAX_SIZE)
        self.assertEqual(result["retainedRequestedBytes"], 2147483648)
        self.assertEqual(result["growthSteps"], 28)
        self.assertEqual(result["growthPeakRequestedBytes"], 4160749569)

    def test_fixed_profile_and_persistent_lifetimes(self):
        result = workset.restricted_workset()
        self.assertEqual(result["profile"], {
            "frameBytes": 256, "topicBytes": 96, "mqttHeaderBytes": 9,
            "initialMqttBytes": 256, "nativeClients": 2,
            "nativeFifoBytes": 1036, "nativeControlBytes": 16,
            "coreAgtBytes": 36,
            "maxPromptRequestedBytes": 20,
            "ipv4StringRequestedBytes": 0,
            "ipv4TextStackBytes": 16,
        })
        self.assertEqual(result["persistentRequestedBytes"], {
            "nativeClients": 2104, "coreAgt": 36, "mqttAfterResize": 361,
            "cold": 2396, "steady": 2501,
        })

    def test_disjoint_phase_envelopes_do_not_sum_two_simultaneous_growths(self):
        phases = workset.restricted_workset()["phasesRequestedBytes"]
        self.assertEqual(phases, {
            "mqttResize": 2757,
            "parserJointTransient": 2943,
            "networkParserWithPrompt": 2963,
            "storageRpcWithSteady": 5444,
            "networkRpcWithSteady": 5464,
            "initialHttpConnect": 5359,
            "refreshHttpConnect": 5464,
        })
        self.assertNotEqual(phases["parserJointTransient"], 2882 + 92)
        self.assertNotEqual(phases["networkRpcWithSteady"], 5464 + 16)
        self.assertEqual(phases["refreshHttpConnect"], 5464)

    def test_profile_validation_rejects_invalid_types_and_total_overflow(self):
        for field, invalid in (("FRAME_BYTES", True), ("TOPIC_BYTES", -1),
                               ("NATIVE_CLIENTS", 1.5),
                               ("MAX_PROMPT_BYTES", workset.UINT32_MAX + 1),
                               ("FRAME_BYTES", workset.UINT32_MAX),
                               ("NATIVE_CLIENTS", workset.UINT32_MAX),
                               ("IPV4_TEXT_STACK_BYTES", True),
                               ("CORE_AGT_BYTES", -1)):
            with self.subTest(field=field, value=invalid), patch.object(
                    workset, field, invalid), self.assertRaises(ValueError):
                workset.restricted_workset()

    def test_model_never_claims_runtime_allocator_or_formal_admission(self):
        result = workset.restricted_workset()
        for field in ("requestedBytesOnly", "metadataExcluded", "alignmentExcluded",
                      "fragmentationExcluded"):
            self.assertIs(result[field], True)
        self.assertIs(result["oomRecoverable"], False)
        self.assertIs(result["formalTaskAdmission"], False)
        self.assertNotIn("fits", result)
        self.assertNotIn("admitted", result)
        self.assertNotIn("heapCapacity", result)
        constraints = " ".join(result["constraints"])
        self.assertIn("timeout to zero", constraints)
        self.assertIn("Guarded IPv4 connects use 16-byte stack text", constraints)
        self.assertIn("no arbitrary reserve/capacity", constraints)

    def test_aligned_initial_packet_avoids_only_reviewed_native_growth(self):
        result = workset.restricted_workset(361)
        self.assertEqual(result["initialMqttPacketBytes"], 361)
        self.assertEqual(result["profile"]["initialMqttBytes"], 361)
        self.assertIs(result["nativeReallocGrowthAvoided"], True)
        self.assertEqual(result["persistentRequestedBytes"], {
            "nativeClients": 2104, "coreAgt": 36, "mqttAfterResize": 361,
            "cold": 2501, "steady": 2501,
        })
        self.assertEqual(result["phasesRequestedBytes"], {
            "mqttResize": 2501,
            "parserJointTransient": 2943,
            "networkParserWithPrompt": 2963,
            "storageRpcWithSteady": 5444,
            "networkRpcWithSteady": 5464,
            "initialHttpConnect": 5464,
            "refreshHttpConnect": 5464,
        })
        self.assertIs(result["oomRecoverable"], False)
        self.assertIs(result["formalTaskAdmission"], False)

    def test_initial_packet_is_only_one_of_two_reviewed_integer_sizes(self):
        for invalid in (True, False, 256.0, "361", None, -1, 0, 255,
                        257, 360, 362, workset.UINT32_MAX + 1):
            with self.subTest(value=invalid), self.assertRaises(ValueError):
                workset.restricted_workset(invalid)

    def test_default_profile_keeps_native_growth_and_original_peak_explicit(self):
        result = workset.restricted_workset()
        self.assertEqual(result["initialMqttPacketBytes"], 256)
        self.assertIs(result["nativeReallocGrowthAvoided"], False)
        self.assertEqual(result["persistentRequestedBytes"]["cold"], 2396)
        self.assertEqual(result["phasesRequestedBytes"]["mqttResize"], 2757)
        self.assertEqual(max(result["phasesRequestedBytes"].values()), 5464)

    def test_ipv4_stack_text_matches_guarded_client_and_excludes_native_ipv6(self):
        result = workset.restricted_workset(361)
        source = (ROOT / "src/BlinkerV2/arduino/ports/uno_r4/UnoR4ModemClient.h").read_text()
        self.assertEqual(result["schema"], "internal-r4-requested-workset/2")
        self.assertEqual(result["profile"]["ipv4StringRequestedBytes"], 0)
        self.assertEqual(result["profile"]["ipv4TextStackBytes"], 16)
        self.assertIn("char host[16];", source)
        self.assertIn('snprintf(host, sizeof(host), "%u.%u.%u.%u"', source)
        self.assertIn("ip.type() != arduino::IPv4) return Native::connect(ip, port);", source)
        self.assertIn("Native::connect(host, port)", source)
        self.assertNotIn("ip.toString()", source)
        constraints = " ".join(result["constraints"])
        self.assertIn("not a heap request", constraints)
        self.assertIn("native IPv6 fallback is excluded", constraints)

    def test_known_core_timer_is_separate_not_an_allocation_census(self):
        result = workset.restricted_workset(361)
        self.assertEqual(result["scope"], "known-enumerated-allocations")
        self.assertEqual(result["profile"]["coreAgtBytes"], 36)
        self.assertEqual(result["persistentRequestedBytes"]["coreAgt"], 36)
        self.assertEqual(result["persistentRequestedBytes"]["steady"],
                         2104 + 36 + 361)
        self.assertIn("not a complete census", " ".join(result["constraints"]))
        self.assertIs(result["metadataExcluded"], True)
        self.assertIs(result["formalTaskAdmission"], False)


if __name__ == "__main__":
    unittest.main()
