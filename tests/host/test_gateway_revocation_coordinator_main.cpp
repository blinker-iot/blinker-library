#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/runtime/GatewayChildRouteBridge.h>
#include <BlinkerV2/runtime/GatewayRevocationCoordinator.h>
#include <BlinkerV2/runtime/GatewayChildManagementRouter.h>
#include <BlinkerV2/runtime/GatewayManagementControlMux.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Gateway revocation check failed at line "          \
                      << __LINE__ << ": " << #condition << std::endl;         \
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
    uint8_t bytes[blinker::GatewayCredentialRenewalStore::serializedSize];
    size_t size;
    bool present;
};

class Clock final : public blinker::IClock {
public:
    Clock() : monotonic(100U), unixSeconds(1000U) {}
    uint32_t monotonicMillis() const override { return monotonic; }
    bool unixTime(uint64_t& output) const override {
        output = unixSeconds;
        return true;
    }
    uint32_t monotonic;
    uint64_t unixSeconds;
};

class ControlChannel {
public:
    ControlChannel()
        : receiver(nullptr), context(nullptr), pending(false), sends(0U),
          authenticatedAttemptId(1U),
          kind(blinker::bbp2::MessageKind::Hello), body() {}
    void setControllerControlReceiver(
        blinker::GatewayChildControlReceiver value,
        void* valueContext) {
        receiver = value;
        context = valueContext;
    }
    bool controllerControlPending() const { return pending; }
    uint32_t authenticatedControllerSessionAttemptId() const {
        return authenticatedAttemptId;
    }
    void cancelControllerControlRequest() { pending = false; }
    blinker::Result sendControllerControlRequest(
        blinker::bbp2::MessageKind valueKind,
        blinker::ByteView valueBody) {
        if (pending) return blinker::Result::failure(
            blinker::ErrorCode::StateConflict);
        kind = valueKind;
        body.assign(valueBody.data, valueBody.data + valueBody.size);
        pending = true;
        ++sends;
        return blinker::Result::success();
    }
    void respond(blinker::ByteView frame) {
        CHECK_TRUE(pending && receiver != nullptr);
        pending = false;
        receiver(context, frame);
    }
    blinker::GatewayChildControlReceiver receiver;
    void* context;
    bool pending;
    uint32_t sends;
    uint32_t authenticatedAttemptId;
    blinker::bbp2::MessageKind kind;
    std::vector<uint8_t> body;
};

void fill(uint8_t* output, size_t size, uint8_t value) {
    std::memset(output, value, size);
}

std::vector<uint8_t> responseFrame(
    blinker::bbp2::MessageKind kind,
    blinker::ByteView body) {
    std::vector<uint8_t> output(blinker::bbp2::kBaseHeaderSize + body.size);
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = blinker::bbp2::FlagIsResponse;
    header.sequence = 1U;
    header.bodyLength = static_cast<uint16_t>(body.size);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(output.data(), output.size())));
    if (!body.empty()) {
        std::memcpy(
            output.data() + blinker::bbp2::kBaseHeaderSize,
            body.data,
            body.size);
    }
    return output;
}

blinker::GatewayAccessRecord activeRecord() {
    blinker::GatewayAccessRecord value;
    fill(value.operationId, sizeof(value.operationId), 0x11U);
    fill(value.envelopeDigest, sizeof(value.envelopeDigest), 0x21U);
    fill(value.accessMaterialDigest,
         sizeof(value.accessMaterialDigest), 0x31U);
    const char child[] = "ble-child-1";
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
    fill(value.activePresence.key,
         sizeof(value.activePresence.key), 0x71U);
    return value;
}

struct EncodedCommand {
    uint8_t bytes[
        blinker::gateway::kGatewayRevocationCommandMaximumEncodedSize];
    size_t size;
};

EncodedCommand command(
    const blinker::GatewayAccessRecord& access,
    blinker::gateway::GatewayRevocationPhase phase,
    blinker::ByteView grant = blinker::ByteView(),
    blinker::ByteView receiptDigest = blinker::ByteView(),
    uint32_t topologyVersion = 8U,
    bool anyAccessStorageRevision = false) {
    blinker::gateway::GatewayRevocationCommandView value;
    value.operationId = blinker::ByteView(
        access.operationId, sizeof(access.operationId));
    value.topologyVersion = topologyVersion;
    value.childDeviceInstanceId = blinker::ByteView(
        access.childDeviceInstanceId,
        sizeof(access.childDeviceInstanceId));
    value.accessEpoch = access.accessEpoch;
    value.controllerId = blinker::ByteView(
        access.controllerId, sizeof(access.controllerId));
    value.gatewayCredentialVersion = access.credentialVersion;
    value.accessStorageRevision = anyAccessStorageRevision
        ? 0U : access.deliveryRevision;
    value.phase = phase;
    value.grant = grant;
    value.receiptDigest = receiptDigest;
    value.expiresAtUnixSeconds =
        (phase == blinker::gateway::GatewayRevocationPhase::Finalize ||
         phase == blinker::gateway::GatewayRevocationPhase::Supersede)
            ? 0U
            : 1100U;
    EncodedCommand output = {};
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
        value,
        blinker::MutableByteSpan(output.bytes, sizeof(output.bytes)),
        output.size));
    return output;
}

