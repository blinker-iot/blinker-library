#include <BlinkerV2Advanced.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {

static_assert(static_cast<uint8_t>(blinker::bbp2::MessageKind::LocalAccessControl) == 0x36U,
              "Physical cloud control carrier must match Broker kind 0x36");
static_assert(static_cast<uint8_t>(blinker::bbp2::MessageKind::PresenceKeyMutation) == 0x34U &&
              static_cast<uint8_t>(blinker::bbp2::MessageKind::PresenceKeyReceipt) == 0x35U,
              "Local control must not reuse Direct PresenceKey wire kinds");

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "CHECK failed at line " << __LINE__ << ": "          \
                      << #condition << std::endl;                               \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

void fillBytes(uint8_t* output, size_t size, uint8_t first) {
    for (size_t index = 0U; index < size; ++index) {
        output[index] = static_cast<uint8_t>(first + index);
    }
}

bool allZero(const uint8_t* value, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | value[index]);
    }
    return combined == 0U;
}

uint8_t hexNibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<uint8_t>(value - 'a' + 10);
    }
    return 0xFFU;
}

bool matchesHex(blinker::ByteView actual, const char* expected) {
    const size_t size = std::strlen(expected);
    if ((size & 1U) != 0U || actual.size != size / 2U) return false;
    for (size_t index = 0U; index < actual.size; ++index) {
        const uint8_t high = hexNibble(expected[index * 2U]);
        const uint8_t low = hexNibble(expected[index * 2U + 1U]);
        if (high > 0x0FU || low > 0x0FU ||
            actual.data[index] != static_cast<uint8_t>((high << 4U) | low)) {
            return false;
        }
    }
    return true;
}

bool matchesGolden(blinker::ByteView actual, const char* key) {
    std::ifstream file(BLINKER_LOCAL_ACCESS_GOLDEN);
    CHECK_TRUE(file.is_open());
    const std::string prefix = std::string(key) + "=";
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.compare(0U, prefix.size(), prefix) == 0) {
            return matchesHex(actual, line.c_str() + prefix.size());
        }
    }
    return false;
}

class FixedDeviceKeySource final : public blinker::IDeviceKeySource {
public:
    FixedDeviceKeySource() : key_() {
        fillBytes(key_.bytes, sizeof(key_.bytes), 1U);
    }

    blinker::Result load(blinker::DeviceKey& output) override {
        output = key_;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }

    const blinker::DeviceKey& value() const { return key_; }

private:
    blinker::DeviceKey key_;
};

class SequenceRandom final : public blinker::IRandom {
public:
    SequenceRandom() : next_(0x80U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (output.data == nullptr || output.empty()) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        fillBytes(output.data, output.size, next_);
        next_ = static_cast<uint8_t>(next_ + 0x20U);
        return blinker::Result::success();
    }

private:
    uint8_t next_;
};

class ExactChildPolicy final : public blinker::ILocalAccessTargetPolicy {
public:
    ExactChildPolicy(const uint8_t* instance, const char* logicalId)
        : instance_(instance), logicalId_(logicalId) {}

    blinker::Result admit(
        const blinker::local_access::GrantView& grant) override {
        const size_t logicalSize = std::strlen(logicalId_);
        const bool matches =
            grant.targetKind == blinker::local_access::TargetKind::Child &&
            grant.accessEpoch == 7U && grant.topologyVersion == 9U &&
            grant.targetLogicalDeviceId.size == logicalSize &&
            std::memcmp(grant.targetLogicalDeviceId.data, logicalId_,
                        logicalSize) == 0 &&
            std::memcmp(grant.targetDeviceInstanceId.data, instance_,
                        blinker::local_access::kIdSize) == 0;
        return matches
                   ? blinker::Result::success()
                   : blinker::Result::failure(
                         blinker::ErrorCode::AuthenticationRequired);
    }

private:
    const uint8_t* instance_;
    const char* logicalId_;
};

void makeSelfGrant(
    const blinker::local_access::ChallengeView& challenge,
    blinker::local_access::GrantView& grant,
    uint8_t* grantId,
    uint8_t* authorizationId,
    uint8_t* callerSessionId) {
    fillBytes(grantId, blinker::local_access::kIdSize, 0x10U);
    fillBytes(authorizationId, blinker::local_access::kIdSize, 0x30U);
    fillBytes(callerSessionId, blinker::local_access::kIdSize, 0x50U);
    grant.grantId = blinker::ByteView(
        grantId, blinker::local_access::kIdSize);
    grant.authorizationId = blinker::ByteView(
        authorizationId, blinker::local_access::kIdSize);
    grant.recipientDeviceInstanceId = challenge.recipientDeviceInstanceId;
    grant.deviceKeyVersion = challenge.deviceKeyVersion;
    grant.receiverSessionId = challenge.receiverSessionId;
    grant.challenge = challenge.challenge;
    grant.callerSessionId = blinker::ByteView(
        callerSessionId, blinker::local_access::kIdSize);
    grant.authorityRevision = challenge.authorityRevision;
    grant.targetKind = blinker::local_access::TargetKind::Self;
    grant.targetLogicalDeviceId = blinker::StringView();
    grant.targetDeviceInstanceId = challenge.recipientDeviceInstanceId;
    grant.accessEpoch = 0U;
    grant.topologyVersion = 0U;
    grant.permissions = blinker::kAuthorizationPermissionObserve |
                        blinker::kAuthorizationPermissionControl;
    grant.lifetimeMillis = 5000U;
    grant.securityProfile = challenge.securityProfile;
}

size_t signAndEncode(
    const blinker::DeviceKey& key,
    blinker::local_access::GrantView& grant,
    uint8_t* authenticator,
    uint8_t* output,
    size_t outputSize) {
    CHECK_TRUE(blinker::security::computeLocalAccessGrantAuthenticator(
        key, grant,
        blinker::MutableByteSpan(
            authenticator, blinker::local_access::kAuthenticatorSize)));
    grant.authenticator = blinker::ByteView(
        authenticator, blinker::local_access::kAuthenticatorSize);
    size_t written = 0U;
    CHECK_TRUE(blinker::local_access::encodeGrant(
        grant, blinker::MutableByteSpan(output, outputSize), written));
    return written;
}

