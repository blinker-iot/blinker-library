#ifndef BLINKER_PROVISIONING_TASKMANAGEMENTCONTRACT_H
#define BLINKER_PROVISIONING_TASKMANAGEMENTCONTRACT_H

#include "../runtime/TaskEngine.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {

// Explicitly installed products only; codecs do not advertise capabilities.
enum : size_t {
    kTaskMutationPrepareMaxSize = 123U,
    kTaskReadPrepareMaxSize = 59U,
    kTaskRetirementPrepareMaxSize = 87U,
    kTaskManagementChallengeMaxSize = 138U,
    kTaskManagementResultMaxSize = 197U,
    kTaskCarrierMaxSize = 202U,
    kTaskCarrierFrameMaxSize = bbp2::kTaskManagementMaximumFrameSize
};

// BBP TaskManagement body = [1, operation, bstr(exact inner CBOR)]. Request
// flags=0, response flags=IsResponse; nonzero outer sequence is correlation
// only, not an authorization or a durable/idempotency identifier.
enum class TaskWireOperation : uint8_t { Authority, Mutation, Read, Commit, Challenge, Result, Failure, Retire, RetirementRead };
inline bool taskWireRequest(TaskWireOperation op) {
    return op <= TaskWireOperation::Commit || op == TaskWireOperation::Retire || op == TaskWireOperation::RetirementRead;
}
enum class TaskWireFailure : uint8_t { Malformed = 1U, Unauthorized, Busy, Conflict, Unavailable, Capacity };
Result encodeTaskCarrier(TaskWireOperation operation, ByteView body, MutableByteSpan output, ByteView& encoded);
Result decodeTaskCarrier(ByteView encoded, TaskWireOperation& operation, ByteView& body);

struct TaskReadPrepare {
    uint8_t requestId[16];
    uint32_t expectedRevision;
    uint32_t index;
    // Absent on the original four-item table read. Five-item catalog reads
    // carry an empty bstr for metadata, or the exact metadata SHA-256 for a page.
    bool catalog;
    uint8_t catalogFingerprint[32];
    // Metadata first, without prior revision knowledge; then exact-CAS pages.
    TaskReadPrepare() : requestId(), expectedRevision(0U), index(UINT32_MAX), catalog(false), catalogFingerprint() {}
};

struct TaskManagementChallenge {
    TaskAuthorityBinding authority;
    uint8_t nonce[16];
    uint8_t requestId[16];
    uint8_t operationDigest[32];
    TaskManagementChallenge() : authority(), nonce(), requestId(), operationDigest() {}
};

// Exact currently loaded authentication root + stored ownership scope, NOT
// necessarily the root in the old table/terminal marker. No cross-owner or
// expired-Key bypass. Query uses its own request ID and never writes Flash.
struct TaskRetirementPrepare {
    TaskAuthorityBinding authority;
    uint8_t requestId[16];
    TaskRetirementPrepare() : authority(), requestId() {}
};
Result encodeTaskRetirementPrepare(const TaskRetirementPrepare& value, MutableByteSpan output, ByteView& encoded);
Result decodeTaskRetirementPrepare(ByteView encoded, TaskRetirementPrepare& value);
Result taskRetirementPrepareDigest(const TaskRetirementPrepare& value, bool query, MutableByteSpan digest);

enum class TaskManagementResultKind : uint8_t { Authority, Mutation, Metadata, Record, Retired, Catalog, Target, Rejected };
struct TaskTargetCatalogPage {
    uint8_t fingerprint[32];
    uint8_t key[kMaxEndpointKeyLength];
    size_t capacity;
    size_t total;
    uint8_t keyLength;
    ValueType valueType;
    TaskTargetCatalogPage() : fingerprint(), key(), capacity(0U), total(0U), keyLength(0U), valueType(ValueType::Boolean) {}
};
struct TaskManagementResult {
    TaskMutationReceipt mutation;
    TaskSnapshotRecord record;
    TaskTargetCatalogPage catalog;
    uint32_t tableRevision;
    uint32_t index;
    size_t count;
    TaskManagementResultKind kind;
    TaskMutationRejection rejection;
    bool hasRecord;
    uint8_t retirementId[16];
    TaskManagementResult() : mutation(), record(), catalog(), tableRevision(0U), index(UINT32_MAX), count(0U),
        kind(TaskManagementResultKind::Authority), rejection(TaskMutationRejection::None), hasRecord(false), retirementId() {}
};

// Key role fixed by the trusted composition/verifier, NOT selected from wire.
// DeviceKey: device <-> service. Admin: device <-> owner App; service has only
// the Admin fingerprint and cannot independently verify that proof.
enum class TaskResultKey : uint8_t { DeviceKey = 1U, Admin = 2U };

Result encodeTaskMutationPrepare(const TaskMutation& request, MutableByteSpan output, ByteView& encoded);
Result decodeTaskMutationPrepare(ByteView encoded, TaskMutation& request);
Result taskMutationPrepareDigest(const TaskMutation& request, ByteView currentRoot, MutableByteSpan digest);
Result encodeTaskReadPrepare(const TaskReadPrepare& request, MutableByteSpan output, ByteView& encoded);
Result decodeTaskReadPrepare(ByteView encoded, TaskReadPrepare& request);
// index==UINT32_MAX requires revision==0; otherwise the exact table revision.
Result taskReadPrepareDigest(ByteView requestId, uint32_t revision, uint32_t index,
    ByteView currentRoot, MutableByteSpan digest);
Result taskReadPrepareDigest(const TaskReadPrepare& request, ByteView currentRoot, MutableByteSpan digest);
Result encodeTaskManagementChallenge(const TaskManagementChallenge& value, MutableByteSpan output, ByteView& encoded);
Result decodeTaskManagementChallenge(ByteView encoded, TaskManagementChallenge& value);

// Streaming HMAC of domain + expected key role + COMPLETE challenge + result.
// Key must be the current 32-byte secret from a trusted source, never rootDigest
// or Admin fingerprint. No new key store. HMAC is integrity/possession proof,
// not encryption or device non-repudiation against another holder of the key.
// Only encode a completed authenticated Session exchange and its device-owned
// context. Rejected proves this attempt did not start persistence, not the
// history of this request. Encoding/send failure never proves rollback.
Result encodeTaskManagementResult(const TaskManagementChallenge& completed,
    const TaskManagementResult& value, TaskResultKey role, ByteView secret,
    MutableByteSpan output, ByteView& encoded);
// No public unverified result decoder: authenticate with the outstanding
// challenge first, then publish owned data. Caller checks operation semantics,
// request lifetime and consumes its outstanding request after successful use.
Result verifyTaskManagementResult(ByteView encoded, const TaskManagementChallenge& expected,
    TaskResultKey role, ByteView secret, TaskManagementResult& value);

} // namespace blinker
#endif