std::vector<uint8_t> revokeGrant(
    const blinker::GatewayAccessRecord& access,
    blinker::ByteView nonce) {
    uint8_t grantId[blinker::kControllerGrantIdSize];
    uint8_t zeroDigest[blinker::kControllerSecretDigestSize] = {};
    uint8_t signature[blinker::kControllerGrantSignatureSize];
    fill(grantId, sizeof(grantId), 0x81U);
    fill(signature, sizeof(signature), 0x91U);
    blinker::ControllerGrant grant;
    grant.operation = blinker::ControllerMutationOperation::Revoke;
    grant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    grant.deviceInstanceId = blinker::ByteView(
        access.childDeviceInstanceId,
        sizeof(access.childDeviceInstanceId));
    grant.ownershipGeneration = access.accessEpoch;
    grant.controllerId = blinker::ByteView(
        access.controllerId, sizeof(access.controllerId));
    grant.expectedCredentialVersion = access.credentialVersion;
    grant.credentialVersion = access.credentialVersion;
    grant.permissions = 0U;
    grant.secretDigest = blinker::ByteView(
        zeroDigest, sizeof(zeroDigest));
    grant.controlNonce = nonce;
    grant.serverKeyId = 1U;
    grant.signatureAlgorithm = blinker::ServerSignatureAlgorithm::Ed25519;
    grant.signature = blinker::ByteView(signature, sizeof(signature));
    std::vector<uint8_t> output(blinker::kControllerGrantMaxEncodedSize);
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeControllerGrant(
        grant,
        blinker::MutableByteSpan(output.data(), output.size()),
        encoded));
    output.resize(encoded.size);
    return output;
}

std::vector<uint8_t> revokeReceipt(
    const std::vector<uint8_t>& encodedGrant) {
    blinker::ControllerGrant grant;
    CHECK_TRUE(blinker::decodeControllerGrant(
        blinker::ByteView(encodedGrant.data(), encodedGrant.size()),
        grant));
    blinker::ControllerMutationReceipt receipt;
    receipt.operation = grant.operation;
    receipt.grantId = grant.grantId;
    receipt.deviceInstanceId = grant.deviceInstanceId;
    receipt.ownershipGeneration = grant.ownershipGeneration;
    receipt.controllerId = grant.controllerId;
    receipt.credentialVersion = grant.credentialVersion;
    receipt.permissions = grant.permissions;
    receipt.secretDigest = grant.secretDigest;
    receipt.proofKind = blinker::ControllerReceiptProofKind::None;
    receipt.proof = blinker::ByteView();
    std::vector<uint8_t> output(blinker::kControllerReceiptMaxEncodedSize);
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeControllerMutationReceipt(
        receipt,
        blinker::MutableByteSpan(output.data(), output.size()),
        encoded));
    output.resize(encoded.size);
    return output;
}

