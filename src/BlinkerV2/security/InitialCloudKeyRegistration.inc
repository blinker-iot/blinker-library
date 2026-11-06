#include "InitialCloudKeyRegistration.h"

#include <stdint.h>
#include <string.h>
#include "../core/HmacSha256.h"
#include "../core/SecureMemory.h"

namespace blinker {
namespace security {
namespace {
const uint8_t initialKeyDomain[] = "blinker.initial-cloud-key-registration.v1";

bool initialKeyExactNonzero(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t any = 0U;
    for (size_t i = 0U; i < size; ++i) any = static_cast<uint8_t>(any | value.data[i]);
    return any != 0U;
}
bool initialKeyOverlaps(ByteView left, ByteView right) {
    if (left.empty() || right.empty()) return false;
    const uintptr_t a = reinterpret_cast<uintptr_t>(left.data);
    const uintptr_t b = reinterpret_cast<uintptr_t>(right.data);
    if (left.size > UINTPTR_MAX - a || right.size > UINTPTR_MAX - b) return true;
    return a < b + right.size && b < a + left.size;
}
void initialKeyWrite32(uint8_t* bytes, uint32_t value) {
    bytes[0] = static_cast<uint8_t>(value >> 24U);
    bytes[1] = static_cast<uint8_t>(value >> 16U);
    bytes[2] = static_cast<uint8_t>(value >> 8U);
    bytes[3] = static_cast<uint8_t>(value);
}
DirectSecureContext initialKeyContext(const ControllerAuthSession& facts) {
    DirectSecureContext value;
    value.controllerId = ByteView(facts.controllerId, sizeof(facts.controllerId));
    value.domain = facts.domain;
    value.ownershipGeneration = facts.ownershipGeneration;
    value.credentialVersion = facts.credentialVersion;
    value.permissions = facts.permissions;
    value.clientNonce = ByteView(facts.clientNonce, sizeof(facts.clientNonce));
    value.deviceNonce = ByteView(facts.deviceNonce, sizeof(facts.deviceNonce));
    return value;
}
bool initialKeyAdminMatches(const ControllerCredential& admin, const ControllerAuthSession& facts) {
    return validateControllerCredential(admin) &&
        admin.suite == ControllerCredentialSuite::HmacSha256_32 &&
        admin.domain == ControllerCredentialDomain::PlatformAccess &&
        admin.permissions == kControllerPermissionAll &&
        admin.ownershipGeneration == facts.ownershipGeneration &&
        admin.credentialVersion == facts.credentialVersion &&
        facts.domain == admin.domain && facts.permissions == admin.permissions &&
        constantTimeEqual(admin.id(), ByteView(facts.controllerId, sizeof(facts.controllerId)));
}
struct InitialKeyBusyGuard {
    bool& busy;
    explicit InitialKeyBusyGuard(bool& value) : busy(value) { busy = true; }
    ~InitialKeyBusyGuard() { busy = false; }
};
struct InitialKeyCredentialGuard {
    ControllerCredential credential;
    ~InitialKeyCredentialGuard() { clearControllerCredential(credential); }
};
} // namespace

Result computeInitialCloudKeyRegistrationProof(ByteView secret,
    const DeviceInstanceId& instance, const DirectSecureContext& context,
    ByteView binding, ByteView intent, const DeviceKey& candidate, MutableByteSpan output) {
    if (!initialKeyExactNonzero(secret, kControllerCredentialSecretSize) ||
        !isValidDeviceInstanceId(instance) ||
        !initialKeyExactNonzero(context.controllerId, kControllerIdSize) ||
        context.domain != ControllerCredentialDomain::PlatformAccess ||
        context.ownershipGeneration == 0U || context.credentialVersion == 0U ||
        context.permissions != kControllerPermissionAll ||
        !initialKeyExactNonzero(context.clientNonce, kDirectSecureAuthNonceSize) ||
        !initialKeyExactNonzero(context.deviceNonce, kDirectSecureAuthNonceSize) ||
        !initialKeyExactNonzero(binding, kDirectSecureBindingSize) ||
        !initialKeyExactNonzero(intent, 16U) || !validateDeviceKey(candidate) ||
        output.data == nullptr || output.size < kSha256Size)
        return Result::failure(ErrorCode::InvalidArgument);
    const ByteView destination(output.data, kSha256Size);
    const ByteView inputs[] = {secret, ByteView(instance.bytes, sizeof(instance.bytes)),
        context.controllerId, context.clientNonce, context.deviceNonce, binding, intent,
        ByteView(candidate.bytes, sizeof(candidate.bytes))};
    for (size_t i = 0U; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
        if (initialKeyOverlaps(destination, inputs[i])) return Result::failure(ErrorCode::InvalidArgument);
    const uint8_t methodSuite[] = {0U, static_cast<uint8_t>(kControllerHmacSha256Method),
        static_cast<uint8_t>(ControllerCredentialSuite::HmacSha256_32)};
    uint8_t metadata[13U] = {};
    metadata[0] = static_cast<uint8_t>(context.domain);
    initialKeyWrite32(metadata + 1U, context.ownershipGeneration);
    initialKeyWrite32(metadata + 5U, context.credentialVersion);
    initialKeyWrite32(metadata + 9U, context.permissions);
    uint8_t proof[kSha256Size] = {};
    HmacSha256 hmac;
    Result result = hmac.begin(secret);
    if (result) result = hmac.update(ByteView(initialKeyDomain, sizeof(initialKeyDomain)));
    if (result) result = hmac.update(ByteView(instance.bytes, sizeof(instance.bytes)));
    if (result) result = hmac.update(ByteView(methodSuite, sizeof(methodSuite)));
    if (result) result = hmac.update(context.controllerId);
    if (result) result = hmac.update(ByteView(metadata, sizeof(metadata)));
    if (result) result = hmac.update(context.clientNonce);
    if (result) result = hmac.update(context.deviceNonce);
    if (result) result = hmac.update(binding);
    if (result) result = hmac.update(intent);
    if (result) result = hmac.update(ByteView(candidate.bytes, sizeof(candidate.bytes)));
    if (result) result = hmac.finish(MutableByteSpan(proof, sizeof(proof)));
    if (result) memcpy(output.data, proof, sizeof(proof));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    secureZero(MutableByteSpan(metadata, sizeof(metadata)));
    return result;
}

struct InitialCloudKeyRegistration::Request {
    RxContext rx;
    ControllerAuthSession facts;
    DeviceKey candidate;
    uint8_t binding[kDirectSecureBindingSize], intent[16U], proof[kSha256Size];
    uint32_t revision, committedAt;
    Request() : rx(), facts(), candidate(), binding(), intent(), proof(), revision(0U), committedAt(0U) {}
    ~Request() { clearDeviceKey(candidate); secureZero(MutableByteSpan(proof, sizeof(proof))); facts.clear(); }
};

class InitialCloudKeyRegistration::CommitGuard final : public IInitialCloudKeyCommitGuard {
public:
    CommitGuard(InitialCloudKeyRegistration& owner, const Request& request)
        : owner_(owner), request_(request), used_(false), validated_(false) {}
    Result validateInitialCloudKeyCommit(const DeviceKey& candidate,
        uint32_t epoch, const ControllerCredential& actualAdmin) override {
        if (used_) return Result::failure(ErrorCode::StateConflict);
        used_ = true;
        const Result result = owner_.validateCommit(request_, candidate, epoch, actualAdmin);
        validated_ = result.ok();
        return result;
    }
    bool validated() const { return validated_; }
private:
    InitialCloudKeyRegistration& owner_;
    const Request& request_;
    bool used_, validated_;
};

InitialCloudKeyRegistration::InitialCloudKeyRegistration(const DeviceInstanceId& instance,
    IControllerCredentialSource& credentials, IRandom& random, IClock& clock,
    IDirectSecureSessionController& direct, ControllerAuthSession* sessions, size_t capacity,
    InitialCloudKeySetupWindow setup)
    : instance_(instance), credentials_(credentials), clock_(clock), setup_(setup), direct_(direct),
      authorizer_(credentials, random, *this, sessions, capacity,
          ControllerAuthTransportPolicy::EstablishDirectSecure),
      facts_(), peer_(), authorizingSession_(), binding_(), committedAt_(0U), revision_(1U),
      authorizingRevision_(0U), queuedRevision_(0U), hasPeer_(false), prepared_(false),
      authorized_(false), active_(false), consumed_(false), busy_(false), authorizing_(false), queued_(false) {}
InitialCloudKeyRegistration::~InitialCloudKeyRegistration() { resetSessions(); }
size_t InitialCloudKeyRegistration::methodCount() const { return authorizer_.methodCount(); }
uint16_t InitialCloudKeyRegistration::methodAt(size_t index) const { return authorizer_.methodAt(index); }
bool InitialCloudKeyRegistration::establishesSecureTransport() const { return authorizer_.establishesSecureTransport(); }
bool InitialCloudKeyRegistration::matchesPeer(uint8_t transport, uint32_t session) const {
    return hasPeer_ && peer_.transportId == transport && peer_.sessionId == session && session != 0U;
}
void InitialCloudKeyRegistration::invalidate() {
    ++revision_;
    facts_.clear(); secureZero(MutableByteSpan(binding_, sizeof(binding_)));
    peer_ = AuthorizationSessionContext(); hasPeer_ = false;
    prepared_ = false; authorized_ = false; active_ = false; consumed_ = false; committedAt_ = 0U;
    // busy_/authorizing_/queued_ belong to their actual call stacks, not cancel.
}
Result InitialCloudKeyRegistration::authorize(uint16_t method, ByteView payload,
    const AuthorizationSessionContext& session, AuthorizationDecision& decision) {
    decision = AuthorizationDecision();
    if (busy_ || authorizing_ || queued_) return Result::failure(ErrorCode::WouldBlock);
    if (active_ || consumed_) return Result::failure(ErrorCode::AlreadyExists);
    if (hasPeer_ && !matchesPeer(session.transportId, session.sessionId))
        return Result::failure(ErrorCode::AuthenticationRequired);
    authorizing_ = true; authorizingSession_ = session; authorizingRevision_ = revision_;
    const Result result = authorizer_.authorize(method, payload, session, decision);
    authorizing_ = false;
    if (revision_ != authorizingRevision_) { decision = AuthorizationDecision(); return Result::failure(ErrorCode::StateConflict); }
    if (result && decision.status == bbp2::AuthStatus::Continue) { peer_ = session; hasPeer_ = true; }
    if (result && decision.status == bbp2::AuthStatus::Authorized) {
        if (!prepared_ || !matchesPeer(session.transportId, session.sessionId)) {
            decision = AuthorizationDecision(); return Result::failure(ErrorCode::AuthenticationRequired);
        }
        authorized_ = true;
    }
    return result;
}
Result InitialCloudKeyRegistration::prepare(uint32_t sessionId, ByteView secret, const DirectSecureContext& context) {
    if (!authorizing_ || revision_ != authorizingRevision_ ||
        sessionId != authorizingSession_.sessionId || sessionId == 0U ||
        (hasPeer_ && !matchesPeer(authorizingSession_.transportId, sessionId)))
        return Result::failure(ErrorCode::AuthenticationRequired);
    const Result result = direct_.prepare(sessionId, secret, context);
    if (!result) return result;
    if (revision_ != authorizingRevision_) { direct_.cancel(sessionId); return Result::failure(ErrorCode::StateConflict); }
    peer_ = authorizingSession_; hasPeer_ = true;
    facts_.clear(); facts_.occupied = true;
    facts_.transportId = peer_.transportId; facts_.sessionId = sessionId;
    facts_.domain = context.domain; facts_.ownershipGeneration = context.ownershipGeneration;
    facts_.credentialVersion = context.credentialVersion; facts_.permissions = context.permissions;
    memcpy(facts_.controllerId, context.controllerId.data, sizeof(facts_.controllerId));
    memcpy(facts_.clientNonce, context.clientNonce.data, sizeof(facts_.clientNonce));
    memcpy(facts_.deviceNonce, context.deviceNonce.data, sizeof(facts_.deviceNonce));
    prepared_ = true;
    return result;
}
void InitialCloudKeyRegistration::authorizationResultQueued(const AuthorizationSessionContext& session) {
    if (busy_ || authorizing_ || queued_ || !prepared_ || !authorized_ ||
        !matchesPeer(session.transportId, session.sessionId)) return;
    queued_ = true; queuedRevision_ = revision_;
    authorizer_.authorizationResultQueued(session);
    queued_ = false;
}
void InitialCloudKeyRegistration::commit(uint32_t sessionId) {
    if (!queued_ || queuedRevision_ != revision_ || !prepared_ || !authorized_ ||
        !matchesPeer(peer_.transportId, sessionId)) return;
    direct_.commit(sessionId);
    const ByteView binding = direct_.committedSessionBinding(sessionId);
    const uint32_t now = clock_.monotonicMillis();
    if (queuedRevision_ != revision_ || !initialKeyExactNonzero(binding, sizeof(binding_))) { invalidate(); return; }
    memcpy(binding_, binding.data, sizeof(binding_)); committedAt_ = now;
    prepared_ = false; active_ = true;
}
void InitialCloudKeyRegistration::cancel(uint32_t sessionId) {
    if (matchesPeer(peer_.transportId, sessionId) ||
        (authorizing_ && authorizingSession_.sessionId == sessionId)) invalidate();
    direct_.cancel(sessionId);
}
void InitialCloudKeyRegistration::sessionClosed(const AuthorizationSessionContext& session) {
    if (!matchesPeer(session.transportId, session.sessionId) &&
        !(authorizing_ && authorizingSession_.transportId == session.transportId &&
          authorizingSession_.sessionId == session.sessionId)) return;
    invalidate();
    authorizer_.sessionClosed(session);
}
void InitialCloudKeyRegistration::resetSessions() {
    const uint32_t session = hasPeer_ ? peer_.sessionId : (authorizing_ ? authorizingSession_.sessionId : 0U);
    invalidate();
    if (session != 0U) direct_.cancel(session);
    authorizer_.resetSessions();
}

Result InitialCloudKeyRegistration::live(const Request& request, bool consumed) {
    if (request.facts.transportId != peer_.transportId || request.facts.sessionId != peer_.sessionId)
        return Result::failure(ErrorCode::AuthenticationRequired);
    return liveSession(request.rx, request.revision, request.committedAt,
        ByteView(request.binding, sizeof(request.binding)), consumed);
}
Result InitialCloudKeyRegistration::liveSession(const RxContext& rx, uint32_t revision, uint32_t committedAt,
    ByteView expectedBinding, bool consumed) {
    if (!busy_ || !active_ || !facts_.occupied || revision_ != revision ||
        consumed_ != consumed || !rx.encrypted || !matchesPeer(rx.transportId, rx.sessionId) ||
        facts_.domain != ControllerCredentialDomain::PlatformAccess || facts_.permissions != kControllerPermissionAll)
        return Result::failure(ErrorCode::AuthenticationRequired);
    const uint32_t now = clock_.monotonicMillis();
    if (now - committedAt >= kInitialCloudKeyRegistrationWindowMillis ||
        (setup_.durationMillis != 0U && !setup_.live(now))) return Result::failure(ErrorCode::StateConflict);
    const ByteView current = direct_.committedSessionBinding(rx.sessionId);
    if (!initialKeyExactNonzero(current, sizeof(binding_)) ||
        !constantTimeEqual(current, expectedBinding) ||
        !constantTimeEqual(current, ByteView(binding_, sizeof(binding_))) || revision_ != revision)
        return Result::failure(ErrorCode::NotConnected);
    return Result::success();
}
Result InitialCloudKeyRegistration::validateHandoffSession(const RxContext& rx, ByteView expectedBinding,
    InitialCloudKeyHandoffAuthority& authority) {
    authority = InitialCloudKeyHandoffAuthority();
    if (busy_ || authorizing_ || queued_) return Result::failure(ErrorCode::WouldBlock);
    InitialKeyBusyGuard busy(busy_);
    const uint32_t revision = revision_;
    const uint32_t committedAt = committedAt_;
    const ControllerAuthSession facts = facts_;
    Result result = liveSession(rx, revision, committedAt, expectedBinding, true);
    if (!result) return result;
    InitialKeyCredentialGuard actual;
    result = credentials_.loadActive(ByteView(facts.controllerId, sizeof(facts.controllerId)),
        facts.domain, facts.ownershipGeneration, actual.credential);
    if (!result) return result;
    if (!initialKeyAdminMatches(actual.credential, facts)) return Result::failure(ErrorCode::AuthenticationRequired);
    result = liveSession(rx, revision, committedAt, expectedBinding, true);
    if (result) {
        authority.instance = instance_;
        memcpy(authority.adminId, facts.controllerId, sizeof(authority.adminId));
        authority.epoch = facts.ownershipGeneration;
        authority.credentialVersion = facts.credentialVersion;
        authority.committedAtMillis = committedAt;
    }
    return result;
}
Result InitialCloudKeyRegistration::consumeNetworkSession(const RxContext& rx, ByteView expectedBinding,
    InitialCloudKeyHandoffAuthority& authority) {
    authority = InitialCloudKeyHandoffAuthority();
    if (busy_ || authorizing_ || queued_) return Result::failure(ErrorCode::WouldBlock);
    {
        InitialKeyBusyGuard busy(busy_);
        const Result result = liveSession(rx, revision_, committedAt_, expectedBinding, false);
        if (!result) return result;
        // Consume before the first credential/storage callback. Failure does
        // not restore this grant; the endpoint also owns a terminal one-shot.
        consumed_ = true;
    }
    return validateHandoffSession(rx, expectedBinding, authority);
}
Result InitialCloudKeyRegistration::validateConsumedNetworkSession(const RxContext& rx, ByteView expectedBinding,
    const InitialCloudKeyHandoffAuthority& authority) {
    InitialCloudKeyHandoffAuthority actual;
    const Result result = validateHandoffSession(rx, expectedBinding, actual);
    if (!result) return result;
    if (!sameDeviceInstanceId(actual.instance, authority.instance) ||
        actual.epoch != authority.epoch || actual.credentialVersion != authority.credentialVersion ||
        actual.committedAtMillis != authority.committedAtMillis ||
        !constantTimeEqual(ByteView(actual.adminId, sizeof(actual.adminId)),
            ByteView(authority.adminId, sizeof(authority.adminId))))
        return Result::failure(ErrorCode::AuthenticationRequired);
    return Result::success();
}
Result InitialCloudKeyRegistration::validateCommit(const Request& request, const DeviceKey& candidate,
    uint32_t epoch, const ControllerCredential& actualAdmin) {
    Result result = live(request, true);
    if (!result) return result;
    if (!sameDeviceKey(candidate, request.candidate) || epoch != request.facts.ownershipGeneration ||
        !initialKeyAdminMatches(actualAdmin, request.facts)) return Result::failure(ErrorCode::AuthenticationRequired);
    uint8_t expected[kSha256Size] = {};
    result = computeInitialCloudKeyRegistrationProof(actualAdmin.authenticationSecret(), instance_,
        initialKeyContext(request.facts), ByteView(request.binding, sizeof(request.binding)),
        ByteView(request.intent, sizeof(request.intent)), request.candidate, MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(ByteView(expected, sizeof(expected)), ByteView(request.proof, sizeof(request.proof))))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result ? live(request, true) : result;
}
Result InitialCloudKeyRegistration::install(const RxContext& rx, ByteView intent, const DeviceKey& candidate,
    ByteView proof, IInitialCloudKeyInstaller& installer) {
    bool attempted = false;
    return attemptInstall(rx, intent, candidate, proof, installer, attempted);
}
Result InitialCloudKeyRegistration::attemptInstall(const RxContext& rx, ByteView intent, const DeviceKey& candidate,
    ByteView proof, IInitialCloudKeyInstaller& installer, bool& attempted) {
    attempted = false;
    if (busy_ || authorizing_ || queued_) return Result::failure(ErrorCode::WouldBlock);
    if (!initialKeyExactNonzero(intent, 16U) || proof.data == nullptr || proof.size != kSha256Size ||
        !validateDeviceKey(candidate)) return Result::failure(ErrorCode::InvalidArgument);
    InitialKeyBusyGuard busy(busy_);
    Request request;
    // Copy every borrowed argument before credential/provider/clock callbacks.
    request.rx = rx; request.candidate = candidate; request.facts = facts_;
    request.revision = revision_; request.committedAt = committedAt_;
    memcpy(request.intent, intent.data, sizeof(request.intent));
    memcpy(request.proof, proof.data, sizeof(request.proof));
    memcpy(request.binding, binding_, sizeof(request.binding));
    Result result = live(request, false);
    if (!result) return result;
    InitialKeyCredentialGuard actual;
    result = credentials_.loadActive(ByteView(request.facts.controllerId, sizeof(request.facts.controllerId)),
        request.facts.domain, request.facts.ownershipGeneration, actual.credential);
    if (!result) return result;
    if (!initialKeyAdminMatches(actual.credential, request.facts)) return Result::failure(ErrorCode::AuthenticationRequired);
    result = live(request, false);
    if (!result) return result;
    uint8_t expected[kSha256Size] = {};
    result = computeInitialCloudKeyRegistrationProof(actual.credential.authenticationSecret(), instance_,
        initialKeyContext(request.facts), ByteView(request.binding, sizeof(request.binding)),
        ByteView(request.intent, sizeof(request.intent)), request.candidate, MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(ByteView(expected, sizeof(expected)), ByteView(request.proof, sizeof(request.proof))))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    if (!result) return result;
    result = live(request, false);
    if (!result) return result;
    consumed_ = true; // Before invoking any installer, including known rejection/UNKNOWN.
    attempted = true;
    CommitGuard guard(*this, request);
    result = installer.installInitialCloudKeyAuthorized(request.candidate,
        request.facts.ownershipGeneration, actual.credential, guard);
    if (result && !guard.validated()) return Result::failure(ErrorCode::AuthenticationRequired);
    // The late guard is the write-admission boundary. Cancellation before it
    // rejects without writing; once the installer starts persistence, retain
    // its exact success/known failure/UNKNOWN. A later disconnect only revokes
    // future use: it cannot roll back or relabel an already-attempted commit.
    return result;
}

} // namespace security
} // namespace blinker
