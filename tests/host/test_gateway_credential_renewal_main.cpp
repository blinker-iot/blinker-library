#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/identity/GatewayCredentialRenewalStore.h>
#include <BlinkerV2/runtime/GatewayCredentialRenewalDeliveryProcessor.h>
#include <BlinkerV2/runtime/GatewayChildDirectory.h>
#include <BlinkerV2/security/GatewayCredentialRenewalEnvelopeSecurity.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Gateway renewal check failed at line "              \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

bool equal(blinker::ByteView first, blinker::ByteView second) {
    return first.size == second.size &&
           (first.empty() || std::memcmp(
               first.data, second.data, first.size) == 0);
}

class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    explicit MemoryBlob(
        blinker::StorageProtection protection =
            blinker::StorageProtection::EncryptedFlash)
        : bytes(), size(0U), present(false), failNextReplace(false),
          replaceCalls(0U), protection_(protection) {}

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
        ++replaceCalls;
        if (failNextReplace) {
            failNextReplace = false;
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        CHECK_TRUE(value.data != nullptr && value.size <= sizeof(bytes));
        std::memset(bytes, 0, sizeof(bytes));
        std::memcpy(bytes, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        blinker::secureZero(blinker::MutableByteSpan(bytes, sizeof(bytes)));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return protection_;
    }

    uint8_t bytes[blinker::GatewayCredentialRenewalStore::serializedSize];
    size_t size;
    bool present;
    bool failNextReplace;
    uint32_t replaceCalls;

private:
    blinker::StorageProtection protection_;
};

void fill(uint8_t* output, size_t size, uint8_t value) {
    std::memset(output, value, size);
}

blinker::GatewayCredentialRenewalRecord candidate() {
    blinker::GatewayCredentialRenewalRecord value;
    fill(value.renewalOperationId, sizeof(value.renewalOperationId), 0x11U);
    fill(value.topologyOperationId, sizeof(value.topologyOperationId), 0x12U);
    fill(value.childDeviceInstanceId,
         sizeof(value.childDeviceInstanceId), 0x22U);
    value.accessEpoch = 9U;
    fill(value.controllerId, sizeof(value.controllerId), 0x33U);
    value.expectedCredentialVersion = 4U;
    value.credentialVersion = 5U;
    value.presenceKeyVersion = 3U;
    value.expiresAtUnixSeconds = 2000000000ULL;
    fill(value.secret, sizeof(value.secret), 0x43U);
    fill(value.accessMaterialDigest,
         sizeof(value.accessMaterialDigest), 0x44U);
    return value;
}

blinker::ByteView renewalId(
    const blinker::GatewayCredentialRenewalRecord& value) {
    return blinker::ByteView(
        value.renewalOperationId, sizeof(value.renewalOperationId));
}

blinker::ByteView topologyId(
    const blinker::GatewayCredentialRenewalRecord& value) {
    return blinker::ByteView(
        value.topologyOperationId, sizeof(value.topologyOperationId));
}

blinker::ByteView materialDigest(
    const blinker::GatewayCredentialRenewalRecord& value) {
    return blinker::ByteView(
        value.accessMaterialDigest, sizeof(value.accessMaterialDigest));
}