void testPrepareApplyReplayAndFinalize(bool lateReceipt = false) {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord active = activeRecord();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(active, outcome));
    blinker::GatewayAccessRecord stored;
    CHECK_TRUE(store.load(stored));
    const std::vector<uint8_t> original(blob.bytes, blob.bytes + blob.size);
    Clock clock;
    ControlChannel channel;
    blinker::BasicGatewayRevocationCoordinator<ControlChannel> coordinator(
        channel, store, clock);
    channel.setControllerControlReceiver(
        &blinker::IGatewayManagementControl::
            controllerControlResponseThunk,
        &coordinator);

    const EncodedCommand prepare = command(
        stored, blinker::gateway::GatewayRevocationPhase::Prepare);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(
        prepare.bytes, prepare.size)));
    CHECK_TRUE(coordinator.busy());
    CHECK_TRUE(coordinator.ownsChildSession());
    CHECK_TRUE(coordinator.pendingResult().empty());
    coordinator.poll();
    CHECK_EQ(blinker::bbp2::MessageKind::ControllerControlOpen, channel.kind);
    uint8_t nonceBytes[blinker::kControllerControlNonceSize];
    fill(nonceBytes, sizeof(nonceBytes), 0xA1U);
    blinker::bbp2::ControllerControlChallengeBody challenge;
    challenge.controlNonce = blinker::ByteView(
        nonceBytes, sizeof(nonceBytes));
    uint8_t bodyBuffer[32U] = {};
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeControllerControlChallengeBody(
        challenge,
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
        body));
    std::vector<uint8_t> frame = responseFrame(
        blinker::bbp2::MessageKind::ControllerControlChallenge,
        body);
    channel.respond(blinker::ByteView(frame.data(), frame.size()));
    blinker::gateway::GatewayRevocationResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        coordinator.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::NonceReady,
             result.status);
    CHECK_TRUE(!coordinator.busy());
    CHECK_TRUE(coordinator.ownsChildSession());
    CHECK_TRUE(std::memcmp(
        result.payload.data, nonceBytes, sizeof(nonceBytes)) == 0);
    coordinator.markResultPublished();
    CHECK_TRUE(coordinator.ownsChildSession());

    const std::vector<uint8_t> grant = revokeGrant(
        stored, blinker::ByteView(nonceBytes, sizeof(nonceBytes)));
    const EncodedCommand apply = command(
        stored,
        blinker::gateway::GatewayRevocationPhase::Apply,
        blinker::ByteView(grant.data(), grant.size()));
    CHECK_TRUE(coordinator.continuesCommand(blinker::ByteView(apply.bytes, apply.size)));
    blinker::GatewayAccessRecord wrong = stored; ++wrong.accessEpoch;
    const EncodedCommand other = command(wrong, blinker::gateway::GatewayRevocationPhase::Apply,
        blinker::ByteView(grant.data(), grant.size()));
    CHECK_TRUE(!coordinator.continuesCommand(blinker::ByteView(other.bytes, other.size)));
    blinker::clearGatewayAccessRecord(wrong);
    if (lateReceipt) { clock.monotonic += 99000; clock.unixSeconds = 999; }
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(
        apply.bytes, apply.size)));
    CHECK_TRUE(coordinator.busy());
    coordinator.poll();
    CHECK_EQ(blinker::bbp2::MessageKind::ControllerMutation, channel.kind);
    blinker::bbp2::ControllerMutationBody mutation;
    CHECK_TRUE(blinker::bbp2::decodeControllerMutationBody(
        blinker::ByteView(channel.body.data(), channel.body.size()),
        mutation));
    CHECK_TRUE(mutation.controllerSecret.empty());
    const std::vector<uint8_t> receipt = revokeReceipt(grant);
    frame = responseFrame(
        blinker::bbp2::MessageKind::ControllerMutationReceipt,
        blinker::ByteView(receipt.data(), receipt.size()));
    if (lateReceipt) clock.monotonic += 1000U; // Prepare->Apply retains the original monotonic budget.
    channel.respond(blinker::ByteView(frame.data(), frame.size()));
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        coordinator.pendingResult(), result));
    if (lateReceipt) {
        CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Expired, result.status);
        CHECK_TRUE(!coordinator.ownsChildSession());
        CHECK_EQ(original.size(), blob.size);
        CHECK_EQ(0, std::memcmp(original.data(), blob.bytes, blob.size));
        return; // Expired does not assert that the child did not mutate.
    }
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Revoked,
             result.status);
    CHECK_TRUE(!coordinator.ownsChildSession());
    CHECK_TRUE(!coordinator.busy());
    CHECK_EQ(receipt.size(), result.payload.size);
    uint8_t receiptDigest[blinker::kSha256Size];
    CHECK_TRUE(blinker::sha256(
        blinker::ByteView(receipt.data(), receipt.size()),
        blinker::MutableByteSpan(
            receiptDigest, sizeof(receiptDigest))));
    CHECK_TRUE(std::memcmp(
        result.receiptDigest.data,
        receiptDigest,
        sizeof(receiptDigest)) == 0);
    CHECK_EQ(blinker::ErrorCode::StateConflict, store.load(stored).code());

    // A Hub reboot replays the exact durable receipt without reconnecting to
    // the child or rewriting flash.
    coordinator.reset();
    ControlChannel replayChannel;
    blinker::BasicGatewayRevocationCoordinator<ControlChannel> replay(
        replayChannel, store, clock);
    replayChannel.setControllerControlReceiver(
        &blinker::IGatewayManagementControl::
            controllerControlResponseThunk,
        &replay);
    CHECK_TRUE(replay.handleCommand(blinker::ByteView(
        apply.bytes, apply.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        replay.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Revoked,
             result.status);
    CHECK_EQ(0U, replayChannel.sends);
    replay.markResultPublished();

    const EncodedCommand finalize = command(
        stored,
        blinker::gateway::GatewayRevocationPhase::Finalize,
        blinker::ByteView(),
        blinker::ByteView(receiptDigest, sizeof(receiptDigest)));
    CHECK_TRUE(replay.handleCommand(blinker::ByteView(
        finalize.bytes, finalize.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        replay.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Finalized,
             result.status);
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(stored).code());
    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayAccessRecord(stored);
    blinker::secureZero(blinker::MutableByteSpan(
        receiptDigest, sizeof(receiptDigest)));
}

void testFinalizeRecoversPreTombstoneCrash() {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord active = activeRecord();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(active, outcome));
    blinker::GatewayAccessRecord stored;
    CHECK_TRUE(store.load(stored));

    uint8_t receiptDigest[blinker::kSha256Size];
    fill(receiptDigest, sizeof(receiptDigest), 0xB1U);
    uint8_t nonceBytes[blinker::kControllerControlNonceSize];
    fill(nonceBytes, sizeof(nonceBytes), 0xA2U);
    const std::vector<uint8_t> grant = revokeGrant(
        stored, blinker::ByteView(nonceBytes, sizeof(nonceBytes)));
    const EncodedCommand staleApply = command(
        stored,
        blinker::gateway::GatewayRevocationPhase::Apply,
        blinker::ByteView(grant.data(), grant.size()),
        blinker::ByteView(),
        8U);
    const EncodedCommand finalize = command(
        stored,
        blinker::gateway::GatewayRevocationPhase::Finalize,
        blinker::ByteView(),
        blinker::ByteView(receiptDigest, sizeof(receiptDigest)),
        9U);
    Clock clock;
    ControlChannel channel;
    blinker::BasicGatewayRevocationCoordinator<ControlChannel> coordinator(
        channel, store, clock);
    channel.setControllerControlReceiver(
        &blinker::IGatewayManagementControl::
            controllerControlResponseThunk,
        &coordinator);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(
        staleApply.bytes, staleApply.size)));
    coordinator.poll();
    CHECK_TRUE(channel.pending);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(
        finalize.bytes, finalize.size)));
    blinker::gateway::GatewayRevocationResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        coordinator.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Finalized,
             result.status);
    CHECK_TRUE(!channel.pending);
    CHECK_EQ(1U, channel.sends);
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(stored).code());

    CHECK_TRUE(store.commit(active, outcome));
    CHECK_TRUE(store.load(stored));
    blinker::GatewayAccessRecord wrong = stored;
    wrong.credentialVersion += 1U;
    const EncodedCommand mismatched = command(
        wrong,
        blinker::gateway::GatewayRevocationPhase::Finalize,
        blinker::ByteView(),
        blinker::ByteView(receiptDigest, sizeof(receiptDigest)));
    coordinator.reset();
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        coordinator.handleCommand(blinker::ByteView(
            mismatched.bytes, mismatched.size)).code());
    CHECK_TRUE(store.load(stored));

    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayAccessRecord(stored);
    blinker::clearGatewayAccessRecord(wrong);
    blinker::secureZero(blinker::MutableByteSpan(
        receiptDigest, sizeof(receiptDigest)));
    blinker::secureZero(blinker::MutableByteSpan(
        nonceBytes, sizeof(nonceBytes)));
}