template <size_t Capacity>
blinker::local_access::ChallengeView issueAndDecode(
    blinker::LocalAccessChallengeSession<Capacity>& session,
    uint32_t now,
    uint8_t* output,
    size_t outputSize,
    size_t& written) {
    CHECK_TRUE(session.issueChallenge(
        now, blinker::MutableByteSpan(output, outputSize), written));
    blinker::local_access::ChallengeView challenge;
    CHECK_TRUE(blinker::local_access::decodeChallenge(
        blinker::ByteView(output, written), challenge));
    return challenge;
}

void testSelfChallengeGrantDeadlineAndReplay() {
    FixedDeviceKeySource keys;
    SequenceRandom random;
    blinker::SelfLocalAccessTargetPolicy policy;
    blinker::LocalAccessAuthority<2U> authority(keys, policy);
    blinker::LocalAccessChallengeSession<2U> first(authority, random);

    uint8_t recipient[blinker::local_access::kIdSize];
    fillBytes(recipient, sizeof(recipient), 0x70U);
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));

    const uint32_t start = UINT32_MAX - 1000U;
    uint8_t challengeBytes[blinker::local_access::kChallengeMaximumEncodedSize];
    size_t challengeSize = 0U;
    const blinker::local_access::ChallengeView challenge = issueAndDecode(
        first, start, challengeBytes, sizeof(challengeBytes), challengeSize);
    CHECK_TRUE(challengeSize <=
               blinker::local_access::kChallengeMaximumEncodedSize);
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(challengeBytes, challengeSize),
        "challenge"));

    uint8_t grantId[blinker::local_access::kIdSize];
    uint8_t authorizationId[blinker::local_access::kIdSize];
    uint8_t callerSessionId[blinker::local_access::kIdSize];
    blinker::local_access::GrantView grant;
    makeSelfGrant(
        challenge, grant, grantId, authorizationId, callerSessionId);
    uint8_t authenticator[blinker::local_access::kAuthenticatorSize];
    uint8_t encoded[blinker::local_access::kGrantMaximumEncodedSize];
    const size_t encodedSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    CHECK_TRUE(encodedSize <= blinker::local_access::kGrantMaximumEncodedSize);
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(authenticator, sizeof(authenticator)),
        "authenticator"));
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(encoded, encodedSize),
        "grant"));

    blinker::LocalAccessAdmission admission;
    CHECK_TRUE(first.acceptGrant(
        blinker::ByteView(encoded, encodedSize), start + 100U, admission));
    CHECK_EQ(5000U, admission.remainingMillis);
    CHECK_TRUE(!allZero(admission.sessionKey, sizeof(admission.sessionKey)));
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(admission.sessionKey, sizeof(admission.sessionKey)),
        "noise_psk"));
    CHECK_TRUE(authority.permits(
        blinker::ByteView(authorizationId, sizeof(authorizationId)),
        blinker::kAuthorizationPermissionControl, start + 4999U));

    // The receiver challenge is one-shot.
    blinker::LocalAccessAdmission replay;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        first.acceptGrant(
                 blinker::ByteView(encoded, encodedSize), start + 101U,
                 replay)
            .code());
    CHECK_TRUE(allZero(replay.sessionKey, sizeof(replay.sessionKey)));

    // Reconnect with the same authorizationId and a fresh signed challenge.
    // Its monotonic deadline remains anchored at the original accept.
    blinker::LocalAccessChallengeSession<2U> second(authority, random);
    uint8_t secondChallengeBytes[
        blinker::local_access::kChallengeMaximumEncodedSize];
    size_t secondChallengeSize = 0U;
    const blinker::local_access::ChallengeView secondChallenge =
        issueAndDecode(
            second, start + 1100U, secondChallengeBytes,
            sizeof(secondChallengeBytes), secondChallengeSize);
    grant.receiverSessionId = secondChallenge.receiverSessionId;
    grant.challenge = secondChallenge.challenge;
    grant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    ++grantId[0];
    const size_t secondGrantSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    blinker::LocalAccessAdmission resumed;
    CHECK_TRUE(second.acceptGrant(
        blinker::ByteView(encoded, secondGrantSize), start + 1200U, resumed));
    CHECK_EQ(3900U, resumed.remainingMillis);
    CHECK_TRUE(!authority.permits(
        blinker::ByteView(authorizationId, sizeof(authorizationId)),
        blinker::kAuthorizationPermissionObserve, start + 5100U));
}

void testTamperExpiryAndRevocation() {
    FixedDeviceKeySource keys;
    SequenceRandom random;
    blinker::SelfLocalAccessTargetPolicy policy;
    blinker::LocalAccessAuthority<2U> authority(keys, policy);
    blinker::LocalAccessChallengeSession<2U> session(authority, random);
    uint8_t recipient[blinker::local_access::kIdSize];
    fillBytes(recipient, sizeof(recipient), 0x70U);
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));

    uint8_t challengeBytes[blinker::local_access::kChallengeMaximumEncodedSize];
    size_t challengeSize = 0U;
    const blinker::local_access::ChallengeView challenge = issueAndDecode(
        session, 100U, challengeBytes, sizeof(challengeBytes), challengeSize);
    uint8_t grantId[blinker::local_access::kIdSize];
    uint8_t authorizationId[blinker::local_access::kIdSize];
    uint8_t callerSessionId[blinker::local_access::kIdSize];
    blinker::local_access::GrantView grant;
    makeSelfGrant(
        challenge, grant, grantId, authorizationId, callerSessionId);
    uint8_t authenticator[blinker::local_access::kAuthenticatorSize];
    uint8_t encoded[blinker::local_access::kGrantMaximumEncodedSize];
    const size_t encodedSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    encoded[encodedSize - 1U] ^= 0x01U;
    blinker::LocalAccessAdmission rejected;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        session.acceptGrant(
                   blinker::ByteView(encoded, encodedSize), 200U, rejected)
            .code());
    CHECK_TRUE(allZero(rejected.sessionKey, sizeof(rejected.sessionKey)));

    // The challenge response window is local monotonic time, not UTC.
    blinker::LocalAccessChallengeSession<2U> expired(authority, random);
    const blinker::local_access::ChallengeView expiredChallenge =
        issueAndDecode(
            expired, 1000U, challengeBytes, sizeof(challengeBytes),
            challengeSize);
    makeSelfGrant(
        expiredChallenge, grant, grantId, authorizationId, callerSessionId);
    const size_t freshSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        expired.acceptGrant(
                   blinker::ByteView(encoded, freshSize),
                   1000U + blinker::local_access::kChallengeResponseMillis,
                   rejected)
            .code());

    blinker::LocalAccessChallengeSession<2U> accepted(authority, random);
    const blinker::local_access::ChallengeView acceptedChallenge =
        issueAndDecode(
            accepted, 2000U, challengeBytes, sizeof(challengeBytes),
            challengeSize);
    makeSelfGrant(
        acceptedChallenge, grant, grantId, authorizationId,
        callerSessionId);
    const size_t acceptedSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    CHECK_TRUE(accepted.acceptGrant(
        blinker::ByteView(encoded, acceptedSize), 2100U, rejected));

    // A reboot constructs a fresh RAM-only authority. Reusing a previously
    // accepted authorizationId cannot restore a local lease.
    blinker::LocalAccessAuthority<2U> rebooted(keys, policy);
    CHECK_TRUE(rebooted.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));
    CHECK_TRUE(!rebooted.permits(
        blinker::ByteView(authorizationId, sizeof(authorizationId)),
        blinker::kAuthorizationPermissionObserve, 2101U));

    authority.updateAuthorityRevision(12U);
    CHECK_TRUE(!authority.permits(
        blinker::ByteView(authorizationId, sizeof(authorizationId)),
        blinker::kAuthorizationPermissionObserve, 2101U));
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 4U, 13U));
    CHECK_TRUE(!authority.permits(
        blinker::ByteView(authorizationId, sizeof(authorizationId)),
        blinker::kAuthorizationPermissionObserve, 2102U));
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        authority.configure(
                     blinker::ByteView(recipient, sizeof(recipient)), 3U, 13U)
            .code());
}

