#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/runtime/GatewayProofCoordinator.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Gateway proof check failed at line "                \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)
#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    MemoryBlob() : bytes(), size(0U), present(false) {}
    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) return blinker::Result::failure(
            blinker::ErrorCode::NotFound);
        if (output.size < size) return blinker::Result::failure(
            blinker::ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView value) override {
        CHECK_TRUE(value.size <= sizeof(bytes));
        std::memset(bytes, 0, sizeof(bytes));
        std::memcpy(bytes, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }
    blinker::Result clear() override {
        std::memset(bytes, 0, sizeof(bytes));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }
    uint8_t bytes[blinker::GatewayAccessStore::serializedSize];
    size_t size;
    bool present;
};

class FixedClock final : public blinker::IClock {
public:
    FixedClock() : monotonic(100U), unixSeconds(1000U), trusted(true) {}
    uint32_t monotonicMillis() const override { return monotonic; }
    bool unixTime(uint64_t& output) const override {
        if (!trusted) return false;
        output = unixSeconds;
        return true;
    }
    uint32_t monotonic;
    uint64_t unixSeconds;
    bool trusted;
};

class FakeChildSession {
public:
    FakeChildSession()
        : stateValue(blinker::GatewayChildSessionState::Idle),
          matchedVersion(0U), authenticatedVersion(0U), startCalls(0U), connectCalls(0U),
          disconnectCalls(0U),
          attempt(0U), synchronousDisconnect(false), blockDisconnect(false),
          connectResult(blinker::Result::success()) {}
    blinker::GatewayChildSessionState state() const { return stateValue; }
    bool secure() const {
        return stateValue == blinker::GatewayChildSessionState::Secure;
    }
    uint32_t matchedPresenceVersion() const { return matchedVersion; }
    uint32_t authenticatedCredentialVersion() const {
        return secure() ? authenticatedVersion : 0U;
    }
    uint32_t attemptId() const { return attempt; }
    blinker::Result start() {
        ++startCalls;
        stateValue = blinker::GatewayChildSessionState::Idle;
        return blinker::Result::success();
    }
    blinker::Result connect() {
        ++connectCalls;
        if (connectResult) {
            ++attempt;
            stateValue = blinker::GatewayChildSessionState::Connecting;
        }
        return connectResult;
    }
    blinker::Result disconnect() {
        ++disconnectCalls;
        if (blockDisconnect) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        stateValue = synchronousDisconnect
            ? blinker::GatewayChildSessionState::Idle
            : blinker::GatewayChildSessionState::Disconnecting;
        return blinker::Result::success();
    }
    blinker::GatewayChildSessionState stateValue;
    uint32_t matchedVersion;
    uint32_t authenticatedVersion;
    uint32_t startCalls;
    uint32_t connectCalls;
    uint32_t disconnectCalls;
    uint32_t attempt;
    bool synchronousDisconnect;
    bool blockDisconnect;
    blinker::Result connectResult;
};

void fill(uint8_t* output, size_t size, uint8_t value) {
    std::memset(output, value, size);
}

blinker::GatewayAccessRecord accessRecord() {
    blinker::GatewayAccessRecord value;
    fill(value.operationId, sizeof(value.operationId), 0x11U);
    fill(value.envelopeDigest, sizeof(value.envelopeDigest), 0x21U);
    fill(value.accessMaterialDigest,
         sizeof(value.accessMaterialDigest), 0x31U);
    const char child[] = "child-1";
    value.childLogicalDeviceIdSize = sizeof(child) - 1U;
    std::memcpy(value.childLogicalDeviceId, child, sizeof(child) - 1U);
    fill(value.childDeviceInstanceId,
         sizeof(value.childDeviceInstanceId), 0x41U);
    value.accessEpoch = 5U;
    fill(value.controllerId, sizeof(value.controllerId), 0x51U);
    value.credentialVersion = 6U;
    value.permissions = blinker::gateway::kGatewayPermissions;
    value.expiresAtUnixSeconds = 5000U;
    fill(value.secret, sizeof(value.secret), 0x61U);
    value.activePresence.version = 7U;
    fill(value.activePresence.key, sizeof(value.activePresence.key), 0x71U);
    return value;
}