void testManagementBudgetBoundaries() {
    blinker::GatewayManagementBudget budget;
    CHECK_TRUE(!budget.active(0U));
    CHECK_TRUE(!budget.begin(0U, 0U, 1000U));
    CHECK_TRUE(!budget.begin(0U, 1000U, 1000U));
    CHECK_TRUE(!budget.begin(0U, 1000U, 1301U));
    CHECK_TRUE(budget.begin(0xfffffff0U, 1000U, 1300U));
    CHECK_TRUE(budget.active(0xfffffff0U + 299999U));
    CHECK_TRUE(!budget.active(0xfffffff0U + 300000U));
    CHECK_TRUE(budget.begin(100U, 1000U, 1001U));
    CHECK_TRUE(!budget.active(99U)); // anomalous monotonic rollback
    budget.clear();
    CHECK_TRUE(!budget.active(100U));
}

void testNonceSessionIsBoundedAndExact() {
    // Frozen/advanced Unix time, replaced/disconnected session, late response,
    // wrong response kind and missing authentication are independent failures.
    for (unsigned mode = 0U; mode < 7U; ++mode) {
        MemoryBlob blob;
        blinker::GatewayAccessStore store(blob);
        blinker::GatewayAccessRecord stored = activeRecord();
        blinker::GatewayAccessCommitOutcome outcome;
        CHECK_TRUE(store.commit(stored, outcome));
        CHECK_TRUE(store.load(stored));
        const std::vector<uint8_t> original(blob.bytes, blob.bytes + blob.size);
        Clock clock;
        ControlChannel channel;
        blinker::BasicGatewayRevocationCoordinator<ControlChannel> coordinator(channel, store, clock);
        channel.setControllerControlReceiver(
            &blinker::IGatewayManagementControl::controllerControlResponseThunk, &coordinator);
        const EncodedCommand prepare = command(stored, blinker::gateway::GatewayRevocationPhase::Prepare);
        CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)));
        CHECK_TRUE(coordinator.ownsChildSession());
        coordinator.poll();
        uint8_t nonce[blinker::kControllerControlNonceSize];
        fill(nonce, sizeof(nonce), 0xA1U);
        blinker::bbp2::ControllerControlChallengeBody challenge;
        challenge.controlNonce = blinker::ByteView(nonce, sizeof(nonce));
        uint8_t encoded[32U] = {};
        blinker::ByteView body;
        CHECK_TRUE(blinker::bbp2::encodeControllerControlChallengeBody(
            challenge, blinker::MutableByteSpan(encoded, sizeof(encoded)), body));
        const std::vector<uint8_t> response = responseFrame(
            mode == 5U ? blinker::bbp2::MessageKind::Hello :
                         blinker::bbp2::MessageKind::ControllerControlChallenge, body);
        if (mode == 4U) clock.monotonic += 100000U;
        if (mode == 6U) channel.authenticatedAttemptId = 0U;
        channel.respond(blinker::ByteView(response.data(), response.size()));
        blinker::gateway::GatewayRevocationResultView result;
        if (mode < 4U) {
            CHECK_TRUE(!coordinator.busy());
            CHECK_TRUE(coordinator.ownsChildSession());
            coordinator.markResultPublished();
            clock.monotonic += 99000U;
            // Exact replay and publication never re-anchor the monotonic budget.
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)));
            CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(coordinator.pendingResult(), result));
            CHECK_EQ(blinker::gateway::GatewayRevocationStatus::NonceReady, result.status);
            coordinator.markResultPublished();
            if (mode == 0U) clock.monotonic += 1000U;
            if (mode == 1U) clock.unixSeconds = 1100U;
            if (mode == 2U) ++channel.authenticatedAttemptId;
            if (mode == 3U) channel.authenticatedAttemptId = 0U;
            CHECK_TRUE(!coordinator.ownsChildSession());
            if (mode == 3U) coordinator.poll();
            // handleCommand can run before poll: stale nonce must not escape.
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)));
        }
        CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(coordinator.pendingResult(), result));
        CHECK_EQ(mode >= 5U ? blinker::gateway::GatewayRevocationStatus::Rejected :
                             blinker::gateway::GatewayRevocationStatus::Expired, result.status);
        CHECK_TRUE(result.payload.empty());
        CHECK_TRUE(!coordinator.ownsChildSession());
        coordinator.markResultPublished();
        clock.unixSeconds = 1000U;
        channel.authenticatedAttemptId = 1U;
        coordinator.poll();
        CHECK_TRUE(!coordinator.ownsChildSession());
        CHECK_EQ(1U, channel.sends);
        CHECK_EQ(original.size(), blob.size);
        CHECK_EQ(0, std::memcmp(original.data(), blob.bytes, blob.size));
    }
}

