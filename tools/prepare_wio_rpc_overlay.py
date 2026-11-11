#!/usr/bin/env python3
"""Prepare a pinned, offline Wio pending-lifetime prerequisite; never install/build.

The native UART, mutex waits, semaphore timeout and nested-call behavior remain
unchanged. This is not a whole-RPC deadline, transport quarantine or admission.
"""
import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEPENDENCY_SHA256 = {
    "library.properties": "86b5f27443642f71d896d6fb9063a33e89b8f237bf1ce2404e23a2172cbd4eca",
    "src/erpc/erpc_transport_arbitrator.h": "e9396f9ef11472a7a31fc28c1b584e0555723d290f06209375d37f9947c56135",
    "src/erpc/erpc_transport_arbitrator.cpp": "f575736e69fca28fe20a7d439bf66e6b990f2467456b9bca4d14a20827790f6f",
    "src/erpc/erpc_arbitrated_client_manager.cpp": "85960f058e3d2933c1f039d8d2e67d6f4f3bc6edc4df5a3260f036161f2fbacd",
}

GRAPH_GUARD = r"""
// Internal offline task4 projection only; this does not enable that graph.
#if (!defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)) || \
    !defined(BLINKER_ENABLE_TASKS) || BLINKER_ENABLE_TASKS != 1 || \
    !defined(BLINKER_INTERNAL_WIO_TASKS) || BLINKER_INTERNAL_WIO_TASKS != 1 || \
    !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1 || \
    !defined(BLINKER_TASK_CAPACITY) || BLINKER_TASK_CAPACITY != 4
#error "Wio RPC pending overlay requires the explicit internal four-slot graph"
#endif
"""