void testProtectedJournal() {
    blinker::GatewayCredentialRenewalRecord input = candidate();
    blinker::GatewayCredentialRenewalCommitOutcome outcome;

    MemoryBlob plain(blinker::StorageProtection::PlainFlash);
    blinker::GatewayCredentialRenewalStore plainStore(plain);
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             plainStore.commit(input, outcome).code());

    MemoryBlob memory;
    blinker::GatewayCredentialRenewalStore store(memory);
    CHECK_TRUE(store.commit(input, outcome).ok());
    CHECK_EQ(blinker::GatewayCredentialRenewalCommitKind::Stored,
             outcome.kind);
    CHECK_EQ(1U, outcome.pendingStorageRevision);
    CHECK_EQ(1U, memory.replaceCalls);

    blinker::GatewayCredentialRenewalStore reopened(memory);
    blinker::GatewayCredentialRenewalRecord loaded;
    CHECK_TRUE(reopened.load(loaded).ok());
    CHECK_EQ(1U, loaded.storageRevision);
    CHECK_EQ(1U, loaded.deliveryRevision);
    CHECK_EQ(blinker::GatewayCredentialRenewalState::Staged, loaded.state);

    CHECK_TRUE(reopened.commit(input, outcome).ok());
    CHECK_EQ(blinker::GatewayCredentialRenewalCommitKind::ExactReplay,
             outcome.kind);
    CHECK_EQ(1U, outcome.pendingStorageRevision);
    CHECK_EQ(1U, memory.replaceCalls);

    blinker::GatewayCredentialRenewalRecord changed = input;
    changed.secret[0] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             reopened.commit(changed, outcome).code());

    uint8_t grantDigestBytes[blinker::kSha256Size];
    fill(grantDigestBytes, sizeof(grantDigestBytes), 0x55U);
    const blinker::ByteView grantDigest(
        grantDigestBytes, sizeof(grantDigestBytes));
    uint32_t revision = 0U;
    uint8_t prematureReceipt = 0x01U;
    uint8_t prematureReceiptDigest[blinker::kSha256Size];
    fill(prematureReceiptDigest, sizeof(prematureReceiptDigest), 0x56U);
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             reopened.storeReceipt(
                 renewalId(input), topologyId(input), materialDigest(input),
                 grantDigest,
                 blinker::ByteView(&prematureReceipt, 1U), revision).code());
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             reopened.markPromoted(
                 renewalId(input), topologyId(input), materialDigest(input),
                 blinker::ByteView(
                     prematureReceiptDigest,
                     sizeof(prematureReceiptDigest)),
                 17U, revision).code());
    CHECK_TRUE(reopened.markMutationAttempted(
        renewalId(input), topologyId(input), materialDigest(input),
        grantDigest, revision).ok());
    CHECK_EQ(2U, revision);
    CHECK_EQ(2U, memory.replaceCalls);

    CHECK_TRUE(reopened.commit(input, outcome).ok());
    CHECK_EQ(blinker::GatewayCredentialRenewalCommitKind::ExactReplay,
             outcome.kind);
    CHECK_EQ(1U, outcome.pendingStorageRevision);
    CHECK_EQ(2U, memory.replaceCalls);
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             reopened.cancel(
                 renewalId(input), topologyId(input),
                 materialDigest(input)).code());

    CHECK_TRUE(reopened.markMutationAttempted(
        renewalId(input), topologyId(input), materialDigest(input),
        grantDigest, revision).ok());
    CHECK_EQ(2U, revision);
    CHECK_EQ(2U, memory.replaceCalls);

    uint8_t receipt[145];
    for (size_t index = 0U; index < sizeof(receipt); ++index) {
        receipt[index] = static_cast<uint8_t>(index + 1U);
    }
    CHECK_TRUE(reopened.storeReceipt(
        renewalId(input), topologyId(input), materialDigest(input),
        grantDigest, blinker::ByteView(receipt, sizeof(receipt)),
        revision).ok());
    CHECK_EQ(3U, revision);
    CHECK_EQ(3U, memory.replaceCalls);
    CHECK_TRUE(reopened.load(loaded).ok());
    CHECK_EQ(blinker::GatewayCredentialRenewalState::ReceiptStored,
             loaded.state);

    uint8_t receiptDigestBytes[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::GatewayCredentialRenewalStore::receiptDigest(
        loaded, blinker::MutableByteSpan(
                    receiptDigestBytes, sizeof(receiptDigestBytes))).ok());
    CHECK_TRUE(reopened.markPromoted(
        renewalId(input), topologyId(input), materialDigest(input),
        blinker::ByteView(receiptDigestBytes, sizeof(receiptDigestBytes)),
        17U, revision).ok());
    CHECK_EQ(4U, revision);
    CHECK_EQ(4U, memory.replaceCalls);

    uint8_t wrongDigest[blinker::kSha256Size];
    fill(wrongDigest, sizeof(wrongDigest), 0xFFU);
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             reopened.finalize(
                 renewalId(input), topologyId(input), materialDigest(input),
                 blinker::ByteView(wrongDigest, sizeof(wrongDigest)),
                 17U).code());
    CHECK_TRUE(memory.present);
    CHECK_TRUE(reopened.finalize(
        renewalId(input), topologyId(input), materialDigest(input),
        blinker::ByteView(receiptDigestBytes, sizeof(receiptDigestBytes)),
        17U).ok());
    CHECK_TRUE(!memory.present);
    CHECK_TRUE(reopened.finalize(
        renewalId(input), topologyId(input), materialDigest(input),
        blinker::ByteView(receiptDigestBytes, sizeof(receiptDigestBytes)),
        17U).ok());

    CHECK_TRUE(reopened.commit(input, outcome).ok());
    CHECK_TRUE(reopened.cancel(
        renewalId(input), topologyId(input), materialDigest(input)).ok());
    CHECK_TRUE(!memory.present);

    memory.failNextReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
             reopened.commit(input, outcome).code());
    CHECK_TRUE(!memory.present);
    CHECK_TRUE(reopened.commit(input, outcome).ok());
    memory.bytes[40] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::InvalidEncoding,
             reopened.load(loaded).code());

    blinker::clearGatewayCredentialRenewalRecord(input);
    blinker::clearGatewayCredentialRenewalRecord(changed);
    blinker::clearGatewayCredentialRenewalRecord(loaded);
    blinker::secureZero(blinker::MutableByteSpan(
        grantDigestBytes, sizeof(grantDigestBytes)));
    blinker::secureZero(blinker::MutableByteSpan(
        prematureReceiptDigest, sizeof(prematureReceiptDigest)));
    blinker::secureZero(blinker::MutableByteSpan(
        receiptDigestBytes, sizeof(receiptDigestBytes)));
    blinker::secureZero(blinker::MutableByteSpan(
        wrongDigest, sizeof(wrongDigest)));
    blinker::secureZero(blinker::MutableByteSpan(receipt, sizeof(receipt)));
}