struct EncodedCommand {
    uint8_t bytes[blinker::gateway::kGatewayProofCommandMaximumEncodedSize];
    size_t size;
};

EncodedCommand command(
    const blinker::GatewayAccessRecord& access,
    uint32_t topologyVersion = 3U,
    uint64_t expiresAt = 1100U) {
    uint8_t nonce[blinker::gateway::kProofNonceSize];
    fill(nonce, sizeof(nonce), 0x81U);
    blinker::gateway::GatewayProofCommandView value;
    value.operationId = blinker::ByteView(
        access.operationId, sizeof(access.operationId));
    value.topologyVersion = topologyVersion;
    value.childLogicalDeviceId = blinker::StringView(
        access.childLogicalDeviceId, access.childLogicalDeviceIdSize);
    value.childDeviceInstanceId = blinker::ByteView(
        access.childDeviceInstanceId, sizeof(access.childDeviceInstanceId));
    value.accessEpoch = access.accessEpoch;
    value.gatewayCredentialVersion = access.credentialVersion;
    value.presenceKeyVersion = access.activePresence.version;
    value.accessMaterialDigest = blinker::ByteView(
        access.accessMaterialDigest, sizeof(access.accessMaterialDigest));
    value.accessStorageRevision = access.deliveryRevision;
    value.proofNonce = blinker::ByteView(nonce, sizeof(nonce));
    value.expiresAtUnixSeconds = expiresAt;
    EncodedCommand encoded = {};
    CHECK_TRUE(blinker::gateway::encodeGatewayProofCommand(
        value, blinker::MutableByteSpan(encoded.bytes, sizeof(encoded.bytes)),
        encoded.size));
    return encoded;
}

struct Fixture {
    MemoryBlob blob;
    blinker::GatewayAccessStore store;
    FixedClock clock;
    FakeChildSession child;
    blinker::BasicGatewayChildConnection<FakeChildSession> connection;
    blinker::BasicGatewayProofCoordinator<FakeChildSession> proof;
    blinker::GatewayAccessRecord access;
    EncodedCommand request;

    Fixture() : blob(), store(blob), clock(), child(), connection(child), proof(child, connection, store, clock),
                access(accessRecord()), request() {
        connection.allowAcquisition(true);
        blinker::GatewayAccessCommitOutcome outcome;
        CHECK_TRUE(store.commit(access, outcome));
        access.deliveryRevision = outcome.storageRevision;
        request = command(access);
    }
    ~Fixture() { blinker::clearGatewayAccessRecord(access); }
    void start() {
        CHECK_TRUE(!proof.continuesCommand(blinker::ByteView(request.bytes, request.size)));
        CHECK_TRUE(proof.handleCommand(blinker::ByteView(request.bytes, request.size)));
        CHECK_TRUE(proof.continuesCommand(blinker::ByteView(request.bytes, request.size)));
        blinker::GatewayAccessRecord wrong = access;
        ++wrong.accessEpoch;
        const EncodedCommand other = command(wrong);
        CHECK_TRUE(!proof.continuesCommand(blinker::ByteView(other.bytes, other.size)));
        blinker::clearGatewayAccessRecord(wrong);
        proof.poll();
        CHECK_EQ(1U, child.connectCalls);
    }
    void authenticate() {
        child.matchedVersion = access.activePresence.version;
        child.authenticatedVersion = access.credentialVersion;
        child.stateValue = blinker::GatewayChildSessionState::Secure;
        proof.poll();
    }
    blinker::gateway::GatewayProofStatus status() const {
        blinker::gateway::GatewayProofResultView result;
        CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(proof.pendingResult(), result));
        return result.status;
    }
};