void testExactChildScopeAndProfileDomain() {
    FixedDeviceKeySource keys;
    SequenceRandom random;
    uint8_t childInstance[blinker::local_access::kIdSize];
    fillBytes(childInstance, sizeof(childInstance), 0xC0U);
    ExactChildPolicy policy(childInstance, "child-A");
    blinker::LocalAccessAuthority<2U> authority(keys, policy);
    blinker::LocalAccessChallengeSession<2U> session(authority, random);
    uint8_t recipient[blinker::local_access::kIdSize];
    fillBytes(recipient, sizeof(recipient), 0x70U);
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));
    uint8_t challengeBytes[blinker::local_access::kChallengeMaximumEncodedSize];
    size_t challengeSize = 0U;
    const blinker::local_access::ChallengeView challenge = issueAndDecode(
        session, 100U, challengeBytes, sizeof(challengeBytes), challengeSize);

    uint8_t grantId[blinker::local_access::kIdSize];
    uint8_t authorizationId[blinker::local_access::kIdSize];
    uint8_t callerSessionId[blinker::local_access::kIdSize];
    blinker::local_access::GrantView grant;
    makeSelfGrant(
        challenge, grant, grantId, authorizationId, callerSessionId);
    grant.targetKind = blinker::local_access::TargetKind::Child;
    grant.targetLogicalDeviceId = blinker::StringView("child-A");
    grant.targetDeviceInstanceId =
        blinker::ByteView(childInstance, sizeof(childInstance));
    grant.accessEpoch = 7U;
    grant.topologyVersion = 9U;
    uint8_t authenticator[blinker::local_access::kAuthenticatorSize];
    uint8_t encoded[blinker::local_access::kGrantMaximumEncodedSize];
    const size_t encodedSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    blinker::LocalAccessAdmission admission;
    CHECK_TRUE(session.acceptGrant(
        blinker::ByteView(encoded, encodedSize), 200U, admission));
    CHECK_EQ(blinker::local_access::TargetKind::Child,
             admission.targetKind);
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(encoded, encodedSize), "child_grant"));
    CHECK_TRUE(matchesGolden(
        blinker::ByteView(admission.sessionKey, sizeof(admission.sessionKey)),
        "child_noise_psk"));
    CHECK_EQ(7U, admission.accessEpoch);
    CHECK_EQ(9U, admission.topologyVersion);

    const blinker::ByteView setup = blinker::noise::localSecureNoisePrologue(
        blinker::noise::NoiseNnPattern::NnPsk0);
    const blinker::ByteView access =
        blinker::noise::localAccessNoisePrologue();
    CHECK_TRUE(matchesGolden(access, "noise_prologue"));
    CHECK_TRUE(!setup.empty() && !access.empty());
    CHECK_TRUE(setup.size != access.size ||
               std::memcmp(setup.data, access.data, setup.size) != 0);

    // A valid service MAC for child B is still rejected by the Hub's exact
    // current-target policy; authenticating to the Hub is not global access.
    blinker::LocalAccessChallengeSession<2U> wrongChild(authority, random);
    const blinker::local_access::ChallengeView wrongChallenge = issueAndDecode(
        wrongChild, 300U, challengeBytes, sizeof(challengeBytes),
        challengeSize);
    makeSelfGrant(
        wrongChallenge, grant, grantId, authorizationId, callerSessionId);
    grant.targetKind = blinker::local_access::TargetKind::Child;
    grant.targetLogicalDeviceId = blinker::StringView("child-B");
    grant.targetDeviceInstanceId =
        blinker::ByteView(childInstance, sizeof(childInstance));
    grant.accessEpoch = 7U;
    grant.topologyVersion = 9U;
    const size_t wrongSize = signAndEncode(
        keys.value(), grant, authenticator, encoded, sizeof(encoded));
    blinker::LocalAccessAdmission denied;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        wrongChild.acceptGrant(
                       blinker::ByteView(encoded, wrongSize), 400U, denied)
            .code());
    CHECK_TRUE(allZero(denied.sessionKey, sizeof(denied.sessionKey)));
}