void testSupersedeRetiresOnlyExactOutboundRecord() {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord active = activeRecord();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(active, outcome));
    blinker::GatewayAccessRecord stored;
    CHECK_TRUE(store.load(stored));

    uint8_t enrollmentReceiptDigest[blinker::kSha256Size];
    fill(enrollmentReceiptDigest, sizeof(enrollmentReceiptDigest), 0xC1U);
    const EncodedCommand supersede = command(
        stored,
        blinker::gateway::GatewayRevocationPhase::Supersede,
        blinker::ByteView(),
        blinker::ByteView(
            enrollmentReceiptDigest,
            sizeof(enrollmentReceiptDigest)),
        9U,
        true);
    Clock clock;
    ControlChannel channel;
    blinker::BasicGatewayRevocationCoordinator<ControlChannel> coordinator(
        channel, store, clock);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(
        supersede.bytes, supersede.size)));
    blinker::gateway::GatewayRevocationResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        coordinator.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Retired,
             result.status);
    CHECK_EQ(0U, channel.sends);
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(stored).code());

    CHECK_TRUE(store.commit(active, outcome));
    CHECK_TRUE(store.load(stored));
    blinker::GatewayAccessRecord wrong = stored;
    ++wrong.accessEpoch;
    const EncodedCommand mismatched = command(
        wrong,
        blinker::gateway::GatewayRevocationPhase::Supersede,
        blinker::ByteView(),
        blinker::ByteView(
            enrollmentReceiptDigest,
            sizeof(enrollmentReceiptDigest)),
        10U);
    coordinator.reset();
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        coordinator.handleCommand(blinker::ByteView(
            mismatched.bytes, mismatched.size)).code());
    CHECK_TRUE(store.load(stored));

    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayAccessRecord(stored);
    blinker::clearGatewayAccessRecord(wrong);
    blinker::secureZero(blinker::MutableByteSpan(
        enrollmentReceiptDigest, sizeof(enrollmentReceiptDigest)));
}