class ExpectedDecryptor final
    : public blinker::IX25519AesGcmCryptoProvider {
public:
    ExpectedDecryptor(
        blinker::ByteView expectedKey,
        blinker::ByteView expectedAad,
        blinker::ByteView plaintext)
        : calls(0U), expectedKey_(expectedKey), expectedAad_(expectedAad),
          plaintext_(plaintext) {}

    blinker::Result x25519PublicKey(
        blinker::ByteView, blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }
    blinker::Result x25519(
        blinker::ByteView, blinker::ByteView,
        blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }
    blinker::Result aes256GcmEncrypt(
        blinker::ByteView, blinker::ByteView, blinker::ByteView,
        blinker::ByteView, blinker::MutableByteSpan,
        size_t& written) override {
        written = 0U;
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }
    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        ++calls;
        if (!equal(key, expectedKey_) || !equal(associatedData, expectedAad_) ||
            nonce.size != blinker::kAesGcmNonceSize ||
            ciphertextAndTag.size !=
                blinker::gateway::kGatewaySecretSize +
                    blinker::kAesGcmTagSize ||
            output.data == nullptr || output.size < plaintext_.size) {
            return blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
        }
        std::memcpy(output.data, plaintext_.data, plaintext_.size);
        written = plaintext_.size;
        return blinker::Result::success();
    }

    uint32_t calls;

private:
    blinker::ByteView expectedKey_;
    blinker::ByteView expectedAad_;
    blinker::ByteView plaintext_;
};

class FixedKeySource final : public blinker::IDeviceKeySource {
public:
    explicit FixedKeySource(const blinker::DeviceKey& value) : key(value) {}
    blinker::Result load(blinker::DeviceKey& output) override {
        output = key;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }
    blinker::DeviceKey key;
};

class FixedClock final : public blinker::IClock {
public:
    uint32_t monotonicMillis() const override { return 0U; }
    bool unixTime(uint64_t& output) const override {
        output = 1700000000ULL;
        return true;
    }
};

