"""Test-only observation hooks for one pinned, already reviewed UART overlay.

No SDK installation, product edits, build, UART access or fault injection occurs
here. The three calls only observe the existing stop order; they neither branch
nor replace the original fault, Close, fail-stop or callback-unbind operations.
"""
import hashlib
from pathlib import Path


ORIGINAL_OVERLAY_SHA256 = "3558a55aa4b8ec59764571a6277939c35b491639ee964f570ac166713605c4d7"


def once(source, before, after):
    if source.count(before) != 1:
        raise ValueError("Pinned UART observation anchor mismatch")
    return source.replace(before, after, 1)


def project(original):
    if hashlib.sha256(original).hexdigest() != ORIGINAL_OVERLAY_SHA256:
        raise ValueError("Unsupported reviewed UART overlay")
    source = original.decode("utf-8").replace("\r\n", "\n")
    source = once(source, "namespace {\n// The reviewed NO_USB", """extern "C" void blinker_r4_uart_gate_stop(unsigned, UART*,
    sci_uart_instance_ctrl_t*, const uart_cfg_t*, bool);

namespace {
// The reviewed NO_USB""")
    source = once(source, "  if (R_SCI_UART_Close(&control) != FSP_SUCCESS)",
                  "  blinker_r4_uart_gate_stop(0U, uart, &control, &config, initialized);\n"
                  "  if (R_SCI_UART_Close(&control) != FSP_SUCCESS)")
    source = once(source, "  initialized = false;", """  blinker_r4_uart_gate_stop(1U, uart, &control, &config, initialized);
  initialized = false;""")
    source = once(source, "    UART::g_uarts[config.channel] = nullptr;", """    UART::g_uarts[config.channel] = nullptr;
  blinker_r4_uart_gate_stop(2U, uart, &control, &config, initialized);""")
    return source.encode("utf-8")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("Refusing to replace an existing fixture")
    result = project(args.original.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print("Test-only UART observation projection prepared; SDK unchanged")
