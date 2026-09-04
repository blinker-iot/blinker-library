#include "BleLocalProvisioningEndpoint.h"

#include <stdint.h>


#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool spansOverlap(MutableByteSpan left, MutableByteSpan right) {
    if (left.empty() || right.empty() || left.data == nullptr ||
        right.data == nullptr) {
        return false;
    }
    const uintptr_t leftBegin = reinterpret_cast<uintptr_t>(left.data);
    const uintptr_t rightBegin = reinterpret_cast<uintptr_t>(right.data);
    if (left.size > UINTPTR_MAX - leftBegin ||
        right.size > UINTPTR_MAX - rightBegin) {
        return true;
    }
    const uintptr_t leftEnd = leftBegin + left.size;
    const uintptr_t rightEnd = rightBegin + right.size;
    return leftBegin < rightEnd && rightBegin < leftEnd;
}

bool contains(MutableByteSpan storage, ByteView value) {
    if (storage.data == nullptr || value.data == nullptr || value.empty()) {
        return false;
    }
    const uintptr_t storageBegin = reinterpret_cast<uintptr_t>(storage.data);
    const uintptr_t valueBegin = reinterpret_cast<uintptr_t>(value.data);
    if (storage.size > UINTPTR_MAX - storageBegin ||
        value.size > UINTPTR_MAX - valueBegin) {
        return false;
    }
    const uintptr_t storageEnd = storageBegin + storage.size;
    const uintptr_t valueEnd = valueBegin + value.size;
    return valueBegin >= storageBegin && valueEnd <= storageEnd;
}

} // namespace

BleLocalProvisioningEndpoint::BleLocalProvisioningEndpoint(
    BleNoiseProvisioningChannel& channel,
    ILocalProvisioningApplication& application,
    MutableByteSpan operationWorkspace,
    MutableByteSpan responseStorage)
    : channel_(channel), application_(application),
      operationWorkspace_(operationWorkspace),
      responseStorage_(responseStorage), metrics_(), pendingResponseSize_(0U),
      activeSessionId_(0U), lastError_(ErrorCode::Ok),
      pendingKind_(BleNoisePayloadKind::Transport), started_(false),
      pendingResponse_(false), recoveryRequired_(false) {}

BleLocalProvisioningEndpoint::~BleLocalProvisioningEndpoint() { stop(); }

Result BleLocalProvisioningEndpoint::start() {
    if (started_)
        return Result::failure(ErrorCode::AlreadyExists);
    if (!validConfiguration() || !channel_.armed()) {
        application_.endSession();
        channel_.cancel();
        wipeWorkspaces();
        return Result::failure(ErrorCode::NotConfigured);
    }

    clearPending();
    activeSessionId_ = 0U;
    lastError_ = ErrorCode::Ok;
    recoveryRequired_ = false;
    channel_.setReceiver(&BleLocalProvisioningEndpoint::payloadThunk, this);
    channel_.setSessionHandlers(
        &BleLocalProvisioningEndpoint::connectedThunk,
        &BleLocalProvisioningEndpoint::disconnectedThunk,
        this);
    channel_.setFaultHandler(&BleLocalProvisioningEndpoint::faultThunk, this);
    Result result = channel_.start();
    if (!result) {
        channel_.setReceiver(nullptr, nullptr);
        channel_.setSessionHandlers(nullptr, nullptr, nullptr);
        channel_.setFaultHandler(nullptr, nullptr);
        application_.endSession();
        channel_.cancel();
        wipeWorkspaces();
        return result;
    }
    started_ = true;
    return Result::success();
}

void BleLocalProvisioningEndpoint::stop() {
    channel_.setReceiver(nullptr, nullptr);
    channel_.setSessionHandlers(nullptr, nullptr, nullptr);
    channel_.setFaultHandler(nullptr, nullptr);
    channel_.stop();
    application_.endSession();
    wipeWorkspaces();
    activeSessionId_ = 0U;
    started_ = false;
    recoveryRequired_ = false;
    lastError_ = ErrorCode::Ok;
}

void BleLocalProvisioningEndpoint::poll(uint32_t budgetMicros) {
    if (!started_)
        return;
    if (recoveryRequired_) {
        channel_.stop();
        return;
    }

    flushPending();
    if (recoveryRequired_) {
        channel_.stop();
        return;
    }
    channel_.poll(budgetMicros);
    if (recoveryRequired_) {
        channel_.stop();
        return;
    }
    flushPending();
    if (recoveryRequired_)
        channel_.stop();
}

TransportState BleLocalProvisioningEndpoint::state() const {
    if (!started_)
        return TransportState::Stopped;
    if (recoveryRequired_)
        return TransportState::Error;
    return channel_.state();
}

void BleLocalProvisioningEndpoint::payloadThunk(void* context,
                                                BleNoisePayloadKind kind,
                                                ByteView payload,
                                                const RxContext& rx) {
    BleLocalProvisioningEndpoint* endpoint =
        static_cast<BleLocalProvisioningEndpoint*>(context);
    if (endpoint != nullptr)
        endpoint->onPayload(kind, payload, rx);
}

