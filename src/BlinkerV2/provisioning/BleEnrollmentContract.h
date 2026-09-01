#ifndef BLINKER_PROVISIONING_BLEENROLLMENTCONTRACT_H
#define BLINKER_PROVISIONING_BLEENROLLMENTCONTRACT_H

#include "BleEnrollmentGrant.h"
#include "ControllerControlContract.h"
#include "../protocol/SetupSession.h"

namespace blinker {

static const uint8_t kBleEnrollmentContractVersion = 2U;

struct BleEnrollmentApplicationConfig {
    BleEnrollmentSecurityProfile securityProfile;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;

    BleEnrollmentApplicationConfig();
};

enum : size_t {
    kBleEnrollmentRequestMaxEncodedSize = 384U,
    kBleEnrollmentResponseMaxEncodedSize = 160U,
    kBleEnrollmentWorkspaceSize = kControllerControlWorkspaceSize
};

enum class BleEnrollmentMessageType : uint8_t {
    HelloRequest = 1U,
    HelloResponse = 2U,
    EnrollRequest = 3U,
    EnrollResponse = 4U,
    ErrorResponse = 5U
};

enum class BleEnrollmentWireError : uint16_t {
    Malformed = 1000U,
    UnsupportedVersion = 1001U,
    AuthenticationRequired = 1004U,
    StateConflict = 1005U,
    SequenceConflict = 1006U,
    Internal = 1099U
};

enum class BleEnrollmentRetryClass : uint8_t {
    Never = 0U,
    CorrectSequence = 1U,
    RestartSession = 2U
};

struct BleEnrollmentRequest {
    BleEnrollmentMessageType type;
    uint32_t requestId;
    ByteView grant;
    ByteView controllerSecret;
    ByteView presenceKey;

    BleEnrollmentRequest()
        : type(BleEnrollmentMessageType::HelloRequest), requestId(0U) {}
};

struct BleEnrollmentResponse {
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupSessionLocator;
    ByteView receipt;
    uint32_t requestId;
    uint32_t accessEpoch;
    uint32_t serverKeyId;
    BleEnrollmentWireError error;
    BleEnrollmentMessageType type;
    BleEnrollmentSecurityProfile securityProfile;
    ServerSignatureAlgorithm signatureAlgorithm;
    BleEnrollmentRetryClass retryClass;

    BleEnrollmentResponse()
        : requestId(0U), accessEpoch(0U), serverKeyId(0U),
          error(BleEnrollmentWireError::Internal),
          type(BleEnrollmentMessageType::ErrorResponse),
          securityProfile(BleEnrollmentSecurityProfile::EducationNearby),
          signatureAlgorithm(ServerSignatureAlgorithm::Ed25519),
          retryClass(BleEnrollmentRetryClass::Never) {}
};

Result encodeBleEnrollmentHelloRequest(uint32_t requestId,
                                       MutableByteSpan output,
                                       ByteView& encoded);
Result encodeBleEnrollmentRequest(uint32_t requestId,
                                  ByteView grant,
                                  ByteView controllerSecret,
                                  ByteView presenceKey,
                                  MutableByteSpan output,
                                  ByteView& encoded);
Result decodeBleEnrollmentRequest(ByteView encoded,
                                  BleEnrollmentRequest& request);

Result encodeBleEnrollmentHelloResponse(
    const BleEnrollmentResponse& response,
    MutableByteSpan output,
    ByteView& encoded);
Result encodeBleEnrollmentResponse(uint32_t requestId,
                                   ByteView receipt,
                                   MutableByteSpan output,
                                   ByteView& encoded);
Result encodeBleEnrollmentError(uint32_t requestId,
                                BleEnrollmentWireError error,
                                BleEnrollmentRetryClass retryClass,
                                MutableByteSpan output,
                                ByteView& encoded);
Result decodeBleEnrollmentResponse(ByteView encoded,
                                   BleEnrollmentResponse& response);

void bleEnrollmentErrorFor(ErrorCode error,
                           BleEnrollmentWireError& wireError,
                           BleEnrollmentRetryClass& retryClass);

} // namespace blinker

#endif