void testBoundedAuthorizationCapacity() {
    FixedDeviceKeySource keys;
    SequenceRandom random;
    blinker::SelfLocalAccessTargetPolicy policy;
    blinker::LocalAccessAuthority<2U> authority(keys, policy);
    uint8_t recipient[blinker::local_access::kIdSize];
    fillBytes(recipient, sizeof(recipient), 0x70U);
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));
    uint8_t challengeBytes[blinker::local_access::kChallengeMaximumEncodedSize];
    uint8_t encoded[blinker::local_access::kGrantMaximumEncodedSize];
    uint8_t authenticator[blinker::local_access::kAuthenticatorSize];
    uint8_t grantId[blinker::local_access::kIdSize];
    uint8_t authorizationId[blinker::local_access::kIdSize];
    uint8_t callerSessionId[blinker::local_access::kIdSize];

    for (uint8_t caller = 0U; caller < 3U; ++caller) {
        blinker::LocalAccessChallengeSession<2U> session(authority, random);
        size_t challengeSize = 0U;
        const blinker::local_access::ChallengeView challenge = issueAndDecode(
            session, 100U + caller, challengeBytes, sizeof(challengeBytes),
            challengeSize);
        blinker::local_access::GrantView grant;
        makeSelfGrant(
            challenge, grant, grantId, authorizationId, callerSessionId);
        grantId[0] = static_cast<uint8_t>(grantId[0] + caller);
        authorizationId[0] =
            static_cast<uint8_t>(authorizationId[0] + caller);
        callerSessionId[0] =
            static_cast<uint8_t>(callerSessionId[0] + caller);
        size_t encodedSize = signAndEncode(
            keys.value(), grant, authenticator, encoded, sizeof(encoded));
        blinker::LocalAccessAdmission admission;
        const blinker::Result result = session.acceptGrant(
            blinker::ByteView(encoded, encodedSize), 200U + caller,
            admission);
        if (caller < 2U) {
            CHECK_TRUE(result);
        } else {
            CHECK_EQ(blinker::ErrorCode::CapacityExceeded, result.code());
            CHECK_TRUE(allZero(
                admission.sessionKey, sizeof(admission.sessionKey)));
        }
    }
}

void testExpiredLeaseTombstones() {
    FixedDeviceKeySource keys;
    SequenceRandom random;
    blinker::SelfLocalAccessTargetPolicy policy;
    blinker::LocalAccessAuthority<2U> authority(keys, policy);
    uint8_t recipient[blinker::local_access::kIdSize];
    fillBytes(recipient, sizeof(recipient), 0x70U);
    CHECK_TRUE(authority.configure(
        blinker::ByteView(recipient, sizeof(recipient)), 3U, 11U));
    blinker::LocalAccessChallengeSession<2U> session(authority, random);
    uint8_t challengeBytes[blinker::local_access::kChallengeMaximumEncodedSize];
    size_t challengeSize = 0U;
    const blinker::local_access::ChallengeView challenge = issueAndDecode(
        session, 0U, challengeBytes, sizeof(challengeBytes), challengeSize);
    uint8_t grantId[blinker::local_access::kIdSize];
    uint8_t authorizationId[blinker::local_access::kIdSize];
    uint8_t callerSessionId[blinker::local_access::kIdSize];
    blinker::local_access::GrantView grant;
    makeSelfGrant(challenge, grant, grantId, authorizationId, callerSessionId);
    grant.lifetimeMillis = 1000U;
    blinker::LocalAccessLeaseLedger<2U> ledger;
    uint32_t remaining = 0U;
    const uint32_t start = UINT32_MAX - 500U;
    CHECK_TRUE(ledger.admit(grant, start, remaining));
    // A caller with two pre-issued challenges cannot resurrect a deadline
    // by causing the first rejection to erase the expired authorization.
    for (uint32_t retry = 0U; retry < 2U; ++retry) {
        CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
            ledger.admit(grant, start + 1000U + retry, remaining).code());
        CHECK_EQ(0U, remaining);
    }
    ++authorizationId[0];
    CHECK_TRUE(ledger.admit(grant, start + 1000U, remaining));
    ++authorizationId[0];
    CHECK_EQ(blinker::ErrorCode::CapacityExceeded,
        ledger.admit(grant, start + 15999U, remaining).code());
    // Reclaim only after every response from the old challenge window is
    // stale; the service must also refuse reissuing expired authorizations.
    CHECK_TRUE(ledger.admit(grant, start + 16000U, remaining));
    CHECK_EQ(1000U, remaining);
    // Main-loop upkeep runs without receive/send. Once observed, expiry cannot
    // become live when the uint32 clock eventually returns near its start.
    ledger.poll(start + 17000U);
    CHECK_TRUE(!ledger.permits(grant.authorizationId, grant.permissions, start + 16001U));
    CHECK_TRUE(!ledger.admit(grant, start + 16001U, remaining));
    ledger.poll(start + 32000U);
    CHECK_TRUE(!ledger.permits(grant.authorizationId, grant.permissions, start + 16001U));

    uint8_t expiredGrant[blinker::local_access::kGrantMaximumEncodedSize];
    uint8_t mac[blinker::local_access::kAuthenticatorSize];
    const size_t expiredSize = signAndEncode(keys.value(), grant, mac, expiredGrant, sizeof(expiredGrant));
    session.poll(blinker::local_access::kChallengeResponseMillis);
    blinker::LocalAccessAdmission admission;
    CHECK_TRUE(!session.acceptGrant(blinker::ByteView(expiredGrant, expiredSize), 1U, admission));
}

size_t signControl(const blinker::DeviceKey& key, blinker::local_access::ControlView value,
                   uint8_t* output, size_t size) {
    uint8_t mac[32] = {};
    CHECK_TRUE(blinker::security::computeLocalAccessControlAuthenticator(key, value, blinker::MutableByteSpan(mac, sizeof(mac))));
    value.authenticator = blinker::ByteView(mac, sizeof(mac));
    size_t written = 0U;
    CHECK_TRUE(blinker::local_access::encodeControl(value, blinker::MutableByteSpan(output, size), written));
    return written;
}

