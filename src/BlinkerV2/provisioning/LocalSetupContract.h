#ifndef BLINKER_PROVISIONING_LOCALSETUPCONTRACT_H
#define BLINKER_PROVISIONING_LOCALSETUPCONTRACT_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/SetupSession.h"

namespace blinker {

static const uint8_t kLocalSetupContractVersion = 1U;

enum : size_t {
    kLocalSetupSessionIdSize = kSetupSessionIdSize,
    kLocalSetupTranscriptHashSize = kSetupTranscriptHashSize,
    kLocalSetupOperationBodyMaxEncodedSize = 512U,
    kLocalSetupOperationScratchSize = 256U,
    kLocalSetupOperationResponseMaxEncodedSize = 256U,
    kLocalSetupOperationWorkspaceSize =
        kLocalSetupOperationScratchSize +
        kLocalSetupOperationResponseMaxEncodedSize,
    kLocalSetupRequestMaxEncodedSize = 529U,
    kLocalSetupResponseMaxEncodedSize = 273U
};

enum class LocalSetupMessageType : uint8_t {
    HelloRequest = 1U,
    HelloResponse = 2U,
    TicketOffer = 3U,
    TicketAccepted = 4U,
    OperationRequest = 5U,
    OperationResponse = 6U,
    ErrorResponse = 7U
};

enum class LocalSetupOperation : uint8_t {
    BeginOwnershipClaim = 1U,
    ApplyOwnershipGrant = 2U,
    CompleteOwnershipClaim = 3U,
    InstallInitialController = 4U,
    ConfigureNetwork = 5U,
    BeginCloudEnrollment = 6U,
    ApplyCloudEnrollmentGrant = 7U,
    CompleteCloudEnrollment = 8U
};

enum class LocalSetupWireError : uint16_t {
    Malformed = 1000U,
    UnsupportedVersion = 1001U,
    UnsupportedOperation = 1002U,
    SequenceConflict = 1003U,
    AuthenticationRequired = 1004U,
    StateConflict = 1005U,
    Busy = 1006U,
    Internal = 1099U
};

enum class LocalSetupRetryClass : uint8_t {
    Never = 0U,
    CorrectSequence = 1U,
    Backoff = 2U,
    RestartSession = 3U
};

struct LocalSetupRequest {
    LocalSetupMessageType type;
    uint32_t requestId;
    LocalSetupOperation operation;
    ByteView body;

    LocalSetupRequest()
        : type(LocalSetupMessageType::HelloRequest), requestId(0U),
          operation(LocalSetupOperation::BeginOwnershipClaim) {}
};

struct LocalSetupHelloResponse {
    uint32_t requestId;
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupSessionLocator;
    uint32_t ownershipGeneration;
    uint32_t ticketOperations;

    LocalSetupHelloResponse()
        : requestId(0U), ownershipGeneration(0U), ticketOperations(0U) {}
};

struct LocalSetupTicketAccepted {
    uint32_t requestId;
    ByteView ticketId;
    uint32_t authorizedOperations;

    LocalSetupTicketAccepted() : requestId(0U), authorizedOperations(0U) {}
};

struct LocalSetupOperationResponse {
    uint32_t requestId;
    LocalSetupOperation operation;
    ByteView body;

    LocalSetupOperationResponse()
        : requestId(0U), operation(LocalSetupOperation::BeginOwnershipClaim) {}
};

struct LocalSetupErrorResponse {
    uint32_t requestId;
    LocalSetupWireError error;
    LocalSetupRetryClass retryClass;

    LocalSetupErrorResponse()
        : requestId(0U), error(LocalSetupWireError::Internal),
          retryClass(LocalSetupRetryClass::Never) {}
};

Result decodeLocalSetupRequest(ByteView encoded, LocalSetupRequest& request);
Result encodeLocalSetupHelloResponse(const LocalSetupHelloResponse& response,
                                     MutableByteSpan output,
                                     ByteView& encoded);
Result encodeLocalSetupTicketAccepted(const LocalSetupTicketAccepted& response,
                                      MutableByteSpan output,
                                      ByteView& encoded);
Result
encodeLocalSetupOperationResponse(const LocalSetupOperationResponse& response,
                                  MutableByteSpan output,
                                  ByteView& encoded);
Result encodeLocalSetupErrorResponse(const LocalSetupErrorResponse& response,
                                     MutableByteSpan output,
                                     ByteView& encoded);

bool validLocalSetupOperation(LocalSetupOperation operation);
bool validLocalSetupAuthorizedOperations(uint32_t operations);
bool localSetupOperationAllowed(LocalSetupOperation operation,
                                uint32_t authorizedOperations);
void localSetupErrorFor(ErrorCode error,
                        LocalSetupWireError& wireError,
                        LocalSetupRetryClass& retryClass);

} // namespace blinker

#endif