void testTerminalResultDoesNotOwnTheConnection() {
    Fixture f;
    f.start();
    f.authenticate();
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Secure, f.status());
    CHECK_EQ(1U, f.child.disconnectCalls);
    CHECK_TRUE(f.proof.busy()); // disconnect accepted, not yet completed
    uint8_t saved[blinker::gateway::kGatewayProofResultMaximumEncodedSize];
    const size_t size = f.proof.pendingResult().size;
    std::memcpy(saved, f.proof.pendingResult().data, size);
    f.proof.markResultPublished();
    EncodedCommand next = command(f.access, 4U);
    CHECK_EQ(blinker::ErrorCode::StateConflict, f.proof.handleCommand(
        blinker::ByteView(next.bytes, next.size)).code());
    f.clock.monotonic += 30000U;
    f.proof.poll();
    CHECK_EQ(1U, f.child.disconnectCalls); // no duplicate request while disconnecting
    f.child.stateValue = blinker::GatewayChildSessionState::Idle;
    f.proof.poll();
    CHECK_TRUE(!f.proof.busy());
    CHECK_EQ(1U, f.child.connectCalls); // published result is not new demand

    // Another user of the shared session starts a new connection. Exact replay,
    // publish acknowledgement and cache reset must not disconnect that owner.
    CHECK_TRUE(f.child.connect());
    f.child.stateValue = blinker::GatewayChildSessionState::Secure;
    CHECK_TRUE(f.proof.handleCommand(blinker::ByteView(f.request.bytes, f.request.size)));
    CHECK_EQ(size, f.proof.pendingResult().size);
    CHECK_EQ(0, std::memcmp(saved, f.proof.pendingResult().data, size));
    f.proof.poll();
    f.proof.markResultPublished();
    f.proof.reset();
    CHECK_EQ(1U, f.child.disconnectCalls);
    CHECK_TRUE(f.child.secure());
}

void testSharedConnectionBarrier() {
    FakeChildSession child;
    child.stateValue = blinker::GatewayChildSessionState::Stopped;
    blinker::BasicGatewayChildConnection<FakeChildSession> connection(child);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(0U, child.startCalls);
    connection.allowAcquisition(true);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(1U, child.startCalls);
    CHECK_EQ(1U, child.connectCalls);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(1U, child.connectCalls); // No second wake while native work is live.
    child.stateValue = blinker::GatewayChildSessionState::Secure;
    CHECK_TRUE(connection.ensureConnected());
    const uint32_t attempt = child.attemptId();
    CHECK_TRUE(connection.retire(attempt + 1U));
    CHECK_EQ(0U, child.disconnectCalls);
    child.blockDisconnect = true;
    CHECK_TRUE(!connection.retire(attempt));
    CHECK_TRUE(connection.closing());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(2U, child.disconnectCalls); // Backpressure retries only retirement.
    child.blockDisconnect = false;
    connection.poll();
    CHECK_EQ(3U, child.disconnectCalls);
    connection.poll();
    CHECK_EQ(3U, child.disconnectCalls); // Accepted asynchronous disconnect not repeated.
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    child.stateValue = blinker::GatewayChildSessionState::Fault;
    CHECK_TRUE(!connection.retire(attempt)); // Same attempt in Fault is not free capacity.
    CHECK_TRUE(connection.closing());
    child.stateValue = blinker::GatewayChildSessionState::Idle;
    connection.allowAcquisition(false); // permit-join owns the now-quiescent radio.
    connection.poll();
    CHECK_TRUE(!connection.closing());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(1U, child.connectCalls);
    connection.allowAcquisition(true);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, connection.ensureConnected().code());
    CHECK_EQ(2U, child.connectCalls);
    CHECK_TRUE(connection.retire(attempt)); // Old cleanup never closes new generation.
    CHECK_EQ(3U, child.disconnectCalls);
}