class RoutingControl final : public blinker::IGatewayManagementControl {
public:
    unsigned calls = 0, resets = 0, polls = 0;
    bool working = false, held = false, reply = false;
    std::vector<uint8_t> last;
    blinker::Result handleCommand(blinker::ByteView bytes) override {
        ++calls; last.assign(bytes.data, bytes.data + bytes.size); return blinker::Result::success();
    }
    bool continuesCommand(blinker::ByteView bytes) const override {
        return !last.empty() && bytes.size == last.size() && std::memcmp(last.data(), bytes.data, bytes.size) == 0;
    }
    void poll() override { ++polls; }
    bool busy() const override { return working; }
    bool ownsChildSession() const override { return held; }
    blinker::ByteView pendingResult() const override { return reply ? blinker::ByteView(last.data(), last.size()) : blinker::ByteView(); }
    void markResultPublished() override { reply = false; }
    void reset() override { ++resets; working = reply = false; last.clear(); } // held models asynchronous native close.
};
class UnusedDelivery final : public blinker::IGatewayManagementDelivery {
public:
    blinker::Result process(const blinker::GatewayAccessDeliveryContext&, blinker::ByteView,
        blinker::MutableByteSpan, size_t& size) override { size = 0; return blinker::Result::failure(blinker::ErrorCode::UnsupportedFeature); }
};
class RoutingAdmission final : public blinker::IGatewayManagementAdmission {
public:
    bool allowed = true;
    bool allowNewManagementWork() override { return allowed; }
};
blinker::ByteView view(const EncodedCommand& value) { return blinker::ByteView(value.bytes, value.size); }
EncodedCommand proofCommand(const blinker::GatewayAccessRecord& record) {
    using namespace blinker;
    gateway::GatewayProofCommandView value;
    value.operationId = ByteView(record.operationId, sizeof(record.operationId)); value.topologyVersion = 1;
    value.childLogicalDeviceId = StringView(record.childLogicalDeviceId, record.childLogicalDeviceIdSize);
    value.childDeviceInstanceId = ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
    value.accessEpoch = record.accessEpoch; value.gatewayCredentialVersion = record.credentialVersion;
    value.presenceKeyVersion = record.activePresence.version;
    value.accessMaterialDigest = ByteView(record.accessMaterialDigest, sizeof(record.accessMaterialDigest));
    value.accessStorageRevision = record.deliveryRevision;
    const uint8_t nonce[gateway::kProofNonceSize] = {1}; value.proofNonce = ByteView(nonce, sizeof(nonce));
    value.expiresAtUnixSeconds = 1100;
    EncodedCommand output{};
    CHECK_TRUE(gateway::encodeGatewayProofCommand(value, MutableByteSpan(output.bytes, sizeof(output.bytes)), output.size));
    return output;
}

