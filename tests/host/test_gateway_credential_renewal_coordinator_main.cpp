#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/identity/GatewayCredentialRenewalStore.h>
#include <BlinkerV2/runtime/GatewayChildRouteBridge.h>
#include <BlinkerV2/runtime/GatewayCredentialRenewalCoordinator.h>
#include <BlinkerV2/runtime/GatewayManagementControlMux.h>
#include <BlinkerV2/runtime/GatewayChildManagementRouter.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Gateway renewal coordinator check failed at line "  \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)
#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class RejectControl final : public blinker::IGatewayManagementControl {
public:
    blinker::Result handleCommand(blinker::ByteView) override {
        return blinker::Result::failure(blinker::ErrorCode::InvalidEncoding);
    }
    void poll() override {}
    bool busy() const override { return false; }
    blinker::ByteView pendingResult() const override { return blinker::ByteView(); }
    void markResultPublished() override {}
    void reset() override {}
};

class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    MemoryBlob() : bytes(), size(0U), present(false) {}
    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView value) override {
        CHECK_TRUE(value.data != nullptr && value.size <= sizeof(bytes));
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
    uint8_t bytes[512U];
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
          retireCalls(0U), authenticatedVersion(0U),
          authenticatedAttemptId(0U), ensureCalls(0U),
          blockRetirement(false), retiredAttemptId(0U),
          kind(blinker::bbp2::MessageKind::Hello), body() {}
    void setControllerControlReceiver(
        blinker::GatewayChildControlReceiver value,
        void* valueContext) {
        receiver = value;
        context = valueContext;
    }
    bool controllerControlPending() const { return pending; }
    void cancelControllerControlRequest() { pending = false; }
    bool retireControllerControlSession(uint32_t attemptId) {
        if (attemptId == 0U || attemptId != authenticatedAttemptId) return true;
        pending = false;
        ++retireCalls;
        retiredAttemptId = attemptId;
        if (blockRetirement) return false;
        authenticatedAttemptId = 0U;
        return true;
    }
    uint32_t authenticatedControllerCredentialVersion() const {
        return authenticatedVersion;
    }
    uint32_t authenticatedControllerSessionAttemptId() const {
        return authenticatedAttemptId;
    }
    blinker::Result ensureControllerControlSession() {
        ++ensureCalls;
        return authenticatedVersion == 0U
            ? blinker::Result::failure(blinker::ErrorCode::WouldBlock)
            : blinker::Result::success();
    }
    blinker::Result sendControllerControlRequest(
        blinker::bbp2::MessageKind valueKind,
        blinker::ByteView valueBody) {
        if (pending) {
            return blinker::Result::failure(
                blinker::ErrorCode::StateConflict);
        }
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
    uint32_t retireCalls;
    uint32_t authenticatedVersion;
    uint32_t authenticatedAttemptId;
    uint32_t ensureCalls;
    bool blockRetirement;
    uint32_t retiredAttemptId;
    blinker::bbp2::MessageKind kind;
    std::vector<uint8_t> body;
};

void fill(uint8_t* output, size_t size, uint8_t value) {
    std::memset(output, value, size);
}

std::vector<uint8_t> frame(
    blinker::bbp2::MessageKind kind,
    blinker::ByteView body) {
    std::vector<uint8_t> output(blinker::bbp2::kBaseHeaderSize + body.size);
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = blinker::bbp2::FlagIsResponse;
    header.sequence = 1U;
    header.bodyLength = static_cast<uint16_t>(body.size);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
        header, blinker::MutableByteSpan(output.data(), output.size())));
    if (!body.empty()) {
        std::memcpy(output.data() + blinker::bbp2::kBaseHeaderSize,
                    body.data, body.size);
    }
    return output;
}

blinker::GatewayAccessRecord activeRecord() {
    blinker::GatewayAccessRecord value;
    fill(value.operationId, sizeof(value.operationId), 0x12U);
    fill(value.envelopeDigest, sizeof(value.envelopeDigest), 0x21U);
    fill(value.accessMaterialDigest,
         sizeof(value.accessMaterialDigest), 0x31U);
    const char child[] = "child-1";
    value.childLogicalDeviceIdSize = sizeof(child) - 1U;
    std::memcpy(value.childLogicalDeviceId, child, sizeof(child) - 1U);
    fill(value.childDeviceInstanceId,
         sizeof(value.childDeviceInstanceId), 0x22U);
    value.accessEpoch = 9U;
    fill(value.controllerId, sizeof(value.controllerId), 0x33U);
    value.credentialVersion = 4U;
    value.permissions = blinker::gateway::kGatewayPermissions;
    value.expiresAtUnixSeconds = 5000U;
    fill(value.secret, sizeof(value.secret), 0x42U);
    value.activePresence.version = 3U;
    fill(value.activePresence.key,
         sizeof(value.activePresence.key), 0x47U);
    return value;
}