void testResetCannotForgetNativeRetirement() {
    Fixture f;
    f.start();
    f.child.blockDisconnect = true;
    f.proof.reset(); // Wire context is gone, resource barrier outlives it.
    CHECK_TRUE(!f.proof.busy());
    CHECK_TRUE(f.connection.closing());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, f.connection.ensureConnected().code());
    CHECK_EQ(1U, f.child.connectCalls);
    f.child.blockDisconnect = false;
    f.connection.poll();
    CHECK_EQ(blinker::GatewayChildSessionState::Disconnecting, f.child.state());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, f.connection.ensureConnected().code());
    f.child.stateValue = blinker::GatewayChildSessionState::Idle;
    CHECK_EQ(blinker::ErrorCode::WouldBlock, f.connection.ensureConnected().code());
    CHECK_EQ(1U, f.child.connectCalls);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, f.connection.ensureConnected().code());
    CHECK_EQ(2U, f.child.connectCalls);
    CHECK_EQ(0U, f.proof.pendingResult().size);
}

void testRetiringSecureSessionIsNotProof() {
    Fixture f;
    f.start();
    f.child.stateValue = blinker::GatewayChildSessionState::Secure;
    f.child.matchedVersion = f.access.activePresence.version;
    f.child.authenticatedVersion = f.access.credentialVersion;
    f.child.blockDisconnect = true;
    CHECK_TRUE(!f.connection.retire(f.child.attemptId()));
    f.proof.poll();
    CHECK_TRUE(f.proof.pendingResult().empty());
    CHECK_TRUE(f.child.secure()); // Native cancellation is still backpressured.
    CHECK_EQ(1U, f.child.connectCalls);
    f.child.blockDisconnect = false;
    f.child.synchronousDisconnect = true;
    f.proof.poll();
    CHECK_TRUE(f.proof.pendingResult().empty());
    CHECK_EQ(blinker::GatewayChildSessionState::Idle, f.child.state());
    CHECK_EQ(1U, f.child.connectCalls); // No replacement in the same poll.
}

void testDisconnectBackpressureAndSynchronousCompletion() {
    Fixture f;
    f.start();
    f.child.blockDisconnect = true;
    f.authenticate();
    CHECK_TRUE(f.proof.busy());
    CHECK_EQ(1U, f.child.disconnectCalls);
    f.child.blockDisconnect = false;
    f.child.synchronousDisconnect = true;
    f.proof.poll();
    CHECK_TRUE(!f.proof.busy());
    CHECK_EQ(2U, f.child.disconnectCalls);
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Secure, f.status());
    f.proof.poll();
    CHECK_EQ(2U, f.child.disconnectCalls);
    CHECK_EQ(1U, f.child.connectCalls);
}

void testExpiryAndRevocationReleaseOwnedAttempt() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        Fixture f;
        f.start();
        if (mode == 0U) f.clock.unixSeconds = 1100U;
        else if (mode == 1U) f.clock.trusted = false;
        else CHECK_TRUE(f.store.clear());
        f.child.synchronousDisconnect = true;
        if (mode == 2U) f.authenticate();
        else f.proof.poll();
        CHECK_EQ(mode == 2U ? blinker::gateway::GatewayProofStatus::Rejected
                            : blinker::gateway::GatewayProofStatus::Expired, f.status());
        CHECK_EQ(1U, f.child.disconnectCalls);
        CHECK_TRUE(!f.proof.busy());
    }
}

