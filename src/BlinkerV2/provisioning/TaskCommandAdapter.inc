#include "TaskCommandAdapter.h"

namespace blinker {
namespace task_adapter_detail {
struct Guard {
    bool& busy;
    explicit Guard(bool& value) : busy(value) { busy = true; }
    ~Guard() { busy = false; }
};
bool valid(const TaskCommandSource& source) {
    uint8_t bits = 0U;
    for (size_t i = 0U; i < sizeof(source.peerId); ++i) bits |= source.peerId[i];
    return source.generation != 0U && (bits != 0U) == source.routed &&
        (source.routed ? source.peerKind <= bbp2::RoutePeerKind::Platform : source.peerKind == bbp2::RoutePeerKind::LogicalDevice) &&
        (source.role == TaskResultKey::DeviceKey || source.role == TaskResultKey::Admin);
}
} // namespace task_adapter_detail

TaskCommandAdapter::TaskCommandAdapter(TaskManagementSession& session, TaskAuthorityCache& authority,
    TaskEngine& engine, const DeviceInstanceId& instance, const TaskRootSources& roots)
    : session_(session), authority_(authority), engine_(engine), instance_(instance), roots_(roots),
      source_(), channel_(0U), active_(false), started_(false), busy_(false) {}

void TaskCommandAdapter::cancel() { session_.cancel(); active_ = false; }
void TaskCommandAdapter::stop() { started_ = false; cancel(); }
void TaskCommandAdapter::poll() { session_.poll(); if (!session_.pending()) active_ = false; }
bool TaskCommandAdapter::matches(const TaskCommandSource& source) const {
    return active_ && source.generation == source_.generation && source.session == source_.session &&
        source.transport == source_.transport && source.role == source_.role &&
        source.peerKind == source_.peerKind && source.routed == source_.routed &&
        constantTimeEqual(ByteView(source.peerId, 16U), ByteView(source_.peerId, 16U));
}
void TaskCommandAdapter::disconnect(const TaskCommandSource& source) { if (matches(source)) cancel(); }
void TaskCommandAdapter::disconnectTransport(uint8_t transport, uint32_t generation) {
    if (active_ && source_.transport == transport && source_.generation == generation) cancel();
}
void TaskCommandAdapter::invalidateRoots() { cancel(); authority_.invalidate(); }
void TaskCommandAdapter::retire() { stop(); authority_.retire(); engine_.retire(); }

Result TaskCommandAdapter::refreshRoots() {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    task_adapter_detail::Guard guard(busy_);
    cancel();
    Result result = authority_.refresh(instance_, roots_);
    if (result && engine_.state() != TaskEngineState::Empty) {
        result = authority_.restore(engine_);
        // A different actual root stays Pending and can be explicitly rebound
        // with a fresh Grant. Fault/retirement never become an empty table.
    }
    return result;
}
Result TaskCommandAdapter::start() {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (started_) return Result::failure(ErrorCode::AlreadyExists);
    started_ = true;
    const Result result = refreshRoots();
    // Root mismatch need not brick explicit signed Rebind. Every operation
    // still independently enforces the Session/Engine authority state.
    return result;
}

Result TaskCommandAdapter::receive(const TaskCommandSource& source, TaskCommand command, ByteView body,
    MutableByteSpan output, ByteView& response) {
    response = ByteView();
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!started_) return Result::failure(ErrorCode::NotConfigured);
    if (!task_adapter_detail::valid(source) || !taskWireRequest(command))
        return Result::failure(ErrorCode::InvalidArgument);
    if (source.role == TaskResultKey::DeviceKey ? roots_.deviceKey == nullptr : roots_.controllers == nullptr)
        return Result::failure(ErrorCode::AuthenticationRequired);
    const size_t required = command == TaskCommand::Commit ? kTaskManagementResultMaxSize : kTaskManagementChallengeMaxSize;
    // Validate output before consuming a challenge or writing Flash. Actual
    // send failure AFTER success is still an uncertain outcome, never rollback.
    if (output.data == nullptr || output.size < required) return Result::failure(ErrorCode::BufferTooSmall);
    poll();
    task_adapter_detail::Guard guard(busy_);
    TaskManagementChallenge context;
    Result result;
    if (command == TaskCommand::Commit) {
        if (!matches(source)) return Result::failure(ErrorCode::AuthenticationRequired);
        const uint32_t generation = authority_.generation();
        const TaskResultKey role = source_.role;
        TaskManagementResult value;
        result = session_.commit(channel_, body, value, &context);
        // Keep source binding during key provider callbacks: disconnect/stop
        // must also suppress a reply after an operation or pre-write rejection.
        if (result) result = encodeTaskManagementReceipt(instance_, roots_, role, context, value, output, response);
        if (result && (!active_ || !started_ || generation != authority_.generation()))
            result = Result::failure(ErrorCode::AuthenticationRequired);
        cancel();
    } else {
        if (active_ || session_.pending()) return Result::failure(ErrorCode::WouldBlock);
        if (channel_ == UINT64_MAX) return Result::failure(ErrorCode::CapacityExceeded);
        ++channel_; // Never reuse an in-process handle after disconnect/restart.
        source_ = source; active_ = true;
        if (command == TaskCommand::Authority) {
            TaskAuthorityChange change;
            result = decodeTaskAuthorityChange(body, change);
            if (result) result = session_.prepareAuthority(channel_, change, context);
        } else if (command == TaskCommand::Mutation) {
            TaskMutation mutation;
            result = decodeTaskMutationPrepare(body, mutation);
            if (result) result = session_.prepareMutation(channel_, mutation, context);
        } else if (command == TaskCommand::Read) {
            TaskReadPrepare read;
            result = decodeTaskReadPrepare(body, read);
            if (result) result = session_.prepareRead(channel_, read, context);
        } else {
            TaskRetirementPrepare request;
            result = decodeTaskRetirementPrepare(body, request);
            if (result) result = session_.prepareRetirement(channel_, request, command == TaskCommand::RetirementRead, context);
        }
        if (result && (!active_ || !started_)) result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result) result = encodeTaskManagementChallenge(context, output, response);
        if (!result) cancel();
    }
    if (!result) {
        if (!response.empty()) secureZero(MutableByteSpan(output.data, response.size));
        response = ByteView();
    }
    return result;
}
} // namespace blinker