void testProgressGoldenAndBounds() {
    using namespace blinker; using namespace blinker::local_access;
    DeviceKey key; fillBytes(key.bytes, sizeof(key.bytes), 1U);
    uint8_t recipient[16], boot[16], nonce[16], proof[16], mac[32], wire[112];
    fillBytes(recipient, 16, 0x70); fillBytes(boot, 16, 0x80); fillBytes(nonce, 16, 0xc0); fillBytes(proof, 16, 0xa0);
    ProgressView value;
    value.recipientDeviceInstanceId = ByteView(recipient, 16); value.deviceKeyVersion = 3;
    value.controlSessionId = ByteView(boot, 16); value.requestNonce = ByteView(nonce, 16);
    value.sequence = 2; value.authorityRevision = 11;
    const char* names[] = {"progress_query", "progress_sync", "progress_refresh"};
    for (size_t i = 0U; i < 3U; ++i) {
        value.kind = i == 0 ? ProgressKind::Query : ProgressKind::Status;
        value.state = i == 0 ? ProgressState::Query : i == 1 ? ProgressState::Synchronized : ProgressState::Refresh;
        CHECK_TRUE(security::computeLocalAccessProgressAuthenticator(key, value, ByteView(proof, 16), MutableByteSpan(mac, 32)));
        value.authenticator = ByteView(mac, 32); size_t size = 0;
        CHECK_TRUE(encodeProgress(value, MutableByteSpan(wire, 112), size));
        CHECK_TRUE(matchesGolden(ByteView(wire, size), names[i]));
        ProgressView decoded;
        CHECK_TRUE(decodeProgress(ByteView(wire, size), decoded));
        CHECK_TRUE(security::verifyLocalAccessProgressAuthenticator(key, decoded, ByteView(proof, 16)));
        proof[0] ^= 1; CHECK_TRUE(!security::verifyLocalAccessProgressAuthenticator(key, decoded, ByteView(proof, 16))); proof[0] ^= 1;
        for (size_t n = 0; n < size; ++n) CHECK_TRUE(!decodeProgress(ByteView(wire, n), decoded));
        wire[size] = 0; CHECK_TRUE(!decodeProgress(ByteView(wire, size + 1), decoded));
        ControlView control; CHECK_TRUE(!decodeControl(ByteView(wire, size), control));
    }
    value.sequence = value.authorityRevision = value.deviceKeyVersion = UINT32_MAX;
    size_t maximum = 0;
    CHECK_TRUE(encodeProgress(value, MutableByteSpan(wire, sizeof(wire)), maximum));
    CHECK_EQ(104U, maximum); // Same 112 B inbox, no carrier-size increase.
    value.version = 2U; value.securityProfile = SecurityProfile::PlainHmacSha256;
    CHECK_TRUE(encodeProgress(value, MutableByteSpan(wire, sizeof(wire)), maximum));
    CHECK_EQ(105U, maximum);
    value.deviceKeyVersion = 3; value.sequence = 2; value.authorityRevision = 11;
    for (size_t i = 0U; i < 2U; ++i) {
        value.kind = i == 0U ? ProgressKind::Query : ProgressKind::Status;
        value.state = i == 0U ? ProgressState::Query : ProgressState::Synchronized;
        CHECK_TRUE(security::computeLocalAccessProgressAuthenticator(key, value, ByteView(proof,16), MutableByteSpan(mac,32)));
        CHECK_TRUE(encodeProgress(value, MutableByteSpan(wire,sizeof(wire)), maximum));
        CHECK_TRUE(matchesGolden(ByteView(wire,maximum), i == 0U ? "plain_progress_query" : "plain_progress_sync"));
        ProgressView decoded; CHECK_TRUE(decodeProgress(ByteView(wire,maximum), decoded));
        CHECK_EQ(SecurityProfile::PlainHmacSha256, decoded.securityProfile);
        CHECK_TRUE(security::verifyLocalAccessProgressAuthenticator(key,decoded,ByteView(proof,16)));
        decoded.securityProfile = SecurityProfile::NoiseNnPsk0AesGcmSha256;
        CHECK_TRUE(!security::verifyLocalAccessProgressAuthenticator(key,decoded,ByteView(proof,16)));
        for (size_t n = 0; n < maximum; ++n) CHECK_TRUE(!decodeProgress(ByteView(wire,n),decoded));
    }
    value.version = 1U; CHECK_TRUE(!validateProgressUnsigned(value));
    value.version = 2U;
    memset(mac, 0, sizeof(mac));
    CHECK_TRUE(!encodeProgress(value, MutableByteSpan(wire, sizeof(wire)), maximum));
}