void testUnownedOrReplacedAttemptIsNotDisconnected() {
    Fixture f;
    CHECK_TRUE(f.child.connect());
    f.child.matchedVersion = f.access.activePresence.version;
    f.child.authenticatedVersion = f.access.credentialVersion;
    f.child.stateValue = blinker::GatewayChildSessionState::Secure;
    f.proof.reset(); // no accepted proof task
    CHECK_EQ(0U, f.child.disconnectCalls);
    CHECK_TRUE(f.proof.handleCommand(blinker::ByteView(f.request.bytes, f.request.size)));
    f.proof.poll(); // existing matching session is evidence, not owned by proof
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Secure, f.status());
    CHECK_TRUE(!f.proof.busy());
    f.proof.reset();
    CHECK_EQ(0U, f.child.disconnectCalls);

    Fixture changed;
    changed.start();
    changed.child.stateValue = blinker::GatewayChildSessionState::Idle;
    CHECK_TRUE(changed.child.connect()); // someone else now owns a different attempt
    changed.proof.reset();
    CHECK_EQ(0U, changed.child.disconnectCalls);
}

void testRetryCadenceWrapAndDeadline() {
    Fixture f;
    f.clock.monotonic = 0xfffffff0U;
    f.start();
    f.proof.poll(); // observe the ongoing attempt
    f.child.stateValue = blinker::GatewayChildSessionState::Idle;
    f.proof.poll(); // lost link starts the bounded retry delay
    f.clock.monotonic += 999U;
    f.proof.poll();
    CHECK_EQ(1U, f.child.connectCalls);
    ++f.clock.monotonic;
    f.proof.poll();
    CHECK_EQ(2U, f.child.connectCalls);
    f.child.synchronousDisconnect = true;
    f.clock.unixSeconds = 1100U;
    f.proof.poll();
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Expired, f.status());
    CHECK_EQ(1U, f.child.disconnectCalls);
    CHECK_TRUE(!f.proof.busy());
    f.clock.monotonic += 30000U;
    f.proof.poll();
    CHECK_EQ(2U, f.child.connectCalls);
}

void testMonotonicDeadlineCannotBeExtendedByReplay() {
    Fixture f;
    f.start();
    CHECK_TRUE(f.proof.ownsChildSession());
    f.clock.monotonic += 99000U;
    f.clock.unixSeconds = 999U; // SNTP rollback cannot buy more attempt time.
    CHECK_TRUE(f.proof.handleCommand(blinker::ByteView(f.request.bytes, f.request.size)));
    f.proof.poll();
    CHECK_TRUE(f.proof.busy());
    f.child.synchronousDisconnect = true;
    f.clock.monotonic += 1000U;
    f.proof.poll();
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Expired, f.status());
    CHECK_TRUE(!f.proof.ownsChildSession());
    CHECK_TRUE(!f.proof.busy());
    CHECK_EQ(1U, f.child.disconnectCalls);
    f.proof.markResultPublished();
    CHECK_TRUE(f.proof.handleCommand(blinker::ByteView(f.request.bytes, f.request.size)));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Expired, f.status());
    f.proof.poll();
    CHECK_EQ(1U, f.child.connectCalls);
}

