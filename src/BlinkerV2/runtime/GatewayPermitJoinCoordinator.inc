#include "GatewayPermitJoinCoordinator.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

GatewayPermitJoinCoordinator::GatewayPermitJoinCoordinator(
    IGatewayPermitJoinAdapter& adapter,
    IClock& clock)
    : adapter_(adapter), clock_(clock), activeWindow_(), terminalWindow_(),
      terminalStatus_(gateway::GatewayPermitJoinStatus::Rejected),
      result_(), resultSize_(0U), budget_(), phase_(Idle), hasTerminal_(false),
      publishPending_(false) {}

GatewayPermitJoinCoordinator::~GatewayPermitJoinCoordinator() {
    reset();
}

bool GatewayPermitJoinCoordinator::continuesCommand(ByteView encoded) const {
    gateway::GatewayPermitJoinCommandView command;
    return phase_ != Idle && gateway::decodeGatewayPermitJoinCommand(encoded, command) && sameWindow(command);
}

Result GatewayPermitJoinCoordinator::handleCommand(ByteView encoded) {
    gateway::GatewayPermitJoinCommandView command;
    Result result = gateway::decodeGatewayPermitJoinCommand(
        encoded, command);
    if (!result) return result;

    if (command.adapterId != adapter_.adapterId()) {
        reply(command, gateway::GatewayPermitJoinStatus::Unsupported);
        return Result::success();
    }

    const bool terminalReplay =
        hasTerminal_ && command.adapterId == terminalWindow_.adapterId &&
        command.expiresAtUnixSeconds ==
            terminalWindow_.expiresAtUnixSeconds &&
        command.operationId.size == sizeof(terminalWindow_.operationId) &&
        memcmp(
            command.operationId.data,
            terminalWindow_.operationId,
            sizeof(terminalWindow_.operationId)) == 0;
    if (terminalReplay) {
        if (phase_ != Closing) reply(command, terminalStatus_);
        return Result::success();
    }

    if (command.action == gateway::GatewayPermitJoinAction::Close) {
        if (phase_ != Idle && !sameWindow(command)) {
            reply(command, gateway::GatewayPermitJoinStatus::Rejected);
            return Result::success();
        }
        copyWindow(command);
        expire(gateway::GatewayPermitJoinStatus::Closed);
        return Result::success();
    }

    if (phase_ != Idle || ownsChildSession()) {
        if (!sameWindow(command)) {
            reply(command, gateway::GatewayPermitJoinStatus::Busy);
        } else if (windowOpen()) {
            reply(command, gateway::GatewayPermitJoinStatus::Ready);
        }
        return Result::success();
    }

    uint64_t now = 0U;
    if (!clock_.unixTime(now) || now == 0U) {
        reply(command, gateway::GatewayPermitJoinStatus::Rejected);
        return Result::success();
    }
    if (now >= command.expiresAtUnixSeconds) {
        copyWindow(command);
        expire(gateway::GatewayPermitJoinStatus::Expired);
        return Result::success();
    }
    // Service permits 60..900 s (300 s by default). Pin its remaining
    // authenticated lifetime once; duplicate Open and clock rollback cannot
    // extend either acquisition or the usable window.
    if (!budget_.begin(clock_.monotonicMillis(), now,
                       command.expiresAtUnixSeconds, 900U)) {
        reply(command, gateway::GatewayPermitJoinStatus::Rejected);
        return Result::success();
    }
    copyWindow(command);
    phase_ = Opening;
    publishPending_ = false;
    result = adapter_.openWindow();
    if (!result && result.code() == ErrorCode::WouldBlock) {
        return Result::success();
    }
    if (!result) {
        expire(openFailureStatus(result.code()));
        return Result::success();
    }
    phase_ = Ready;
    reply(command, gateway::GatewayPermitJoinStatus::Ready);
    return Result::success();
}

void GatewayPermitJoinCoordinator::poll() {
    if (phase_ == Opening || phase_ == Ready) {
        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U) {
            expire(gateway::GatewayPermitJoinStatus::Rejected);
        } else if (now >= activeWindow_.expiresAtUnixSeconds ||
                   !budget_.active(clock_.monotonicMillis())) {
            expire(gateway::GatewayPermitJoinStatus::Expired);
        }
    }
    adapter_.poll(0U);
    if (phase_ == Closing) {
        finishClose();
    } else if (phase_ == Opening &&
               adapter_.state() == GatewayPermitJoinAdapterState::Ready) {
        phase_ = Ready;
        replyWindow(activeWindow_, gateway::GatewayPermitJoinStatus::Ready);
    } else if ((phase_ == Opening &&
                adapter_.state() != GatewayPermitJoinAdapterState::Opening) ||
               (phase_ == Ready &&
                adapter_.state() != GatewayPermitJoinAdapterState::Ready)) {
        expire(gateway::GatewayPermitJoinStatus::Rejected);
    }
}

ByteView GatewayPermitJoinCoordinator::pendingResult() const {
    return publishPending_ ? ByteView(result_, resultSize_) : ByteView();
}

