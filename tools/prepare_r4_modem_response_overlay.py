#!/usr/bin/env python3
"""Prepare one hash-bound Core 1.5.3 Modem/UART overlay; never edit the SDK.

This is an internal no-DMA, cooperative experiment, not ordinary ZIP policy or
task admission. Retain the original FSM/UART ABI, add response guards, shared
owner/RPC budget and safe TX stop. All bounds are explicit build flags.
"""

import argparse
import hashlib
import json
from pathlib import Path
import r4_modem_uart_overlay as uart_overlay


SOURCE_SHA256 = {
    "Modem.cpp": "2c2aa923ea176cf723932e55f5afedf9af25de4b7ba738595f068bc0b2caad26",
    "Modem.h": "e76dda2966be1d767caabcbbd2a60931c1e1aa3f9c6783cdab4a71cd1f32a548",
    "WiFiCommands.h": "9cee648d5c7cb1cf218d52024076caf12f5c432b0539e379920007de2815f265",
    "StringHelpers.h": "bb674991bafddedb613f252384282aab330154efafbd3cd08e2599afc5cfed73",
}
COPIED_HEADERS = ("Modem.h", "WiFiCommands.h", "StringHelpers.h")
HELPER_INCLUDE = "BlinkerV2/arduino/ports/uno_r4/UnoR4ModemResponseBounds.h"
BOUNDARY_INCLUDE = "BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h"


