#ifndef BLINKER_SECURITY_CONTROLLERHMACSHA256AUTHORIZER_H
#define BLINKER_SECURITY_CONTROLLERHMACSHA256AUTHORIZER_H

#include "../core/HmacSha256.h"
#include "../interface/IAuthorizationProvider.h"
#include "../interface/IControllerCredentialSource.h"
#include "../interface/IRandom.h"
#include "DirectSecureSession.h"

namespace blinker {
namespace security {

static const uint16_t kControllerHmacSha256Method = 2U;
static const uint8_t kControllerAuthTranscriptVersion = 1U;
static const size_t kControllerAuthNonceSize = 16U;
static const size_t kControllerAuthProofSize = 32U;
static const size_t kControllerAuthInitPayloadSize =
    1U + kControllerIdSize + 1U + 4U + 4U +
    kControllerAuthNonceSize;
static const size_t kControllerAuthProofPayloadSize =
    1U + kControllerAuthProofSize;
static const size_t kControllerAuthChallengePayloadSize =
    1U + kControllerAuthNonceSize + 4U + kControllerAuthProofSize;

enum ControllerAuthPayloadType : uint8_t {
    ControllerAuthPayloadInit = 1U,
    ControllerAuthPayloadProof = 2U,
    ControllerAuthPayloadChallenge = 0x81U
};

enum class ControllerAuthProofRole : uint8_t {
    Device = 1U,
    App = 2U
};

enum class ControllerAuthTransportPolicy : uint8_t {
    RequireEncrypted = 0U,
    EstablishDirectSecure = 1U
};

struct ControllerAuthSession {
    uint32_t sessionId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    uint32_t permissions;
    uint8_t controllerId[kControllerIdSize];
    uint8_t clientNonce[kControllerAuthNonceSize];
    uint8_t deviceNonce[kControllerAuthNonceSize];
    uint8_t transportId;
    ControllerCredentialDomain domain;
    bool challengeActive;
    bool occupied;

    ControllerAuthSession();
    void clear();
};

// HMAC-SHA256 over the role domain followed by this exact binary transcript:
// transcriptVersion:u8, method:u16be, suite:u8, controllerId:16,
// domain:u8, ownershipGeneration:u32be, credentialVersion:u32be,
// permissions:u32be, clientNonce:16, deviceNonce:16.
Result computeControllerAuthProof(
    ByteView secret,
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration,
    uint32_t credentialVersion,
    uint32_t permissions,
    ByteView clientNonce,
    ByteView deviceNonce,
    ControllerAuthProofRole role,
    MutableByteSpan output);

class ControllerHmacSha256Authorizer : public IAuthorizationProvider {
public:
    ControllerHmacSha256Authorizer(
        IControllerCredentialSource& credentials,
        IRandom& random,
        IDirectSecureSessionController& directSessions,
        ControllerAuthSession* sessions,
        size_t sessionCapacity,
        ControllerAuthTransportPolicy transportPolicy =
            ControllerAuthTransportPolicy::RequireEncrypted);
    ~ControllerHmacSha256Authorizer() override;
    ControllerHmacSha256Authorizer(
        const ControllerHmacSha256Authorizer&) = delete;
    ControllerHmacSha256Authorizer& operator=(
        const ControllerHmacSha256Authorizer&) = delete;

    size_t methodCount() const override;
    uint16_t methodAt(size_t index) const override;
    bool establishesSecureTransport() const override;
    Result authorize(
        uint16_t method,
        ByteView requestPayload,
        const AuthorizationSessionContext& session,
        AuthorizationDecision& decision) override;
    void authorizationResultQueued(
        const AuthorizationSessionContext& session) override;
    void sessionClosed(
        const AuthorizationSessionContext& session) override;
    void resetSessions() override;

private:
    ControllerAuthSession* findSession(
        uint8_t transportId,
        uint32_t sessionId);
    ControllerAuthSession* allocateSession(
        uint8_t transportId,
        uint32_t sessionId);
    Result loadCredential(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t ownershipGeneration,
        uint32_t credentialVersion,
        ControllerCredential& credential,
        bool& found);
    Result writeChallenge(
        const ControllerCredential& credential,
        ControllerAuthSession& session,
        AuthorizationDecision& decision);

    IControllerCredentialSource& credentials_;
    IRandom& random_;
    IDirectSecureSessionController& directSessions_;
    ControllerAuthSession* sessions_;
    size_t sessionCapacity_;
    ControllerAuthTransportPolicy transportPolicy_;
    uint8_t response_[kControllerAuthChallengePayloadSize];
};

} // namespace security
} // namespace blinker

#endif
