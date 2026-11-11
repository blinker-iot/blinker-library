#!/usr/bin/env python3
"""Offline, pinned Wio STA/Plain RPC candidate. Never install, build or admit."""
import argparse
import hashlib
import json
from pathlib import Path

import prepare_wio_rpc_overlay as pending
import wio_rpc_deadline_client as client
import wio_rpc_deadline_transport as transport

ROOT = Path(__file__).resolve().parents[1]
BOUNDARY = ROOT / "src/BlinkerV2/arduino/ports/wio_terminal/WioTerminalRpcBoundary.h"
PORT = ROOT / "tools/templates/wio_rpc_deadline_port.inc"
RPC_SHA256 = {
    **pending.DEPENDENCY_SHA256,
    "src/erpc/erpc_client_manager.h": "3f7cd988e2254a060d148ed26fc66f0e86d3e463178ceb4c4c4f21f452d69d96",
    "src/erpc/erpc_client_manager.cpp": "4bdf4103670568f731e1d9639e7c6e16b4f0b7eabfa57d05c8333fc0c95426a4",
    "src/erpc/erpc_config.h": "53ea732242998c0430ebf23f5fb584ce351a9cf3995f2ba79ce2d5092bcf82c2",
    "src/erpc/erpc_config_internal.h": "296ee23f533537d191751726db849584871d1d0afe82d5c3e114594344c4db93",
    "src/erpc/erpc_common.h": "b440ee17c13ba0e909d25fe253bb0b8d1e317ac655547999fd6d19ea4a91e7e7",
    "src/erpc/erpc_threading.h": "05efea1998f61c2302446692e69d843e3d35205bcae65bbc68380e8fcf647661",
    "src/erpc/erpc_threading_freertos.cpp": "9a569c17493293f681fe12de28a2d014e40d64739139b109df48d6d31d5945ff",
    "src/erpc/rpc_wifi_api_client.cpp": "c42c953415dc65c3dcb0caae9ce4695cce14f141bf1ad9cbbc9b548c76a598c4",
    "src/erpc/rpc_system_client.cpp": "55e3457532ca74838b42efc2b3274b5413927ddf58ad84c29dc64e91990aea26",
    "src/lwip/sockets.c": "deb4357ed31b84d43ef60f7b3133d408acd82fd3219dee0aea3ce37313f345df",
    "src/lwip/netdb.c": "ae842e6788ca2c5a1969a08564326b70394f20ef1321c42e4c7af639536c6295",
    "src/rtl_wifi/rpc_wifi_api_hal.cpp": "8d16e0b78c5796ef7e6e60fa292666177ffec83c0e4bff37c33a387daba8cace",
    "src/erpc/erpc_arduino_uart_transport.h": "d8582f41d062b9b173422c8ff0689804f9a9b2383145964df262627499d01bc2",
    "src/erpc/erpc_arduino_uart_transport.cpp": "ff85d54526e2aeec645c9eedb5e5316723e637203c7293059be79d11e166a6bf",
    "src/erpc/erpc_framed_transport.cpp": "7cd88e0182ec2eed38be1c70a9c05cd5998db221d14ca98d9b46c0141f12a653",
    "src/erpc_Unified_init.cpp": "b3bf6103c46aca6c2369482320ce89f59932375a8056143a24cabcd85cf29e82",
}
WIFI_SHA256 = {
    "library.properties": "326a8cbe9df6d86ac90f124419aea0695a3d2df9dd4c1b9eb67aed500473aacb",
    "src/WiFiSTA.cpp": "e16796176ce7c477516cfa8bfcf6e81cb96703415bf5e312bdba1187c618077a",
    "src/WiFiGeneric.cpp": "f23daefb0c0b864e3eefc734ebb59d7632671bceb502eb1b32d39d225403c5ab",
    "src/WiFiClient.cpp": "84900b64a7a6c346c19f0a58c66b0a8aee51769baf9a605840be8c5a2a737db3",
}


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise ValueError("RPC deadline anchor must occur exactly once: " + before[:70])
    return text.replace(before, after, 1)


def replace_function(text, signature, following, replacement):
    if text.count(signature) != 1 or text.count(following) != 1:
        raise ValueError("RPC deadline function boundary must occur once: " + signature)
    start, end = text.index(signature), text.index(following)
    if end <= start:
        raise ValueError("RPC deadline function ordering changed")
    return text[:start] + replacement.rstrip() + "\n\n" + text[end:]