void testSecureAndReplay() {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord access = accessRecord();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(access, outcome));
    access.deliveryRevision = outcome.storageRevision;
    blinker::GatewayAccessRecord stored;
    CHECK_TRUE(store.load(stored));
    EncodedCommand encoded = command(stored);
    FixedClock clock;
    FakeChildSession child;
    child.synchronousDisconnect = true;
    blinker::BasicGatewayChildConnection<FakeChildSession> connection(child);
    connection.allowAcquisition(true);
    blinker::BasicGatewayProofCoordinator<FakeChildSession> proof(
        child, connection, store, clock);
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        encoded.bytes, encoded.size)));
    child.stateValue = blinker::GatewayChildSessionState::Stopped;
    proof.poll();
    CHECK_EQ(1U, child.startCalls);
    CHECK_EQ(1U, child.connectCalls);
    child.matchedVersion = stored.activePresence.version;
    child.authenticatedVersion = stored.credentialVersion;
    child.stateValue = blinker::GatewayChildSessionState::Secure;
    proof.poll();
    blinker::gateway::GatewayProofResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(
        proof.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Secure, result.status);
    proof.markResultPublished();
    CHECK_TRUE(proof.pendingResult().empty());
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        encoded.bytes, encoded.size)));
    CHECK_TRUE(!proof.pendingResult().empty());

    // Promoting the durable store cannot turn an already-secure old Method 2
    // session into proof of a new credential version.
    proof.markResultPublished();
    EncodedCommand wrongSessionVersion = command(stored, 5U, 1300U);
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        wrongSessionVersion.bytes, wrongSessionVersion.size)));
    proof.poll();
    CHECK_EQ(2U, child.connectCalls);
    child.stateValue = blinker::GatewayChildSessionState::Secure;
    child.matchedVersion = stored.activePresence.version;
    child.authenticatedVersion = stored.credentialVersion - 1U;
    proof.poll();
    CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(
        proof.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Rejected, result.status);
    CHECK_EQ(2U, child.disconnectCalls);
    proof.markResultPublished();

    // Service caps each phase at 300 s; this new command stays in that window.
    EncodedCommand next = command(stored, 6U, 1300U);
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        next.bytes, next.size)));
    CHECK_TRUE(proof.busy());
    CHECK_TRUE(proof.pendingResult().empty());
    CHECK_EQ(2U, child.disconnectCalls);
    child.stateValue = blinker::GatewayChildSessionState::Idle;
    clock.monotonic += 1000U;
    proof.poll();
    CHECK_EQ(3U, child.connectCalls);
    clock.monotonic += 1000U;
    proof.poll();
    CHECK_EQ(3U, child.connectCalls);
    proof.reset();
    CHECK_EQ(3U, child.disconnectCalls);
    CHECK_TRUE(!proof.busy());
    blinker::clearGatewayAccessRecord(access);
    blinker::clearGatewayAccessRecord(stored);
}

void testMismatchAndExpiry() {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord access = accessRecord();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(access, outcome));
    blinker::GatewayAccessRecord stored;
    CHECK_TRUE(store.load(stored));
    FixedClock clock;
    FakeChildSession child;
    blinker::BasicGatewayChildConnection<FakeChildSession> connection(child);
    connection.allowAcquisition(true);
    blinker::BasicGatewayProofCoordinator<FakeChildSession> proof(
        child, connection, store, clock);

    EncodedCommand mismatched = command(stored);
    CHECK_TRUE(!proof.handleCommand(blinker::ByteView(
        mismatched.bytes, mismatched.size - 1U)));

    EncodedCommand expired = command(stored, 3U, clock.unixSeconds);
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        expired.bytes, expired.size)));
    blinker::gateway::GatewayProofResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(
        proof.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Expired, result.status);
    CHECK_TRUE(!proof.busy());
    proof.reset();

    EncodedCommand wrongRecord = command(stored);
    wrongRecord.bytes[4U] ^= 0x01U;
    CHECK_TRUE(proof.handleCommand(blinker::ByteView(
        wrongRecord.bytes, wrongRecord.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(
        proof.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Rejected, result.status);
    CHECK_EQ(0U, child.connectCalls);
    blinker::clearGatewayAccessRecord(access);
    blinker::clearGatewayAccessRecord(stored);
}

} // namespace

int main() {
    testMonotonicDeadlineCannotBeExtendedByReplay();
    testTerminalResultDoesNotOwnTheConnection();
    testSharedConnectionBarrier();
    testResetCannotForgetNativeRetirement();
    testRetiringSecureSessionIsNotProof();
    testDisconnectBackpressureAndSynchronousCompletion();
    testExpiryAndRevocationReleaseOwnedAttempt();
    testUnownedOrReplacedAttemptIsNotDisconnected();
    testRetryCadenceWrapAndDeadline();
    testSecureAndReplay();
    testMismatchAndExpiry();
    std::cout << "Gateway proof coordinator tests passed" << std::endl;
    return 0;
}
