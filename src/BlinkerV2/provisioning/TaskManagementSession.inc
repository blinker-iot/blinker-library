#include "TaskManagementSession.h"
#include <string.h>

namespace blinker {
namespace task_session_detail {
bool nonzero(ByteView bytes, size_t size) {
    if (bytes.data == nullptr || bytes.size != size) return false;
    uint8_t bits = 0U;
    for (size_t i = 0U; i < size; ++i) bits |= bytes.data[i];
    return bits != 0U;
}
struct Guard {
    bool& busy;
    explicit Guard(bool& value) : busy(value) { busy = true; }
    ~Guard() { busy = false; }
};
} // namespace task_session_detail

TaskManagementSession::TaskManagementSession(TaskEngine& engine, TaskAuthorityCache& authority,
    IRandom& random, IServerSignatureVerifier& verifier, IClock& clock,
    uint32_t lifetimeMillis, uint32_t minimumChallengeIntervalMillis)
    : engine_(engine), authority_(authority), random_(random), verifier_(verifier), clock_(clock),
      request_(), expected_(), nonce_(), digest_(), channel_(0U), generation_(0U),
      lifetime_(lifetimeMillis), interval_(minimumChallengeIntervalMillis), lastTick_(clock.monotonicMillis()),
      remaining_(0U), cooldown_(0U), operation_(Operation::Initialize), pending_(false), busy_(false), aborted_(false) {}

void TaskManagementSession::cancel() {
    pending_ = false;
    aborted_ = true;
    remaining_ = 0U;
    secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
}
void TaskManagementSession::disconnect(uint64_t channel) { if (channel == channel_) cancel(); }

bool TaskManagementSession::validContext() const {
    TaskAuthorityBinding root;
    return !aborted_ && remaining_ != 0U && generation_ == authority_.generation() && authority_.root(root).ok();
}

void TaskManagementSession::poll() {
    const uint32_t now = clock_.monotonicMillis();
    const uint32_t elapsed = now - lastTick_;
    lastTick_ = now;
    if (elapsed > 0x7FFFFFFFU) {
        cancel();
        cooldown_ = interval_; // Invalid tick cannot bypass challenge throttling.
        return;
    }
    cooldown_ = elapsed >= cooldown_ ? 0U : cooldown_ - elapsed;
    remaining_ = elapsed >= remaining_ ? 0U : remaining_ - elapsed;
    if ((pending_ || busy_) && !validContext()) cancel();
}

Result TaskManagementSession::available(uint64_t channel) {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    poll();
    if (channel == 0U || lifetime_ == 0U || lifetime_ > 0x7FFFFFFFU || interval_ == 0U || interval_ > 0x7FFFFFFFU)
        return Result::failure(ErrorCode::InvalidArgument);
    if (pending_ || cooldown_ != 0U) return Result::failure(ErrorCode::WouldBlock);
    return Result::success();
}

Result TaskManagementSession::issue(uint64_t channel, uint32_t generation, TaskManagementChallenge& output) {
    task_session_detail::Guard guard(busy_);
    channel_ = channel;
    generation_ = generation;
    remaining_ = lifetime_;
    cooldown_ = interval_;
    aborted_ = false;
    Result result = random_.fill(MutableByteSpan(nonce_, sizeof(nonce_)));
    poll(); // Includes provider time, cancellation and reentrant root changes.
    if (result && (!validContext() || !task_session_detail::nonzero(ByteView(nonce_, sizeof(nonce_)), 16U)))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    TaskAuthorityBinding current;
    if (result) result = authority_.root(current);
    if (!result) { cancel(); return result; }
    memcpy(current.taskScope, expected_.taskScope, 16U);
    output.authority = current;
    memcpy(output.nonce, nonce_, sizeof(nonce_));
    memcpy(output.requestId, request_.requestId, sizeof(request_.requestId));
    memcpy(output.operationDigest, digest_, sizeof(digest_));
    pending_ = true;
    return Result::success();
}

Result TaskManagementSession::prepareAuthority(uint64_t channel, const TaskAuthorityChange& change,
    TaskManagementChallenge& output) {
    output = TaskManagementChallenge();
    Result result = available(channel);
    if (!result) return result;
    result = taskAuthorityChangeDigest(change, MutableByteSpan(digest_, sizeof(digest_)));
    if (!result) return result;
    TaskAuthorityBinding current;
    result = authority_.root(current);
    if (!result) return result;
    if (!constantTimeEqual(ByteView(current.deviceInstanceId, 16U), change.deviceInstanceId) ||
        !constantTimeEqual(ByteView(current.rootDigest, 32U), change.currentRoot))
        return Result::failure(ErrorCode::AuthenticationRequired);
    const bool initialize = change.previousRoot.empty();
    if (initialize) {
        if (engine_.state() != TaskEngineState::Empty) return Result::failure(ErrorCode::StateConflict);
        expected_ = current;
        memcpy(expected_.taskScope, change.taskScope.data, 16U);
    } else {
        result = engine_.authorityBinding(expected_);
        if (!result) return result;
        if (!constantTimeEqual(ByteView(expected_.deviceInstanceId, 16U), change.deviceInstanceId) ||
            !constantTimeEqual(ByteView(expected_.taskScope, 16U), change.taskScope) ||
            !constantTimeEqual(ByteView(expected_.rootDigest, 32U), change.previousRoot))
            return Result::failure(ErrorCode::AuthenticationRequired);
    }
    request_ = TaskMutation();
    memcpy(request_.requestId, change.requestId.data, 16U);
    operation_ = initialize ? Operation::Initialize : Operation::Rebind;
    return issue(channel, authority_.generation(), output);
}

Result TaskManagementSession::prepareMutation(uint64_t channel, const TaskMutation& request,
    TaskManagementChallenge& output) {
    output = TaskManagementChallenge();
    Result result = available(channel);
    if (!result) return result;
    TaskAuthorityObservation current;
    authority_.observe(current);
    if (current.state != TaskAuthorityState::Current) return Result::failure(ErrorCode::AuthenticationRequired);
    result = taskMutationPrepareDigest(request, ByteView(current.binding.rootDigest, 32U), MutableByteSpan(digest_, 32U));
    if (!result) return result;
    expected_ = current.binding;
    request_ = request;
    operation_ = Operation::Mutate;
    return issue(channel, authority_.generation(), output);
}

Result TaskManagementSession::prepareRead(uint64_t channel, ByteView requestId, uint32_t revision, uint32_t index,
    TaskManagementChallenge& output) {
    if (!task_session_detail::nonzero(requestId, 16U)) { output = TaskManagementChallenge(); return Result::failure(ErrorCode::InvalidArgument); }
    TaskReadPrepare request; memcpy(request.requestId, requestId.data, 16U);
    request.expectedRevision = revision; request.index = index;
    return prepareRead(channel, request, output);
}
Result TaskManagementSession::prepareRead(uint64_t channel, const TaskReadPrepare& request, TaskManagementChallenge& output) {
    output = TaskManagementChallenge();
    Result result = available(channel);
    if (!result) return result;
    TaskAuthorityObservation current;
    authority_.observe(current);
    if (current.state != TaskAuthorityState::Current) return Result::failure(ErrorCode::AuthenticationRequired);
    result = taskReadPrepareDigest(request, ByteView(current.binding.rootDigest, 32U), MutableByteSpan(digest_, 32U));
    if (!result) return result;
    expected_ = current.binding;
    request_ = TaskMutation();
    memcpy(request_.requestId, request.requestId, 16U);
    request_.expectedRevision = request.expectedRevision;
    request_.taskId = request.index;
    // Read has no definition; reuse that owned storage instead of enlarging
    // the one pending slot or retaining caller memory across the challenge.
    if (request.catalog) memcpy(request_.definition.key, request.catalogFingerprint, 32U);
    operation_ = request.catalog ? Operation::Catalog : Operation::Read;
    return issue(channel, authority_.generation(), output);
}

Result TaskManagementSession::retirementBinding(TaskAuthorityBinding& output) const {
    TaskSnapshotHeader marker;
    if (engine_.state() == TaskEngineState::Retired) {
        const Result result = engine_.retirementRecord(marker);
        output = result ? marker.authority : TaskAuthorityBinding();
        return result;
    }
    return engine_.authorityBinding(output);
}

Result TaskManagementSession::prepareRetirement(uint64_t channel, const TaskRetirementPrepare& request,
    bool query, TaskManagementChallenge& output) {
    output = TaskManagementChallenge();
    Result result = available(channel);
    if (result) result = taskRetirementPrepareDigest(request, query, MutableByteSpan(digest_, 32U));
    TaskAuthorityBinding stored, root;
    if (result) result = retirementBinding(stored);
    if (result) result = authority_.root(root);
    memcpy(root.taskScope, request.authority.taskScope, 16U);
    // The current root authorizes termination of this ownership tenure, not
    // restoration of the old executable root. Pin the EXACT stored binding
    // separately in expected_; generation pins the independently loaded root.
    if (result && (root != request.authority || !stored.sameOwner(root)))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    if (!result) return result;
    TaskSnapshotHeader marker;
    if (query || engine_.state() == TaskEngineState::Retired) {
        result = engine_.retirementRecord(marker);
        if (!result) return result;
        if (!query && !constantTimeEqual(ByteView(marker.requestId, 16U), ByteView(request.requestId, 16U)))
            return Result::failure(ErrorCode::StateConflict);
    }
    expected_ = stored;
    request_ = TaskMutation(); memcpy(request_.requestId, request.requestId, 16U);
    operation_ = query ? Operation::RetirementRead : Operation::Retire;
    return issue(channel, authority_.generation(), output);
}

Result TaskManagementSession::commit(uint64_t channel, ByteView grant, TaskManagementResult& output,
    TaskManagementChallenge* completed) {
    output = TaskManagementResult();
    if (completed != nullptr) *completed = TaskManagementChallenge();
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    poll();
    if (!pending_ || channel != channel_) return Result::failure(ErrorCode::AuthenticationRequired);
    task_session_detail::Guard guard(busy_);
    pending_ = false; // One verification attempt, including malformed/failed proof.
    TaskGrantBinding binding;
    binding.deviceInstanceId = ByteView(expected_.deviceInstanceId, 16U);
    binding.taskScope = ByteView(expected_.taskScope, 16U);
    binding.nonce = ByteView(nonce_, sizeof(nonce_));
    binding.requestId = ByteView(request_.requestId, 16U);
    binding.operationDigest = ByteView(digest_, sizeof(digest_));
    TaskManagementGrant verified;
    Result result = verifyTaskManagementGrant(grant, binding, verifier_, verified);
    poll();
    if (result && !validContext()) result = Result::failure(ErrorCode::AuthenticationRequired);
    if (result && (operation_ == Operation::Initialize || operation_ == Operation::Rebind)) {
        TaskAuthorityBinding replacement;
        result = authority_.root(replacement);
        memcpy(replacement.taskScope, expected_.taskScope, 16U);
        if (result) result = authority_.selectVerified(replacement, generation_);
        if (result) result = operation_ == Operation::Initialize
            ? engine_.initializeVerified(replacement) : engine_.rebindVerified(expected_, replacement);
        // Do not leave an unpersisted proposal Current, nor read a failed handle
        // to guess durability. Engine exposes metadata only in validated states.
        if (generation_ == authority_.generation()) authority_.restore(engine_);
    } else if (result && (operation_ == Operation::Retire || operation_ == Operation::RetirementRead)) {
        TaskAuthorityBinding stored, root;
        result = retirementBinding(stored);
        if (result) result = authority_.root(root);
        memcpy(root.taskScope, expected_.taskScope, 16U);
        // validContext has rechecked the current-root generation after Grant
        // verification. The signed operation digest binds that root, while
        // expected_ must still match the entire original stored table/marker.
        if (result && (stored != expected_ || !root.sameOwner(expected_)))
            result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result && operation_ == Operation::Retire)
            result = engine_.retireVerified(expected_, ByteView(request_.requestId, 16U));
        TaskSnapshotHeader marker;
        if (result) result = engine_.retirementRecord(marker);
        if (result) {
            output.kind = TaskManagementResultKind::Retired;
            memcpy(output.retirementId, marker.requestId, 16U);
        }
        // Clear executable scope, but keep independently loaded roots available
        // for fresh authenticated queries. Never erase keys or retire the cache.
        authority_.restore(engine_);
    } else if (result && operation_ == Operation::Mutate) {
        TaskMutationRejection rejection = TaskMutationRejection::None;
        result = engine_.mutateVerified(expected_, request_, output.mutation, &rejection);
        output.kind = TaskManagementResultKind::Mutation;
        if (!result && rejection != TaskMutationRejection::None) {
            // This is a completed exchange, NOT a successful mutation. Only the
            // Engine's exact pre-write decisions qualify; generic WouldBlock,
            // validation and storage errors remain unauthenticated failure.
            output.kind = TaskManagementResultKind::Rejected;
            output.rejection = rejection;
            result = Result::success();
        }
    } else if (result) {
        output.kind = TaskManagementResultKind::Metadata;
        TaskAuthorityObservation current;
        authority_.observe(current);
        TaskAuthorityBinding stored;
        result = engine_.authorityBinding(stored);
        if (result && (current.state != TaskAuthorityState::Current || current.binding != expected_ || stored != expected_))
            result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result && operation_ == Operation::Catalog) {
            TaskTargetView target; TaskTargetCatalogPage& page = output.catalog;
            result = engine_.describeTargets(request_.taskId, target, page.total, MutableByteSpan(page.fingerprint, 32U));
            if (result && request_.taskId != UINT32_MAX &&
                !constantTimeEqual(ByteView(page.fingerprint, 32U), ByteView(request_.definition.key, 32U)))
                result = Result::failure(ErrorCode::SequenceConflict);
            if (result) {
                page.capacity = engine_.capacity(); output.index = request_.taskId;
                output.kind = request_.taskId == UINT32_MAX ? TaskManagementResultKind::Catalog : TaskManagementResultKind::Target;
                if (request_.taskId != UINT32_MAX) {
                    memcpy(page.key, target.key.data, target.key.size);
                    page.keyLength = static_cast<uint8_t>(target.key.size); page.valueType = target.valueType;
                }
            }
        }
        if (result && operation_ == Operation::Read && request_.taskId != UINT32_MAX && engine_.tableRevision() != request_.expectedRevision)
            result = Result::failure(ErrorCode::SequenceConflict);
        if (result && operation_ == Operation::Read && request_.taskId != UINT32_MAX) {
            result = engine_.recordAt(request_.taskId, output.record);
            output.hasRecord = result.ok();
            output.kind = TaskManagementResultKind::Record;
            output.index = request_.taskId;
        }
    }
    poll();
    if (result && !validContext()) result = Result::failure(ErrorCode::AuthenticationRequired);
    if (result) {
        output.tableRevision = engine_.tableRevision();
        output.count = engine_.count();
        if (completed != nullptr) {
            // Recover the ACTUAL consumed context before nonce zeroization.
            // Root was rechecked above, including same-owner rebind's new root.
            authority_.root(completed->authority);
            memcpy(completed->authority.taskScope, expected_.taskScope, 16U);
            memcpy(completed->nonce, nonce_, sizeof(nonce_));
            memcpy(completed->requestId, request_.requestId, 16U);
            memcpy(completed->operationDigest, digest_, sizeof(digest_));
        }
    } else output = TaskManagementResult();
    cancel();
    return result;
}

} // namespace blinker
