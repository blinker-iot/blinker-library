#include "GatewayPermitJoinCoordinator.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

GatewayPermitJoinCoordinator::GatewayPermitJoinCoordinator(
    IGatewayPermitJoinAdapter& adapter,
    IClock& clock)
    : adapter_(adapter), clock_(clock), activeWindow_(), terminalWindow_(),
      terminalStatus_(gateway::GatewayPermitJoinStatus::Rejected),
      result_(), resultSize_(0U), active_(false), hasTerminal_(false),
      publishPending_(false) {}

GatewayPermitJoinCoordinator::~GatewayPermitJoinCoordinator() {
    reset();
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
        reply(command, terminalStatus_);
        return Result::success();
    }

    if (command.action == gateway::GatewayPermitJoinAction::Close) {
        if (active_ && !sameWindow(command)) {
            reply(command, gateway::GatewayPermitJoinStatus::Rejected);
            return Result::success();
        }
        if (active_) {
            adapter_.closeWindow();
            active_ = false;
        }
        copyWindow(command);
        terminalWindow_ = activeWindow_;
        terminalStatus_ = gateway::GatewayPermitJoinStatus::Closed;
        hasTerminal_ = true;
        reply(command, terminalStatus_);
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(&activeWindow_),
            sizeof(activeWindow_)));
        return Result::success();
    }

    if (active_) {
        reply(
            command,
            sameWindow(command)
                ? gateway::GatewayPermitJoinStatus::Ready
                : gateway::GatewayPermitJoinStatus::Busy);
        return Result::success();
    }

    uint64_t now = 0U;
    if (!clock_.unixTime(now) || now == 0U) {
        reply(command, gateway::GatewayPermitJoinStatus::Rejected);
        return Result::success();
    }
    if (now >= command.expiresAtUnixSeconds) {
        copyWindow(command);
        terminalWindow_ = activeWindow_;
        terminalStatus_ = gateway::GatewayPermitJoinStatus::Expired;
        hasTerminal_ = true;
        reply(command, terminalStatus_);
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(&activeWindow_),
            sizeof(activeWindow_)));
        return Result::success();
    }

    result = adapter_.openWindow();
    if (!result) {
        reply(command, openFailureStatus(result.code()));
        return Result::success();
    }
    copyWindow(command);
    active_ = true;
    reply(command, gateway::GatewayPermitJoinStatus::Ready);
    return Result::success();
}

void GatewayPermitJoinCoordinator::poll() {
    if (!active_) return;
    adapter_.poll(0U);
    uint64_t now = 0U;
    if (!clock_.unixTime(now) || now == 0U) {
        expire(gateway::GatewayPermitJoinStatus::Rejected);
        return;
    }
    if (now >= activeWindow_.expiresAtUnixSeconds) {
        expire(gateway::GatewayPermitJoinStatus::Expired);
        return;
    }
    if (adapter_.state() != GatewayPermitJoinAdapterState::Ready) {
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
    if (active_ ||
        adapter_.state() != GatewayPermitJoinAdapterState::Stopped) {
        adapter_.closeWindow();
    }
    clearState();
}

uint16_t GatewayPermitJoinCoordinator::activeAdapterId() const {
    return active_ ? activeWindow_.adapterId : 0U;
}

ByteView GatewayPermitJoinCoordinator::activeOperationId() const {
    return active_
               ? ByteView(
                     activeWindow_.operationId,
                     sizeof(activeWindow_.operationId))
               : ByteView();
}

uint64_t GatewayPermitJoinCoordinator::expiresAtUnixSeconds() const {
    return active_ ? activeWindow_.expiresAtUnixSeconds : 0U;
}

bool GatewayPermitJoinCoordinator::sameWindow(
    const gateway::GatewayPermitJoinCommandView& command) const {
    return active_ && command.adapterId == activeWindow_.adapterId &&
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
    if (error == ErrorCode::AlreadyExists ||
        error == ErrorCode::WouldBlock) {
        return gateway::GatewayPermitJoinStatus::Busy;
    }
    return gateway::GatewayPermitJoinStatus::Rejected;
}

void GatewayPermitJoinCoordinator::expire(
    gateway::GatewayPermitJoinStatus status) {
    adapter_.closeWindow();
    terminalWindow_ = activeWindow_;
    terminalStatus_ = status;
    hasTerminal_ = true;
    gateway::GatewayPermitJoinCommandView command;
    command.operationId = ByteView(
        activeWindow_.operationId,
        sizeof(activeWindow_.operationId));
    command.adapterId = activeWindow_.adapterId;
    command.action = gateway::GatewayPermitJoinAction::Open;
    command.expiresAtUnixSeconds = activeWindow_.expiresAtUnixSeconds;
    active_ = false;
    reply(command, status);
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&activeWindow_),
        sizeof(activeWindow_)));
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
    active_ = false;
    hasTerminal_ = false;
    publishPending_ = false;
}

} // namespace blinker