# Each old fragment must occur once in the hash-pinned original source. Tests
# can exercise the transformation contract without vendoring the SDK itself.
REPLACEMENTS = {
    "src/erpc/erpc_transport_arbitrator.h": [
        ("#define __embedded_rpc__transport_arbitrator__\n",
         "#define __embedded_rpc__transport_arbitrator__\n" + GRAPH_GUARD),
        ("    erpc_status_t clientReceive(client_token_t token);",
         "    erpc_status_t clientReceive(client_token_t token);\n\n"
         "    // The preparing caller may consume its token once on a pre-receive failure.\n"
         "    // Not a timeout/cross-thread cancellation or reusable recovery handle.\n"
         "    void cancelClientReceive(client_token_t token);"),
        ("    PendingClientInfo *addPendingClient(void);",
         "    PendingClientInfo *addPendingClient(RequestContext &request);"),
    ],
    "src/erpc/erpc_transport_arbitrator.cpp": [
        ("""        PendingClientInfo *client = m_clientList;
        for (; client; client = client->m_next)
        {
            if (client->m_isValid && sequence == client->m_request->getSequence())
            {
                // Swap the received message buffer with the client's message buffer.
                client->m_request->getCodec()->getBuffer()->swap(message);

                // Wake up the client receive thread.
                client->m_sem.put();
                break;
            }
        }
""", """        bool matchedClient = false;
        {
            // Do not hold the pending-state lock across UART receive or codec decode.
            Mutex::Guard lock(m_clientListMutex);
            for (PendingClientInfo *client = m_clientList; client; client = client->m_next)
            {
                if (client->m_request && sequence == client->m_request->getSequence())
                {
                    matchedClient = true;
                    // Keep the request borrowed until its caller detaches it, but
                    // deliver once. A duplicate must not swap buffers or add a signal.
                    if (client->m_isValid)
                    {
                        client->m_isValid = false;
                        client->m_request->getCodec()->getBuffer()->swap(message);
                        client->m_sem.put();
                    }
                    break;
                }
            }
        }
"""),
        ("""#if ERPC_NESTED_CALLS
        // If received answer is not for postponed client, it can be for nested server call.
        if (client == NULL)
        {
            return kErpcStatus_Success;
        }
#endif
""", """        if (matchedClient) continue;
#if ERPC_NESTED_CALLS
        // Preserve the original nested-server handling of an unmatched reply.
        return kErpcStatus_Success;
#endif
"""),
        ("""    PendingClientInfo *info = addPendingClient();
    if (NULL != info)
    {
        info->m_request = &request;
        info->m_isValid = true;
    }
""", "    PendingClientInfo *info = addPendingClient(request);\n"),
        ("""    info->m_sem.get(Semaphore::kWaitForever);

    removePendingClient(info);

    return kErpcStatus_Success;
}

TransportArbitrator::PendingClientInfo *TransportArbitrator::addPendingClient(void)
""", """    const bool received = info->m_sem.get(Semaphore::kWaitForever);

    removePendingClient(info);

    return received ? kErpcStatus_Success : kErpcStatus_Timeout;
}

void TransportArbitrator::cancelClientReceive(client_token_t token)
{
    assert(token != 0 && "invalid client token");
    removePendingClient(reinterpret_cast<PendingClientInfo *>(token));
}

TransportArbitrator::PendingClientInfo *TransportArbitrator::addPendingClient(RequestContext &request)
"""),
        ("""    // Add to active list.
    if (!m_clientList)
""", """    // A delivery can leave at most one unconsumed signal on a cancelled node.
    // Refuse unexpected counts instead of an unbounded drain under this mutex.
    if (!info) return NULL;
    const int staleSignals = info->m_sem.getCount();
    if (staleSignals < 0 || staleSignals > 1 || (staleSignals == 1 && !info->m_sem.get(0)))
    {
        info->m_next = m_clientFreeList;
        m_clientFreeList = info;
        return NULL;
    }
    info->m_next = NULL;
    info->m_request = &request;
    info->m_isValid = true;

    // Publish only after binding the request and resetting recycled state.
    if (!m_clientList)
"""),
        ("""    // Clear fields.
    info->m_request = NULL;
    info->m_isValid = false;

    // Remove from active list.
""", "    // Detach while receive cannot borrow the request.\n"),
        ("""        PendingClientInfo *node = m_clientList;
        while (node)
        {
            if (node->m_next == info)
            {
                node->m_next = info->m_next;
                break;
            }
            node = node->m_next;
        }
""", """        PendingClientInfo *node = m_clientList;
        while (node && node->m_next != info)
            node = node->m_next;
        if (!node) return;
        node->m_next = info->m_next;
"""),
        ("""    // Add to free list.
    info->m_next = m_clientFreeList;
""", """    info->m_request = NULL;
    info->m_isValid = false;

    // Add to free list only after all pending-state borrowers have left.
    info->m_next = m_clientFreeList;
"""),
    ],
    "src/erpc/erpc_arbitrated_client_manager.cpp": [
        ("""    err = logMessage(request.getCodec()->getBuffer());
    if (err)
    {
        request.getCodec()->updateStatus(err);
        return;
    }
""", """    err = logMessage(request.getCodec()->getBuffer());
    if (err)
    {
        if (token) m_arbitrator->cancelClientReceive(token);
        request.getCodec()->updateStatus(err);
        return;
    }
"""),
        ("""    err = m_arbitrator->send(request.getCodec()->getBuffer());
    if (err)
    {
        request.getCodec()->updateStatus(err);
        return;
    }
""", """    err = m_arbitrator->send(request.getCodec()->getBuffer());
    if (err)
    {
        if (token) m_arbitrator->cancelClientReceive(token);
        request.getCodec()->updateStatus(err);
        return;
    }
"""),
    ],
}