blinker::GatewayCredentialRenewalRecord pendingRecord(
    const blinker::GatewayAccessRecord& active) {
    blinker::GatewayCredentialRenewalRecord value;
    fill(value.renewalOperationId,
         sizeof(value.renewalOperationId), 0x11U);
    std::memcpy(value.topologyOperationId, active.operationId,
                sizeof(value.topologyOperationId));
    std::memcpy(value.childDeviceInstanceId,
                active.childDeviceInstanceId,
                sizeof(value.childDeviceInstanceId));
    value.accessEpoch = active.accessEpoch;
    std::memcpy(value.controllerId, active.controllerId,
                sizeof(value.controllerId));
    value.expectedCredentialVersion = active.credentialVersion;
    value.credentialVersion = active.credentialVersion + 1U;
    value.presenceKeyVersion = active.activePresence.version;
    value.expiresAtUnixSeconds = 6000U;
    fill(value.secret, sizeof(value.secret), 0x43U);
    blinker::Sha256 hash;
    CHECK_TRUE(hash.update(blinker::ByteView(
        value.secret, sizeof(value.secret))));
    CHECK_TRUE(hash.update(blinker::ByteView(
        active.activePresence.key, sizeof(active.activePresence.key))));
    CHECK_TRUE(hash.finish(blinker::MutableByteSpan(
        value.accessMaterialDigest,
        sizeof(value.accessMaterialDigest))));
    return value;
}

std::vector<uint8_t> rotateGrant(
    const blinker::GatewayCredentialRenewalRecord& pending,
    blinker::ByteView nonce) {
    uint8_t grantId[blinker::kControllerGrantIdSize];
    uint8_t secretDigest[blinker::kControllerSecretDigestSize] = {};
    uint8_t signature[blinker::kControllerGrantSignatureSize];
    fill(grantId, sizeof(grantId), 0x81U);
    fill(signature, sizeof(signature), 0x91U);
    CHECK_TRUE(blinker::sha256(
        blinker::ByteView(pending.secret, sizeof(pending.secret)),
        blinker::MutableByteSpan(secretDigest, sizeof(secretDigest))));
    blinker::ControllerGrant grant;
    grant.operation = blinker::ControllerMutationOperation::Rotate;
    grant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    grant.deviceInstanceId = blinker::ByteView(
        pending.childDeviceInstanceId,
        sizeof(pending.childDeviceInstanceId));
    grant.ownershipGeneration = pending.accessEpoch;
    grant.controllerId = blinker::ByteView(
        pending.controllerId, sizeof(pending.controllerId));
    grant.expectedCredentialVersion = pending.expectedCredentialVersion;
    grant.credentialVersion = pending.credentialVersion;
    grant.permissions = blinker::gateway::kGatewayPermissions;
    grant.secretDigest = blinker::ByteView(
        secretDigest, sizeof(secretDigest));
    grant.controlNonce = nonce;
    grant.serverKeyId = 1U;
    grant.signatureAlgorithm = blinker::ServerSignatureAlgorithm::Ed25519;
    grant.signature = blinker::ByteView(signature, sizeof(signature));
    std::vector<uint8_t> output(blinker::kControllerGrantMaxEncodedSize);
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeControllerGrant(
        grant, blinker::MutableByteSpan(output.data(), output.size()),
        encoded));
    output.resize(encoded.size);
    return output;
}

std::vector<uint8_t> rotateReceipt(
    const std::vector<uint8_t>& encodedGrant,
    blinker::ByteView secret) {
    blinker::ControllerGrant grant;
    CHECK_TRUE(blinker::decodeControllerGrant(
        blinker::ByteView(encodedGrant.data(), encodedGrant.size()), grant));
    blinker::ControllerMutationReceipt receipt;
    receipt.operation = grant.operation;
    receipt.grantId = grant.grantId;
    receipt.deviceInstanceId = grant.deviceInstanceId;
    receipt.ownershipGeneration = grant.ownershipGeneration;
    receipt.controllerId = grant.controllerId;
    receipt.credentialVersion = grant.credentialVersion;
    receipt.permissions = grant.permissions;
    receipt.secretDigest = grant.secretDigest;
    receipt.proofKind = blinker::ControllerReceiptProofKind::HmacSha256;
    uint8_t proof[blinker::kControllerReceiptProofSize] = {};
    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    CHECK_TRUE(blinker::computeControllerMutationReceiptProof(
        secret, receipt,
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(proof, sizeof(proof))));
    receipt.proof = blinker::ByteView(proof, sizeof(proof));
    std::vector<uint8_t> output(blinker::kControllerReceiptMaxEncodedSize);
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeControllerMutationReceipt(
        receipt, blinker::MutableByteSpan(output.data(), output.size()),
        encoded));
    output.resize(encoded.size);
    return output;
}

struct EncodedCommand {
    uint8_t bytes[
        blinker::gateway::kGatewayCredentialRenewalCommandMaximumEncodedSize];
    size_t size;
};

EncodedCommand command(
    const blinker::GatewayCredentialRenewalRecord& pending,
    blinker::gateway::GatewayCredentialRenewalPhase phase,
    blinker::ByteView grant = blinker::ByteView(),
    blinker::ByteView receiptDigest = blinker::ByteView(),
    uint32_t activeRevision = 0U) {
    blinker::gateway::GatewayCredentialRenewalCommandView value;
    value.renewalOperationId = blinker::ByteView(
        pending.renewalOperationId, sizeof(pending.renewalOperationId));
    value.topologyOperationId = blinker::ByteView(
        pending.topologyOperationId, sizeof(pending.topologyOperationId));
    value.topologyVersion = 8U;
    value.childDeviceInstanceId = blinker::ByteView(
        pending.childDeviceInstanceId,
        sizeof(pending.childDeviceInstanceId));
    value.accessEpoch = pending.accessEpoch;
    value.controllerId = blinker::ByteView(
        pending.controllerId, sizeof(pending.controllerId));
    value.expectedCredentialVersion = pending.expectedCredentialVersion;
    value.credentialVersion = pending.credentialVersion;
    value.pendingStorageRevision = pending.deliveryRevision;
    value.activeStorageRevision = activeRevision;
    value.accessMaterialDigest = blinker::ByteView(
        pending.accessMaterialDigest,
        sizeof(pending.accessMaterialDigest));
    value.phase = phase;
    value.grant = grant;
    value.receiptDigest = receiptDigest;
    value.expiresAtUnixSeconds =
        phase == blinker::gateway::GatewayCredentialRenewalPhase::Prepare ||
                phase == blinker::gateway::GatewayCredentialRenewalPhase::Apply
            ? 1100U
            : 0U;
    EncodedCommand output = {};
    CHECK_TRUE(blinker::gateway::encodeGatewayCredentialRenewalCommand(
        value, blinker::MutableByteSpan(output.bytes, sizeof(output.bytes)),
        output.size));
    return output;
}

