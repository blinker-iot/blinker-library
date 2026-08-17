#ifndef BLINKER_PROVISIONING_LOCALSETUPAPPLICATION_H
#define BLINKER_PROVISIONING_LOCALSETUPAPPLICATION_H

#include "../identity/DeviceInstanceId.h"
#include "../interface/IClock.h"
#include "../interface/ILocalProvisioningApplication.h"
#include "../interface/ILocalSetupOperationHandler.h"
#include "../interface/IRandom.h"
#include "EnrollmentTicketVerifier.h"

namespace blinker {

enum class LocalSetupApplicationState : uint8_t {
    Idle = 0U,
    AwaitingHello,
    AwaitingSecureSession,
    AwaitingTicket,
    Authorized
};

// Allocation-free application protocol above one Noise provisioning channel.
// It owns the full random setupSessionId, request sequencing and the verified
// Ticket metadata. Durable ownership/controller/WiFi mutations stay behind
// ILocalSetupOperationHandler so a product can provide one atomic saga.
class LocalSetupApplication : public ILocalProvisioningApplication {
public:
    LocalSetupApplication(const DeviceInstanceId& deviceInstanceId,
                          IRandom& random,
                          IClock& clock,
                          EnrollmentTicketVerifier& ticketVerifier,
                          ILocalSetupOperationHandler& operationHandler,
                          uint32_t ticketOperations);
    ~LocalSetupApplication();
    LocalSetupApplication(const LocalSetupApplication&) = delete;
    LocalSetupApplication& operator=(const LocalSetupApplication&) = delete;

    Result beginSession(ByteView setupSessionLocator) override;
    Result secureSessionReady(ByteView setupTranscriptHash) override;
    void endSession() override;
    Result handle(ByteView request,
                  MutableByteSpan operationWorkspace,
                  MutableByteSpan output,
                  ByteView& response) override;

    LocalSetupApplicationState state() const { return state_; }
    ByteView setupSessionId() const {
        return windowValid_ ? ByteView(setupSessionId_, sizeof(setupSessionId_))
                            : ByteView();
    }
    uint32_t ownershipGeneration() const { return ownershipGeneration_; }

private:
    enum class SequenceDisposition : uint8_t {
        NewRequest = 0U,
        ExactReplay,
        Gap,
        Conflict
    };

    Result handleHello(const LocalSetupRequest& request,
                       MutableByteSpan output,
                       ByteView& response);
    Result handleTicket(const LocalSetupRequest& request,
                        MutableByteSpan ticketWorkspace,
                        MutableByteSpan output,
                        ByteView& response);
    Result handleOperation(const LocalSetupRequest& request,
                           ByteView requestDigest,
                           bool exactReplay,
                           MutableByteSpan operationWorkspace,
                           MutableByteSpan output,
                           ByteView& response);
    Result encodeError(uint32_t requestId,
                       ErrorCode error,
                       MutableByteSpan output,
                       ByteView& response) const;
    SequenceDisposition classifySequence(uint32_t requestId,
                                         ByteView requestDigest) const;
    void rememberRequest(uint32_t requestId, ByteView requestDigest);
    LocalSetupAuthorization authorization() const;
    bool ticketExpired() const;
    void clearConnection();
    void clearAuthorization();
    void clearWindow();

    const DeviceInstanceId& deviceInstanceId_;
    IRandom& random_;
    IClock& clock_;
    EnrollmentTicketVerifier& ticketVerifier_;
    ILocalSetupOperationHandler& operationHandler_;
    uint32_t ticketOperations_;
    uint32_t ownershipGeneration_;
    uint32_t lastRequestId_;
    uint32_t controllerPermissions_;
    uint32_t authorizedOperations_;
    uint64_t ticketExpiresAt_;
    uint8_t setupSessionLocator_[kSetupSessionLocatorSize];
    uint8_t setupSessionId_[kLocalSetupSessionIdSize];
    uint8_t setupTranscriptHash_[kLocalSetupTranscriptHashSize];
    uint8_t ticketId_[kEnrollmentTicketIdSize];
    uint8_t controllerId_[kControllerIdSize];
    uint8_t
        controllerSecretDigest_[kEnrollmentTicketControllerSecretDigestSize];
    uint8_t lastRequestDigest_[kSha256Size];
    LocalSetupApplicationState state_;
    bool windowValid_;
    bool handlerSessionActive_;
};

} // namespace blinker

#endif