void BleLocalProvisioningEndpoint::connectedThunk(void* context,
                                                  const RxContext& rx) {
    BleLocalProvisioningEndpoint* endpoint =
        static_cast<BleLocalProvisioningEndpoint*>(context);
    if (endpoint != nullptr)
        endpoint->onConnected(rx);
}

void BleLocalProvisioningEndpoint::disconnectedThunk(void* context,
                                                     const RxContext& rx) {
    BleLocalProvisioningEndpoint* endpoint =
        static_cast<BleLocalProvisioningEndpoint*>(context);
    if (endpoint != nullptr)
        endpoint->onDisconnected(rx);
}

void BleLocalProvisioningEndpoint::faultThunk(void* context,
                                              ErrorCode error,
                                              const RxContext& rx) {
    BleLocalProvisioningEndpoint* endpoint =
        static_cast<BleLocalProvisioningEndpoint*>(context);
    if (endpoint != nullptr)
        endpoint->onFault(error, rx);
}

void BleLocalProvisioningEndpoint::onPayload(BleNoisePayloadKind kind,
                                             ByteView payload,
                                             const RxContext& rx) {
    if (!started_ || recoveryRequired_)
        return;
    if (activeSessionId_ == 0U || rx.sessionId != activeSessionId_) {
        ++metrics_.ignoredForeignSessionEvents;
        return;
    }
    if (pendingResponse_) {
        enterRecovery(ErrorCode::ProtocolError);
        return;
    }

    ++metrics_.requests;
    ByteView response;
    Result result = application_.handle(
        payload, operationWorkspace_, responseStorage_, response);
    secureZero(operationWorkspace_);
    if (!result || !contains(responseStorage_, response)) {
        enterRecovery(result ? ErrorCode::InternalError : result.code());
        return;
    }
    result = sendResponse(kind, response);
    if (result.code() == ErrorCode::WouldBlock) {
        pendingKind_ = kind;
        pendingResponseSize_ = response.size;
        pendingResponse_ = true;
        ++metrics_.backpressureEvents;
        return;
    }
    if (!result) {
        enterRecovery(result.code());
        return;
    }
    ++metrics_.responses;
    clearPending();
}

void BleLocalProvisioningEndpoint::onConnected(const RxContext& rx) {
    if (!started_ || recoveryRequired_)
        return;
    if (rx.sessionId != 0U && rx.sessionId == channel_.activeSessionId() &&
        activeSessionId_ == 0U) {
        activeSessionId_ = rx.sessionId;
        return;
    }
    if (rx.sessionId != activeSessionId_) {
        ++metrics_.ignoredForeignSessionEvents;
    }
}

void BleLocalProvisioningEndpoint::onDisconnected(const RxContext& rx) {
    if (!started_ || recoveryRequired_)
        return;
    if (activeSessionId_ != 0U && rx.sessionId == activeSessionId_) {
        activeSessionId_ = 0U;
        enterRecovery(ErrorCode::NotConnected);
        return;
    }
    ++metrics_.ignoredForeignSessionEvents;
}

void BleLocalProvisioningEndpoint::onFault(ErrorCode error,
                                           const RxContext& rx) {
    if (!started_ || recoveryRequired_)
        return;
    if (activeSessionId_ != 0U && rx.sessionId != activeSessionId_) {
        ++metrics_.ignoredForeignSessionEvents;
        return;
    }
    enterRecovery(error == ErrorCode::Ok ? ErrorCode::InternalError : error);
}

Result BleLocalProvisioningEndpoint::sendResponse(BleNoisePayloadKind kind,
                                                  ByteView response) {
    if (kind != BleNoisePayloadKind::InitiatorHandshake) {
        return channel_.sendTransport(response);
    }
    Result result = channel_.sendHandshakeResponse(response);
    ByteView transcriptHash;
    if (result)
        result = channel_.handshakeHash(transcriptHash);
    if (result)
        result = application_.secureSessionReady(transcriptHash);
    return result;
}

void BleLocalProvisioningEndpoint::flushPending() {
    if (!pendingResponse_ || recoveryRequired_)
        return;
    Result result = sendResponse(
        pendingKind_, ByteView(responseStorage_.data, pendingResponseSize_));
    if (result.code() == ErrorCode::WouldBlock) {
        ++metrics_.backpressureEvents;
        return;
    }
    if (!result) {
        enterRecovery(result.code());
        return;
    }
    ++metrics_.responses;
    clearPending();
}

void BleLocalProvisioningEndpoint::enterRecovery(ErrorCode error) {
    if (recoveryRequired_)
        return;
    recoveryRequired_ = true;
    lastError_ = error;
    application_.endSession();
    wipeWorkspaces();
}

void BleLocalProvisioningEndpoint::clearPending() {
    secureZero(responseStorage_);
    pendingResponseSize_ = 0U;
    pendingKind_ = BleNoisePayloadKind::Transport;
    pendingResponse_ = false;
}

void BleLocalProvisioningEndpoint::wipeWorkspaces() {
    secureZero(operationWorkspace_);
    clearPending();
}

bool BleLocalProvisioningEndpoint::validConfiguration() const {
    return operationWorkspace_.data != nullptr &&
           !operationWorkspace_.empty() && responseStorage_.data != nullptr &&
           !responseStorage_.empty() &&
           !spansOverlap(operationWorkspace_, responseStorage_);
}

} // namespace blinker
