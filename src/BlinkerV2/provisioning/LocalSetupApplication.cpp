#include "LocalSetupApplication.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace {

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected)
        return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool overlap(ByteView left, MutableByteSpan right) {
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
    return leftBegin < rightBegin + right.size &&
           rightBegin < leftBegin + left.size;
}

bool overlap(MutableByteSpan left, MutableByteSpan right) {
    return overlap(ByteView(left.data, left.size), right);
}

bool containsOrEmpty(MutableByteSpan storage, ByteView value) {
    if (value.empty())
        return true;
    if (storage.data == nullptr || value.data == nullptr)
        return false;
    const uintptr_t storageBegin =
        reinterpret_cast<uintptr_t>(storage.data);
    const uintptr_t valueBegin = reinterpret_cast<uintptr_t>(value.data);
    if (storage.size > UINTPTR_MAX - storageBegin ||
        value.size > UINTPTR_MAX - valueBegin) {
        return false;
    }
    return valueBegin >= storageBegin &&
           valueBegin + value.size <= storageBegin + storage.size;
}

} // namespace

LocalSetupApplication::LocalSetupApplication(
    const DeviceInstanceId& deviceInstanceId,
    IRandom& random,
    IClock& clock,
    EnrollmentTicketVerifier& ticketVerifier,
    ILocalSetupOperationHandler& operationHandler,
    uint32_t ticketOperations)
    : deviceInstanceId_(deviceInstanceId), random_(random), clock_(clock),
      ticketVerifier_(ticketVerifier),
      operationHandler_(operationHandler), ticketOperations_(ticketOperations),
      ownershipGeneration_(0U), lastRequestId_(0U), controllerPermissions_(0U),
      authorizedOperations_(0U), ticketExpiresAt_(0U), setupSessionLocator_(),
      setupSessionId_(), setupTranscriptHash_(), ticketId_(), controllerId_(),
      controllerSecretDigest_(), lastRequestDigest_(),
      state_(LocalSetupApplicationState::Idle), windowValid_(false),
      handlerSessionActive_(false) {}

LocalSetupApplication::~LocalSetupApplication() {
    clearWindow();
}

Result LocalSetupApplication::beginSession(ByteView setupSessionLocator) {
    clearConnection();
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !exactNonZero(setupSessionLocator, kSetupSessionLocatorSize) ||
        !validLocalSetupAuthorizedOperations(ticketOperations_)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint32_t generation = 0U;
    Result result = operationHandler_.prepareSetupSession(
        setupSessionLocator,
        generation);
    if (result && generation == 0U) {
        result = Result::failure(ErrorCode::InternalError);
    }
    if (!result) {
        if (result.code() == ErrorCode::StateConflict)
            clearWindow();
        return result;
    }

    const bool sameWindow =
        windowValid_ &&
        constantTimeEqual(
            setupSessionLocator,
            ByteView(setupSessionLocator_, sizeof(setupSessionLocator_)));
    if (!sameWindow) {
        clearWindow();
        result = random_.fill(
            MutableByteSpan(setupSessionId_, sizeof(setupSessionId_)));
        if (!result ||
            !exactNonZero(ByteView(setupSessionId_, sizeof(setupSessionId_)),
                          sizeof(setupSessionId_))) {
            clearWindow();
            return result ? Result::failure(ErrorCode::InternalError) : result;
        }
        memcpy(setupSessionLocator_,
               setupSessionLocator.data,
               sizeof(setupSessionLocator_));
        windowValid_ = true;
    }
    ownershipGeneration_ = generation;
    state_ = LocalSetupApplicationState::AwaitingHello;
    return Result::success();
}

Result LocalSetupApplication::secureSessionReady(ByteView setupTranscriptHash) {
    if (state_ != LocalSetupApplicationState::AwaitingSecureSession) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!exactNonZero(setupTranscriptHash, sizeof(setupTranscriptHash_))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    memcpy(setupTranscriptHash_,
           setupTranscriptHash.data,
           sizeof(setupTranscriptHash_));
    state_ = LocalSetupApplicationState::AwaitingTicket;
    return Result::success();
}

