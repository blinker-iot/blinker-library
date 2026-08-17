#include "LocalControllerPairingApplication.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool overlaps(ByteView left, MutableByteSpan right) {
    if (left.data == nullptr || left.empty() || right.data == nullptr ||
        right.empty()) {
        return false;
    }
    const uintptr_t leftBegin = reinterpret_cast<uintptr_t>(left.data);
    const uintptr_t rightBegin = reinterpret_cast<uintptr_t>(right.data);
    if (left.size > UINTPTR_MAX - leftBegin ||
        right.size > UINTPTR_MAX - rightBegin) {
        return true;
    }
    return leftBegin < rightBegin + right.size &&
           rightBegin < leftBegin + left.size;
}

bool overlaps(MutableByteSpan left, MutableByteSpan right) {
    return overlaps(ByteView(left.data, left.size), right);
}

} // namespace

LocalControllerPairingApplication::LocalControllerPairingApplication(
    const DeviceInstanceId& deviceInstanceId,
    LocalControllerPairingCoordinator& coordinator)
    : deviceInstanceId_(deviceInstanceId), coordinator_(coordinator),
      setupSessionLocator_(), setupTranscriptHash_(),
      state_(LocalControllerPairingApplicationState::Idle) {}

LocalControllerPairingApplication::~LocalControllerPairingApplication() {
    endSession();
}

Result LocalControllerPairingApplication::beginSession(
    ByteView setupSessionLocator) {
    clearSession();
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !exactNonZero(setupSessionLocator, kSetupSessionLocatorSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    memcpy(setupSessionLocator_,
           setupSessionLocator.data,
           sizeof(setupSessionLocator_));
    state_ = LocalControllerPairingApplicationState::AwaitingHello;
    return Result::success();
}

Result LocalControllerPairingApplication::secureSessionReady(
    ByteView setupTranscriptHash) {
    if (state_ !=
            LocalControllerPairingApplicationState::AwaitingSecureSession ||
        !exactNonZero(
            setupTranscriptHash,
            sizeof(setupTranscriptHash_))) {
        return Result::failure(
            state_ != LocalControllerPairingApplicationState::
                          AwaitingSecureSession
                ? ErrorCode::StateConflict
                : ErrorCode::InvalidArgument);
    }
    memcpy(setupTranscriptHash_,
           setupTranscriptHash.data,
           sizeof(setupTranscriptHash_));
    state_ = LocalControllerPairingApplicationState::Ready;
    return Result::success();
}

void LocalControllerPairingApplication::endSession() { clearSession(); }

Result LocalControllerPairingApplication::confirmPhysicalPresence() {
    if (state_ != LocalControllerPairingApplicationState::Ready) {
        return Result::failure(ErrorCode::NotConnected);
    }
    return coordinator_.beginConfirmedPairing(ByteView(
        setupTranscriptHash_, sizeof(setupTranscriptHash_)));
}

Result LocalControllerPairingApplication::encodeError(
    uint32_t requestId,
    ErrorCode error,
    MutableByteSpan output,
    ByteView& response) const {
    LocalControllerPairingErrorResponse value;
    value.requestId = requestId;
    localControllerPairingErrorFor(
        error, value.error, value.retryClass);
    return encodeLocalControllerPairingErrorResponse(
        value, output, response);
}

Result LocalControllerPairingApplication::handle(
    ByteView encodedRequest,
    MutableByteSpan operationWorkspace,
    MutableByteSpan output,
    ByteView& response) {
    response = ByteView();
    if (state_ == LocalControllerPairingApplicationState::Idle ||
        encodedRequest.data == nullptr || encodedRequest.empty() ||
        operationWorkspace.data == nullptr ||
        operationWorkspace.size <
            kLocalControllerPairingApplicationWorkspaceSize ||
        output.data == nullptr ||
        output.size <
            kLocalControllerPairingApplicationResponseMaxEncodedSize ||
        overlaps(encodedRequest, operationWorkspace) ||
        overlaps(encodedRequest, output) ||
        overlaps(operationWorkspace, output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    LocalControllerPairingApplicationRequest request;
    Result result = decodeLocalControllerPairingApplicationRequest(
        encodedRequest, request);
    if (!result) {
        secureZero(operationWorkspace);
        return encodeError(request.requestId, result.code(), output, response);
    }

    if (request.type == LocalControllerPairingMessageType::HelloRequest) {
        if (state_ !=
            LocalControllerPairingApplicationState::AwaitingHello) {
            result = Result::failure(ErrorCode::StateConflict);
        } else {
            LocalControllerPairingHelloResponse hello;
            hello.requestId = request.requestId;
            hello.deviceInstanceId = ByteView(
                deviceInstanceId_.bytes,
                sizeof(deviceInstanceId_.bytes));
            hello.setupSessionLocator = ByteView(
                setupSessionLocator_,
                sizeof(setupSessionLocator_));
            result = encodeLocalControllerPairingHelloResponse(
                hello, output, response);
            if (result) {
                state_ = LocalControllerPairingApplicationState::
                    AwaitingSecureSession;
            }
        }
    } else if (state_ != LocalControllerPairingApplicationState::Ready) {
        result = Result::failure(ErrorCode::StateConflict);
    } else {
        ByteView body;
        if (request.type ==
            LocalControllerPairingMessageType::ChallengeRequest) {
            result = coordinator_.challenge(operationWorkspace, body);
        } else if (request.type ==
                   LocalControllerPairingMessageType::PairingRequest) {
            result = coordinator_.apply(
                request.body, operationWorkspace, body);
        } else {
            result = Result::failure(ErrorCode::ProtocolError);
        }
        if (result) {
            LocalControllerPairingApplicationResponse value;
            value.type = request.type ==
                                 LocalControllerPairingMessageType::
                                     ChallengeRequest
                             ? LocalControllerPairingMessageType::
                                   ChallengeResponse
                             : LocalControllerPairingMessageType::
                                   PairingReceipt;
            value.requestId = request.requestId;
            value.body = body;
            result = encodeLocalControllerPairingApplicationResponse(
                value, output, response);
        }
    }

    secureZero(operationWorkspace);
    if (!result) {
        return encodeError(
            request.requestId, result.code(), output, response);
    }
    return Result::success();
}

void LocalControllerPairingApplication::clearSession() {
    coordinator_.endPairing();
    secureZero(MutableByteSpan(
        setupSessionLocator_, sizeof(setupSessionLocator_)));
    secureZero(MutableByteSpan(
        setupTranscriptHash_, sizeof(setupTranscriptHash_)));
    state_ = LocalControllerPairingApplicationState::Idle;
}

} // namespace blinker
