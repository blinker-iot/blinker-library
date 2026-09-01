#ifndef BLINKER_SECURITY_CONTROLLERHMACSHA256INITIATOR_H
#define BLINKER_SECURITY_CONTROLLERHMACSHA256INITIATOR_H

#include "../identity/ControllerCredential.h"
#include "../interface/IRandom.h"
#include "../protocol/bbp2/Messages.h"
#include "ControllerHmacSha256Authorizer.h"
#include "DirectSecureSession.h"

namespace blinker {
namespace security {

enum class ControllerAuthInitiatorState : uint8_t {
    Idle = 0U,
    AwaitingChallenge = 1U,
    AwaitingAuthorization = 2U,
    Authorized = 3U
};

// Allocation-free Method 2 initiator shared by App-like controllers and Edge
// Hubs. Transport code owns framing, retries and deadlines; this class owns
// only one authentication transcript and its DirectSecure initiator session.
class ControllerHmacSha256Initiator {
public:
    explicit ControllerHmacSha256Initiator(IRandom& random);
    ~ControllerHmacSha256Initiator();
    ControllerHmacSha256Initiator(
        const ControllerHmacSha256Initiator&) = delete;
    ControllerHmacSha256Initiator& operator=(
        const ControllerHmacSha256Initiator&) = delete;

    Result begin(
        const ControllerCredential& credential,
        MutableByteSpan output,
        ByteView& requestPayload);
    Result handleChallenge(
        const bbp2::AuthResultBody& resultBody,
        MutableByteSpan output,
        ByteView& requestPayload);
    Result handleAuthorization(const bbp2::AuthResultBody& resultBody);
    void reset();

    ControllerAuthInitiatorState state() const { return state_; }
    bool authorized() const {
        return state_ == ControllerAuthInitiatorState::Authorized &&
               secureSession_.ready();
    }
    DirectSecureSession& secureSession() { return secureSession_; }
    const DirectSecureSession& secureSession() const {
        return secureSession_;
    }

private:
    Result fail(ErrorCode error);
    void clearHandshake();

    IRandom& random_;
    ControllerCredential credential_;
    uint8_t clientNonce_[kControllerAuthNonceSize];
    uint8_t deviceNonce_[kControllerAuthNonceSize];
    DirectSecureSession secureSession_;
    ControllerAuthInitiatorState state_;
};

} // namespace security
} // namespace blinker

#endif