def pending_deadline(inputs):
    header = inputs["erpc/erpc_transport_arbitrator.h"]
    header = replace_once(header, '#include "erpc_transport.h"', '#include "erpc_transport.h"\n#include "blinker_wio_rpc_boundary.h"')
    header = replace_once(header, "    Mutex m_clientListMutex;             //!< Mutex guarding the client active and free lists.\n", "")
    cpp = inputs["erpc/erpc_transport_arbitrator.cpp"]
    cpp = replace_once(cpp, ", m_clientListMutex()\n", "")
    cpp = replace_once(cpp, "    while (true)\n    {\n        // Receive a message.",
                       "    while (true)\n    {\n        if (!blinker::wio_rpc::boundary.ready()) return kErpcStatus_Fail;\n        // Receive a message.")
    cpp = replace_once(cpp, """        if (err)
        {
            return err;
        }

        m_codec->setBuffer(*message);""", """        if (err)
        {
            blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Transport);
            return err;
        }

        m_codec->setBuffer(*message);""")
    cpp = replace_once(cpp, """        if (err)
        {
            continue;
        }

        // If this message is an invocation,""", """        if (err)
        {
            blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
            return err;
        }

        // If this message is an invocation,""")
    cpp = replace_once(cpp, """            Mutex::Guard lock(m_clientListMutex);
            for (PendingClientInfo""", """            blinker::wio_rpc::Guard lock;
            if (!blinker::wio_rpc::boundary.ready()) return kErpcStatus_Fail;
            for (PendingClientInfo""")
    cpp = replace_once(cpp, """        if (matchedClient) continue;
#if ERPC_NESTED_CALLS
        // Preserve the original nested-server handling of an unmatched reply.
        return kErpcStatus_Success;
#endif""", """        // This fixed STA candidate rejects nested client calls. Unknown/late
        // sequences must never be forwarded to the server or a recycled request.
        (void)matchedClient;
        continue;""")
    cpp = replace_function(cpp, "erpc_status_t TransportArbitrator::clientReceive(client_token_t token)",
                           "void TransportArbitrator::cancelClientReceive", r"""
erpc_status_t TransportArbitrator::clientReceive(client_token_t token)
{
    assert(token != 0 && "invalid client token");
    PendingClientInfo *info = reinterpret_cast<PendingClientInfo *>(token);
    while (true)
    {
        {
            blinker::wio_rpc::Guard guard;
            if (!blinker::wio_rpc::boundary.ready())
            {
                removePendingClient(info);
                return kErpcStatus_Timeout;
            }
            if (!info->m_isValid)
            {
                const bool received = info->m_sem.get(0);
                if (!received) blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Transport);
                removePendingClient(info);
                return received ? kErpcStatus_Success : kErpcStatus_Fail;
            }
        }
        blinker_wio_rpc_pause();
    }
}
""")
    cpp = replace_function(cpp, "TransportArbitrator::PendingClientInfo *TransportArbitrator::addPendingClient(RequestContext &request)",
                           "void TransportArbitrator::removePendingClient", r"""
TransportArbitrator::PendingClientInfo *TransportArbitrator::addPendingClient(RequestContext &request)
{
    PendingClientInfo *info = NULL;
    {
        blinker::wio_rpc::Guard guard;
        if (!blinker::wio_rpc::boundary.activeFor(request.getSequence())) return NULL;
        if (m_clientFreeList)
        {
            info = m_clientFreeList;
            m_clientFreeList = info->m_next;
        }
    }
    // Allocation/RTOS semaphore construction must not run with IRQs masked.
    if (!info) info = new PendingClientInfo();
    if (!info)
    {
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
        return NULL;
    }
    blinker::wio_rpc::Guard guard;
    const int signals = info->m_sem.getCount();
    if (!blinker::wio_rpc::boundary.activeFor(request.getSequence()) ||
        signals < 0 || signals > 1 || (signals == 1 && !info->m_sem.get(0)))
    {
        info->m_request = NULL;
        info->m_isValid = false;
        info->m_next = m_clientFreeList;
        m_clientFreeList = info;
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Transport);
        return NULL;
    }
    info->m_request = &request;
    info->m_isValid = true;
    info->m_next = m_clientList;
    m_clientList = info;
    return info;
}
""")
    cpp = replace_once(cpp, "    Mutex::Guard lock(m_clientListMutex);", "    blinker::wio_rpc::Guard lock;")
    return {"erpc/erpc_transport_arbitrator.h": header, "erpc/erpc_transport_arbitrator.cpp": cpp}


def digest(content):
    return hashlib.sha256(content).hexdigest()


def read_pinned(root, pins):
    data = {}
    for name, expected in pins.items():
        file = root / name
        content = file.read_bytes()
        if file.is_symlink() or digest(content) != expected:
            raise ValueError("Unreviewed Wio RPC dependency: " + name)
        data[name] = content
    return data


