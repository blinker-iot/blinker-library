"""Finite offline projection contract, not a hardware/IRQ-behaviour test."""
import hashlib
import importlib.util
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
spec = importlib.util.spec_from_file_location("fixture_prepare", HERE / "prepare_fixture.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
ORIGINAL = REPO / "build/r4-rpc-deadline-20261016/overlay-final/Serial.cpp"


class ProjectionContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = ORIGINAL.read_bytes()
        cls.projected = module.project(cls.original).decode()

    def test_pinned_input(self):
        self.assertEqual(hashlib.sha256(self.original).hexdigest(), module.ORIGINAL_OVERLAY_SHA256)

    def test_tampered_input_rejected(self):
        with self.assertRaises(ValueError):
            module.project(self.original + b"\n")

    def test_second_projection_rejected(self):
        with self.assertRaises(ValueError):
            module.project(self.projected.encode())

    def test_exact_three_hooks(self):
        self.assertEqual(self.projected.count("  blinker_r4_uart_gate_stop("), 3)
        for stage in range(3):
            self.assertEqual(self.projected.count(f"blinker_r4_uart_gate_stop({stage}U,"), 1)

    def test_original_flow_is_exactly_preserved(self):
        removed = self.projected.replace('extern "C" void blinker_r4_uart_gate_stop(unsigned, UART*,\n'
            '    sci_uart_instance_ctrl_t*, const uart_cfg_t*, bool);\n\n', "")
        for stage in range(3):
            removed = removed.replace(f"  blinker_r4_uart_gate_stop({stage}U, uart, &control, &config, initialized);\n", "")
        self.assertEqual(removed, self.original.decode().replace("\r\n", "\n"))

    def test_close_before_unbind_order(self):
        start = self.projected.index("void blinker_r4_uart_stop(")
        ordered = ["blinker_r4_rpc_fault();", "blinker_r4_uart_require_no_transfer(config);",
            "blinker_r4_uart_gate_stop(0U", "R_SCI_UART_Close(&control)", "blinker_r4_rpc_fail_stop();",
            "blinker_r4_uart_gate_stop(1U", "initialized = false;", "UART::g_uarts[config.channel] = nullptr;",
            "blinker_r4_uart_gate_stop(2U"]
        for value in ordered:
            start = self.projected.index(value, start) + len(value)

    def test_ambiguous_anchor_rejected(self):
        with self.assertRaises(ValueError):
            module.once("abcabc", "abc", "def")

    def test_missing_anchor_rejected(self):
        with self.assertRaises(ValueError):
            module.once("abc", "xyz", "def")


if __name__ == "__main__":
    unittest.main(verbosity=2)
