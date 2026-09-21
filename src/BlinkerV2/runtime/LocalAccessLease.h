#ifndef BLINKER_RUNTIME_LOCALACCESSLEASE_H
#define BLINKER_RUNTIME_LOCALACCESSLEASE_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../interface/IDeviceKeySource.h"
#include "../interface/ILocalAccessTargetPolicy.h"
#include "../interface/IRandom.h"
#include "../security/LocalAccessSecurity.h"

namespace blinker {

struct LocalAccessAdmission {
    uint8_t authorizationId[local_access::kIdSize];
    uint8_t callerSessionId[local_access::kIdSize];
    uint8_t targetDeviceInstanceId[local_access::kIdSize];
    char targetLogicalDeviceId[local_access::kMaximumLogicalDeviceIdSize];
    uint8_t targetLogicalDeviceIdSize;
    local_access::TargetKind targetKind;
    uint32_t accessEpoch;
    uint32_t topologyVersion;
    uint32_t authorityRevision;
    uint32_t permissions;
    uint32_t remainingMillis;
    uint8_t sessionKey[kDeviceAuthKeySize];

    LocalAccessAdmission()
        : authorizationId(), callerSessionId(), targetDeviceInstanceId(),
          targetLogicalDeviceId(), targetLogicalDeviceIdSize(0U),
          targetKind(local_access::TargetKind::Self), accessEpoch(0U),
          topologyVersion(0U), authorityRevision(0U), permissions(0U),
          remainingMillis(0U), sessionKey() {}

    ~LocalAccessAdmission() { clear(); }
    void clear() {
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(this), sizeof(*this)));
        targetKind = local_access::TargetKind::Self;
    }

private:
    LocalAccessAdmission(const LocalAccessAdmission&);
    LocalAccessAdmission& operator=(const LocalAccessAdmission&);
};

struct LocalAccessLease {
    uint8_t authorizationId[local_access::kIdSize];
    uint8_t callerSessionId[local_access::kIdSize];
    uint8_t targetDeviceInstanceId[local_access::kIdSize];
    char targetLogicalDeviceId[local_access::kMaximumLogicalDeviceIdSize];
    uint8_t targetLogicalDeviceIdSize;
    local_access::TargetKind targetKind;
    local_access::SecurityProfile securityProfile;
    uint32_t accessEpoch;
    uint32_t topologyVersion;
    uint32_t authorityRevision;
    uint32_t permissions;
    uint32_t startedAt;
    uint32_t lifetimeMillis;
    bool occupied;
    bool expired;

    LocalAccessLease()
        : authorizationId(), callerSessionId(), targetDeviceInstanceId(),
          targetLogicalDeviceId(), targetLogicalDeviceIdSize(0U),
          targetKind(local_access::TargetKind::Self), securityProfile(local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256), accessEpoch(0U),
          topologyVersion(0U), authorityRevision(0U), permissions(0U),
          startedAt(0U), lifetimeMillis(0U), occupied(false), expired(false) {}

    void clear() {
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(this), sizeof(*this)));
        targetKind = local_access::TargetKind::Self;
    }
};

template <size_t Capacity>
class LocalAccessLeaseLedger {
public:
    LocalAccessLeaseLedger() : leases_() {
        static_assert(Capacity != 0U, "local access needs one lease slot");
    }
    ~LocalAccessLeaseLedger() { clear(); }

    // Product main-loop maintenance, even with no LAN/cloud traffic. Latch
    // expiry so a later millis wrap cannot make an observed deadline live.
    // Like other uint32_t timers, the caller must poll within a clock cycle.
    void poll(uint32_t now) {
        for (size_t index = 0U; index < Capacity; ++index) {
            LocalAccessLease& lease = leases_[index];
            if (!lease.occupied) continue;
            const uint32_t elapsed = static_cast<uint32_t>(now - lease.startedAt);
            if (elapsed >= lease.lifetimeMillis) {
                lease.expired = true;
                if (elapsed - lease.lifetimeMillis >= local_access::kChallengeResponseMillis) lease.clear();
            }
        }
    }