def expected_overlay(rpc_dir, wifi_dir, output, sdk_root):
    rpc_dir, wifi_dir, output, sdk_root = [Path(p).resolve() for p in (rpc_dir, wifi_dir, output, sdk_root)]
    if not all(p.is_dir() for p in (rpc_dir, wifi_dir, sdk_root)):
        raise ValueError("Original RPC/WiFi dependencies and SDK fence must exist")
    if any(output.is_relative_to(p) for p in (rpc_dir, wifi_dir, sdk_root, ROOT / "src")):
        raise ValueError("Candidate output must be outside original SDK/dependencies/src")
    originals = read_pinned(rpc_dir, RPC_SHA256)
    wifi = read_pinned(wifi_dir, WIFI_SHA256)
    base, _receipt = pending.expected_overlay(rpc_dir, output, sdk_root)
    inputs = {name.removeprefix("src/"): data.decode("utf-8").replace("\r\n", "\n")
              for name, data in originals.items() if name.startswith("src/")}
    inputs.update({name: value.decode("utf-8") for name, value in base.items()})
    inputs.update(pending_deadline(inputs))
    inputs.update(client.transform(inputs))
    inputs.update(transport.transform(inputs, PORT.read_text(encoding="utf-8")))
    files = {name: content.encode("utf-8") for name, content in inputs.items()
             if "src/" + name not in originals or
             content != originals["src/" + name].decode("utf-8").replace("\r\n", "\n")}
    boundary = BOUNDARY.read_bytes()
    if boundary.count(b"kBudgetMillis = 1000U;") != 1:
        raise ValueError("Unreviewed internal RPC observation budget")
    logging_guard = "\n#if defined(ERPC_MESSAGE_LOGGING) && ERPC_MESSAGE_LOGGING != 0\n#error \"Wio deadline candidate does not admit additional RPC log transports\"\n#endif\n"
    files["erpc/blinker_wio_rpc_boundary.h"] = (pending.GRAPH_GUARD + logging_guard).encode() + boundary
    operators = [Path(__file__), Path(pending.__file__), Path(client.__file__), Path(transport.__file__), BOUNDARY, PORT]
    receipt = {
        "schema": "internal-wio-rpc-deadline/1", "state": "internal-offline-only",
        "scope": "fixed-sta-plain-single-generated-rpc", "budgetMillis": 1000,
        "dependencyDirectory": str(rpc_dir), "wifiDirectory": str(wifi_dir), "sdkPathFence": str(sdk_root),
        "dependencySha256": {name: digest(data) for name, data in originals.items()},
        "wifiSha256": {name: digest(data) for name, data in wifi.items()},
        "operatorSha256": {p.relative_to(ROOT).as_posix(): digest(p.read_bytes()) for p in operators},
        "outputSha256": {name: digest(data) for name, data in files.items()},
        "requiredCore": "1.8.5", "requiredRpcUnified": "2.1.4",
        "sdkVerified": False, "buildLinked": False, "hardwareWritten": False,
        "wholeRpcDeadlineComplete": False, "physicalIsolationVerified": False,
        "transportQuarantineComplete": False, "formalTaskAdmission": False, "ordinarySdkDependency": False,
        "requiredBuildSteps": [
            "Copy the complete pinned rpcUnified source tree into a private projection, then overlay exactly these files.",
            "Rebuild all changed units and every actual consumer of changed headers, preserving the Small256/MQTT361/newlib/profile graph.",
            "Use fixed STA/Plain, dedicated SERCOM0 RX84/TX85, runClient-only RPC; no AP/scan/BLE/native user event callbacks or SPI1.",
            "Keep original SDK and old evidence unchanged; a preparation receipt is not compile/runtime/physical admission proof.",
        ],
        "unsupported": [
            "Ordinary SDK installation, public task admission or live-runtime reset/reconnect/replay after fault.",
            "Multi-RPC TCP/HTTP outer-operation deadline or an IRQ/allocator/scheduler worst-case latency guarantee.",
            "Generic SDK APIs, callbacks, AP/scan/rawTCP/BLE, extra RPC log transports or shared SPI1; full OOM/physical stop/resource admission.",
        ],
    }
    return files, receipt


def prepare(rpc_dir, wifi_dir, output, sdk_root):
    output = Path(output).resolve()
    files, receipt = expected_overlay(rpc_dir, wifi_dir, output, sdk_root)
    output.mkdir(parents=True, exist_ok=False)
    for name, content in {**files, "overlay.json": (json.dumps(receipt, sort_keys=True, indent=2) + "\n").encode()}.items():
        file = output / name
        file.parent.mkdir(parents=True, exist_ok=True)
        with file.open("xb") as stream:
            stream.write(content)
    return receipt


def verify(rpc_dir, wifi_dir, output, sdk_root):
    output = Path(output).resolve()
    files, receipt = expected_overlay(rpc_dir, wifi_dir, output, sdk_root)
    actual = {p.relative_to(output).as_posix() for p in output.rglob("*") if p.is_file()}
    if actual != set(files) | {"overlay.json"} or any(p.is_symlink() for p in output.rglob("*")):
        raise ValueError("Unexpected/missing/non-regular Wio deadline output")
    if any((output / name).read_bytes() != data for name, data in files.items()):
        raise ValueError("Wio deadline generated content mismatch")
    if json.loads((output / "overlay.json").read_text(encoding="utf-8")) != receipt:
        raise ValueError("Wio deadline receipt/operator/dependency mismatch")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rpc-dir", "wifi-dir", "sdk-root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    receipt = (verify if args.verify_only else prepare)(args.rpc_dir, args.wifi_dir, args.output, args.sdk_root)
    print(json.dumps({key: receipt[key] for key in ("schema", "scope", "budgetMillis", "sdkVerified", "buildLinked",
        "wholeRpcDeadlineComplete", "physicalIsolationVerified", "formalTaskAdmission")}))


if __name__ == "__main__":
    main()