void LocalSetupApplication::endSession() { clearConnection(); }

LocalSetupApplication::SequenceDisposition
LocalSetupApplication::classifySequence(uint32_t requestId,
                                        ByteView requestDigest) const {
    if (lastRequestId_ == 0U ||
        (lastRequestId_ != UINT32_MAX && requestId == lastRequestId_ + 1U)) {
        return SequenceDisposition::NewRequest;
    }
    if (requestId == lastRequestId_) {
        return constantTimeEqual(
                   requestDigest,
                   ByteView(lastRequestDigest_, sizeof(lastRequestDigest_)))
                   ? SequenceDisposition::ExactReplay
                   : SequenceDisposition::Conflict;
    }
    return SequenceDisposition::Gap;
}

void LocalSetupApplication::rememberRequest(uint32_t requestId,
                                            ByteView requestDigest) {
    lastRequestId_ = requestId;
    memcpy(lastRequestDigest_, requestDigest.data, sizeof(lastRequestDigest_));
}

Result LocalSetupApplication::encodeError(uint32_t requestId,
                                          ErrorCode error,
                                          MutableByteSpan output,
                                          ByteView& response) const {
    LocalSetupErrorResponse body;
    body.requestId = requestId;
    localSetupErrorFor(error, body.error, body.retryClass);
    return encodeLocalSetupErrorResponse(body, output, response);
}

