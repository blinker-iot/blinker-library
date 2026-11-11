#ifndef BLINKER_SECURITY_INITIALCLOUDKEYREGISTRATION_H
#define BLINKER_SECURITY_INITIALCLOUDKEYREGISTRATION_H

#include "ControllerHmacSha256Authorizer.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IClock.h"
#include "../interface/IInitialCloudKeyInstaller.h"
#include "../transport/IFrameTransport.h"

namespace blinker {
namespace security {

constexpr uint32_t kInitialCloudKeyRegistrationWindowMillis = 120000U;

struct InitialCloudKeySetupWindow {
    InitialCloudKeySetupWindow(uint32_t opened = 0U, uint32_t duration = 0U)
        : openedAtMillis(opened), durationMillis(duration) {}
    bool live(uint32_t now) const {
        return durationMillis != 0U && durationMillis <= UINT32_MAX / 2U &&
            now - openedAtMillis < durationMillis;
    }
    uint32_t openedAtMillis;
    uint32_t durationMillis;
};

// Non-secret, process-local proof coordinates captured only from a live,
// committed Method 2 session. This is not persisted or sent over the wire.
struct InitialCloudKeyHandoffAuthority {
    DeviceInstanceId instance;
    uint8_t adminId[kControllerIdSize];
    uint32_t epoch;
    uint32_t credentialVersion;
    uint32_t committedAtMillis;
    InitialCloudKeyHandoffAuthority()
        : instance(), adminId(), epoch(0U), credentialVersion(0U), committedAtMillis(0U) {}
};

// Fixed-length v1 transcript; includes the domain's trailing NUL, physical
// instance, Method2/suite, complete context, committed binding, intent and raw
// candidate Key. Pure proof computation is NOT an authorization producer.
Result computeInitialCloudKeyRegistrationProof(ByteView controllerSecret,
    const DeviceInstanceId& instance, const DirectSecureContext& context,
    ByteView committedBinding, ByteView intent, const DeviceKey& candidate,
    MutableByteSpan output);

// Optional internal registration-purpose provider, never attached by ordinary
// products. Only its owned original Method2 verifier can prepare facts, and
// only queued AuthResult plus the actual committed transport activates them.
// One peer/one installation attempt; no secret/DeviceKey is retained. The
// caller still needs the later BC-3 source/target intent and BC-4 radio wiring.
// All calls belong to the original cooperative/runtime owner. busy only rejects
// same-stack reentry; it is not a thread lock. Native callbacks keep their
// original event forwarding and must not call install or storage from an IRQ.
class InitialCloudKeyRegistration final : public IAuthorizationProvider,
    private IDirectSecureSessionController {
public:
    InitialCloudKeyRegistration(const DeviceInstanceId& instance,
        IControllerCredentialSource& credentials, IRandom& random, IClock& clock,
        IDirectSecureSessionController& direct,
        ControllerAuthSession* sessions, size_t sessionCapacity,
        InitialCloudKeySetupWindow setup = InitialCloudKeySetupWindow());
    ~InitialCloudKeyRegistration() override;
    InitialCloudKeyRegistration(const InitialCloudKeyRegistration&) = delete;
    InitialCloudKeyRegistration& operator=(const InitialCloudKeyRegistration&) = delete;

    size_t methodCount() const override;
    uint16_t methodAt(size_t index) const override;
    bool establishesSecureTransport() const override;
    Result authorize(uint16_t method, ByteView payload,
        const AuthorizationSessionContext& session, AuthorizationDecision& decision) override;
    void authorizationResultQueued(const AuthorizationSessionContext& session) override;
    void sessionClosed(const AuthorizationSessionContext& session) override;
    void resetSessions() override;

    Result install(const RxContext& rx, ByteView intent, const DeviceKey& candidate,
        ByteView proof, IInitialCloudKeyInstaller& installer);

private:
    friend class InitialCloudKeyRegistrationEndpoint;
    friend class InitialCloudNetworkRecoveryEndpoint;
    struct Request;
    class CommitGuard;
    Result prepare(uint32_t sessionId, ByteView secret,
        const DirectSecureContext& context) override;
    void commit(uint32_t sessionId) override;
    void cancel(uint32_t sessionId) override;
    void invalidate();
    bool matchesPeer(uint8_t transportId, uint32_t sessionId) const;
    Result live(const Request& request, bool consumed);
    Result liveSession(const RxContext& rx, uint32_t revision, uint32_t committedAt,
        ByteView expectedBinding, bool consumed);
    Result validateHandoffSession(const RxContext& rx, ByteView expectedBinding,
        InitialCloudKeyHandoffAuthority& authority);
    // Separate installed-Key network purpose. This consumes the original
    // committed Method 2 grant without claiming a Key installation occurred.
    Result consumeNetworkSession(const RxContext& rx, ByteView expectedBinding,
        InitialCloudKeyHandoffAuthority& authority);
    Result validateConsumedNetworkSession(const RxContext& rx, ByteView expectedBinding,
        const InitialCloudKeyHandoffAuthority& authority);
    Result attemptInstall(const RxContext& rx, ByteView intent, const DeviceKey& candidate,
        ByteView proof, IInitialCloudKeyInstaller& installer, bool& attempted);
    Result validateCommit(const Request& request, const DeviceKey& candidate,
        uint32_t epoch, const ControllerCredential& actualAdmin);

    const DeviceInstanceId instance_;
    IControllerCredentialSource& credentials_;
    IClock& clock_;
    const InitialCloudKeySetupWindow setup_;
    IDirectSecureSessionController& direct_;
    ControllerHmacSha256Authorizer authorizer_;
    ControllerAuthSession facts_;
    AuthorizationSessionContext peer_, authorizingSession_;
    uint8_t binding_[kDirectSecureBindingSize];
    uint32_t committedAt_, revision_, authorizingRevision_, queuedRevision_;
    bool hasPeer_, prepared_, authorized_, active_, consumed_, busy_, authorizing_, queued_;
};

} // namespace security
} // namespace blinker
#endif