def sha256(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def json_bytes(value: dict) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode("utf-8")


def expected_overlay(rpc_dir: Path, output: Path, sdk_root: Path) -> tuple[dict, dict]:
    rpc_dir, output, sdk_root = rpc_dir.resolve(), output.resolve(), sdk_root.resolve()
    if not rpc_dir.is_dir() or not sdk_root.is_dir():
        raise ValueError("original rpcUnified and SDK path fence must exist")
    if any(output.is_relative_to(root) for root in (sdk_root, ROOT / "src", rpc_dir)):
        raise ValueError("overlay output must be outside SDK, library src and rpcUnified")
    original = {}
    for name, digest in DEPENDENCY_SHA256.items():
        file = rpc_dir / name
        if file.is_symlink():
            raise ValueError("rpcUnified inputs must be regular original files")
        original[name] = file.read_bytes()
        if sha256(original[name]) != digest:
            raise ValueError("unreviewed rpcUnified 2.1.4 input: " + name)
    if "version=2.1.4" not in original["library.properties"].decode("utf-8").splitlines():
        raise ValueError("rpcUnified 2.1.4 metadata required")
    files = {}
    for name, replacements in REPLACEMENTS.items():
        text = original[name].decode("utf-8").replace("\r\n", "\n")
        for before, after in replacements:
            if text.count(before) != 1:
                raise ValueError("pinned RPC patch anchor must occur exactly once: " + name)
            text = text.replace(before, after, 1)
        files[name.removeprefix("src/")] = text.encode("utf-8")
    receipt = {
        "schema": "internal-wio-rpc-pending/1", "state": "internal-offline-only",
        "scope": "pending-lifecycle-only", "requiredRpcUnified": "2.1.4", "requiredCore": "1.8.5",
        "sdkPathFence": str(sdk_root), "sdkVerified": False, "buildLinked": False,
        "wholeRpcDeadlineComplete": False, "transportQuarantineComplete": False,
        "formalTaskAdmission": False, "hardwareWritten": False, "ordinarySdkDependency": False,
        "originalWaitForeverUnchanged": True, "crossThreadCancellationSupported": False,
        "dependencyDirectory": str(rpc_dir),
        "dependencySha256": {name: sha256(content) for name, content in original.items()},
        "operatorSha256": sha256(Path(__file__).read_bytes()),
        "outputSha256": {name: sha256(content) for name, content in files.items()},
        "requiredBuildSteps": [
            "Copy the complete pinned rpcUnified source tree into an isolated private projection and overlay these three files; never edit the installed SDK.",
            "Rebuild every actual header consumer, including erpc_Unified_init.cpp; quoted includes must resolve the projected header, not the original one.",
            "Preserve the reviewed explicit Wio task4, Small256/MQTT361, allocator hooks, profile and complete link graph; this tool enables none of them.",
            "The preparing caller consumes its token once by clientReceive or a pre-receive failure cleanup; the handle must not be reused or shared for asynchronous cancellation.",
        ],
        "unsupported": [
            "Whole-RPC deadline, UART stop, lock acquisition deadline, global recovery, sequence-wrap/session fencing or complete allocation-failure support.",
            "Ordinary SDK installation/release, task admission, nested-call policy changes or replay of ambiguous operations.",
        ],
    }
    return files, receipt


def prepare(rpc_dir: Path, output: Path, sdk_root: Path) -> dict:
    files, receipt = expected_overlay(rpc_dir, output, sdk_root)
    output.mkdir(parents=True, exist_ok=False)
    for name, content in {**files, "overlay.json": json_bytes(receipt)}.items():
        file = output / name
        file.parent.mkdir(parents=True, exist_ok=True)
        with file.open("xb") as stream:
            stream.write(content)
    return receipt


def verify(rpc_dir: Path, output: Path, sdk_root: Path) -> dict:
    files, receipt = expected_overlay(rpc_dir, output, sdk_root)
    actual = {file.relative_to(output).as_posix() for file in output.rglob("*") if file.is_file()}
    if actual != set(files) | {"overlay.json"}:
        raise ValueError("RPC overlay has missing or unlisted files")
    if any(file.is_symlink() for file in output.rglob("*")):
        raise ValueError("RPC overlay outputs must be regular files")
    for name, content in files.items():
        if (output / name).read_bytes() != content:
            raise ValueError("RPC overlay output mismatch: " + name)
    if json.loads((output / "overlay.json").read_text(encoding="utf-8")) != receipt:
        raise ValueError("RPC overlay receipt/source mismatch")
    return receipt


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rpc-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--sdk-root", type=Path, required=True, help="existing SDK path used only as an output fence")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    receipt = (verify if args.verify_only else prepare)(args.rpc_dir, args.output, args.sdk_root)
    print(json.dumps({name: receipt[name] for name in ("schema", "scope", "sdkVerified", "buildLinked",
        "wholeRpcDeadlineComplete", "transportQuarantineComplete", "formalTaskAdmission", "hardwareWritten")}))


if __name__ == "__main__":
    main()