Result LocalSetupApplication::handleHello(const LocalSetupRequest& request,
                                          MutableByteSpan output,
                                          ByteView& response) {
    LocalSetupHelloResponse hello;
    hello.requestId = request.requestId;
    hello.deviceInstanceId =
        ByteView(deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
    hello.setupSessionId = ByteView(setupSessionId_, sizeof(setupSessionId_));
    hello.setupSessionLocator =
        ByteView(setupSessionLocator_, sizeof(setupSessionLocator_));
    hello.ownershipGeneration = ownershipGeneration_;
    hello.ticketOperations = ticketOperations_;
    Result result = encodeLocalSetupHelloResponse(hello, output, response);
    if (result && state_ == LocalSetupApplicationState::AwaitingHello) {
        state_ = LocalSetupApplicationState::AwaitingSecureSession;
    }
    return result;
}

Result LocalSetupApplication::handleTicket(const LocalSetupRequest& request,
                                           MutableByteSpan ticketWorkspace,
                                           MutableByteSpan output,
                                           ByteView& response) {
    EnrollmentTicket candidate;
    Result result = decodeEnrollmentTicket(request.body, candidate);
    if (result &&
        (!validLocalSetupAuthorizedOperations(candidate.authorizedOperations) ||
         candidate.authorizedOperations != ticketOperations_)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    uint64_t now = 0U;
    const bool hasTrustedTime = clock_.unixTime(now);
    EnrollmentTicket verified;
    if (result) {
        EnrollmentTicketVerificationContext context;
        context.deviceInstanceId =
            ByteView(deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
        context.setupSessionId =
            ByteView(setupSessionId_, sizeof(setupSessionId_));
        context.setupTranscriptHash =
            ByteView(setupTranscriptHash_, sizeof(setupTranscriptHash_));
        context.ownershipGeneration = ownershipGeneration_;
        context.controllerId = candidate.controllerId;
        context.controllerSecretDigest = candidate.controllerSecretDigest;
        context.controllerPermissions = candidate.controllerPermissions;
        context.authorizedOperations = ticketOperations_;
        context.nowEpochSeconds = now;
        context.hasTrustedTime = hasTrustedTime;
        result = ticketVerifier_.verify(
            request.body, context, ticketWorkspace, verified);
    }
    if (!result)
        return result;

    memcpy(ticketId_, verified.ticketId.data, sizeof(ticketId_));
    memcpy(controllerId_, verified.controllerId.data, sizeof(controllerId_));
    memcpy(controllerSecretDigest_,
           verified.controllerSecretDigest.data,
           sizeof(controllerSecretDigest_));
    controllerPermissions_ = verified.controllerPermissions;
    authorizedOperations_ = verified.authorizedOperations;
    ticketExpiresAt_ = verified.expiresAt;

    result = operationHandler_.beginAuthorizedSession(authorization());
    if (!result) {
        operationHandler_.endAuthorizedSession();
        clearAuthorization();
        if (result.code() == ErrorCode::AuthenticationRequired)
            return result;
        return encodeError(request.requestId, result.code(), output, response);
    }
    handlerSessionActive_ = true;
    state_ = LocalSetupApplicationState::Authorized;

    LocalSetupTicketAccepted accepted;
    accepted.requestId = request.requestId;
    accepted.ticketId = ByteView(ticketId_, sizeof(ticketId_));
    accepted.authorizedOperations = authorizedOperations_;
    return encodeLocalSetupTicketAccepted(accepted, output, response);
}

LocalSetupAuthorization LocalSetupApplication::authorization() const {
    LocalSetupAuthorization value;
    value.ticketId = ByteView(ticketId_, sizeof(ticketId_));
    value.controllerId = ByteView(controllerId_, sizeof(controllerId_));
    value.controllerSecretDigest =
        ByteView(controllerSecretDigest_, sizeof(controllerSecretDigest_));
    value.ownershipGeneration = ownershipGeneration_;
    value.controllerPermissions = controllerPermissions_;
    value.authorizedOperations = authorizedOperations_;
    return value;
}

bool LocalSetupApplication::ticketExpired() const {
    uint64_t now = 0U;
    return clock_.unixTime(now) && (now == 0U || now >= ticketExpiresAt_);
}

Result
LocalSetupApplication::handleOperation(const LocalSetupRequest& request,
                                       ByteView requestDigest,
                                       bool exactReplay,
                                       MutableByteSpan operationWorkspace,
                                       MutableByteSpan output,
                                       ByteView& response) {
    if (ticketExpired()) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!localSetupOperationAllowed(request.operation, authorizedOperations_)) {
        return encodeError(request.requestId,
                           ErrorCode::UnsupportedFeature,
                           output,
                           response);
    }

    MutableByteSpan operationScratch(operationWorkspace.data,
                                     kLocalSetupOperationScratchSize);
    MutableByteSpan responseBodyStorage(
        operationWorkspace.data + operationScratch.size,
        kLocalSetupOperationResponseMaxEncodedSize);
    LocalSetupOperationContext context;
    context.authorization = authorization();
    context.requestDigest = requestDigest;
    context.requestId = request.requestId;
    context.exactReplay = exactReplay;
    ByteView responseBody;
    Result result =
        operationHandler_.handleAuthorizedOperation(context,
                                                    request.operation,
                                                    request.body,
                                                    operationScratch,
                                                    responseBodyStorage,
                                                    responseBody);
    if (!result) {
        if (result.code() == ErrorCode::AuthenticationRequired) {
            return result;
        }
        return encodeError(request.requestId, result.code(), output, response);
    }
    if (!containsOrEmpty(responseBodyStorage, responseBody)) {
        return Result::failure(ErrorCode::InternalError);
    }

    LocalSetupOperationResponse body;
    body.requestId = request.requestId;
    body.operation = request.operation;
    body.body = responseBody;
    return encodeLocalSetupOperationResponse(body, output, response);
}

Result LocalSetupApplication::handle(ByteView encodedRequest,
                                     MutableByteSpan operationWorkspace,
                                     MutableByteSpan output,
                                     ByteView& response) {
    response = ByteView();
    if (state_ == LocalSetupApplicationState::Idle ||
        encodedRequest.data == nullptr || encodedRequest.empty() ||
        operationWorkspace.data == nullptr ||
        operationWorkspace.size < kLocalSetupOperationWorkspaceSize ||
        output.data == nullptr ||
        output.size < kLocalSetupResponseMaxEncodedSize ||
        overlap(encodedRequest, operationWorkspace) ||
        overlap(encodedRequest, output) ||
        overlap(operationWorkspace, output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    LocalSetupRequest request;
    Result result = decodeLocalSetupRequest(encodedRequest, request);
    uint8_t requestDigest[kSha256Size] = {};
    if (result) {
        result = sha256(encodedRequest,
                        MutableByteSpan(requestDigest, sizeof(requestDigest)));
    }
    if (!result) {
        secureZero(operationWorkspace);
        secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
        return result;
    }

    const ByteView digest(requestDigest, sizeof(requestDigest));
    const SequenceDisposition sequence =
        classifySequence(request.requestId, digest);
    if (sequence == SequenceDisposition::Conflict) {
        secureZero(operationWorkspace);
        secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (sequence == SequenceDisposition::Gap) {
        result = encodeError(
            request.requestId, ErrorCode::SequenceConflict, output, response);
        secureZero(operationWorkspace);
        secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
        return result;
    }
    const bool replay = sequence == SequenceDisposition::ExactReplay;

    bool consumed = false;
    if (request.type == LocalSetupMessageType::HelloRequest &&
        ((!replay && state_ == LocalSetupApplicationState::AwaitingHello) ||
         (replay && state_ != LocalSetupApplicationState::Idle))) {
        result = handleHello(request, output, response);
        consumed = true;
    } else if (request.type == LocalSetupMessageType::TicketOffer &&
               ((!replay &&
                 state_ == LocalSetupApplicationState::AwaitingTicket) ||
                (replay && state_ == LocalSetupApplicationState::Authorized))) {
        if (replay) {
            if (ticketExpired()) {
                secureZero(operationWorkspace);
                secureZero(
                    MutableByteSpan(requestDigest, sizeof(requestDigest)));
                return Result::failure(ErrorCode::AuthenticationRequired);
            }
            LocalSetupTicketAccepted accepted;
            accepted.requestId = request.requestId;
            accepted.ticketId = ByteView(ticketId_, sizeof(ticketId_));
            accepted.authorizedOperations = authorizedOperations_;
            result = encodeLocalSetupTicketAccepted(accepted, output, response);
        } else {
            result =
                handleTicket(request,
                             MutableByteSpan(operationWorkspace.data,
                                             kEnrollmentTicketWorkspaceSize),
                             output,
                             response);
        }
        consumed = replay ||
                   state_ == LocalSetupApplicationState::Authorized;
    } else if (request.type == LocalSetupMessageType::OperationRequest &&
               state_ == LocalSetupApplicationState::Authorized) {
        result = handleOperation(
            request, digest, replay, operationWorkspace, output, response);
        consumed = true;
    } else {
        result = encodeError(
            request.requestId, ErrorCode::StateConflict, output, response);
    }

    if (result && !replay && consumed) {
        rememberRequest(request.requestId, digest);
    }
    secureZero(operationWorkspace);
    secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
    return result;
}

void LocalSetupApplication::clearAuthorization() {
    if (handlerSessionActive_) {
        operationHandler_.endAuthorizedSession();
    }
    handlerSessionActive_ = false;
    controllerPermissions_ = 0U;
    authorizedOperations_ = 0U;
    ticketExpiresAt_ = 0U;
    secureZero(MutableByteSpan(ticketId_, sizeof(ticketId_)));
    secureZero(MutableByteSpan(controllerId_, sizeof(controllerId_)));
    secureZero(MutableByteSpan(controllerSecretDigest_,
                               sizeof(controllerSecretDigest_)));
}

void LocalSetupApplication::clearConnection() {
    clearAuthorization();
    ownershipGeneration_ = 0U;
    lastRequestId_ = 0U;
    secureZero(
        MutableByteSpan(setupTranscriptHash_, sizeof(setupTranscriptHash_)));
    secureZero(MutableByteSpan(lastRequestDigest_, sizeof(lastRequestDigest_)));
    state_ = LocalSetupApplicationState::Idle;
}

void LocalSetupApplication::clearWindow() {
    clearConnection();
    secureZero(
        MutableByteSpan(setupSessionLocator_, sizeof(setupSessionLocator_)));
    secureZero(MutableByteSpan(setupSessionId_, sizeof(setupSessionId_)));
    windowValid_ = false;
}

} // namespace blinker