void testPrepareRotatePromoteFinalize(unsigned boundary = 0U) {
    MemoryBlob activeBlob;
    MemoryBlob pendingBlob;
    blinker::GatewayAccessStore activeStore(activeBlob);
    blinker::GatewayCredentialRenewalStore pendingStore(pendingBlob);
    blinker::GatewayAccessRecord active = activeRecord();
    blinker::GatewayAccessCommitOutcome accessOutcome;
    CHECK_TRUE(activeStore.commit(active, accessOutcome));
    blinker::GatewayCredentialRenewalRecord staged = pendingRecord(active);
    blinker::GatewayCredentialRenewalCommitOutcome pendingOutcome;
    CHECK_TRUE(pendingStore.commit(staged, pendingOutcome));
    staged.deliveryRevision = pendingOutcome.pendingStorageRevision;
    const std::vector<uint8_t> originalActive(activeBlob.bytes, activeBlob.bytes + activeBlob.size);
    const std::vector<uint8_t> originalPending(pendingBlob.bytes, pendingBlob.bytes + pendingBlob.size);

    Clock clock;
    ControlChannel channel;
    channel.authenticatedAttemptId = 1U;
    blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
        coordinator(channel, activeStore, pendingStore, clock);
    RejectControl unused;
    blinker::GatewayManagementControlMux mux(
        unused, unused, coordinator);
    class Delivery final : public blinker::IGatewayManagementDelivery {
        blinker::Result process(const blinker::GatewayAccessDeliveryContext&, blinker::ByteView,
                               blinker::MutableByteSpan, size_t& written) override {
            written = 0; return blinker::Result::failure(blinker::ErrorCode::UnsupportedFeature);
        }
    } delivery;
    blinker::GatewayChildRecords records(activeBlob, pendingBlob);
    blinker::GatewayChildDirectoryEntry entry(records, delivery, &mux);
    blinker::GatewayChildDirectory directory(&entry, 1U);
    blinker::GatewayChildManagementRouter router(directory, unused);
    channel.setControllerControlReceiver(
        &blinker::IGatewayManagementControl::
            controllerControlResponseThunk,
        &mux);

    EncodedCommand prepare = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Prepare);
    CHECK_TRUE(router.handleCommand(blinker::ByteView(
        prepare.bytes, prepare.size)));
    CHECK_TRUE(mux.busy());
    CHECK_TRUE(mux.ownsChildSession());
    mux.poll();
    CHECK_EQ(blinker::bbp2::MessageKind::ControllerControlOpen,
             channel.kind);
    uint8_t nonce[blinker::kControllerControlNonceSize];
    fill(nonce, sizeof(nonce), 0xA1U);
    blinker::bbp2::ControllerControlChallengeBody challenge;
    challenge.controlNonce = blinker::ByteView(nonce, sizeof(nonce));
    uint8_t challengeBytes[32U] = {};
    blinker::ByteView challengeBody;
    CHECK_TRUE(blinker::bbp2::encodeControllerControlChallengeBody(
        challenge,
        blinker::MutableByteSpan(challengeBytes, sizeof(challengeBytes)),
        challengeBody));
    std::vector<uint8_t> response = frame(
        boundary == 7U ? blinker::bbp2::MessageKind::Hello :
                         blinker::bbp2::MessageKind::ControllerControlChallenge,
        challengeBody);
    if (boundary == 8U) channel.authenticatedAttemptId = 0U;
    if (boundary == 9U) clock.monotonic += 100000U;
    channel.respond(blinker::ByteView(response.data(), response.size()));
    blinker::gateway::GatewayCredentialRenewalResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        mux.pendingResult(), result));
    if (boundary >= 7U) {
        CHECK_EQ(boundary == 9U ? blinker::gateway::GatewayCredentialRenewalStatus::Expired :
                                 blinker::gateway::GatewayCredentialRenewalStatus::Rejected, result.status);
        CHECK_TRUE(!mux.ownsChildSession());
        CHECK_EQ(0, std::memcmp(originalPending.data(), pendingBlob.bytes, originalPending.size()));
        return;
    }
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::NonceReady,
             result.status);
    CHECK_TRUE(!mux.busy());
    CHECK_TRUE(mux.ownsChildSession());
    mux.markResultPublished();
    CHECK_TRUE(mux.ownsChildSession());

    // A published nonce is still a live session dependency: another family
    // cannot reset it, even though busy() and pendingResult() are both empty.
    uint8_t permitBytes[blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize] = {};
    blinker::gateway::GatewayPermitJoinCommandView permit;
    permit.operationId = blinker::ByteView(staged.renewalOperationId, sizeof(staged.renewalOperationId));
    permit.adapterId = blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    permit.action = blinker::gateway::GatewayPermitJoinAction::Open;
    permit.expiresAtUnixSeconds = 1100U;
    size_t permitSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
        permit, blinker::MutableByteSpan(permitBytes, sizeof(permitBytes)), permitSize));
    CHECK_EQ(blinker::ErrorCode::InvalidEncoding,
        mux.handleCommand(blinker::ByteView(permitBytes, permitSize)).code());
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        router.handleCommand(blinker::ByteView(permitBytes, permitSize)).code());
    if (boundary >= 4U) {
        clock.monotonic += 99000U;
        CHECK_TRUE(mux.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)));
        CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(mux.pendingResult(), result));
        CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::NonceReady, result.status);
        mux.markResultPublished();
        if (boundary == 4U) { clock.unixSeconds = 999U; clock.monotonic += 1000U; }
        if (boundary == 5U) ++channel.authenticatedAttemptId;
        if (boundary == 6U) clock.unixSeconds = 1100U;
        CHECK_TRUE(!mux.ownsChildSession());
        CHECK_TRUE(mux.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)));
        CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(mux.pendingResult(), result));
        CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Expired, result.status);
        CHECK_TRUE(result.payload.empty());
        CHECK_EQ(blinker::ErrorCode::WouldBlock,
            router.handleCommand(blinker::ByteView(permitBytes, permitSize)).code());
        mux.markResultPublished();
        // It can now reach the selected validator, rather than being blocked
        // by the mux's still-owned renewal session.
        CHECK_EQ(blinker::ErrorCode::InvalidEncoding,
            router.handleCommand(blinker::ByteView(permitBytes, permitSize)).code());
        CHECK_EQ(1U, channel.sends);
        CHECK_EQ(0, std::memcmp(originalActive.data(), activeBlob.bytes, originalActive.size()));
        CHECK_EQ(0, std::memcmp(originalPending.data(), pendingBlob.bytes, originalPending.size()));
        return;
    }

    std::vector<uint8_t> grant = rotateGrant(
        staged, blinker::ByteView(nonce, sizeof(nonce)));
    EncodedCommand apply = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Apply,
        blinker::ByteView(grant.data(), grant.size()));
    CHECK_TRUE(coordinator.continuesCommand(blinker::ByteView(apply.bytes, apply.size)));
    class Admission final : public blinker::IGatewayManagementAdmission {
    public:
        bool allowed = false;
        bool allowNewManagementWork() override { return allowed; }
    } admission;
    if (boundary <= 1U) {
        mux.setAdmission(admission);
        blinker::GatewayCredentialRenewalRecord wrong = staged; ++wrong.accessEpoch;
        EncodedCommand other = command(wrong, blinker::gateway::GatewayCredentialRenewalPhase::Apply,
            blinker::ByteView(grant.data(), grant.size()));
        CHECK_EQ(blinker::ErrorCode::WouldBlock, mux.handleCommand(blinker::ByteView(other.bytes, other.size)).code());
        CHECK_EQ(blinker::ErrorCode::WouldBlock, router.handleCommand(blinker::ByteView(permitBytes, permitSize)).code());
        blinker::clearGatewayCredentialRenewalRecord(wrong);
    }
    if (boundary == 1U) { clock.monotonic += 99000; clock.unixSeconds = 999; }
    CHECK_TRUE(mux.handleCommand(blinker::ByteView(
        apply.bytes, apply.size)));
    blinker::GatewayCredentialRenewalRecord durable;
    CHECK_TRUE(pendingStore.load(durable));
    CHECK_EQ(blinker::GatewayCredentialRenewalState::MutationAttempted,
             durable.state);
    mux.poll();
    CHECK_EQ(blinker::bbp2::MessageKind::ControllerMutation, channel.kind);
    blinker::bbp2::ControllerMutationBody mutation;
    CHECK_TRUE(blinker::bbp2::decodeControllerMutationBody(
        blinker::ByteView(channel.body.data(), channel.body.size()),
        mutation));
    CHECK_TRUE(mutation.controllerSecret.size == sizeof(staged.secret));
    CHECK_TRUE(std::memcmp(mutation.controllerSecret.data, staged.secret,
                           sizeof(staged.secret)) == 0);

    std::vector<uint8_t> receipt = rotateReceipt(
        grant, blinker::ByteView(staged.secret, sizeof(staged.secret)));
    response = frame(
        blinker::bbp2::MessageKind::ControllerMutationReceipt,
        blinker::ByteView(receipt.data(), receipt.size()));
    const std::vector<uint8_t> attempted(pendingBlob.bytes, pendingBlob.bytes + pendingBlob.size);
    if (boundary == 1U) clock.monotonic += 1000U;
    channel.respond(blinker::ByteView(response.data(), response.size()));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        mux.pendingResult(), result));
    if (boundary == 1U) {
        // A cryptographically valid late receipt is not authority to promote.
        // Preserve the mutation journal for explicit forward recovery.
        CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::ForwardRecoveryRequired, result.status);
        CHECK_TRUE(!mux.ownsChildSession());
        CHECK_EQ(0, std::memcmp(originalActive.data(), activeBlob.bytes, originalActive.size()));
        CHECK_EQ(0, std::memcmp(attempted.data(), pendingBlob.bytes, attempted.size()));
        return;
    }
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Rotated,
             result.status);
    CHECK_TRUE(result.activeStorageRevision != 0U);
    CHECK_TRUE(result.payload.size == receipt.size());
    CHECK_TRUE(mux.ownsChildSession());
    if (boundary == 2U) {
        channel.blockRetirement = true;
        mux.poll();
        CHECK_TRUE(mux.busy());
        CHECK_TRUE(mux.ownsChildSession());
        mux.markResultPublished();
        CHECK_EQ(blinker::ErrorCode::StateConflict,
            mux.handleCommand(blinker::ByteView(prepare.bytes, prepare.size)).code());
        channel.blockRetirement = false;
    }
    if (boundary == 3U) ++channel.authenticatedAttemptId;
    mux.poll();
    CHECK_EQ(boundary == 3U ? 0U : boundary == 2U ? 2U : 1U, channel.retireCalls);
    CHECK_TRUE(!mux.ownsChildSession());
    CHECK_TRUE(!mux.busy());

    blinker::GatewayAccessRecord promoted;
    CHECK_TRUE(activeStore.load(promoted));
    CHECK_EQ(staged.credentialVersion, promoted.credentialVersion);
    CHECK_TRUE(std::memcmp(promoted.secret, staged.secret,
                           sizeof(staged.secret)) == 0);
    CHECK_TRUE(pendingStore.load(durable));
    CHECK_EQ(blinker::GatewayCredentialRenewalState::Promoted,
             durable.state);
    CHECK_EQ(promoted.deliveryRevision, durable.activeStorageRevision);

    uint8_t receiptDigest[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::sha256(
        blinker::ByteView(receipt.data(), receipt.size()),
        blinker::MutableByteSpan(receiptDigest, sizeof(receiptDigest))));
    mux.markResultPublished();
    EncodedCommand finalize = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Finalize,
        blinker::ByteView(),
        blinker::ByteView(receiptDigest, sizeof(receiptDigest)),
        promoted.deliveryRevision);
    if (boundary == 0U) {
        CHECK_EQ(blinker::ErrorCode::WouldBlock, mux.handleCommand(blinker::ByteView(finalize.bytes, finalize.size)).code());
        admission.allowed = true;
    }
    CHECK_TRUE(mux.handleCommand(blinker::ByteView(
        finalize.bytes, finalize.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        mux.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Finalized,
             result.status);
    CHECK_TRUE(!pendingBlob.present);

    // A reset after the durable clear but before MQTT result publication
    // must replay the terminal fact instead of changing it to Rejected.
    mux.reset();
    CHECK_TRUE(mux.handleCommand(blinker::ByteView(
        finalize.bytes, finalize.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        mux.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Finalized,
             result.status);
    mux.reset();
    EncodedCommand cancelled = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Cancel);
    CHECK_TRUE(mux.handleCommand(blinker::ByteView(
        cancelled.bytes, cancelled.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        mux.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Cancelled,
             result.status);

    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayAccessRecord(promoted);
    blinker::clearGatewayCredentialRenewalRecord(staged);
    blinker::clearGatewayCredentialRenewalRecord(durable);
}

enum class ApplyRestartBoundary : uint8_t {
    MutationAttempted,
    ReceiptStored,
    ActivePromoted,
    JournalPromoted
};

void testRestartRecoveryBoundaries() {
    const ApplyRestartBoundary boundaries[] = {
        ApplyRestartBoundary::MutationAttempted,
        ApplyRestartBoundary::ReceiptStored,
        ApplyRestartBoundary::ActivePromoted,
        ApplyRestartBoundary::JournalPromoted
    };
    for (size_t index = 0U;
         index < sizeof(boundaries) / sizeof(boundaries[0]); ++index) {
        MemoryBlob activeBlob;
        MemoryBlob pendingBlob;
        blinker::GatewayAccessStore activeStore(activeBlob);
        blinker::GatewayCredentialRenewalStore pendingStore(pendingBlob);
        blinker::GatewayAccessRecord oldActive = activeRecord();
        blinker::GatewayAccessCommitOutcome accessOutcome;
        CHECK_TRUE(activeStore.commit(oldActive, accessOutcome));
        blinker::GatewayCredentialRenewalRecord staged =
            pendingRecord(oldActive);
        blinker::GatewayCredentialRenewalCommitOutcome pendingOutcome;
        CHECK_TRUE(pendingStore.commit(staged, pendingOutcome));
        staged.deliveryRevision = pendingOutcome.pendingStorageRevision;

        Clock clock;
        uint8_t nonce[blinker::kControllerControlNonceSize];
        fill(nonce, sizeof(nonce), 0xA1U);
        std::vector<uint8_t> grant = rotateGrant(
            staged, blinker::ByteView(nonce, sizeof(nonce)));
        std::vector<uint8_t> receipt = rotateReceipt(
            grant, blinker::ByteView(staged.secret, sizeof(staged.secret)));
        EncodedCommand prepare = command(
            staged,
            blinker::gateway::GatewayCredentialRenewalPhase::Prepare);
        EncodedCommand apply = command(
            staged, blinker::gateway::GatewayCredentialRenewalPhase::Apply,
            blinker::ByteView(grant.data(), grant.size()));

        // A reset after sending Prepare has no hidden in-memory recovery
        // state: the same durable command simply opens a fresh control nonce.
        {
            ControlChannel interrupted;
            blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
                coordinator(interrupted, activeStore, pendingStore, clock);
            interrupted.setControllerControlReceiver(
                &blinker::IGatewayManagementControl::
                    controllerControlResponseThunk,
                &coordinator);
            CHECK_TRUE(coordinator.handleCommand(
                blinker::ByteView(prepare.bytes, prepare.size)));
            coordinator.poll();
            CHECK_EQ(blinker::bbp2::MessageKind::ControllerControlOpen,
                     interrupted.kind);
        }
        {
            ControlChannel resumed;
            resumed.authenticatedAttemptId = 1U;
            blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
                coordinator(resumed, activeStore, pendingStore, clock);
            resumed.setControllerControlReceiver(
                &blinker::IGatewayManagementControl::
                    controllerControlResponseThunk,
                &coordinator);
            CHECK_TRUE(coordinator.handleCommand(
                blinker::ByteView(prepare.bytes, prepare.size)));
            coordinator.poll();
            CHECK_EQ(blinker::bbp2::MessageKind::ControllerControlOpen,
                     resumed.kind);
            blinker::bbp2::ControllerControlChallengeBody challenge;
            challenge.controlNonce = blinker::ByteView(nonce, sizeof(nonce));
            uint8_t challengeBytes[32U] = {};
            blinker::ByteView challengeBody;
            CHECK_TRUE(blinker::bbp2::encodeControllerControlChallengeBody(
                challenge,
                blinker::MutableByteSpan(
                    challengeBytes, sizeof(challengeBytes)),
                challengeBody));
            std::vector<uint8_t> response = frame(
                blinker::bbp2::MessageKind::ControllerControlChallenge,
                challengeBody);
            resumed.respond(blinker::ByteView(
                response.data(), response.size()));
            blinker::gateway::GatewayCredentialRenewalResultView result;
            CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
                coordinator.pendingResult(), result));
            CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::
                         NonceReady,
                     result.status);
        }

        uint8_t grantDigest[blinker::kSha256Size] = {};
        CHECK_TRUE(blinker::sha256(
            blinker::ByteView(grant.data(), grant.size()),
            blinker::MutableByteSpan(grantDigest, sizeof(grantDigest))));
        uint32_t journalRevision = 0U;
        CHECK_TRUE(pendingStore.markMutationAttempted(
            blinker::ByteView(staged.renewalOperationId,
                              sizeof(staged.renewalOperationId)),
            blinker::ByteView(staged.topologyOperationId,
                              sizeof(staged.topologyOperationId)),
            blinker::ByteView(staged.accessMaterialDigest,
                              sizeof(staged.accessMaterialDigest)),
            blinker::ByteView(grantDigest, sizeof(grantDigest)),
            journalRevision));

        uint8_t receiptDigest[blinker::kSha256Size] = {};
        CHECK_TRUE(blinker::sha256(
            blinker::ByteView(receipt.data(), receipt.size()),
            blinker::MutableByteSpan(
                receiptDigest, sizeof(receiptDigest))));
        if (boundaries[index] != ApplyRestartBoundary::MutationAttempted) {
            CHECK_TRUE(pendingStore.storeReceipt(
                blinker::ByteView(staged.renewalOperationId,
                                  sizeof(staged.renewalOperationId)),
                blinker::ByteView(staged.topologyOperationId,
                                  sizeof(staged.topologyOperationId)),
                blinker::ByteView(staged.accessMaterialDigest,
                                  sizeof(staged.accessMaterialDigest)),
                blinker::ByteView(grantDigest, sizeof(grantDigest)),
                blinker::ByteView(receipt.data(), receipt.size()),
                journalRevision));
        }
        uint32_t activeRevision = 0U;
        if (boundaries[index] == ApplyRestartBoundary::ActivePromoted ||
            boundaries[index] == ApplyRestartBoundary::JournalPromoted) {
            blinker::GatewayCredentialRenewalRecord durable;
            CHECK_TRUE(pendingStore.load(durable));
            CHECK_TRUE(activeStore.promoteCredentialRenewal(
                durable, activeRevision));
            blinker::clearGatewayCredentialRenewalRecord(durable);
        }
        if (boundaries[index] == ApplyRestartBoundary::JournalPromoted) {
            CHECK_TRUE(pendingStore.markPromoted(
                blinker::ByteView(staged.renewalOperationId,
                                  sizeof(staged.renewalOperationId)),
                blinker::ByteView(staged.topologyOperationId,
                                  sizeof(staged.topologyOperationId)),
                blinker::ByteView(staged.accessMaterialDigest,
                                  sizeof(staged.accessMaterialDigest)),
                blinker::ByteView(receiptDigest, sizeof(receiptDigest)),
                activeRevision, journalRevision));
        }

        blinker::GatewayAccessRecord beforeResume;
        CHECK_TRUE(activeStore.load(beforeResume));
        if (boundaries[index] == ApplyRestartBoundary::MutationAttempted ||
            boundaries[index] == ApplyRestartBoundary::ReceiptStored) {
            CHECK_EQ(staged.expectedCredentialVersion,
                     beforeResume.credentialVersion);
        } else {
            CHECK_EQ(staged.credentialVersion,
                     beforeResume.credentialVersion);
        }
        blinker::clearGatewayAccessRecord(beforeResume);

        ControlChannel resumed;
        blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
            coordinator(resumed, activeStore, pendingStore, clock);
        resumed.setControllerControlReceiver(
            &blinker::IGatewayManagementControl::controllerControlResponseThunk,
            &coordinator);
        CHECK_TRUE(coordinator.handleCommand(
            blinker::ByteView(apply.bytes, apply.size)));
        if (boundaries[index] == ApplyRestartBoundary::MutationAttempted) {
            // Recovery must bring up a management-only DirectSecure session;
            // the data route can be withdrawn at this fail-closed boundary.
            coordinator.poll();
            CHECK_EQ(1U, resumed.ensureCalls);
            CHECK_TRUE(coordinator.pendingResult().empty());
            resumed.authenticatedVersion = staged.credentialVersion;
            resumed.authenticatedAttemptId = 2U;
            // A new Method 2 session with N+1 is the durable evidence that the
            // child committed before the old-session receipt was lost.
            coordinator.poll();
            CHECK_EQ(0U, resumed.sends);
        } else {
            coordinator.poll();
        }
        blinker::gateway::GatewayCredentialRenewalResultView result;
        CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
            coordinator.pendingResult(), result));
        CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Rotated,
                 result.status);
        if (boundaries[index] == ApplyRestartBoundary::MutationAttempted) {
            CHECK_TRUE(result.payload.size == receipt.size());
            CHECK_TRUE(std::memcmp(
                result.payload.data, receipt.data(), receipt.size()) == 0);
        }

        blinker::GatewayAccessRecord promoted;
        CHECK_TRUE(activeStore.load(promoted));
        CHECK_EQ(staged.credentialVersion, promoted.credentialVersion);
        blinker::GatewayCredentialRenewalRecord durable;
        CHECK_TRUE(pendingStore.load(durable));
        CHECK_EQ(blinker::GatewayCredentialRenewalState::Promoted,
                 durable.state);
        CHECK_EQ(promoted.deliveryRevision, durable.activeStorageRevision);

        // A second reboot must replay the exact durable terminal fact without
        // another child mutation or another active-slot version increase.
        ControlChannel replayChannel;
        blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
            replayed(replayChannel, activeStore, pendingStore, clock);
        CHECK_TRUE(replayed.handleCommand(
            blinker::ByteView(apply.bytes, apply.size)));
        replayed.poll();
        CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
            replayed.pendingResult(), result));
        CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Rotated,
                 result.status);
        CHECK_EQ(0U, replayChannel.sends);

        blinker::clearGatewayAccessRecord(oldActive);
        blinker::clearGatewayAccessRecord(promoted);
        blinker::clearGatewayCredentialRenewalRecord(staged);
        blinker::clearGatewayCredentialRenewalRecord(durable);
        blinker::secureZero(blinker::MutableByteSpan(
            nonce, sizeof(nonce)));
        blinker::secureZero(blinker::MutableByteSpan(
            grantDigest, sizeof(grantDigest)));
        blinker::secureZero(blinker::MutableByteSpan(
            receiptDigest, sizeof(receiptDigest)));
    }
}

