"""Pinned Core 1.5.3 modem-UART bounded-wait overlay, not a UART replacement.

Imported by the internal Modem overlay generator. No installed SDK is changed;
the original Serial.h ABI is retained and every other UART keeps SDK behavior.
"""
import hashlib
from pathlib import Path


SOURCE_SHA256 = {
    "Serial.cpp": "682b7c9cbe70b9b8e99d71cd1861ccd2222dafaf2d2a6f85b12b2b08820eddd3",
    "Serial.h": "cf13c2cbd253d977152dd0da61a9d3d2a284f7be6ad61e31ee0ee17b61552904",
}


def _replace(source: str, before: str, after: str) -> str:
    if source.count(before) != 1:
        raise ValueError("original Serial source anchor mismatch")
    return source.replace(before, after, 1)


def patch_serial(original: bytes) -> bytes:
    if hashlib.sha256(original).hexdigest() != SOURCE_SHA256["Serial.cpp"]:
        raise ValueError("unsupported original Serial.cpp SHA256")
    source = original.decode("utf-8").replace("\r\n", "\n")
    source = _replace(source, '#include "IRQManager.h"', '''#include "IRQManager.h"
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>

namespace {
// The reviewed NO_USB UNO R4 modem is Serial2/_UART3_, SCI1, no DMA/DTC.
// Faulting never means a remote operation was cancelled or never executed.
void blinker_r4_uart_require_no_transfer(const uart_cfg_t& config) {
  if (config.p_transfer_tx != nullptr || config.p_transfer_rx != nullptr) {
    blinker_r4_rpc_fault();
    blinker_r4_rpc_fail_stop();
  }
}
void blinker_r4_uart_stop(UART* uart, sci_uart_instance_ctrl_t& control,
                          const uart_cfg_t& config, bool& initialized) {
  blinker_r4_rpc_fault();
  blinker_r4_uart_require_no_transfer(config);
  // Abort polls FIFO reset without a deadline. The pinned no-transfer Close
  // disables SCI/its four IRQs before the caller's source buffer may expire.
  if (R_SCI_UART_Close(&control) != FSP_SUCCESS)
    blinker_r4_rpc_fail_stop();
  initialized = false;
  if (config.channel < MAX_UARTS && UART::g_uarts[config.channel] == uart)
    UART::g_uarts[config.channel] = nullptr;
}
}
''')
    for signature, failure in (
        ("size_t UART::write(uint8_t c) {", "return 0;"),
        ("size_t  UART::write(uint8_t* c, size_t len) {", "return 0;"),
        ("size_t UART::write_raw(uint8_t* c, size_t len) {", "return 0;"),
        ("void UART::flush() {", "return;"),
        ("void UART::begin(unsigned long baudrate, uint16_t config) {", "return;"),
    ):
        source = _replace(source, signature, signature + '''
  if (this == &_UART3_ && blinker_r4_rpc_faulted()) ''' + failure)
    for call, count in (("&c, 1", "1"), ("c, len", "len")):
        before = ("    R_SCI_UART_Write(&uart_ctrl, " + call + ");\n"
                  "    while (!tx_empty) {}\n    return " + count + ";")
        after = '''    if (this == &_UART3_ && blinker_r4_rpc_active()) {
      blinker_r4_uart_require_no_transfer(uart_cfg);
      if (blinker_r4_rpc_faulted() || blinker_r4_rpc_expired()) {
        blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
        return 0;
      }
      if (R_SCI_UART_Write(&uart_ctrl, ''' + call + ''') != FSP_SUCCESS) {
        blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
        return 0;
      }
      while (!tx_empty) {
        if (blinker_r4_rpc_faulted() || blinker_r4_rpc_expired()) {
          blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
          return 0;
        }
      }
      if (blinker_r4_rpc_faulted() || blinker_r4_rpc_expired()) {
        blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
        return 0;
      }
    } else {
      R_SCI_UART_Write(&uart_ctrl, ''' + call + ''');
      while (!tx_empty) {}
    }
    return ''' + count + ";"
        source = _replace(source, before, after)
    source = _replace(source, "  while(!tx_complete);", '''  if (this == &_UART3_ && blinker_r4_rpc_active())
    blinker_r4_uart_require_no_transfer(uart_cfg);
  while(!tx_complete) {
    if (this == &_UART3_ && (blinker_r4_rpc_faulted() ||
        (blinker_r4_rpc_active() && blinker_r4_rpc_expired()))) {
      blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
      return;
    }
  }
  if (this == &_UART3_ && (blinker_r4_rpc_faulted() ||
      (blinker_r4_rpc_active() && blinker_r4_rpc_expired())))
    blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);''')
    source = _replace(source, "  size_t i = 0;\n  while (i < len)", '''  if (this == &_UART3_ && blinker_r4_rpc_active())
    blinker_r4_uart_require_no_transfer(uart_cfg);
  size_t i = 0;
  while (i < len)''')
    source = _replace(source, "    uart_ctrl.p_reg->TDR = *(c+i);\n    while (uart_ctrl.p_reg->SSR_b.TEND == 0) {}", '''    if (this == &_UART3_ && (blinker_r4_rpc_faulted() ||
        (blinker_r4_rpc_active() && blinker_r4_rpc_expired()))) {
      blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
      return i;
    }
    uart_ctrl.p_reg->TDR = *(c+i);
    while (uart_ctrl.p_reg->SSR_b.TEND == 0) {
      if (this == &_UART3_ && (blinker_r4_rpc_faulted() ||
          (blinker_r4_rpc_active() && blinker_r4_rpc_expired()))) {
        blinker_r4_uart_stop(this, uart_ctrl, uart_cfg, init_ok);
        return i;
      }
    }''')
    return source.encode("utf-8")


def verify_serial_overlay(core_dir: Path, output: Path) -> None:
    """Check original and generated bytes, never trust an editable receipt."""
    core_dir, output = core_dir.resolve(), output.resolve()
    if core_dir.parts[-2:] != ("cores", "arduino"):
        raise ValueError("core source must name the original SDK cores/arduino directory")
    if output.is_relative_to(core_dir.parents[1]):
        raise ValueError("overlay output must be outside the original SDK tree")
    original = {}
    for name, digest in SOURCE_SHA256.items():
        original[name] = (core_dir / name).read_bytes()
        if hashlib.sha256(original[name]).hexdigest() != digest:
            raise ValueError("unsupported original " + name + " SHA256")
    expected = {"Serial.cpp": patch_serial(original["Serial.cpp"]), "Serial.h": original["Serial.h"]}
    for name, content in expected.items():
        if (output / name).read_bytes() != content:
            raise ValueError("prepared UART overlay content mismatch: " + name)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--verify-only", action="store_true", required=True)
    args = parser.parse_args()
    verify_serial_overlay(args.core_dir, args.output)
    print("Pinned original UART overlay bytes verified; no SDK edits")