    Result admit(
        const local_access::GrantView& grant,
        uint32_t now,
        uint32_t& remainingMillis) {
        remainingMillis = 0U;
        LocalAccessLease* available = nullptr;
        for (size_t index = 0U; index < Capacity; ++index) {
            LocalAccessLease& lease = leases_[index];
            if (!lease.occupied) {
                if (available == nullptr) available = &lease;
                continue;
            }
            const bool sameAuthorization = memcmp(
                lease.authorizationId, grant.authorizationId.data,
                local_access::kIdSize) == 0;
            const uint32_t elapsed = static_cast<uint32_t>(
                now - lease.startedAt);
            if (lease.expired || elapsed >= lease.lifetimeMillis) {
                lease.expired = true;
                // Retain a tombstone while an already-issued fresh challenge
                // can still arrive. Clearing on the first rejection would let
                // a second connection restart this expired authorization.
                if (sameAuthorization) {
                    return Result::failure(ErrorCode::AuthenticationRequired);
                }
                if (elapsed < lease.lifetimeMillis || elapsed - lease.lifetimeMillis <
                    local_access::kChallengeResponseMillis) continue;
                lease.clear();
                if (available == nullptr) available = &lease;
                continue;
            }
            if (!sameAuthorization) continue;
            if (!sameGrantScope(lease, grant)) {
                return Result::failure(ErrorCode::StateConflict);
            }
            remainingMillis = lease.lifetimeMillis - elapsed;
            return Result::success();
        }
        if (available == nullptr) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        copyGrant(*available, grant, now);
        remainingMillis = available->lifetimeMillis;
        return Result::success();
    }

    bool permits(
        ByteView authorizationId,
        uint32_t requiredPermission,
        uint32_t now) const {
        if (authorizationId.data == nullptr ||
            authorizationId.size != local_access::kIdSize ||
            requiredPermission == 0U) {
            return false;
        }
        for (size_t index = 0U; index < Capacity; ++index) {
            const LocalAccessLease& lease = leases_[index];
            if (lease.occupied && !lease.expired &&
                static_cast<uint32_t>(now - lease.startedAt) <
                    lease.lifetimeMillis &&
                (lease.permissions & requiredPermission) ==
                    requiredPermission &&
                memcmp(lease.authorizationId, authorizationId.data,
                       local_access::kIdSize) == 0) {
                return true;
            }
        }
        return false;
    }

    void retireBefore(uint32_t authorityRevision) {
        for (size_t index = 0U; index < Capacity; ++index) {
            if (leases_[index].occupied &&
                leases_[index].authorityRevision < authorityRevision) {
                leases_[index].clear();
            }
        }
    }

    void retire(ByteView authorizationId) {
        for (size_t index = 0U; index < Capacity; ++index) {
            if (leases_[index].occupied &&
                memcmp(leases_[index].authorizationId, authorizationId.data,
                       local_access::kIdSize) == 0) leases_[index].clear();
        }
    }

    void clear() {
        for (size_t index = 0U; index < Capacity; ++index) {
            leases_[index].clear();
        }
    }

private:
    static bool sameGrantScope(
        const LocalAccessLease& lease,
        const local_access::GrantView& grant) {
        return lease.targetKind == grant.targetKind && lease.securityProfile == grant.securityProfile &&
               lease.accessEpoch == grant.accessEpoch &&
               lease.topologyVersion == grant.topologyVersion &&
               lease.authorityRevision == grant.authorityRevision &&
               lease.permissions == grant.permissions &&
               lease.lifetimeMillis == grant.lifetimeMillis &&
               lease.targetLogicalDeviceIdSize ==
                   grant.targetLogicalDeviceId.size &&
               memcmp(lease.callerSessionId, grant.callerSessionId.data,
                      local_access::kIdSize) == 0 &&
               memcmp(lease.targetDeviceInstanceId,
                      grant.targetDeviceInstanceId.data,
                      local_access::kIdSize) == 0 &&
               (lease.targetLogicalDeviceIdSize == 0U ||
                memcmp(lease.targetLogicalDeviceId,
                       grant.targetLogicalDeviceId.data,
                       lease.targetLogicalDeviceIdSize) == 0);
    }

    static void copyGrant(
        LocalAccessLease& lease,
        const local_access::GrantView& grant,
        uint32_t now) {
        lease.clear();
        memcpy(lease.authorizationId, grant.authorizationId.data,
               local_access::kIdSize);
        memcpy(lease.callerSessionId, grant.callerSessionId.data,
               local_access::kIdSize);
        memcpy(lease.targetDeviceInstanceId,
               grant.targetDeviceInstanceId.data, local_access::kIdSize);
        if (!grant.targetLogicalDeviceId.empty()) {
            memcpy(lease.targetLogicalDeviceId,
                   grant.targetLogicalDeviceId.data,
                   grant.targetLogicalDeviceId.size);
        }
        lease.targetLogicalDeviceIdSize =
            static_cast<uint8_t>(grant.targetLogicalDeviceId.size);
        lease.targetKind = grant.targetKind;
        lease.securityProfile = grant.securityProfile;
        lease.accessEpoch = grant.accessEpoch;
        lease.topologyVersion = grant.topologyVersion;
        lease.authorityRevision = grant.authorityRevision;
        lease.permissions = grant.permissions;
        lease.startedAt = now;
        lease.lifetimeMillis = grant.lifetimeMillis;
        lease.occupied = true;
    }