void testRestartProvesMutationWasNotCommitted() {
    MemoryBlob activeBlob;
    MemoryBlob pendingBlob;
    blinker::GatewayAccessStore activeStore(activeBlob);
    blinker::GatewayCredentialRenewalStore pendingStore(pendingBlob);
    blinker::GatewayAccessRecord active = activeRecord();
    blinker::GatewayAccessCommitOutcome accessOutcome;
    CHECK_TRUE(activeStore.commit(active, accessOutcome));
    blinker::GatewayCredentialRenewalRecord staged = pendingRecord(active);
    blinker::GatewayCredentialRenewalCommitOutcome pendingOutcome;
    CHECK_TRUE(pendingStore.commit(staged, pendingOutcome));
    staged.deliveryRevision = pendingOutcome.pendingStorageRevision;

    uint8_t nonce[blinker::kControllerControlNonceSize];
    fill(nonce, sizeof(nonce), 0xA1U);
    std::vector<uint8_t> grant = rotateGrant(
        staged, blinker::ByteView(nonce, sizeof(nonce)));
    uint8_t grantDigest[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::sha256(
        blinker::ByteView(grant.data(), grant.size()),
        blinker::MutableByteSpan(grantDigest, sizeof(grantDigest))));
    uint32_t journalRevision = 0U;
    CHECK_TRUE(pendingStore.markMutationAttempted(
        blinker::ByteView(staged.renewalOperationId,
                          sizeof(staged.renewalOperationId)),
        blinker::ByteView(staged.topologyOperationId,
                          sizeof(staged.topologyOperationId)),
        blinker::ByteView(staged.accessMaterialDigest,
                          sizeof(staged.accessMaterialDigest)),
        blinker::ByteView(grantDigest, sizeof(grantDigest)),
        journalRevision));
    EncodedCommand apply = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Apply,
        blinker::ByteView(grant.data(), grant.size()));

    Clock clock;
    ControlChannel channel;
    channel.authenticatedVersion = staged.expectedCredentialVersion;
    channel.authenticatedAttemptId = 2U;
    blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
        coordinator(channel, activeStore, pendingStore, clock);
    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(apply.bytes, apply.size)));
    coordinator.poll();
    blinker::gateway::GatewayCredentialRenewalResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        coordinator.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Rejected,
             result.status);
    CHECK_EQ(0U, channel.sends);

    blinker::GatewayCredentialRenewalRecord durable;
    CHECK_TRUE(pendingStore.load(durable));
    CHECK_EQ(blinker::GatewayCredentialRenewalState::Abandoned,
             durable.state);
    blinker::GatewayAccessRecord unchanged;
    CHECK_TRUE(activeStore.load(unchanged));
    CHECK_EQ(staged.expectedCredentialVersion,
             unchanged.credentialVersion);

    // A lost Rejected result is replayed from the terminal journal marker.
    ControlChannel replayChannel;
    blinker::BasicGatewayCredentialRenewalCoordinator<ControlChannel>
        replayed(replayChannel, activeStore, pendingStore, clock);
    CHECK_TRUE(replayed.handleCommand(
        blinker::ByteView(apply.bytes, apply.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        replayed.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Rejected,
             result.status);
    CHECK_EQ(0U, replayChannel.sends);

    EncodedCommand cancel = command(
        staged, blinker::gateway::GatewayCredentialRenewalPhase::Cancel);
    replayed.markResultPublished();
    CHECK_TRUE(replayed.handleCommand(
        blinker::ByteView(cancel.bytes, cancel.size)));
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalResult(
        replayed.pendingResult(), result));
    CHECK_EQ(blinker::gateway::GatewayCredentialRenewalStatus::Cancelled,
             result.status);
    CHECK_TRUE(!pendingBlob.present);

    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayAccessRecord(unchanged);
    blinker::clearGatewayCredentialRenewalRecord(staged);
    blinker::clearGatewayCredentialRenewalRecord(durable);
    blinker::secureZero(blinker::MutableByteSpan(nonce, sizeof(nonce)));
    blinker::secureZero(blinker::MutableByteSpan(
        grantDigest, sizeof(grantDigest)));
}