void testAuthenticatedControlAndPreciseRetirement() {
    using namespace blinker;
    using namespace blinker::local_access;
    // Routing policy is independently tested above; this fixture permits two
    // valid child targets to test authorization retirement, not ACL issuance.
    class Targets final : public ILocalAccessTargetPolicy {
        Result admit(const GrantView&) override { return Result::success(); }
    } targets;
    FixedDeviceKeySource keys;
    SequenceRandom random;
    LocalAccessAuthority<2U> authority(keys, targets);
    LocalAccessControlReceiver<2U> receiver(authority, keys, random);
    uint8_t instance[16]; fillBytes(instance, sizeof(instance), 0x70U);
    uint8_t proofBytes[16]; fillBytes(proofBytes, sizeof(proofBytes), 0xa0U);
    const ByteView proof(proofBytes, sizeof(proofBytes));
    uint8_t helloWire[112], commandWire[112], ackWire[112], savedAck[112];
    size_t helloSize = 0U, ackSize = 0U;
    CHECK_TRUE(receiver.begin(ByteView(instance, sizeof(instance)), 3U));
    CHECK_TRUE(!authority.configured());
    CHECK_TRUE(!receiver.begin(ByteView(instance, sizeof(instance)), 3U));
    CHECK_TRUE(!receiver.hello(ByteView(), MutableByteSpan(helloWire, sizeof(helloWire)), helloSize));
    CHECK_EQ(0U, helloSize);
    CHECK_TRUE(receiver.hello(proof, MutableByteSpan(helloWire, sizeof(helloWire)), helloSize));
    CHECK_TRUE(matchesGolden(ByteView(helloWire, helloSize), "control_hello"));
    ControlView command;
    CHECK_TRUE(decodeControl(ByteView(helloWire, helloSize), command));
    CHECK_TRUE(security::verifyLocalAccessControlAuthenticator(keys.value(), command, proof));
    CHECK_TRUE(!security::verifyLocalAccessControlAuthenticator(keys.value(), command));
    proofBytes[0] ^= 1U;
    CHECK_TRUE(!security::verifyLocalAccessControlAuthenticator(keys.value(), command, proof));
    proofBytes[0] ^= 1U;
    command.kind = ControlKind::Command;
    command.operation = ControlOperation::Enable;
    command.sequence = 1U;
    command.authorityRevision = 11U;
    size_t commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(matchesGolden(ByteView(commandWire, commandSize), "control_enable"));
    // Truncation/malformed wire, tampering and insufficient output cannot apply.
    for (size_t size = 0U; size < commandSize; ++size) {
        CHECK_TRUE(!receiver.accept(ByteView(commandWire, size), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
        CHECK_EQ(0U, ackSize);
    }
    commandWire[commandSize - 1U] ^= 1U;
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    commandWire[commandSize - 1U] ^= 1U;
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(commandWire, sizeof(commandWire)), ackSize));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, 1U), ackSize));
    CHECK_TRUE(!authority.configured());
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(authority.enabled());
    ControlView ack;
    CHECK_TRUE(decodeControl(ByteView(ackWire, ackSize), ack));
    CHECK_EQ(ControlKind::Ack, ack.kind);
    CHECK_TRUE(security::verifyLocalAccessControlAuthenticator(keys.value(), ack));

    LocalAccessChallengeSession<2U> sessionA(authority, random), sessionB(authority, random), lateA(authority, random);
    uint8_t challengeA[96], challengeB[96], challengeLate[96], grantWire[272], delayedWire[272];
    uint8_t grantId[16], authA[16], caller[16], authB[16], mac[32];
    size_t size = 0U;
    GrantView grant;
    auto challenge = issueAndDecode(sessionA, 100U, challengeA, sizeof(challengeA), size);
    makeSelfGrant(challenge, grant, grantId, authA, caller);
    grant.targetKind = TargetKind::Child; grant.targetLogicalDeviceId = StringView("child-A");
    grant.accessEpoch = 7U; grant.topologyVersion = 9U;
    size_t grantSize = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    LocalAccessAdmission admission;
    CHECK_TRUE(sessionA.acceptGrant(ByteView(grantWire, grantSize), 101U, admission));
    challenge = issueAndDecode(lateA, 102U, challengeLate, sizeof(challengeLate), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    const size_t delayedSize = signAndEncode(keys.value(), grant, mac, delayedWire, sizeof(delayedWire));
    challenge = issueAndDecode(sessionB, 103U, challengeB, sizeof(challengeB), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    fillBytes(authB, sizeof(authB), 0x40U); grant.authorizationId = ByteView(authB, sizeof(authB));
    grant.targetLogicalDeviceId = StringView("child-B"); ++caller[0];
    grantSize = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    CHECK_TRUE(sessionB.acceptGrant(ByteView(grantWire, grantSize), 104U, admission));
    command.sequence = 2U; command.operation = ControlOperation::Retire; command.authorizationId = ByteView(authA, sizeof(authA));
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(matchesGolden(ByteView(commandWire, commandSize), "control_retire"));
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(matchesGolden(ByteView(ackWire, ackSize), "control_retire_ack"));
    memcpy(savedAck, ackWire, ackSize);
    CHECK_TRUE(!authority.permits(ByteView(authA, sizeof(authA)), 1U, 105U));
    CHECK_TRUE(authority.permits(ByteView(authB, sizeof(authB)), 3U, 105U));
    CHECK_TRUE(!lateA.acceptGrant(ByteView(delayedWire, delayedSize), 106U, admission));
    CHECK_TRUE(allZero(admission.sessionKey, sizeof(admission.sessionKey)));
    // Fresh B challenge survives an exact retransmission of A's retirement.
    challenge = issueAndDecode(sessionB, 107U, challengeB, sizeof(challengeB), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    grantSize = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    const uint32_t generation = authority.challengeGeneration();
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(memcmp(savedAck, ackWire, ackSize) == 0);
    CHECK_EQ(generation, authority.challengeGeneration());
    CHECK_TRUE(sessionB.acceptGrant(ByteView(grantWire, grantSize), 108U, admission));
    CHECK_EQ(4996U, admission.remainingMillis);
    // Same sequence with another target, gaps and ACK reflection are rejected.
    command.authorizationId = ByteView(authB, sizeof(authB));
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    command.sequence = 4U;
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    command.kind = ControlKind::Ack; command.sequence = 3U;
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(authority.permits(ByteView(authB, sizeof(authB)), 3U, 109U));

    // Retirement also blocks a grant never installed in the ledger. Capacity
    // is immediately reusable; no revoked-ID table or timed global blackout.
    challenge = issueAndDecode(lateA, 110U, challengeLate, sizeof(challengeLate), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    authA[0] = 0x60U; grant.authorizationId = ByteView(authA, sizeof(authA)); grant.targetLogicalDeviceId = StringView("child-A");
    grantSize = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    command.kind = ControlKind::Command; command.authorizationId = ByteView(authA, sizeof(authA));
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(!lateA.acceptGrant(ByteView(grantWire, grantSize), 111U, admission));
    challenge = issueAndDecode(sessionA, 112U, challengeA, sizeof(challengeA), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    ++authA[0]; // Fresh online authorization, not reissuing a retired ID.
    grantSize = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    CHECK_TRUE(sessionA.acceptGrant(ByteView(grantWire, grantSize), 113U, admission));
    CHECK_TRUE(authority.permits(ByteView(authB, sizeof(authB)), 3U, 113U));
    CHECK_TRUE(receiver.hello(proof, MutableByteSpan(helloWire, sizeof(helloWire)), helloSize));
    ControlView progress;
    CHECK_TRUE(decodeControl(ByteView(helloWire, helloSize), progress));
    CHECK_EQ(3U, progress.sequence); CHECK_EQ(11U, progress.authorityRevision);
    CHECK_TRUE(authority.permits(ByteView(authB, sizeof(authB)), 3U, 114U));

    command.sequence = 4U; command.operation = ControlOperation::Disable; command.authorizationId = ByteView();
    command.authorityRevision = 12U;
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(!authority.enabled()); CHECK_EQ(12U, authority.authorityRevision());
    CHECK_TRUE(!authority.permits(ByteView(authB, sizeof(authB)), 1U, 115U));
    command.sequence = 5U; command.operation = ControlOperation::Enable;
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    command.authorityRevision = 13U;
    commandSize = signControl(keys.value(), command, commandWire, sizeof(commandWire));
    CHECK_TRUE(receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    receiver.clear();
    CHECK_TRUE(receiver.begin(ByteView(instance, sizeof(instance)), 3U));
    CHECK_TRUE(!receiver.accept(ByteView(commandWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_TRUE(!authority.configured());
    receiver.clear();
    CHECK_TRUE(authority.configure(ByteView(instance, sizeof(instance)), 3U, 11U));
    // A reset may borrow the previous authority's identity view. Copy it
    // before clearing that authority, rather than silently storing zeros.
    CHECK_TRUE(receiver.begin(authority.recipientDeviceInstanceId(), 3U));
    CHECK_TRUE(receiver.hello(proof, MutableByteSpan(helloWire, sizeof(helloWire)), helloSize));
    CHECK_TRUE(decodeControl(ByteView(helloWire, helloSize), progress));
    CHECK_TRUE(memcmp(progress.recipientDeviceInstanceId.data, instance, sizeof(instance)) == 0);
}

void testPhysicalProfileDeclaration() {
    using namespace blinker; using namespace blinker::local_access;
    FixedDeviceKeySource keys; SequenceRandom random; SelfLocalAccessTargetPolicy policy;
    LocalAccessAuthority<1U> authority(keys,policy);
    LocalAccessControlReceiver<1U,SecurityProfile::PlainHmacSha256> control(authority,keys,random);
    uint8_t instance[16], nonce[16], proof[16], wire[112], mac[32]; size_t written = 0U;
    fillBytes(instance,16,0x10); fillBytes(nonce,16,0x20); fillBytes(proof,16,0x30);
    CHECK_TRUE(control.begin(ByteView(instance,16),3));
    CHECK_TRUE(control.query(ByteView(nonce,16),ByteView(proof,16),MutableByteSpan(wire,sizeof(wire)),written));
    ProgressView query, decoded; CHECK_TRUE(decodeProgress(ByteView(wire,written),query));
    CHECK_EQ(2U,query.version); CHECK_EQ(SecurityProfile::PlainHmacSha256,query.securityProfile);
    query.kind = ProgressKind::Status; query.state = ProgressState::Pending;
    for (size_t i = 0U; i < 3U; ++i) {
        if (i == 1U) query.securityProfile = SecurityProfile::NoiseNnPsk0AesGcmSha256;
        if (i == 2U) query.version = 1U;
        CHECK_TRUE(security::computeLocalAccessProgressAuthenticator(keys.value(),query,ByteView(proof,16),MutableByteSpan(mac,32)));
        query.authenticator = ByteView(mac,32);
        CHECK_TRUE(encodeProgress(query,MutableByteSpan(wire,sizeof(wire)),written));
        const Result accepted = control.progress(ByteView(wire,written),ByteView(nonce,16),ByteView(proof,16),decoded);
        CHECK_EQ(i == 0U,accepted.ok());
    }
}

void testPlainGoldenAndModeIsolation() {
    using namespace blinker;
    using namespace blinker::local_access;
    FixedDeviceKeySource keys; SequenceRandom random; SelfLocalAccessTargetPolicy policy;
    LocalAccessAuthority<2U> authority(keys, policy);
    LocalAccessChallengeSession<2U> plain(authority, random, SecurityProfile::PlainHmacSha256);
    uint8_t instance[16], wire[96], encoded[272], mac[32], grantId[16], authId[16], callerId[16];
    fillBytes(instance, sizeof(instance), 0x70); CHECK_TRUE(authority.configure(ByteView(instance,16), 3, 11));
    size_t size = 0U;
    const ChallengeView challenge = issueAndDecode(plain, 100U, wire, sizeof(wire), size);
    CHECK_TRUE(matchesGolden(ByteView(wire,size), "plain_challenge"));
    GrantView grant; makeSelfGrant(challenge, grant, grantId, authId, callerId);
    size = signAndEncode(keys.value(), grant, mac, encoded, sizeof(encoded));
    CHECK_TRUE(matchesGolden(ByteView(encoded,size), "plain_grant"));
    CHECK_TRUE(matchesGolden(ByteView(mac,32), "plain_authenticator"));
    LocalAccessAdmission admission;
    CHECK_TRUE(plain.acceptGrant(ByteView(encoded,size), 101, admission));
    CHECK_TRUE(matchesGolden(ByteView(admission.sessionKey,32), "plain_session_key"));
    uint8_t proof[32], proofWire[37]; size_t proofSize = 0;
    for (uint8_t kind : {kCallerProofKind, kRecipientProofKind}) {
        CHECK_TRUE(security::computeLocalAccessPlainProof(ByteView(admission.sessionKey,32), ByteView(mac,32), kind, MutableByteSpan(proof,32)));
        CHECK_TRUE(encodePlainProof(kind, ByteView(proof,32), MutableByteSpan(proofWire,37), proofSize));
        CHECK_EQ(37U, proofSize);
        CHECK_TRUE(matchesGolden(ByteView(proofWire,37), kind == kCallerProofKind ? "plain_caller_proof" : "plain_recipient_proof"));
        ByteView decoded;
        CHECK_TRUE(decodePlainProof(ByteView(proofWire,37), kind, decoded));
        CHECK_TRUE(constantTimeEqual(ByteView(proof,32), decoded));
        CHECK_TRUE(!decodePlainProof(ByteView(proofWire,37), kind == kCallerProofKind ? kRecipientProofKind : kCallerProofKind, decoded));
        for (size_t n = 0; n < 37; ++n) CHECK_TRUE(!decodePlainProof(ByteView(proofWire,n),kind,decoded));
    }
    uint8_t zero[32] = {};
    CHECK_TRUE(!security::computeLocalAccessPlainProof(ByteView(zero,32), ByteView(mac,32),
        kCallerProofKind, MutableByteSpan(proof,32)));
    CHECK_TRUE(allZero(proof,32));
    CHECK_TRUE(!security::computeLocalAccessPlainProof(ByteView(admission.sessionKey,32), ByteView(zero,32),
        kRecipientProofKind, MutableByteSpan(proof,32)));
    CHECK_TRUE(allZero(proof,32));
    CHECK_TRUE(!plain.acceptGrant(ByteView(encoded,size), 102, admission)); // Consumed challenge.
    // Same authority/authorization cannot change security scope, even with a valid new MAC.
    LocalAccessChallengeSession<2U> noise(authority, random);
    const ChallengeView second = issueAndDecode(noise, 102, wire, sizeof(wire), size);
    makeSelfGrant(second, grant, grantId, authId, callerId);
    size = signAndEncode(keys.value(), grant, mac, encoded, sizeof(encoded));
    CHECK_EQ(ErrorCode::StateConflict, noise.acceptGrant(ByteView(encoded,size),103,admission).code());
    // A properly signed Noise grant cannot answer a fresh Plain challenge.
    const ChallengeView third = issueAndDecode(plain, 104, wire, sizeof(wire), size);
    makeSelfGrant(third, grant, grantId, authId, callerId);
    grant.securityProfile = SecurityProfile::NoiseNnPsk0AesGcmSha256;
    size = signAndEncode(keys.value(), grant, mac, encoded, sizeof(encoded));
    CHECK_EQ(ErrorCode::AuthenticationRequired, plain.acceptGrant(ByteView(encoded,size),105,admission).code());
    grant.securityProfile = static_cast<SecurityProfile>(3U);
    CHECK_TRUE(!validateGrantUnsigned(grant));
}

template <blinker::local_access::SecurityProfile Profile>
void testSingleSlotScopeHandover() {
    using namespace blinker;
    using namespace blinker::local_access;
    FixedDeviceKeySource keys; SequenceRandom random; SelfLocalAccessTargetPolicy policy;
    LocalAccessAuthority<1U> authority(keys, policy);
    LocalAccessControlReceiver<1U, Profile> receiver(authority, keys, random);
    uint8_t instance[16], proof[16], controlWire[112], ackWire[112], savedAck[112];
    fillBytes(instance, 16, 0x70U); fillBytes(proof, 16, 0xa0U);
    CHECK_TRUE(receiver.begin(ByteView(instance, 16), 3U));
    size_t size = 0U, ackSize = 0U;
    CHECK_TRUE(receiver.hello(ByteView(proof, 16), MutableByteSpan(controlWire, sizeof(controlWire)), size));
    ControlView command; CHECK_TRUE(decodeControl(ByteView(controlWire, size), command));
    command.kind = ControlKind::Command; command.operation = ControlOperation::Enable;
    command.sequence = 1U; command.authorityRevision = 11U;
    size = signControl(keys.value(), command, controlWire, sizeof(controlWire));
    CHECK_TRUE(receiver.accept(ByteView(controlWire, size), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    LocalAccessChallengeSession<1U> first(authority, random, Profile), late(authority, random, Profile), next(authority, random, Profile);
    uint8_t challengeWire[96], lateChallenge[96], grantWire[272], lateWire[272], mac[32], grantId[16], authId[16], caller[16], oldAuth[16];
    GrantView grant; LocalAccessAdmission admission;
    auto challenge = issueAndDecode(first, 100U, challengeWire, sizeof(challengeWire), size);
    makeSelfGrant(challenge, grant, grantId, authId, caller); memcpy(oldAuth, authId, sizeof(oldAuth));
    size = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    CHECK_TRUE(first.acceptGrant(ByteView(grantWire, size), 101U, admission));
    challenge = issueAndDecode(late, 102U, lateChallenge, sizeof(lateChallenge), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    const size_t lateSize = signAndEncode(keys.value(), grant, mac, lateWire, sizeof(lateWire));
    challenge = issueAndDecode(next, 103U, challengeWire, sizeof(challengeWire), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    ++authId[0]; grant.permissions = 1U;
    size = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    CHECK_EQ(ErrorCode::CapacityExceeded, next.acceptGrant(ByteView(grantWire, size), 104U, admission).code());
    CHECK_TRUE(allZero(admission.sessionKey, sizeof(admission.sessionKey)));
    command.operation = ControlOperation::Retire; command.sequence = 2U; command.authorizationId = ByteView(oldAuth, 16);
    const size_t commandSize = signControl(keys.value(), command, controlWire, sizeof(controlWire));
    CHECK_TRUE(!receiver.accept(ByteView(controlWire, commandSize), MutableByteSpan(ackWire, 1U), ackSize));
    CHECK_TRUE(authority.permits(ByteView(oldAuth, 16), 3U, 105U));
    CHECK_TRUE(receiver.accept(ByteView(controlWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    memcpy(savedAck, ackWire, ackSize);
    CHECK_TRUE(!authority.permits(ByteView(oldAuth, 16), 1U, 106U));
    CHECK_TRUE(!late.acceptGrant(ByteView(lateWire, lateSize), 106U, admission));
    challenge = issueAndDecode(next, 107U, challengeWire, sizeof(challengeWire), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    size = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    const uint32_t generation = authority.challengeGeneration();
    CHECK_TRUE(receiver.accept(ByteView(controlWire, commandSize), MutableByteSpan(ackWire, sizeof(ackWire)), ackSize));
    CHECK_EQ(generation, authority.challengeGeneration()); CHECK_TRUE(memcmp(savedAck, ackWire, ackSize) == 0);
    CHECK_TRUE(next.acceptGrant(ByteView(grantWire, size), 108U, admission));
    CHECK_TRUE(authority.permits(ByteView(authId, 16), 1U, 109U));
    CHECK_TRUE(!authority.permits(ByteView(authId, 16), 2U, 109U));
    challenge = issueAndDecode(next, 208U, challengeWire, sizeof(challengeWire), size);
    grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
    size = signAndEncode(keys.value(), grant, mac, grantWire, sizeof(grantWire));
    CHECK_TRUE(next.acceptGrant(ByteView(grantWire, size), 209U, admission));
    CHECK_EQ(4899U, admission.remainingMillis); // Neither reconnect nor duplicate Retire refunds time.
}

} // namespace

int main() {
    testPhysicalProfileDeclaration();
    testSelfChallengeGrantDeadlineAndReplay();
    testTamperExpiryAndRevocation();
    testExactChildScopeAndProfileDomain();
    testBoundedAuthorizationCapacity();
    testExpiredLeaseTombstones();
    testAuthenticatedControlAndPreciseRetirement();
    testProgressGoldenAndBounds();
    testPlainGoldenAndModeIsolation();
    testSingleSlotScopeHandover<blinker::local_access::SecurityProfile::PlainHmacSha256>();
    testSingleSlotScopeHandover<blinker::local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256>();
    std::cout << "Local access tests passed"
              << " (lease=" << sizeof(blinker::LocalAccessLease)
              << ", admission=" << sizeof(blinker::LocalAccessAdmission)
              << ", ledger2="
              << sizeof(blinker::LocalAccessLeaseLedger<2U>)
              << ", authority2="
              << sizeof(blinker::LocalAccessAuthority<2U>)
              << ", challengeSession="
              << sizeof(blinker::LocalAccessChallengeSession<2U>)
              << ", controlReceiver=" << sizeof(blinker::LocalAccessControlReceiver<2U>)
              << ")" << std::endl;
    return 0;
}