    LocalAccessLease leases_[Capacity];

    LocalAccessLeaseLedger(const LocalAccessLeaseLedger&);
    LocalAccessLeaseLedger& operator=(const LocalAccessLeaseLedger&);
};

class SelfLocalAccessTargetPolicy final : public ILocalAccessTargetPolicy {
public:
    Result admit(const local_access::GrantView& grant) override {
        return grant.targetKind == local_access::TargetKind::Self
                   ? Result::success()
                   : Result::failure(ErrorCode::AuthenticationRequired);
    }
};

template <size_t Capacity>
class LocalAccessAuthority {
public:
    LocalAccessAuthority(
        IDeviceKeySource& deviceKey,
        ILocalAccessTargetPolicy& targetPolicy)
        : deviceKey_(deviceKey), targetPolicy_(targetPolicy), leases_(),
          recipient_(), deviceKeyVersion_(0U), authorityRevision_(0U),
          challengeGeneration_(1U), configured_(false), enabled_(false) {}

    ~LocalAccessAuthority() { clear(); }

    Result configure(
        ByteView recipientDeviceInstanceId,
        uint32_t deviceKeyVersion,
        uint32_t authorityRevision) {
        if (recipientDeviceInstanceId.data == nullptr ||
            recipientDeviceInstanceId.size != local_access::kIdSize ||
            deviceKeyVersion == 0U || authorityRevision == 0U ||
            !nonZero(recipientDeviceInstanceId)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        const bool sameRecipient = configured_ &&
            memcmp(recipient_, recipientDeviceInstanceId.data,
                   sizeof(recipient_)) == 0;
        if (sameRecipient &&
            (deviceKeyVersion < deviceKeyVersion_ ||
             (deviceKeyVersion == deviceKeyVersion_ &&
              (authorityRevision < authorityRevision_ ||
               (authorityRevision == authorityRevision_ && !enabled_))))) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (!sameRecipient || deviceKeyVersion != deviceKeyVersion_) {
            leases_.clear();
            invalidateChallenges();
        } else if (authorityRevision > authorityRevision_) {
            leases_.retireBefore(authorityRevision);
            invalidateChallenges();
        }
        memcpy(recipient_, recipientDeviceInstanceId.data, sizeof(recipient_));
        deviceKeyVersion_ = deviceKeyVersion;
        authorityRevision_ = authorityRevision;
        configured_ = true;
        enabled_ = true;
        return Result::success();
    }

    void updateAuthorityRevision(uint32_t authorityRevision) {
        if (!configured_ || authorityRevision == 0U ||
            authorityRevision <= authorityRevision_) {
            return;
        }
        authorityRevision_ = authorityRevision;
        leases_.retireBefore(authorityRevision);
        invalidateChallenges();
    }

    // Called only after authenticated control, never directly from LAN input.
    // Invalidate all pending challenges, including ones for a grant that has
    // not arrived yet. Established leases for other targets/callers survive.
    Result retireAuthorization(ByteView authorizationId) {
        if (!configured_ || authorizationId.size != local_access::kIdSize ||
            !nonZero(authorizationId)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        leases_.retire(authorizationId);
        invalidateChallenges();
        return Result::success();
    }

    void disable() {
        leases_.clear();
        enabled_ = false;
        invalidateChallenges();
    }

    bool permits(
        ByteView authorizationId,
        uint32_t requiredPermission,
        uint32_t now) const {
        return configured_ && enabled_ &&
               leases_.permits(authorizationId, requiredPermission, now);
    }

    void poll(uint32_t now) { leases_.poll(now); }

    void clear() {
        leases_.clear();
        secureZero(MutableByteSpan(recipient_, sizeof(recipient_)));
        deviceKeyVersion_ = 0U;
        authorityRevision_ = 0U;
        configured_ = false;
        enabled_ = false;
        invalidateChallenges();
    }

    bool configured() const { return configured_; }
    bool enabled() const { return configured_ && enabled_; }
    bool acceptingGrants() const { return enabled() && challengeGeneration_ != 0U; }
    uint32_t challengeGeneration() const { return challengeGeneration_; }
    ByteView recipientDeviceInstanceId() const {
        return configured_ ? ByteView(recipient_, sizeof(recipient_))
                           : ByteView();
    }
    uint32_t deviceKeyVersion() const { return deviceKeyVersion_; }
    uint32_t authorityRevision() const { return authorityRevision_; }

    Result admitGrant(
        const local_access::GrantView& grant,
        uint32_t now,
        LocalAccessAdmission& admission,
        bool& authenticated) {
        authenticated = false;
        admission.clear();
        Result result = local_access::validateGrant(grant);
        if (!result) return result;
        if (!acceptingGrants() || grant.deviceKeyVersion != deviceKeyVersion_ ||
            grant.authorityRevision != authorityRevision_ ||
            memcmp(grant.recipientDeviceInstanceId.data, recipient_,
                   sizeof(recipient_)) != 0) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        DeviceKey key;
        result = deviceKey_.load(key);
        if (result) {
            result = security::deriveLocalAccessSessionKey(
                key, grant,
                MutableByteSpan(admission.sessionKey,
                                sizeof(admission.sessionKey)));
        }
        clearDeviceKey(key);
        if (result) authenticated = true;
        if (result) result = targetPolicy_.admit(grant);
        uint32_t remaining = 0U;
        if (result) result = leases_.admit(grant, now, remaining);
        if (result) copyAdmission(grant, remaining, admission);
        if (!result) admission.clear();
        return result;
    }

private:
    void invalidateChallenges() {
        // Zero is a terminal admission barrier until object reconstruction;
        // never wrap and accidentally accept an ancient pending challenge.
        if (challengeGeneration_ != 0U) ++challengeGeneration_;
    }

    static bool nonZero(ByteView value) {
        uint8_t combined = 0U;
        if (value.data == nullptr || value.empty()) return false;
        for (size_t index = 0U; index < value.size; ++index) {
            combined = static_cast<uint8_t>(combined | value.data[index]);
        }
        return combined != 0U;
    }

    static void copyAdmission(
        const local_access::GrantView& grant,
        uint32_t remaining,
        LocalAccessAdmission& admission) {
        memcpy(admission.authorizationId, grant.authorizationId.data,
               local_access::kIdSize);
        memcpy(admission.callerSessionId, grant.callerSessionId.data,
               local_access::kIdSize);
        memcpy(admission.targetDeviceInstanceId,
               grant.targetDeviceInstanceId.data, local_access::kIdSize);
        if (!grant.targetLogicalDeviceId.empty()) {
            memcpy(admission.targetLogicalDeviceId,
                   grant.targetLogicalDeviceId.data,
                   grant.targetLogicalDeviceId.size);
        }
        admission.targetLogicalDeviceIdSize =
            static_cast<uint8_t>(grant.targetLogicalDeviceId.size);
        admission.targetKind = grant.targetKind;
        admission.accessEpoch = grant.accessEpoch;
        admission.topologyVersion = grant.topologyVersion;
        admission.authorityRevision = grant.authorityRevision;
        admission.permissions = grant.permissions;
        admission.remainingMillis = remaining;
    }

    IDeviceKeySource& deviceKey_;
    ILocalAccessTargetPolicy& targetPolicy_;
    LocalAccessLeaseLedger<Capacity> leases_;
    uint8_t recipient_[local_access::kIdSize];
    uint32_t deviceKeyVersion_;
    uint32_t authorityRevision_;
    uint32_t challengeGeneration_;
    bool configured_;
    bool enabled_;

    LocalAccessAuthority(const LocalAccessAuthority&);
    LocalAccessAuthority& operator=(const LocalAccessAuthority&);
};

template <size_t Capacity>
class LocalAccessChallengeSession {
public:
    LocalAccessChallengeSession(
        LocalAccessAuthority<Capacity>& authority,
        IRandom& random, local_access::SecurityProfile profile = local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256)
        : authority_(authority), random_(random), receiverSessionId_(),
          challenge_(), challengeIssuedAt_(0U), challengeGeneration_(0U),
          challengeActive_(false), profile_(profile) {}

    ~LocalAccessChallengeSession() { clear(); }

    void poll(uint32_t now) {
        if (challengeActive_ && (!authority_.acceptingGrants() ||
            challengeGeneration_ != authority_.challengeGeneration() ||
            static_cast<uint32_t>(now - challengeIssuedAt_) >= local_access::kChallengeResponseMillis)) {
            clearChallenge();
        }
    }

    Result issueChallenge(
        uint32_t now,
        MutableByteSpan output,
        size_t& written) {
        written = 0U;
        if (!authority_.acceptingGrants()) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        uint8_t randomBytes[local_access::kIdSize +
                            local_access::kChallengeSize] = {};
        Result result = random_.fill(
            MutableByteSpan(randomBytes, sizeof(randomBytes)));
        if (result && !nonZero(ByteView(randomBytes, sizeof(randomBytes)))) {
            result = Result::failure(ErrorCode::InternalError);
        }
        if (result) {
            memcpy(receiverSessionId_, randomBytes,
                   sizeof(receiverSessionId_));
            memcpy(challenge_, randomBytes + sizeof(receiverSessionId_),
                   sizeof(challenge_));
            local_access::ChallengeView value;
            value.recipientDeviceInstanceId =
                authority_.recipientDeviceInstanceId();
            value.deviceKeyVersion = authority_.deviceKeyVersion();
            value.receiverSessionId =
                ByteView(receiverSessionId_, sizeof(receiverSessionId_));
            value.challenge = ByteView(challenge_, sizeof(challenge_));
            value.authorityRevision = authority_.authorityRevision();
            value.securityProfile = profile_;
            result = local_access::encodeChallenge(value, output, written);
        }
        secureZero(MutableByteSpan(randomBytes, sizeof(randomBytes)));
        if (!result) {
            clearChallenge();
            return result;
        }
        challengeIssuedAt_ = now;
        challengeGeneration_ = authority_.challengeGeneration();
        challengeActive_ = true;
        return Result::success();
    }

    Result acceptGrant(
        ByteView encodedGrant,
        uint32_t now,
        LocalAccessAdmission& admission) {
        admission.clear();
        poll(now);
        if (!challengeActive_ || !authority_.acceptingGrants() ||
            challengeGeneration_ != authority_.challengeGeneration() ||
            static_cast<uint32_t>(now - challengeIssuedAt_) >=
                local_access::kChallengeResponseMillis) {
            clearChallenge();
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        local_access::GrantView grant;
        Result result = local_access::decodeGrant(encodedGrant, grant);
        if (result && !matchesChallenge(grant)) {
            result = Result::failure(ErrorCode::AuthenticationRequired);
        }
        bool authenticated = false;
        if (result) {
            result = authority_.admitGrant(
                grant, now, admission, authenticated);
        }
        if (authenticated) {
            // A valid MAC consumes this receiver challenge even when the
            // signed scope is stale or local capacity is exhausted.
            clearChallenge();
        }
        if (!result) admission.clear();
        return result;
    }

    void clear() {
        clearChallenge();
    }

private:
    static bool nonZero(ByteView value) {
        uint8_t combined = 0U;
        if (value.data == nullptr || value.empty()) return false;
        for (size_t index = 0U; index < value.size; ++index) {
            combined = static_cast<uint8_t>(combined | value.data[index]);
        }
        return combined != 0U;
    }

    bool matchesChallenge(const local_access::GrantView& grant) const {
        return grant.securityProfile == profile_ && memcmp(grant.receiverSessionId.data, receiverSessionId_,
                      sizeof(receiverSessionId_)) == 0 &&
               memcmp(grant.challenge.data, challenge_,
                      sizeof(challenge_)) == 0;
    }

    void clearChallenge() {
        secureZero(MutableByteSpan(
            receiverSessionId_, sizeof(receiverSessionId_)));
        secureZero(MutableByteSpan(challenge_, sizeof(challenge_)));
        challengeIssuedAt_ = 0U;
        challengeGeneration_ = 0U;
        challengeActive_ = false;
    }

    LocalAccessAuthority<Capacity>& authority_;
    IRandom& random_;
    uint8_t receiverSessionId_[local_access::kIdSize];
    uint8_t challenge_[local_access::kChallengeSize];
    uint32_t challengeIssuedAt_;
    uint32_t challengeGeneration_;
    bool challengeActive_;
    const local_access::SecurityProfile profile_;

    LocalAccessChallengeSession(const LocalAccessChallengeSession&);
    LocalAccessChallengeSession& operator=(
        const LocalAccessChallengeSession&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(LocalAccessLease) <= 160U,
    "LocalAccessLease exceeds its 32-bit ABI gate");
static_assert(
    sizeof(LocalAccessAdmission) <= 184U,
    "LocalAccessAdmission exceeds its 32-bit ABI gate");
static_assert(
    sizeof(LocalAccessAuthority<2U>) <= 328U,
    "two-target LocalAccessAuthority exceeds its 32-bit ABI gate");
static_assert(
    sizeof(LocalAccessChallengeSession<2U>) <= 52U,
    "LocalAccessChallengeSession exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
