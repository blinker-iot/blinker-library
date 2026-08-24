#ifndef BLINKER_PROVISIONING_BLEENROLLMENTAPPLICATION_H
#define BLINKER_PROVISIONING_BLEENROLLMENTAPPLICATION_H

#include "BleEnrollmentContract.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IClock.h"
#include "../interface/IDeviceAccessStore.h"
#include "../interface/ILocalProvisioningApplication.h"
#include "../interface/IRandom.h"

namespace blinker {

enum class BleEnrollmentApplicationState : uint8_t {
    Idle = 0U,
    AwaitingSecureSession,
    AwaitingHello,
    AwaitingEnrollment,
    Enrolled
};

// Minimal BLE-only onboarding application. A server-signed grant and the raw
// controller secret arrive together inside Noise; one atomic store write is
// the entire durable transaction.
class BleEnrollmentApplication final : public ILocalProvisioningApplication {
public:
    BleEnrollmentApplication(const DeviceInstanceId& deviceInstanceId,
                             IRandom& random,
                             IClock& clock,
                             BleEnrollmentGrantVerifier& grantVerifier,
                             IDeviceAccessStore& accessStore,
                             const BleEnrollmentApplicationConfig& config);
    ~BleEnrollmentApplication();

    Result beginSession(ByteView setupSessionLocator) override;
    Result secureSessionReady(ByteView setupTranscriptHash) override;
    void endSession() override;
    Result handle(ByteView request,
                  MutableByteSpan workspace,
                  MutableByteSpan output,
                  ByteView& response) override;

    BleEnrollmentApplicationState state() const { return state_; }
    // Random non-zero generation bound to the current physical setup window.
    uint32_t accessEpoch() const { return accessEpoch_; }

private:
    enum class SequenceDisposition : uint8_t {
        NewRequest = 0U,
        ExactReplay,
        Gap,
        Conflict
    };

    Result handleHello(uint32_t requestId,
                       MutableByteSpan output,
                       ByteView& response);
    Result handleEnrollment(const BleEnrollmentRequest& request,
                            bool exactReplay,
                            MutableByteSpan workspace,
                            MutableByteSpan output,
                            ByteView& response);
    Result encodeError(uint32_t requestId,
                       ErrorCode error,
                       MutableByteSpan output,
                       ByteView& response) const;
    SequenceDisposition classify(uint32_t requestId,
                                 ByteView digest) const;
    void remember(uint32_t requestId, ByteView digest);
    void clearConnection();
    void clearWindow();

    const DeviceInstanceId& deviceInstanceId_;
    IRandom& random_;
    IClock& clock_;
    BleEnrollmentGrantVerifier& grantVerifier_;
    IDeviceAccessStore& accessStore_;
    BleEnrollmentApplicationConfig config_;
    uint32_t accessEpoch_;
    uint32_t lastRequestId_;
    uint8_t setupSessionLocator_[kSetupSessionLocatorSize];
    uint8_t setupSessionId_[kSetupSessionIdSize];
    uint8_t setupTranscriptHash_[kSha256Size];
    uint8_t lastRequestDigest_[kSha256Size];
    BleEnrollmentApplicationState state_;
    bool windowValid_;

    BleEnrollmentApplication(const BleEnrollmentApplication&);
    BleEnrollmentApplication& operator=(const BleEnrollmentApplication&);
};

} // namespace blinker

#endif
