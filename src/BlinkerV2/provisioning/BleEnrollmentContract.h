#ifndef BLINKER_PROVISIONING_BLEENROLLMENTCONTRACT_H
#define BLINKER_PROVISIONING_BLEENROLLMENTCONTRACT_H

#include "BleEnrollmentGrant.h"
#include "ControllerControlContract.h"
#include "../protocol/SetupSession.h"

namespace blinker {

static const uint8_t kBleEnrollmentContractVersion = 1U;

struct BleEnrollmentApplicationConfig {
    BleEnrollmentSecurityProfile securityProfile;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;

    BleEnrollmentApplicationConfig();
};

enum : size_t {
    kBleEnrollmentRequestMaxEncodedSize = 320U,
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

    BleEnrollmentRequest()
        : type(BleEnrollmentMessageType::HelloRequest), requestId(0U) {}
};

struct BleEnrollmentResponse {
    BleEnrollmentMessageType type;
    uint32_t requestId;
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupSessionLocator;
    uint32_t accessEpoch;
    BleEnrollmentSecurityProfile securityProfile;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView receipt;
    BleEnrollmentWireError error;
    BleEnrollmentRetryClass retryClass;

    BleEnrollmentResponse()
        : type(BleEnrollmentMessageType::ErrorResponse), requestId(0U),
          accessEpoch(0U),
          securityProfile(BleEnrollmentSecurityProfile::EducationNearby),
          serverKeyId(0U),
          signatureAlgorithm(ServerSignatureAlgorithm::Ed25519),
          error(BleEnrollmentWireError::Internal),
          retryClass(BleEnrollmentRetryClass::Never) {}
};

Result encodeBleEnrollmentHelloRequest(uint32_t requestId,
                                       MutableByteSpan output,
                                       ByteView& encoded);
Result encodeBleEnrollmentRequest(uint32_t requestId,
                                  ByteView grant,
                                  ByteView controllerSecret,
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