void testDirectoryManagementOwnership() {
    using namespace blinker;
    MemoryBlob aBlob, aJournal, bBlob, bJournal;
    GatewayChildRecords a(aBlob, aJournal), b(bBlob, bJournal);
    auto first = activeRecord(), second = first;
    second.operationId[0] ^= 1; second.childDeviceInstanceId[0] ^= 1;
    second.childLogicalDeviceId[second.childLogicalDeviceIdSize - 1] = '2';
    GatewayAccessCommitOutcome outcome; CHECK_TRUE(a.access.commit(first, outcome)); CHECK_TRUE(b.access.commit(second, outcome));
    CHECK_TRUE(a.access.load(first)); CHECK_TRUE(b.access.load(second));
    RoutingControl aProof, aRevoke, aRenewal, bProof, bRevoke, bRenewal, permit;
    GatewayManagementControlMux aMux(aProof, aRevoke, aRenewal), bMux(bProof, bRevoke, bRenewal);
    UnusedDelivery delivery;
    GatewayChildDirectoryEntry entries[] = {{a, delivery, &aMux}, {b, delivery, &bMux}};
    GatewayChildDirectory directory(entries, 2); GatewayChildManagementRouter router(directory, permit);
    RoutingAdmission admission, bAdmission, permitAdmission;
    aMux.setAdmission(admission); bMux.setAdmission(bAdmission);
    router.setPermitAdmission(permitAdmission);
    const auto aCommand = proofCommand(first), bCommand = proofCommand(second);
    CHECK_TRUE(router.handleCommand(view(aCommand))); CHECK_EQ(1U, aProof.calls); CHECK_EQ(0U, bProof.calls);
    aProof.held = true; admission.allowed = false;
    CHECK_TRUE(router.continuesCommand(view(aCommand))); CHECK_TRUE(router.handleCommand(view(aCommand)));
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(bCommand)).code()); CHECK_EQ(0U, aProof.resets);
    auto wrong = second; wrong.operationId[0] ^= 0x40;
    CHECK_EQ(ErrorCode::NotFound, router.handleCommand(view(proofCommand(wrong))).code()); CHECK_EQ(0U, aProof.resets);
    wrong = first; ++wrong.accessEpoch;
    CHECK_EQ(ErrorCode::StateConflict, router.handleCommand(view(proofCommand(wrong))).code());
    aProof.held = false; aProof.reply = true;
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(bCommand)).code());
    router.markResultPublished(); CHECK_TRUE(router.handleCommand(view(bCommand)));
    CHECK_EQ(1U, aProof.resets); CHECK_EQ(1U, bProof.calls);
    CHECK_TRUE(!admission.allowed); // A's Direct/quiet barrier never gates B.
    CHECK_EQ(0U, permit.resets); // Switching children must not reset Hub permit.
    // Permit remains one Hub task, not a child identity or one instance per child.
    gateway::GatewayPermitJoinCommandView open;
    const uint8_t operation[16] = {0x77}; open.operationId = ByteView(operation, sizeof(operation));
    open.adapterId = gateway::kGatewaySouthboundAdapterNativeGatt;
    open.action = gateway::GatewayPermitJoinAction::Open; open.expiresAtUnixSeconds = 1100;
    EncodedCommand permitCommand{};
    CHECK_TRUE(gateway::encodeGatewayPermitJoinCommand(open, MutableByteSpan(permitCommand.bytes, sizeof(permitCommand.bytes)), permitCommand.size));
    // A child mux cannot decode, poll, publish or reset a Hub permit window.
    CHECK_EQ(ErrorCode::InvalidEncoding, aMux.handleCommand(view(permitCommand)).code());
    aMux.poll(); aMux.markResultPublished(); aMux.reset();
    CHECK_EQ(0U, permit.calls); CHECK_EQ(0U, permit.polls); CHECK_EQ(0U, permit.resets);
    permitAdmission.allowed = false;
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(permitCommand)).code());
    CHECK_TRUE(router.handleCommand(view(bCommand))); // Closed permit admission is not a Hub-wide barrier.
    permitAdmission.allowed = true;
    CHECK_TRUE(router.handleCommand(view(permitCommand))); CHECK_EQ(1U, permit.calls);
    const unsigned permitPolls = permit.polls;
    router.poll(); CHECK_EQ(permitPolls + 1U, permit.polls); // Exactly one Hub tick.
    permit.held = true; CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(aCommand)).code());
    bMux.reset(); CHECK_EQ(0U, permit.resets); CHECK_TRUE(router.ownsChildSession());
    router.reset(); CHECK_TRUE(router.ownsChildSession()); router.poll(); CHECK_TRUE(permit.polls > 0);
    CHECK_EQ(1U, permit.resets); // Whole-Hub reset does not fan out through child muxes.
    permit.held = false;
    admission.allowed = true;
    CHECK_TRUE(router.handleCommand(view(aCommand)));
    aProof.held = true; router.reset();
    CHECK_TRUE(router.ownsChildSession()); // Child async drain survives a wire reset too.
    const auto aPolls = aProof.polls; router.poll(); CHECK_EQ(aPolls + 1U, aProof.polls);
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(bCommand)).code());
    aProof.held = false;
    // A dormant record without an activity handler is not assigned to B.
    router.reset(); entries[0].control = nullptr;
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(view(aCommand)).code());
    entries[0].control = &bMux;
    CHECK_EQ(ErrorCode::StateConflict, router.handleCommand(view(aCommand)).code());
    clearGatewayAccessRecord(first); clearGatewayAccessRecord(second); clearGatewayAccessRecord(wrong);
}