def sha256(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def replace_once(source: str, before: str, after: str) -> str:
    if source.count(before) != 1:
        raise ValueError("original Modem source anchor mismatch")
    return source.replace(before, after, 1)


def patch_modem(original: bytes) -> bytes:
    if sha256(original) != SOURCE_SHA256["Modem.cpp"]:
        raise ValueError("unsupported original Modem.cpp SHA256")
    source = original.decode("utf-8").replace("\r\n", "\n")
    preamble = '''#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemResponseBounds.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>
#if !defined(BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES) || !defined(BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES)
#error "Internal Modem overlay requires explicit command and response bounds"
#endif
static_assert(BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES > 0 &&
              BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES <= UINT_MAX,
              "Invalid internal Modem command bound");
static_assert(BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES > 0 &&
              BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES <= UINT_MAX,
              "Invalid internal Modem response bound");
#if !defined(BLINKER_INTERNAL_R4_MODEM_RPC_MS)
#error "Internal Modem overlay requires an explicit whole-RPC budget"
#endif
static_assert(BLINKER_INTERNAL_R4_MODEM_RPC_MS > 0 &&
              BLINKER_INTERNAL_R4_MODEM_RPC_MS <= INT32_MAX,
              "Invalid internal whole-RPC budget");
namespace {
blinker::RenesasUnoModemRpcBoundary& blinker_r4_boundary() {
   static blinker::RenesasUnoModemRpcBoundary boundary(
      blinker::renesasUnoModemOwner(), BLINKER_INTERNAL_R4_MODEM_RPC_MS);
   return boundary;
}
bool blinker_r4_rpc_start(size_t timeout) {
   return timeout <= INT32_MAX && blinker_r4_boundary().begin(static_cast<uint32_t>(timeout));
}
}
extern "C" bool blinker_r4_rpc_active() { return blinker_r4_boundary().active(); }
extern "C" bool blinker_r4_rpc_expired() { return blinker_r4_boundary().expired(); }
extern "C" bool blinker_r4_rpc_faulted() { return blinker_r4_boundary().faulted(); }
extern "C" void blinker_r4_rpc_fault() { blinker_r4_boundary().fault(); }
extern "C" [[noreturn]] void blinker_r4_rpc_fail_stop() {
   blinker_r4_rpc_fault();
   // A failed hardware close cannot authorize return of an in-flight buffer.
   // No allocation, logging, reopen or retry from this irrecoverable boundary.
   for (;;) {}
}
'''
    source = replace_once(source, '#include "Modem.h"\n', '#include "Modem.h"\n\n' + preamble)
    for signature, failure in (
        ("void ModemClass::begin(int badurate, int retry){", "return;"),
        ("bool ModemClass::passthrough(const uint8_t *data, size_t size) {", "return false;"),
        ("void ModemClass::write_nowait(const string &cmd, string &str, const char * fmt, ...) {", "return;"),
        ("bool ModemClass::write(const string &prompt, string &data_res, const char * fmt, ...){", "return false;"),
        ("ModemClass::ParseResult ModemClass::buf_read(const string &prompt, string &data_res) {", "return ParseError;"),
    ):
        source = replace_once(source, signature, signature + "\n   if(blinker_r4_rpc_faulted()) " + failure)
    source = replace_once(source, '   if(_serial != nullptr && !beginned) {', '''   if(_serial != nullptr && !beginned) {
      if(_serial_debug != nullptr) { blinker_r4_rpc_fault(); return; }
      if(!blinker_r4_rpc_start(_timeout)) return;''')
    source = replace_once(source, '      _serial->flush();', '''      _serial->flush();
      if(!blinker_r4_boundary().finish()) return;''')
    source = replace_once(source, '   data_res.clear();\n   va_list va;', '''   if(_serial_debug != nullptr) { blinker_r4_rpc_fault(); return false; }
   if(!blinker_r4_rpc_start(_timeout)) return false;
   data_res.clear();
   va_list va;''')
    source = replace_once(source, '   (void)cmd;', '''   if(_serial_debug != nullptr) { blinker_r4_rpc_fault(); return; }
   if(!blinker_r4_rpc_start(_timeout)) return;
   (void)cmd;''')
    source = replace_once(source, '   _serial->write(data,size);', '''   if(!blinker_r4_rpc_active()) return false; // No standalone payload or fresh budget.
   if(blinker_r4_rpc_expired()) { blinker_r4_rpc_fault(); return false; }
   if(_serial->write(data,size) != size || blinker_r4_rpc_expired() || blinker_r4_rpc_faulted()) {
      blinker_r4_rpc_fault(); return false;
   }''')
    # The two variadic functions format into the original fixed member buffer.
    anchor = '   vsnprintf((char *)tx_buff, MAX_BUFF_SIZE, fmt, va);'
    if source.count(anchor) != 2:
        raise ValueError('original Modem format count mismatch')
    source = source.replace(anchor, '   const int formatted = vsnprintf((char *)tx_buff, MAX_BUFF_SIZE, fmt, va);')
    # Distinct anchors preserve void/bool signatures without another parser.
    source = replace_once(source, '   va_end (va);\n\n   if(_serial_debug &&', '''   va_end (va);
   if(formatted < 0 || formatted >= MAX_BUFF_SIZE) { blinker_r4_rpc_fault(); return; }

   if(_serial_debug &&''')
    source = replace_once(source, '   va_end (va);\n\n   if(_serial_debug) {', '''   va_end (va);
   if(formatted < 0 || formatted >= MAX_BUFF_SIZE) { blinker_r4_rpc_fault(); return false; }

   if(_serial_debug) {''')
    source = replace_once(source, '   _serial->write(tx_buff,strlen((char *)tx_buff));\n   return;', '''   if(_serial->write(tx_buff, static_cast<size_t>(formatted)) != static_cast<size_t>(formatted) ||
      blinker_r4_rpc_expired() || blinker_r4_rpc_faulted()) blinker_r4_rpc_fault();
   return;''')
    source = replace_once(source, '   _serial->write(tx_buff,strlen((char *)tx_buff));\n   auto res', '''   if(_serial->write(tx_buff, static_cast<size_t>(formatted)) != static_cast<size_t>(formatted) ||
      blinker_r4_rpc_expired() || blinker_r4_rpc_faulted()) {
      blinker_r4_rpc_fault(); return false;
   }
   auto res''')
    if source.count('   return res == Ok;') != 2:
        raise ValueError('original Modem result count mismatch')
    source = source.replace('   return res == Ok;', '''   if(!blinker_r4_boundary().finish()) return false;
   return res == Ok;''')
    source = replace_once(source, '   unsigned long start_time = millis();', '   // One deadline began before TX; never reset it on RX or payload.')
    source = replace_once(source, '      if(millis() - start_time > _timeout) {\n         res = Timeout;', '''      if(blinker_r4_rpc_expired() || blinker_r4_rpc_faulted()) {
         blinker_r4_rpc_fault();
         res = Timeout;''')
    for variable, macro in (("commandName", "COMMAND"), ("data_res", "RESPONSE")):
        anchor = variable + " += c;"
        if source.count(anchor) != 3:
            raise ValueError("original Modem append count mismatch")
        guard = (
            "if(!blinker::unoR4ModemAppendAllowed(" + variable + ".size(), "
            "BLINKER_INTERNAL_R4_MODEM_" + macro + "_BYTES)) {\n"
            "               blinker_r4_rpc_fault();\n"
            "               goto blinker_r4_parser_cleanup;\n"
            "            }\n            " + anchor
        )
        source = source.replace(anchor, guard)
    source = replace_once(source, "sized_read_size = atoi(data_res.c_str());", '''if(!blinker::unoR4ModemSizedLength(data_res.data(), data_res.size(),
                  BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES, sized_read_size)) {
               blinker_r4_rpc_fault();
               goto blinker_r4_parser_cleanup;
            }''')
    source = replace_once(source, "   trim_results = true;", '''blinker_r4_parser_cleanup:
   trim_results = true;
   if(blinker_r4_rpc_faulted()) return ParseError;''')
    return source.encode("utf-8")


def helper_path() -> Path:
    return Path(__file__).resolve().parents[1] / "src" / HELPER_INCLUDE


def expected_overlay(modem_dir: Path, output: Path) -> tuple[dict, dict]:
    modem_dir, output = modem_dir.resolve(), output.resolve()
    if modem_dir.parts[-3:] != ("libraries", "WiFiS3", "src"):
        raise ValueError("source must be the original SDK libraries/WiFiS3/src directory")
    if output.is_relative_to(modem_dir.parents[2]):
        raise ValueError("overlay output must be outside the original SDK tree")
    files = {name: (modem_dir / name).read_bytes() for name in ("Modem.cpp", *COPIED_HEADERS)}
    for name, expected in SOURCE_SHA256.items():
        if sha256(files[name]) != expected:
            raise ValueError("unsupported original " + name + " SHA256")
    core_dir = modem_dir.parents[2] / 'cores' / 'arduino'
    for name, expected in uart_overlay.SOURCE_SHA256.items():
        files[name] = (core_dir / name).read_bytes()
        if sha256(files[name]) != expected:
            raise ValueError('unsupported original ' + name + ' SHA256')
    generated = dict(files, **{"Modem.cpp": patch_modem(files["Modem.cpp"]),
                              "Serial.cpp": uart_overlay.patch_serial(files['Serial.cpp'])})
    root = Path(__file__).resolve().parents[1]
    receipt = {
        "schema": "internal-r4-modem-response-overlay/2",
        "core": "1.5.3",
        "helperSha256": sha256(helper_path().read_bytes()),
        "generatorSha256": sha256(Path(__file__).read_bytes()),
        "uartGeneratorSha256": sha256(Path(uart_overlay.__file__).read_bytes()),
        "boundarySha256": sha256((root / 'src' / BOUNDARY_INCLUDE).read_bytes()),
        "ownerSha256": sha256((root / 'src' / 'BlinkerV2/arduino/ports/uno_r4/UnoR4ModemOwner.h').read_bytes()),
        "sourceSha256": {name: sha256(value) for name, value in files.items()},
        "overlaySha256": {name: sha256(value) for name, value in generated.items()},
        "originalHeaderUnchanged": generated["Modem.h"] == files["Modem.h"],
        "classLayoutChanged": False,
        "explicitBoundsRequired": True,
        "stickyFaultWithoutRuntimeUnlock": True,
        "ownerOrProductIntegrated": True,
        "ordinaryProductEnabled": False,
        "txDeadlineImplemented": True,
        "hardwareStopValidated": False,
        "oomRecoverable": False,
        "ordinaryZipDependency": False,
    }
    return generated, receipt


def prepare(modem_dir: Path, output: Path) -> dict:
    generated, receipt = expected_overlay(modem_dir, output)
    output.mkdir(parents=True, exist_ok=False)
    for name, value in generated.items():
        with (output / name).open("xb") as stream:
            stream.write(value)
    with (output / "overlay.json").open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt


def verify(modem_dir: Path, output: Path) -> dict:
    # Recompute from pinned original bytes, not from editable receipt hashes.
    generated, expected = expected_overlay(modem_dir, output)
    for name, value in generated.items():
        if (output / name).read_bytes() != value:
            raise ValueError("prepared overlay content mismatch: " + name)
    receipt = json.loads((output / "overlay.json").read_text(encoding="utf-8"))
    if json.dumps(receipt, sort_keys=True) != json.dumps(expected, sort_keys=True):
        raise ValueError("prepared overlay receipt/helper/generator mismatch")
    return expected


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modem-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--verify-only", action="store_true")
    arguments = parser.parse_args()
    operation = verify if arguments.verify_only else prepare
    result = operation(arguments.modem_dir, arguments.output)
    print(json.dumps({key: result[key] for key in (
        "schema", "core", "originalHeaderUnchanged", "ownerOrProductIntegrated",
        "txDeadlineImplemented", "oomRecoverable")}))


if __name__ == "__main__":
    main()
