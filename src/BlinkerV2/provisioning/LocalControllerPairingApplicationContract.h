#ifndef BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGAPPLICATIONCONTRACT_H
#define BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGAPPLICATIONCONTRACT_H

#include "LocalControllerPairingContract.h"

namespace blinker {

static const uint8_t kLocalControllerPairingApplicationVersion = 1U;

enum : size_t {
    kLocalControllerPairingApplicationRequestMaxEncodedSize = 148U,
    kLocalControllerPairingApplicationResponseMaxEncodedSize = 188U,
    kLocalControllerPairingApplicationWorkspaceSize =
        kLocalControllerPairingReceiptMaxEncodedSize
};

enum class LocalControllerPairingMessageType : uint8_t {
    HelloRequest = 1U,
    ChallengeRequest = 2U,
    PairingRequest = 3U,
    HelloResponse = 129U,
    ChallengeResponse = 130U,
    PairingReceipt = 131U,
    ErrorResponse = 255U
};

enum class LocalControllerPairingWireError : uint16_t {
    Malformed = 1200U,
    UnsupportedVersion = 1201U,
    AuthenticationRequired = 1202U,
    StateConflict = 1203U,
    SequenceConflict = 1204U,
    Busy = 1205U,
    Internal = 1299U
};

enum class LocalControllerPairingRetryClass : uint8_t {
    Never = 0U,
    Backoff = 1U,
    PhysicalConfirm = 2U,
    RestartSession = 3U
};

struct LocalControllerPairingApplicationRequest {
    LocalControllerPairingMessageType type;
    uint32_t requestId;
    ByteView body;

    LocalControllerPairingApplicationRequest()
        : type(LocalControllerPairingMessageType::HelloRequest),
          requestId(0U) {}
};

struct LocalControllerPairingHelloResponse {
    uint32_t requestId;
    ByteView deviceInstanceId;
    ByteView setupSessionLocator;

    LocalControllerPairingHelloResponse() : requestId(0U) {}
};

struct LocalControllerPairingApplicationResponse {
    LocalControllerPairingMessageType type;
    uint32_t requestId;
    ByteView body;

    LocalControllerPairingApplicationResponse()
        : type(LocalControllerPairingMessageType::ChallengeResponse),
          requestId(0U) {}
};

struct LocalControllerPairingErrorResponse {
    uint32_t requestId;
    LocalControllerPairingWireError error;
    LocalControllerPairingRetryClass retryClass;

    LocalControllerPairingErrorResponse()
        : requestId(0U), error(LocalControllerPairingWireError::Internal),
          retryClass(LocalControllerPairingRetryClass::Never) {}
};

Result decodeLocalControllerPairingApplicationRequest(
    ByteView encoded,
    LocalControllerPairingApplicationRequest& request);
Result encodeLocalControllerPairingHelloResponse(
    const LocalControllerPairingHelloResponse& response,
    MutableByteSpan output,
    ByteView& encoded);
Result encodeLocalControllerPairingApplicationResponse(
    const LocalControllerPairingApplicationResponse& response,
    MutableByteSpan output,
    ByteView& encoded);
Result encodeLocalControllerPairingErrorResponse(
    const LocalControllerPairingErrorResponse& response,
    MutableByteSpan output,
    ByteView& encoded);
void localControllerPairingErrorFor(
    ErrorCode error,
    LocalControllerPairingWireError& wireError,
    LocalControllerPairingRetryClass& retryClass);

} // namespace blinker

#endif