void testDirectoryTerminalReplay() {
    using namespace blinker;
    MemoryBlob aBlob, aJournal, bBlob, bJournal;
    GatewayChildRecords a(aBlob, aJournal), b(bBlob, bJournal);
    auto original = activeRecord(), newer = original;
    ++newer.accessEpoch; newer.operationId[0] ^= 1;
    GatewayAccessCommitOutcome outcome; CHECK_TRUE(a.access.commit(original, outcome)); CHECK_TRUE(a.access.load(original));
    Clock clock; ControlChannel channel;
    BasicGatewayRevocationCoordinator<ControlChannel> revocation(channel, a.access, clock);
    RoutingControl unused, other, permit;
    GatewayManagementControlMux child(unused, revocation, unused);
    UnusedDelivery delivery;
    GatewayChildDirectoryEntry entries[] = {{a, delivery, &child}, {b, delivery, &other}};
    GatewayChildDirectory directory(entries, 2); GatewayChildManagementRouter router(directory, permit);
    uint8_t digest[32]; fill(digest, sizeof(digest), 0x55);
    const auto finalize = command(original, gateway::GatewayRevocationPhase::Finalize, ByteView(), ByteView(digest, sizeof(digest)), UINT32_MAX);
    // Clear the exact active record (the already-existing pre-tombstone crash
    // recovery path), then compare its real coordinator result to absence ACK.
    CHECK_TRUE(router.handleCommand(view(finalize))); CHECK_TRUE(!aBlob.present);
    const auto firstResult = router.pendingResult(); CHECK_TRUE(!firstResult.empty());
    std::vector<uint8_t> exact(firstResult.data, firstResult.data + firstResult.size);
    router.markResultPublished(); router.reset();
    // Reuse A's old slot for a NEW epoch. The delayed command must not touch it.
    CHECK_TRUE(a.access.commit(newer, outcome));
    const std::vector<uint8_t> persisted(aBlob.bytes, aBlob.bytes + aBlob.size);
    GatewayChildManagementRouter restarted(directory, permit);
    CHECK_TRUE(restarted.handleCommand(view(finalize)));
    CHECK_TRUE(std::vector<uint8_t>(restarted.pendingResult().data,
        restarted.pendingResult().data + restarted.pendingResult().size) == exact);
    CHECK_TRUE(std::vector<uint8_t>(aBlob.bytes, aBlob.bytes + aBlob.size) == persisted); CHECK_EQ(0U, channel.sends);
    restarted.markResultPublished();
    // Same authority under another operation is NOT absence (Presence rotation).
    CHECK_TRUE(a.access.clear()); newer = activeRecord(); newer.operationId[0] ^= 1;
    CHECK_TRUE(a.access.commit(newer, outcome));
    CHECK_EQ(ErrorCode::SequenceConflict, restarted.handleCommand(view(finalize)).code());
    CHECK_TRUE(restarted.pendingResult().empty());
    CHECK_TRUE(a.access.clear());
    const auto supersede = command(original, gateway::GatewayRevocationPhase::Supersede, ByteView(), ByteView(digest, sizeof(digest)), UINT32_MAX);
    CHECK_TRUE(restarted.handleCommand(view(supersede)));
    gateway::GatewayRevocationResultView decoded;
    CHECK_TRUE(gateway::decodeGatewayRevocationResult(restarted.pendingResult(), decoded));
    CHECK_EQ(gateway::GatewayRevocationStatus::Retired, decoded.status);
    restarted.markResultPublished();
    // Corruption in any other slot forbids a global absence proof.
    bBlob.present = true; bBlob.size = GatewayAccessStore::serializedSize; directory.invalidate();
    CHECK_EQ(ErrorCode::InvalidEncoding, restarted.handleCommand(view(finalize)).code());
    CHECK_TRUE(restarted.pendingResult().empty()); b.access.clear();
    // A journal may still recover old authority even when access is empty.
    GatewayCredentialRenewalRecord pending;
    fill(pending.renewalOperationId, sizeof(pending.renewalOperationId), 0x44);
    std::memcpy(pending.topologyOperationId, original.operationId, sizeof(pending.topologyOperationId));
    std::memcpy(pending.childDeviceInstanceId, original.childDeviceInstanceId, sizeof(pending.childDeviceInstanceId));
    std::memcpy(pending.controllerId, original.controllerId, sizeof(pending.controllerId));
    fill(pending.secret, sizeof(pending.secret), 0x45); fill(pending.accessMaterialDigest, sizeof(pending.accessMaterialDigest), 0x46);
    pending.accessEpoch = original.accessEpoch; pending.expectedCredentialVersion = original.credentialVersion;
    pending.credentialVersion = original.credentialVersion + 1; pending.presenceKeyVersion = 1; pending.expiresAtUnixSeconds = 5000;
    GatewayCredentialRenewalCommitOutcome renewalOutcome; CHECK_TRUE(a.renewal.commit(pending, renewalOutcome));
    CHECK_EQ(ErrorCode::WouldBlock, restarted.handleCommand(view(finalize)).code()); CHECK_TRUE(aJournal.present);
    clearGatewayCredentialRenewalRecord(pending); clearGatewayAccessRecord(original); clearGatewayAccessRecord(newer);
}

} // namespace

int main() {
    testManagementBudgetBoundaries();
    testNonceSessionIsBoundedAndExact();
    testPrepareApplyReplayAndFinalize();
    testPrepareApplyReplayAndFinalize(true);
    testFinalizeRecoversPreTombstoneCrash();
    testSupersedeRetiresOnlyExactOutboundRecord();
    testDirectoryManagementOwnership();
    testDirectoryTerminalReplay();
    std::cout << "Gateway revocation coordinator tests passed" << std::endl;
    return 0;
}
