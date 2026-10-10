#ifndef BLINKER_PROVISIONING_TASKMANAGEMENTGRANT_H
#define BLINKER_PROVISIONING_TASKMANAGEMENTGRANT_H

#include "../interface/IServerSignatureVerifier.h"

namespace blinker {

static const size_t kTaskGrantIdSize = 16U;
static const size_t kTaskGrantDigestSize = 32U;
static const size_t kTaskGrantSignatureSize = 64U;
static const size_t kTaskManagementGrantMaxSize = 176U;

// Borrowed views. For verification, these MUST come from current trusted scope
// and a device-owned pending operation, never from the received grant itself.
struct TaskGrantBinding {
    ByteView deviceInstanceId;
    ByteView taskScope;
    ByteView nonce;
    ByteView requestId;
    ByteView operationDigest;
};

struct TaskManagementGrant {
    TaskGrantBinding binding;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    TaskManagementGrant()
        : serverKeyId(0U), signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}
};

// Internal grant v1; carried by the optional TaskManagement BBP capability.
Result encodeTaskManagementGrant(
    const TaskManagementGrant& grant, MutableByteSpan output, ByteView& encoded);
Result decodeTaskManagementGrant(ByteView encoded, TaskManagementGrant& grant);

// Hash canonical [domain, version, instance, scope, nonce, requestId, digest,
// keyId, algorithm] directly into SHA-256; no full transcript workspace.
// Signature is not read. Output must be exactly 32 bytes.
Result taskManagementGrantDigest(
    const TaskManagementGrant& grant, MutableByteSpan digest);

// Established scope only: empty/zero expected scope is NOT an initialization
// wildcard. Caller owns pending timeout, one-shot consumption and pre-commit
// revalidation. Success alone does NOT consume a nonce or authorize Engine.
// Output borrows encoded; every failure resets it to the empty value.
Result verifyTaskManagementGrant(
    ByteView encoded, const TaskGrantBinding& expected,
    IServerSignatureVerifier& verifier, TaskManagementGrant& grant);

} // namespace blinker

#endif