void testRenewalEnvelopeSecurity() {
    blinker::DeviceKey deviceKey;
    fill(deviceKey.bytes, sizeof(deviceKey.bytes), 0x42U);
    const char environment[] = "prod-cn";
    const char tenant[] = "tenant-edge";
    const char hub[] = "edge-hub-1";
    const char child[] = "child-1";
    uint8_t renewalOperation[16];
    uint8_t topologyOperation[16];
    uint8_t childInstance[16];
    uint8_t controller[16];
    uint8_t nonce[12];
    uint8_t ciphertext[32];
    uint8_t tag[16];
    uint8_t secret[32];
    uint8_t presenceKey[16];
    fill(renewalOperation, sizeof(renewalOperation), 0x11U);
    fill(topologyOperation, sizeof(topologyOperation), 0x12U);
    fill(childInstance, sizeof(childInstance), 0x22U);
    fill(controller, sizeof(controller), 0x33U);
    fill(nonce, sizeof(nonce), 0x44U);
    fill(ciphertext, sizeof(ciphertext), 0x45U);
    fill(tag, sizeof(tag), 0x46U);
    fill(secret, sizeof(secret), 0x43U);
    fill(presenceKey, sizeof(presenceKey), 0x47U);

    uint8_t digest[blinker::kSha256Size] = {};
    blinker::Sha256 hash;
    CHECK_TRUE(hash.update(blinker::ByteView(secret, sizeof(secret))).ok());
    CHECK_TRUE(hash.update(
        blinker::ByteView(presenceKey, sizeof(presenceKey))).ok());
    CHECK_TRUE(hash.finish(
        blinker::MutableByteSpan(digest, sizeof(digest))).ok());

    blinker::gateway::GatewayCredentialRenewalEnvelopeView envelope;
    envelope.renewalOperationId = blinker::ByteView(
        renewalOperation, sizeof(renewalOperation));
    envelope.topologyOperationId = blinker::ByteView(
        topologyOperation, sizeof(topologyOperation));
    envelope.environmentId = blinker::StringView(environment);
    envelope.tenantId = blinker::StringView(tenant);
    envelope.edgeHubLogicalDeviceId = blinker::StringView(hub);
    envelope.deviceKeyVersion = 7U;
    envelope.childLogicalDeviceId = blinker::StringView(child);
    envelope.childDeviceInstanceId = blinker::ByteView(
        childInstance, sizeof(childInstance));
    envelope.accessEpoch = 9U;
    envelope.controllerId = blinker::ByteView(controller, sizeof(controller));
    envelope.expectedCredentialVersion = 4U;
    envelope.credentialVersion = 5U;
    envelope.permissions = blinker::gateway::kGatewayPermissions;
    envelope.presenceKeyVersion = 3U;
    envelope.accessMaterialDigest = blinker::ByteView(digest, sizeof(digest));
    envelope.expiresAtUnixSeconds = 2000000000ULL;
    envelope.nonce = blinker::ByteView(nonce, sizeof(nonce));
    envelope.ciphertext = blinker::ByteView(ciphertext, sizeof(ciphertext));
    envelope.tag = blinker::ByteView(tag, sizeof(tag));

    blinker::GatewayAccessWrapContext context;
    context.environmentId = envelope.environmentId;
    context.tenantId = envelope.tenantId;
    context.edgeHubLogicalDeviceId = envelope.edgeHubLogicalDeviceId;
    context.deviceKeyVersion = envelope.deviceKeyVersion;
    uint8_t wrapKey[blinker::kAes256KeySize] = {};
    CHECK_TRUE(blinker::deriveGatewayCredentialRenewalWrapKey(
        deviceKey, context,
        blinker::MutableByteSpan(wrapKey, sizeof(wrapKey))).ok());
    const uint8_t expectedWrapKey[blinker::kAes256KeySize] = {
        0x01, 0x2a, 0x02, 0xf4, 0xfe, 0x36, 0x9d, 0x52,
        0xa6, 0xea, 0xcd, 0x3f, 0x8f, 0xe9, 0x98, 0xe9,
        0xd5, 0x02, 0x82, 0x4f, 0x50, 0x85, 0x36, 0x25,
        0x58, 0x3d, 0xf9, 0xe2, 0x4d, 0xc6, 0x4f, 0x37};
    CHECK_TRUE(equal(
        blinker::ByteView(wrapKey, sizeof(wrapKey)),
        blinker::ByteView(expectedWrapKey, sizeof(expectedWrapKey))));

    uint8_t expectedAad[
        blinker::gateway::kGatewayCredentialRenewalAadMaximumEncodedSize] = {};
    size_t expectedAadSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayCredentialRenewalAad(
        envelope,
        blinker::MutableByteSpan(expectedAad, sizeof(expectedAad)),
        expectedAadSize).ok());
    ExpectedDecryptor crypto(
        blinker::ByteView(wrapKey, sizeof(wrapKey)),
        blinker::ByteView(expectedAad, expectedAadSize),
        blinker::ByteView(secret, sizeof(secret)));
    uint8_t aadScratch[
        blinker::gateway::kGatewayCredentialRenewalAadMaximumEncodedSize] = {};
    uint8_t opened[32] = {};
    CHECK_TRUE(blinker::openGatewayCredentialRenewalEnvelope(
        deviceKey, envelope,
        blinker::ByteView(presenceKey, sizeof(presenceKey)), crypto,
        blinker::MutableByteSpan(aadScratch, sizeof(aadScratch)),
        blinker::MutableByteSpan(opened, sizeof(opened))).ok());
    CHECK_EQ(1U, crypto.calls);
    CHECK_TRUE(equal(blinker::ByteView(opened, sizeof(opened)),
                     blinker::ByteView(secret, sizeof(secret))));

    // The delivery processor binds the envelope to the exact active record,
    // commits only the separate protected pending journal and replays the
    // original durable ACK without rewriting either record.
    MemoryBlob activeMemory;
    MemoryBlob pendingMemory;
    blinker::GatewayChildRecords records(activeMemory, pendingMemory);
    auto& activeStore = records.access;
    auto& pendingStore = records.renewal;
    blinker::GatewayAccessRecord active;
    std::memcpy(active.operationId, topologyOperation,
                sizeof(active.operationId));
    fill(active.envelopeDigest, sizeof(active.envelopeDigest), 0x21U);
    fill(active.accessMaterialDigest,
         sizeof(active.accessMaterialDigest), 0x31U);
    active.childLogicalDeviceIdSize = sizeof(child) - 1U;
    std::memcpy(active.childLogicalDeviceId, child, sizeof(child) - 1U);
    std::memcpy(active.childDeviceInstanceId, childInstance,
                sizeof(active.childDeviceInstanceId));
    active.accessEpoch = envelope.accessEpoch;
    std::memcpy(active.controllerId, controller,
                sizeof(active.controllerId));
    active.credentialVersion = envelope.expectedCredentialVersion;
    active.permissions = blinker::gateway::kGatewayPermissions;
    active.expiresAtUnixSeconds = envelope.expiresAtUnixSeconds;
    fill(active.secret, sizeof(active.secret), 0x41U);
    active.activePresence.version = envelope.presenceKeyVersion;
    std::memcpy(active.activePresence.key, presenceKey,
                sizeof(active.activePresence.key));
    blinker::GatewayAccessCommitOutcome activeOutcome;
    CHECK_TRUE(activeStore.commit(active, activeOutcome));
    FixedKeySource keySource(deviceKey);
    FixedClock clock;
    blinker::GatewayCredentialRenewalDeliveryProcessor renewalProcessor(
        activeStore, pendingStore, keySource, crypto, clock);
    MemoryBlob otherActive, otherPending;
    blinker::GatewayChildRecords otherRecords(otherActive, otherPending);
    blinker::GatewayCredentialRenewalDeliveryProcessor otherProcessor(
        otherRecords.access, otherRecords.renewal, keySource, crypto, clock);
    auto other = active;
    other.childLogicalDeviceId[other.childLogicalDeviceIdSize - 1U] = '2';
    other.childDeviceInstanceId[0] ^= 1U; other.operationId[0] ^= 1U;
    CHECK_TRUE(otherRecords.access.commit(other, activeOutcome));
    blinker::clearGatewayAccessRecord(other);
    blinker::GatewayChildDirectoryEntry entries[] = {{otherRecords, otherProcessor}, {records, renewalProcessor}};
    blinker::GatewayChildDirectory processor(entries, 2U);
    uint8_t exactEnvelope[
        blinker::gateway::kGatewayCredentialRenewalEnvelopeMaximumEncodedSize]
        = {};
    size_t exactEnvelopeSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayCredentialRenewalEnvelope(
        envelope,
        blinker::MutableByteSpan(exactEnvelope, sizeof(exactEnvelope)),
        exactEnvelopeSize));
    blinker::GatewayAccessDeliveryContext deliveryContext;
    deliveryContext.environmentId = envelope.environmentId;
    deliveryContext.tenantId = envelope.tenantId;
    deliveryContext.edgeHubLogicalDeviceId =
        envelope.edgeHubLogicalDeviceId;
    deliveryContext.deviceKeyVersion = envelope.deviceKeyVersion;
    uint8_t ack[
        blinker::gateway::
            kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize] = {};
    size_t ackSize = 0U;
    {
        // Unknown renewal never allocates a new slot or reaches the wrong
        // child's crypto processor. A changed topology id also fails closed.
        auto unknown = envelope;
        const uint8_t unknownInstance[16] = {0x77};
        const uint8_t unknownOperation[16] = {0x78};
        unknown.childLogicalDeviceId = blinker::StringView("child-3");
        unknown.childDeviceInstanceId = blinker::ByteView(unknownInstance, sizeof(unknownInstance));
        unknown.topologyOperationId = blinker::ByteView(unknownOperation, sizeof(unknownOperation));
        uint8_t encoded[sizeof(exactEnvelope)] = {}; size_t size = 0;
        CHECK_TRUE(blinker::gateway::encodeGatewayCredentialRenewalEnvelope(unknown,
            blinker::MutableByteSpan(encoded, sizeof(encoded)), size));
        CHECK_EQ(blinker::ErrorCode::NotFound, processor.process(deliveryContext,
            blinker::ByteView(encoded, size), blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
        CHECK_EQ(0U, ackSize); CHECK_EQ(0U, pendingMemory.replaceCalls); CHECK_EQ(0U, otherPending.replaceCalls);
        unknown = envelope; unknown.topologyOperationId = blinker::ByteView(unknownOperation, sizeof(unknownOperation));
        CHECK_TRUE(blinker::gateway::encodeGatewayCredentialRenewalEnvelope(unknown,
            blinker::MutableByteSpan(encoded, sizeof(encoded)), size));
        CHECK_EQ(blinker::ErrorCode::StateConflict, processor.process(deliveryContext,
            blinker::ByteView(encoded, size), blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    }
    CHECK_TRUE(processor.process(
        deliveryContext,
        blinker::ByteView(exactEnvelope, exactEnvelopeSize),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    blinker::gateway::GatewayCredentialRenewalDeliveryAckView decodedAck;
    CHECK_TRUE(blinker::gateway::decodeGatewayCredentialRenewalDeliveryAck(
        blinker::ByteView(ack, ackSize), decodedAck));
    CHECK_EQ(1U, decodedAck.pendingStorageRevision);
    CHECK_EQ(0U, otherPending.replaceCalls);
    const blinker::GatewayChildIdentity* identity = nullptr;
    CHECK_TRUE(records.identity(identity)); CHECK_TRUE(identity->hasRenewal);
    CHECK_EQ(envelope.expectedCredentialVersion, identity->credentialVersion);
    blinker::GatewayCredentialRenewalRecord delivered;
    CHECK_TRUE(pendingStore.load(delivered));
    CHECK_EQ(blinker::GatewayCredentialRenewalState::Staged,
             delivered.state);
    CHECK_TRUE(std::memcmp(delivered.secret, secret,
                           sizeof(delivered.secret)) == 0);
    const uint32_t writesAfterDelivery = pendingMemory.replaceCalls;
    uint8_t replayAck[
        blinker::gateway::
            kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize] = {};
    size_t replayAckSize = 0U;
    blinker::GatewayCredentialRenewalDeliveryProcessor reopenedProcessor(
        activeStore, pendingStore, keySource, crypto, clock);
    CHECK_TRUE(reopenedProcessor.process(
        deliveryContext,
        blinker::ByteView(exactEnvelope, exactEnvelopeSize),
        blinker::MutableByteSpan(replayAck, sizeof(replayAck)),
        replayAckSize));
    CHECK_EQ(writesAfterDelivery, pendingMemory.replaceCalls);
    CHECK_TRUE(equal(blinker::ByteView(ack, ackSize),
                     blinker::ByteView(replayAck, replayAckSize)));
    uint8_t deliveryGrantDigest[blinker::kSha256Size];
    fill(deliveryGrantDigest, sizeof(deliveryGrantDigest), 0x88U);
    uint8_t deliveryReceipt = 0x01U;
    uint32_t transitionRevision = 0U;
    CHECK_TRUE(pendingStore.markMutationAttempted(
        decodedAck.renewalOperationId, decodedAck.topologyOperationId,
        decodedAck.accessMaterialDigest,
        blinker::ByteView(deliveryGrantDigest,
                          sizeof(deliveryGrantDigest)),
        transitionRevision));
    CHECK_TRUE(pendingStore.storeReceipt(
        decodedAck.renewalOperationId, decodedAck.topologyOperationId,
        decodedAck.accessMaterialDigest,
        blinker::ByteView(deliveryGrantDigest,
                          sizeof(deliveryGrantDigest)),
        blinker::ByteView(&deliveryReceipt, sizeof(deliveryReceipt)),
        transitionRevision));
    CHECK_TRUE(pendingStore.load(delivered));
    uint32_t activeRevision = 0U;
    CHECK_TRUE(activeStore.promoteCredentialRenewal(
        delivered, activeRevision));
    uint8_t deliveryReceiptDigest[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::GatewayCredentialRenewalStore::receiptDigest(
        delivered,
        blinker::MutableByteSpan(deliveryReceiptDigest,
                                 sizeof(deliveryReceiptDigest))));
    CHECK_TRUE(pendingStore.markPromoted(
        decodedAck.renewalOperationId, decodedAck.topologyOperationId,
        decodedAck.accessMaterialDigest,
        blinker::ByteView(deliveryReceiptDigest,
                          sizeof(deliveryReceiptDigest)),
        activeRevision, transitionRevision));
    const uint32_t pendingWrites = pendingMemory.replaceCalls;
    CHECK_TRUE(processor.process(
        deliveryContext,
        blinker::ByteView(exactEnvelope, exactEnvelopeSize),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(pendingWrites, pendingMemory.replaceCalls);
    CHECK_TRUE(records.identity(identity)); CHECK_EQ(envelope.credentialVersion, identity->credentialVersion);
    CHECK_EQ(0U, otherPending.replaceCalls); CHECK_EQ(1U, otherActive.replaceCalls);
    blinker::clearGatewayAccessRecord(active);
    blinker::clearGatewayCredentialRenewalRecord(delivered);

    // PresenceKey is not inside the sealed plaintext. A changed current
    // locator must still fail the post-decrypt material binding.
    presenceKey[0] ^= 0x01U;
    fill(opened, sizeof(opened), 0xFFU);
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             blinker::openGatewayCredentialRenewalEnvelope(
                 deviceKey, envelope,
                 blinker::ByteView(presenceKey, sizeof(presenceKey)), crypto,
                 blinker::MutableByteSpan(aadScratch, sizeof(aadScratch)),
                 blinker::MutableByteSpan(opened, sizeof(opened))).code());
    uint8_t zero[sizeof(opened)] = {};
    CHECK_TRUE(equal(blinker::ByteView(opened, sizeof(opened)),
                     blinker::ByteView(zero, sizeof(zero))));

    blinker::clearDeviceKey(deviceKey);
    blinker::secureZero(blinker::MutableByteSpan(
        renewalOperation, sizeof(renewalOperation)));
    blinker::secureZero(blinker::MutableByteSpan(
        topologyOperation, sizeof(topologyOperation)));
    blinker::secureZero(blinker::MutableByteSpan(
        childInstance, sizeof(childInstance)));
    blinker::secureZero(blinker::MutableByteSpan(controller, sizeof(controller)));
    blinker::secureZero(blinker::MutableByteSpan(nonce, sizeof(nonce)));
    blinker::secureZero(blinker::MutableByteSpan(ciphertext, sizeof(ciphertext)));
    blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
    blinker::secureZero(blinker::MutableByteSpan(secret, sizeof(secret)));
    blinker::secureZero(blinker::MutableByteSpan(
        presenceKey, sizeof(presenceKey)));
    blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    blinker::secureZero(blinker::MutableByteSpan(wrapKey, sizeof(wrapKey)));
    blinker::secureZero(blinker::MutableByteSpan(
        expectedAad, sizeof(expectedAad)));
    blinker::secureZero(blinker::MutableByteSpan(
        aadScratch, sizeof(aadScratch)));
    blinker::secureZero(blinker::MutableByteSpan(opened, sizeof(opened)));
    blinker::secureZero(blinker::MutableByteSpan(
        exactEnvelope, sizeof(exactEnvelope)));
    blinker::secureZero(blinker::MutableByteSpan(ack, sizeof(ack)));
    blinker::secureZero(blinker::MutableByteSpan(
        replayAck, sizeof(replayAck)));
    blinker::secureZero(blinker::MutableByteSpan(
        deliveryGrantDigest, sizeof(deliveryGrantDigest)));
    blinker::secureZero(blinker::MutableByteSpan(
        deliveryReceiptDigest, sizeof(deliveryReceiptDigest)));
}

} // namespace

int main() {
    testProtectedJournal();
    testRenewalEnvelopeSecurity();
    std::cout << "Gateway credential renewal delivery/store tests passed"
              << std::endl;
    return 0;
}
