#ifndef BLINKER_PROVISIONING_TASKAUTHORITYCONTRACT_H
#define BLINKER_PROVISIONING_TASKAUTHORITYCONTRACT_H

#include "TaskManagementGrant.h"
#include "TaskManagementContract.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IDeviceKeySource.h"
#include "../interface/IControllerCredentialSource.h"
#include "../runtime/TaskAuthority.h"

namespace blinker {

// Fixed by product composition, NOT chosen by a received request or route.
// WiFi: key only; BLE: Admin only; WiFi+BLE: BOTH, no missing-root fallback.
// Selector comes from the installed/verified Admin lifecycle, not permissions
// on an incoming connection. Refresh at lifecycle boundaries, not every poll:
// sources can perform storage I/O. Caller serializes against root mutations.
struct TaskRootSources {
    IDeviceKeySource* deviceKey;
    IControllerCredentialSource* controllers;
    ByteView adminControllerId;
    uint32_t accessEpoch;
    TaskRootSources() : deviceKey(nullptr), controllers(nullptr), accessEpoch(0U) {}
};

// Streams a domain-separated hash of actual loaded root material. Never hashes
// MQTT credentials, shared roots, MAC or a stored task binding. No writes.
// Fingerprint is NOT owner proof; raw loaded secrets are cleared on all exits.
Result taskAuthorityRootDigest(const DeviceInstanceId& instance,
    const TaskRootSources& sources, MutableByteSpan digest);

// Reload each fixed root once, compare the FULL commitment with completed,
// then sign with the selected actual secret. Role is chosen by composition,
// never decoded from the request. No secret escapes or is persisted here.
Result encodeTaskManagementReceipt(const DeviceInstanceId& instance,
    const TaskRootSources& sources, TaskResultKey role,
    const TaskManagementChallenge& completed, const TaskManagementResult& result,
    MutableByteSpan output, ByteView& encoded);

enum : size_t { kTaskAuthorityChangeMaxSize = 121U };

// Borrowed internal Prepare sub-contract, no public kind. Empty previousRoot
// explicitly means Initialize; a 32-byte previousRoot means same-owner Rebind.
// Pending owner must construct previousRoot/scope from validated durable state
// (or confirm Empty), never trust a request to declare that state itself.
struct TaskAuthorityChange {
    ByteView requestId;
    ByteView deviceInstanceId;
    ByteView taskScope;
    ByteView previousRoot;
    ByteView currentRoot;
};

Result encodeTaskAuthorityChange(const TaskAuthorityChange& change,
    MutableByteSpan output, ByteView& encoded);
Result decodeTaskAuthorityChange(ByteView encoded, TaskAuthorityChange& change);
Result taskAuthorityChangeDigest(const TaskAuthorityChange& change, MutableByteSpan digest);

// Explicit bootstrap/rebind verifier, reusing Grant v1 without a scope wildcard.
// expected/nonce belong to the device pending operation; currentInstance/root
// are freshly established from actual sources. Proposed scope becomes trusted
// ONLY after signature AND pending/state checks, not merely decoding this body.
// Stateless: does NOT consume nonce, check TTL/CAS, persist or invoke Engine.
// Caller must recheck roots and state after verifier returns, before committing.
Result verifyTaskAuthorityChangeGrant(ByteView encodedGrant,
    const TaskAuthorityChange& expected, ByteView nonce,
    const DeviceInstanceId& currentInstance, ByteView currentRoot,
    IServerSignatureVerifier& verifier, TaskManagementGrant& grant);

class TaskEngine;
class TaskManagementSession;

// Optional, single cooperative owner. Holds commitments, never raw secrets.
// Fixed product root sources are supplied at lifecycle refresh boundaries.
// Invalidate BEFORE changing installed roots; native callbacks only post events.
class TaskAuthorityCache {
public:
    TaskAuthorityCache();
    void invalidate();
    void retire(); // Terminal, even if a provider is currently being called.
    Result refresh(const DeviceInstanceId& instance, const TaskRootSources& sources);
    // No Flash I/O: match independently refreshed roots to validated Engine
    // metadata. Never accepts a binding supplied by a network request.
    Result restore(const TaskEngine& engine);
    static void read(void* context, TaskAuthorityObservation& output);
    void observe(TaskAuthorityObservation& output) const { output = observation_; }
    Result root(TaskAuthorityBinding& output) const;
    uint32_t generation() const { return generation_; }

private:
    friend class TaskManagementSession;
    Result selectVerified(const TaskAuthorityBinding& binding, uint32_t generation);
    void clearScope();
    TaskAuthorityObservation observation_;
    uint32_t generation_;
    bool rootsReady_;
    bool refreshing_;
    TaskAuthorityCache(const TaskAuthorityCache&);
    TaskAuthorityCache& operator=(const TaskAuthorityCache&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(TaskAuthorityCache) <= 80U, "Task root cache exceeds its 32-bit ABI budget");
#endif

} // namespace blinker
#endif