void testDirectoryRenewalCommands() {
    using namespace blinker;
    class Delivery final : public IGatewayManagementDelivery {
        Result process(const GatewayAccessDeliveryContext&, ByteView, MutableByteSpan, size_t& written) override {
            written = 0; return Result::failure(ErrorCode::UnsupportedFeature);
        }
    } delivery;
    MemoryBlob aBlob, aJournal, bBlob, bJournal;
    GatewayChildRecords a(aBlob, aJournal), b(bBlob, bJournal);
    auto first = activeRecord(), second = first;
    second.operationId[0] ^= 1; second.childDeviceInstanceId[0] ^= 1;
    second.childLogicalDeviceId[second.childLogicalDeviceIdSize - 1] = '2';
    auto aPending = pendingRecord(first), bPending = pendingRecord(second); bPending.renewalOperationId[0] ^= 1;
    GatewayAccessCommitOutcome accessOutcome;
    CHECK_TRUE(a.access.commit(first, accessOutcome)); CHECK_TRUE(b.access.commit(second, accessOutcome));
    GatewayCredentialRenewalCommitOutcome outcome;
    CHECK_TRUE(a.renewal.commit(aPending, outcome)); aPending.deliveryRevision = outcome.pendingStorageRevision;
    CHECK_TRUE(b.renewal.commit(bPending, outcome)); bPending.deliveryRevision = outcome.pendingStorageRevision;
    Clock clock; ControlChannel aChannel, bChannel;
    BasicGatewayCredentialRenewalCoordinator<ControlChannel> aRenewal(aChannel, a.access, a.renewal, clock);
    BasicGatewayCredentialRenewalCoordinator<ControlChannel> bRenewal(bChannel, b.access, b.renewal, clock);
    RejectControl unused;
    GatewayManagementControlMux aMux(unused, unused, aRenewal), bMux(unused, unused, bRenewal);
    GatewayChildDirectoryEntry entries[] = {{a, delivery, &aMux}, {b, delivery, &bMux}};
    GatewayChildDirectory directory(entries, 2); GatewayChildManagementRouter router(directory, unused);
    const auto prepareA = command(aPending, gateway::GatewayCredentialRenewalPhase::Prepare);
    const auto prepareB = command(bPending, gateway::GatewayCredentialRenewalPhase::Prepare);
    auto wrong = aPending; wrong.renewalOperationId[0] ^= 0x40;
    const auto wrongCommand = command(wrong, gateway::GatewayCredentialRenewalPhase::Prepare);
    CHECK_EQ(ErrorCode::StateConflict, router.handleCommand(ByteView(wrongCommand.bytes, wrongCommand.size)).code());
    CHECK_EQ(0U, aChannel.sends); CHECK_EQ(0U, bChannel.sends);
    CHECK_TRUE(router.handleCommand(ByteView(prepareA.bytes, prepareA.size))); router.poll();
    CHECK_EQ(1U, aChannel.sends); CHECK_EQ(0U, bChannel.sends);
    CHECK_EQ(ErrorCode::WouldBlock, router.handleCommand(ByteView(prepareB.bytes, prepareB.size)).code());
    // Cancel only A's still-staged journal. B's pending credential survives.
    const auto cancelA = command(aPending, gateway::GatewayCredentialRenewalPhase::Cancel);
    router.reset(); // Explicitly retire this test's waiting nonce request first.
    CHECK_TRUE(router.handleCommand(ByteView(cancelA.bytes, cancelA.size)));
    CHECK_TRUE(!aJournal.present); CHECK_TRUE(bJournal.present);
    gateway::GatewayCredentialRenewalResultView result;
    CHECK_TRUE(gateway::decodeGatewayCredentialRenewalResult(router.pendingResult(), result));
    CHECK_EQ(gateway::GatewayCredentialRenewalStatus::Cancelled, result.status);
    router.markResultPublished(); router.poll();
    CHECK_TRUE(router.handleCommand(ByteView(prepareB.bytes, prepareB.size))); router.poll();
    CHECK_EQ(1U, bChannel.sends);
    clearGatewayAccessRecord(first); clearGatewayAccessRecord(second);
    clearGatewayCredentialRenewalRecord(aPending); clearGatewayCredentialRenewalRecord(bPending); clearGatewayCredentialRenewalRecord(wrong);
}

} // namespace

int main() {
    testDirectoryRenewalCommands();
    for (unsigned boundary = 0U; boundary < 10U; ++boundary) {
        testPrepareRotatePromoteFinalize(boundary);
    }
    testRestartRecoveryBoundaries();
    testRestartProvesMutationWasNotCommitted();
    std::cout << "Gateway credential renewal coordinator tests passed"
              << std::endl;
    return 0;
}