void GatewayPermitJoinCoordinator::markResultPublished() {
    publishPending_ = false;
}

void GatewayPermitJoinCoordinator::reset() {
    if (phase_ != Idle ||
        adapter_.state() != GatewayPermitJoinAdapterState::Stopped) {
        adapter_.closeWindow();
    }
    clearState();
}

void GatewayPermitJoinCoordinator::rejectWindow() {
    if (phase_ == Opening || phase_ == Ready) {
        expire(gateway::GatewayPermitJoinStatus::Rejected);
    }
}

uint16_t GatewayPermitJoinCoordinator::activeAdapterId() const {
    return windowOpen() ? activeWindow_.adapterId : 0U;
}

ByteView GatewayPermitJoinCoordinator::activeOperationId() const {
    return windowOpen()
               ? ByteView(
                     activeWindow_.operationId,
                     sizeof(activeWindow_.operationId))
               : ByteView();
}

uint64_t GatewayPermitJoinCoordinator::expiresAtUnixSeconds() const {
    return windowOpen() ? activeWindow_.expiresAtUnixSeconds : 0U;
}

bool GatewayPermitJoinCoordinator::sameWindow(
    const gateway::GatewayPermitJoinCommandView& command) const {
    return phase_ != Idle && command.adapterId == activeWindow_.adapterId &&
           command.expiresAtUnixSeconds ==
               activeWindow_.expiresAtUnixSeconds &&
           command.operationId.size == sizeof(activeWindow_.operationId) &&
           memcmp(
               command.operationId.data,
               activeWindow_.operationId,
               sizeof(activeWindow_.operationId)) == 0;
}

void GatewayPermitJoinCoordinator::copyWindow(
    const gateway::GatewayPermitJoinCommandView& command) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&activeWindow_),
        sizeof(activeWindow_)));
    memcpy(
        activeWindow_.operationId,
        command.operationId.data,
        sizeof(activeWindow_.operationId));
    activeWindow_.adapterId = command.adapterId;
    activeWindow_.expiresAtUnixSeconds = command.expiresAtUnixSeconds;
}

void GatewayPermitJoinCoordinator::reply(
    const gateway::GatewayPermitJoinCommandView& command,
    gateway::GatewayPermitJoinStatus status) {
    gateway::GatewayPermitJoinResultView result;
    result.operationId = command.operationId;
    result.adapterId = command.adapterId;
    result.status = status;
    result.expiresAtUnixSeconds = command.expiresAtUnixSeconds;
    size_t written = 0U;
    const Result encoded = gateway::encodeGatewayPermitJoinResult(
        result, MutableByteSpan(result_, sizeof(result_)), written);
    if (!encoded) {
        secureZero(MutableByteSpan(result_, sizeof(result_)));
        resultSize_ = 0U;
        publishPending_ = false;
        return;
    }
    resultSize_ = written;
    publishPending_ = true;
}

gateway::GatewayPermitJoinStatus
GatewayPermitJoinCoordinator::openFailureStatus(ErrorCode error) const {
    if (error == ErrorCode::UnsupportedFeature) {
        return gateway::GatewayPermitJoinStatus::Unsupported;
    }
    if (error == ErrorCode::AlreadyExists || error == ErrorCode::CapacityExceeded) {
        return gateway::GatewayPermitJoinStatus::Busy;
    }
    return gateway::GatewayPermitJoinStatus::Rejected;
}

void GatewayPermitJoinCoordinator::expire(
    gateway::GatewayPermitJoinStatus status) {
    if (phase_ != Idle || ownsChildSession()) adapter_.closeWindow();
    terminalWindow_ = activeWindow_;
    terminalStatus_ = status;
    hasTerminal_ = true;
    phase_ = Closing;
    budget_.clear();
    publishPending_ = false;
    finishClose();
}

void GatewayPermitJoinCoordinator::finishClose() {
    if (ownsChildSession()) return;
    phase_ = Idle;
    replyWindow(terminalWindow_, terminalStatus_);
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&activeWindow_), sizeof(activeWindow_)));
}

void GatewayPermitJoinCoordinator::replyWindow(
    const OwnedWindow& window, gateway::GatewayPermitJoinStatus status) {
    gateway::GatewayPermitJoinCommandView command;
    command.operationId = ByteView(
        window.operationId, sizeof(window.operationId));
    command.adapterId = window.adapterId;
    command.action = gateway::GatewayPermitJoinAction::Open;
    command.expiresAtUnixSeconds = window.expiresAtUnixSeconds;
    reply(command, status);
}

void GatewayPermitJoinCoordinator::clearState() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&activeWindow_),
        sizeof(activeWindow_)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&terminalWindow_),
        sizeof(terminalWindow_)));
    secureZero(MutableByteSpan(result_, sizeof(result_)));
    terminalStatus_ = gateway::GatewayPermitJoinStatus::Rejected;
    resultSize_ = 0U;
    phase_ = Idle;
    budget_.clear();
    hasTerminal_ = false;
    publishPending_ = false;
}

} // namespace blinker
