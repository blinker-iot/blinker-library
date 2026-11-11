#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/runtime/GatewayManagementClient.h>
#include <BlinkerV2/runtime/GatewayAccessDeliveryProcessor.h>
#include <BlinkerV2/runtime/GatewayChildDirectory.h>
#include <BlinkerV2/runtime/GatewayChildBinding.h>
#include <BlinkerV2/runtime/GatewayManagementDeliveryMux.h>
#include <BlinkerV2/runtime/GatewayCredentialRenewalDeliveryProcessor.h>
#include <BlinkerV2/runtime/GatewayPermitJoinCoordinator.h>
#include <BlinkerV2/runtime/GatewayPermitJoinRelayClient.h>
#include <BlinkerV2/security/GatewayAccessEnvelopeSecurity.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Gateway access check failed at line "              \
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
        ++loadCalls;
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

    unsigned loadCalls = 0U;
    uint8_t bytes[blinker::GatewayCredentialRenewalStore::serializedSize];
    size_t size;
    bool present;
    bool failNextReplace;
    uint32_t replaceCalls;

private:
    blinker::StorageProtection protection_;
};

class FixedKeySource final : public blinker::IDeviceKeySource {
public:
    FixedKeySource() : key() {
        std::memset(key.bytes, 0x42, sizeof(key.bytes));
    }

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
    FixedClock()
        : synchronized(true), now(1700000000ULL), monotonic(0U) {}

    uint32_t monotonicMillis() const override { return monotonic; }

    bool unixTime(uint64_t& output) const override {
        if (!synchronized) return false;
        output = now;
        return true;
    }

    bool synchronized;
    uint64_t now;
    uint32_t monotonic;
};

class NoopProofControl final : public blinker::IGatewayManagementControl {
public:
    NoopProofControl()
        : bytes(), size(0U), pending(false), commands(0U), resetCalls(0U) {}
    blinker::Result handleCommand(blinker::ByteView encoded) override {
        blinker::gateway::GatewayProofCommandView command;
        blinker::Result result = blinker::gateway::decodeGatewayProofCommand(
            encoded, command);
        if (!result) return result;
        blinker::gateway::GatewayProofResultView proof;
        proof.operationId = command.operationId;
        proof.topologyVersion = command.topologyVersion;
        proof.proofNonce = command.proofNonce;
        proof.status = blinker::gateway::GatewayProofStatus::Secure;
        proof.presenceKeyVersion = command.presenceKeyVersion;
        result = blinker::gateway::encodeGatewayProofResult(
            proof, blinker::MutableByteSpan(bytes, sizeof(bytes)), size);
        if (result) {
            ++commands;
            pending = true;
        }
        return result;
    }
    void poll() override {}
    bool busy() const override { return false; }
    blinker::ByteView pendingResult() const override {
        return pending ? blinker::ByteView(bytes, size) : blinker::ByteView();
    }
    void markResultPublished() override { pending = false; }
    void reset() override {
        ++resetCalls;
        pending = false;
        size = 0U;
    }
    uint8_t bytes[blinker::gateway::kGatewayProofResultMaximumEncodedSize];
    size_t size;
    bool pending;
    uint32_t commands;
    uint32_t resetCalls;
};

class FakeMqtt final : public blinker::IMqttClient {
public:
    FakeMqtt()
        : handler(nullptr), handlerContext(nullptr), connectedValue(false),
          failPublish(false), subscribeCalls(0U), publishCalls(0U),
          lastSubscribeQos(0U), lastPublishQos(0U), lastRetained(false),
          lastSubscribeTopic(), lastPublishTopic(), lastPayload() {}

    blinker::Result connect(
        const blinker::MqttConnectOptions&) override {
        connectedValue = true;
        return blinker::Result::success();
    }

    void disconnect() override { connectedValue = false; }
    bool connected() const override { return connectedValue; }

    blinker::Result subscribe(
        blinker::StringView topic,
        uint8_t qos) override {
        ++subscribeCalls;
        lastSubscribeTopic.assign(topic.data, topic.size);
        lastSubscribeQos = qos;
        return blinker::Result::success();
    }

    blinker::Result publish(
        blinker::StringView topic,
        blinker::ByteView payload,
        const blinker::MqttPublishOptions& options) override {
        if (failPublish) {
            failPublish = false;
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        ++publishCalls;
        lastPublishTopic.assign(topic.data, topic.size);
        lastPayload.assign(payload.data, payload.data + payload.size);
        lastPublishQos = options.qos;
        lastRetained = options.retained;
        return blinker::Result::success();
    }

    void poll(uint32_t) override {}

    void setMessageHandler(
        blinker::MqttMessageHandler value,
        void* context) override {
        handler = value;
        handlerContext = context;
    }

    void deliver(
        const char* topic,
        blinker::ByteView payload,
        uint8_t qos,
        bool retained) {
        CHECK_TRUE(handler != nullptr);
        blinker::MqttMessageInfo info;
        info.qos = qos;
        info.qosKnown = true;
        info.retained = retained;
        info.retainedKnown = true;
        handler(handlerContext, blinker::StringView(topic, std::strlen(topic)),
                payload, info);
    }

    blinker::MqttMessageHandler handler;
    void* handlerContext;
    bool connectedValue;
    bool failPublish;
    uint32_t subscribeCalls;
    uint32_t publishCalls;
    uint8_t lastSubscribeQos;
    uint8_t lastPublishQos;
    bool lastRetained;
    std::string lastSubscribeTopic;
    std::string lastPublishTopic;
    std::vector<uint8_t> lastPayload;
};

class FakePermitJoinRelayAdapter final
    : public blinker::IGatewayPermitJoinAdapter,
      public blinker::IGatewayPermitJoinRelayAdapter {
public:
    FakePermitJoinRelayAdapter()
        : stateValue(blinker::GatewayPermitJoinAdapterState::Stopped),
          connectedValue(false), selected(false), downPacket(),
          downSize(0U), downWrites(0U), upPacket(), upSize(0U),
          upPending(false), downWouldBlock(false), blockAfterNextWrite(false), polls(0U) {
        candidate.token[0] = 0xA1U;
        candidate.token[1] = 0xB2U;
        candidate.token[2] = 0xC3U;
        candidate.tokenSize = 3U;
        candidate.wireVersion = 2U;
        candidate.capabilities = 0x1234U;
        candidate.signalQuality = 87U;
    }

    uint16_t adapterId() const override {
        return blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    }
    blinker::Result openWindow() override {
        stateValue = blinker::GatewayPermitJoinAdapterState::Ready;
        return blinker::Result::success();
    }
    void closeWindow() override {
        stateValue = blinker::GatewayPermitJoinAdapterState::Stopped;
        clearRelaySelection();
    }
    void poll(uint32_t) override {
        ++polls;
        if (stateValue == blinker::GatewayPermitJoinAdapterState::Closing) {
            stateValue = blinker::GatewayPermitJoinAdapterState::Stopped;
        }
    }
    blinker::GatewayPermitJoinAdapterState state() const override {
        return stateValue;
    }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::Ok;
    }

    size_t relayCandidateCount() const override { return 1U; }
    blinker::Result readRelayCandidate(
        size_t index,
        blinker::GatewayPermitJoinCandidate& output) const override {
        if (index != 0U) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = candidate;
        return blinker::Result::success();
    }
    blinker::Result selectRelayCandidate(blinker::ByteView token) override {
        if (token.size != candidate.tokenSize || token.data == nullptr ||
            std::memcmp(token.data, candidate.token, token.size) != 0) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        selected = true;
        return blinker::Result::success();
    }
    void clearRelaySelection() override {
        selected = false;
        connectedValue = false;
        downSize = 0U;
        upSize = 0U;
        upPending = false;
    }
    blinker::Result queueRelayPacket(blinker::ByteView packet) override {
        if (downWouldBlock) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (!relayConnected() || packet.data == nullptr || packet.empty() ||
            packet.size > sizeof(downPacket)) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        std::memcpy(downPacket, packet.data, packet.size);
        downSize = packet.size;
        ++downWrites;
        if (blockAfterNextWrite) {
            blockAfterNextWrite = false;
            downWouldBlock = true;
        }
        return blinker::Result::success();
    }
    blinker::Result takeRelayPacket(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!upPending) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < upSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, upPacket, upSize);
        written = upSize;
        upPending = false;
        return blinker::Result::success();
    }
    size_t relayPacketCount() const override { return upPending ? 1U : 0U; }
    bool relayConnected() const override {
        return selected && connectedValue;
    }
    uint16_t relayPacketSize() const override { return 20U; }

    void queueUp(const uint8_t* bytes, size_t size) {
        CHECK_TRUE(bytes != nullptr && size > 0U && size <= sizeof(upPacket));
        std::memcpy(upPacket, bytes, size);
        upSize = size;
        upPending = true;
    }

    blinker::GatewayPermitJoinAdapterState stateValue;
    bool connectedValue;
    bool selected;
    blinker::GatewayPermitJoinCandidate candidate;
    uint8_t downPacket[blinker::gateway::kPermitJoinRelayPacketMaximumSize];
    size_t downSize;
    uint32_t downWrites;
    uint8_t upPacket[blinker::gateway::kPermitJoinRelayPacketMaximumSize];
    size_t upSize;
    bool upPending;
    bool downWouldBlock;
    bool blockAfterNextWrite;
    uint32_t polls;
};

// Deterministic authenticated transform for orchestration tests. Production
// platform AES-GCM implementations have their own golden tests; this fake lets
// these tests focus on exact KDF/AAD, durable ordering and replay behavior.
class DeterministicAead final
    : public blinker::IX25519AesGcmCryptoProvider {
public:
    blinker::Result x25519PublicKey(
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::Result x25519(
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::Result aes256GcmEncrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView plaintext,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        blinker::Result result = validate(
            key, nonce, associatedData, plaintext, output,
            plaintext.size + blinker::kAesGcmTagSize);
        if (!result) return result;
        for (size_t index = 0U; index < plaintext.size; ++index) {
            output.data[index] = static_cast<uint8_t>(
                plaintext.data[index] ^ key.data[index % key.size] ^
                nonce.data[index % nonce.size]);
        }
        uint8_t tag[blinker::kSha256Size] = {};
        result = makeTag(
            key, nonce, associatedData,
            blinker::ByteView(output.data, plaintext.size), tag);
        if (result) {
            std::memcpy(output.data + plaintext.size, tag,
                        blinker::kAesGcmTagSize);
            written = plaintext.size + blinker::kAesGcmTagSize;
        }
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (ciphertextAndTag.data == nullptr ||
            ciphertextAndTag.size < blinker::kAesGcmTagSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        const size_t ciphertextSize =
            ciphertextAndTag.size - blinker::kAesGcmTagSize;
        blinker::Result result = validate(
            key, nonce, associatedData,
            blinker::ByteView(ciphertextAndTag.data, ciphertextSize),
            output, ciphertextSize);
        uint8_t tag[blinker::kSha256Size] = {};
        if (result) {
            result = makeTag(
                key, nonce, associatedData,
                blinker::ByteView(ciphertextAndTag.data, ciphertextSize),
                tag);
        }
        if (result && !blinker::constantTimeEqual(
                          blinker::ByteView(tag, blinker::kAesGcmTagSize),
                          blinker::ByteView(
                              ciphertextAndTag.data + ciphertextSize,
                              blinker::kAesGcmTagSize))) {
            result = blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
        }
        if (result) {
            for (size_t index = 0U; index < ciphertextSize; ++index) {
                output.data[index] = static_cast<uint8_t>(
                    ciphertextAndTag.data[index] ^
                    key.data[index % key.size] ^
                    nonce.data[index % nonce.size]);
            }
            written = ciphertextSize;
        }
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

private:
    static blinker::Result validate(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView input,
        blinker::MutableByteSpan output,
        size_t required) {
        if (key.data == nullptr || key.size != blinker::kAes256KeySize ||
            nonce.data == nullptr || nonce.size != blinker::kAesGcmNonceSize ||
            (associatedData.data == nullptr && !associatedData.empty()) ||
            (input.data == nullptr && !input.empty())) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        if (output.data == nullptr || output.size < required) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        return blinker::Result::success();
    }

    static blinker::Result makeTag(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertext,
        uint8_t output[blinker::kSha256Size]) {
        blinker::Sha256 hash;
        blinker::Result result = hash.update(key);
        if (result) result = hash.update(nonce);
        if (result) result = hash.update(associatedData);
        if (result) result = hash.update(ciphertext);
        if (result) {
            result = hash.finish(blinker::MutableByteSpan(
                output, blinker::kSha256Size));
        }
        return result;
    }
};

void fill(uint8_t* output, size_t size, uint8_t value) {
    std::memset(output, value, size);
}

void testProtocolMaximums() {
    const std::string environment(32U, 'e');
    const std::string tenant(128U, 't');
    const std::string hub(128U, 'h');
    const std::string child(128U, 'c');
    uint8_t operation[blinker::gateway::kOperationIdSize];
    uint8_t instance[blinker::gateway::kDeviceInstanceIdSize];
    uint8_t controller[blinker::gateway::kControllerIdSize];
    uint8_t nonce[blinker::gateway::kEnvelopeNonceSize];
    uint8_t ciphertext[blinker::gateway::kGatewayAccessMaterialSize];
    uint8_t tag[blinker::gateway::kEnvelopeTagSize];
    fill(operation, sizeof(operation), 1U);
    fill(instance, sizeof(instance), 2U);
    fill(controller, sizeof(controller), 3U);
    fill(nonce, sizeof(nonce), 4U);
    fill(ciphertext, sizeof(ciphertext), 5U);
    fill(tag, sizeof(tag), 6U);

    blinker::gateway::GatewayAccessEnvelopeView envelope;
    envelope.operationId = blinker::ByteView(operation, sizeof(operation));
    envelope.environmentId = blinker::StringView(
        environment.data(), environment.size());
    envelope.tenantId = blinker::StringView(tenant.data(), tenant.size());
    envelope.edgeHubLogicalDeviceId = blinker::StringView(
        hub.data(), hub.size());
    envelope.deviceKeyVersion = UINT32_MAX;
    envelope.childLogicalDeviceId = blinker::StringView(
        child.data(), child.size());
    envelope.childDeviceInstanceId = blinker::ByteView(
        instance, sizeof(instance));
    envelope.accessEpoch = UINT32_MAX;
    envelope.controllerId = blinker::ByteView(controller, sizeof(controller));
    envelope.credentialVersion = UINT32_MAX;
    envelope.permissions = blinker::gateway::kGatewayPermissions;
    envelope.presenceKeyVersion = UINT32_MAX;
    envelope.expiresAtUnixSeconds = UINT64_MAX;
    envelope.nonce = blinker::ByteView(nonce, sizeof(nonce));
    envelope.ciphertext = blinker::ByteView(ciphertext, sizeof(ciphertext));
    envelope.tag = blinker::ByteView(tag, sizeof(tag));

    uint8_t encoded[
        blinker::gateway::kGatewayAccessEnvelopeMaximumEncodedSize] = {};
    size_t written = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayAccessEnvelope(
        envelope, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    CHECK_EQ(sizeof(encoded), written);
    uint8_t aad[blinker::gateway::kGatewayAccessAadMaximumEncodedSize] = {};
    CHECK_TRUE(blinker::gateway::encodeGatewayAccessAad(
        envelope, blinker::MutableByteSpan(aad, sizeof(aad)), written));
    CHECK_EQ(sizeof(aad), written);
}

blinker::GatewayAccessRecord candidate(
    uint8_t operation,
    uint8_t envelopeDigest,
    uint8_t secret,
    uint32_t epoch,
    uint32_t version,
    uint32_t presenceVersion = 1U,
    uint8_t presenceKey = 0x71U) {
    blinker::GatewayAccessRecord value;
    fill(value.operationId, sizeof(value.operationId), operation);
    fill(value.envelopeDigest, sizeof(value.envelopeDigest), envelopeDigest);
    fill(value.accessMaterialDigest, sizeof(value.accessMaterialDigest), secret);
    const char id[] = "child-1";
    value.childLogicalDeviceIdSize = sizeof(id) - 1U;
    std::memcpy(value.childLogicalDeviceId, id, sizeof(id) - 1U);
    fill(value.childDeviceInstanceId,
         sizeof(value.childDeviceInstanceId), 0x22U);
    value.accessEpoch = epoch;
    fill(value.controllerId, sizeof(value.controllerId), 0x33U);
    value.credentialVersion = version;
    value.permissions = blinker::gateway::kGatewayPermissions;
    value.expiresAtUnixSeconds = 2000000000ULL;
    fill(value.secret, sizeof(value.secret), secret);
    value.activePresence.version = presenceVersion;
    fill(value.activePresence.key,
         sizeof(value.activePresence.key), presenceKey);
    return value;
}

void testStore() {
    MemoryBlob memory;
    blinker::GatewayAccessStore store(memory);
    blinker::GatewayAccessCommitOutcome outcome;
    blinker::GatewayAccessRecord first = candidate(0x11U, 0x51U, 0x61U, 9U, 1U);

    CHECK_TRUE(store.commit(first, outcome));
    CHECK_EQ(blinker::GatewayAccessCommitKind::Stored, outcome.kind);
    CHECK_EQ(1U, outcome.storageRevision);
    CHECK_EQ(1U, memory.replaceCalls);

    CHECK_TRUE(store.commit(first, outcome));
    CHECK_EQ(blinker::GatewayAccessCommitKind::ExactReplay, outcome.kind);
    CHECK_EQ(1U, outcome.storageRevision);
    CHECK_EQ(1U, memory.replaceCalls);

    blinker::GatewayAccessRecord rewrapped = first;
    fill(rewrapped.envelopeDigest,
         sizeof(rewrapped.envelopeDigest), 0x52U);
    CHECK_TRUE(store.commit(rewrapped, outcome));
    CHECK_EQ(blinker::GatewayAccessCommitKind::RewrappedReplay, outcome.kind);
    CHECK_EQ(1U, outcome.storageRevision);
    CHECK_EQ(2U, memory.replaceCalls);

    blinker::GatewayAccessRecord conflict = rewrapped;
    conflict.secret[0] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             store.commit(conflict, outcome).code());
    CHECK_EQ(2U, memory.replaceCalls);

    blinker::GatewayAccessRecord rotated = candidate(
        0x12U, 0x53U, 0x61U, 9U, 1U, 2U, 0x72U);
    CHECK_TRUE(store.commit(rotated, outcome));
    CHECK_EQ(2U, outcome.storageRevision);
    CHECK_EQ(3U, memory.replaceCalls);

    blinker::GatewayAccessRecord loaded;
    CHECK_TRUE(store.load(loaded));
    CHECK_EQ(1U, loaded.activePresence.version);
    CHECK_EQ(2U, loaded.pendingPresence.version);
    uint32_t mutationRevision = 0U;
    CHECK_TRUE(store.activatePendingPresence(2U, mutationRevision));
    CHECK_EQ(3U, mutationRevision);
    CHECK_EQ(4U, memory.replaceCalls);
    CHECK_TRUE(store.commit(rotated, outcome));
    CHECK_EQ(blinker::GatewayAccessCommitKind::ExactReplay, outcome.kind);
    CHECK_EQ(2U, outcome.storageRevision);
    CHECK_EQ(4U, memory.replaceCalls);

    blinker::GatewayAccessRecord stale = candidate(
        0x13U, 0x54U, 0x61U, 9U, 1U, 2U, 0x72U);
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             store.commit(stale, outcome).code());

    blinker::GatewayAccessRecord another = candidate(
        0x14U, 0x55U, 0x64U, 10U, 1U);
    another.childLogicalDeviceId[6] = '2';
    CHECK_EQ(blinker::ErrorCode::CapacityExceeded,
             store.commit(another, outcome).code());

    blinker::GatewayAccessRecord next = candidate(
        0x15U, 0x56U, 0x61U, 9U, 1U, 3U, 0x73U);
    memory.failNextReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
             store.commit(next, outcome).code());
    CHECK_TRUE(store.load(loaded));
    CHECK_EQ(3U, loaded.storageRevision);
    CHECK_EQ(2U, loaded.deliveryRevision);
    CHECK_TRUE(equal(
        blinker::ByteView(rotated.operationId, sizeof(rotated.operationId)),
        blinker::ByteView(loaded.operationId, sizeof(loaded.operationId))));

    CHECK_TRUE(store.commit(next, outcome));
    CHECK_EQ(4U, outcome.storageRevision);
    CHECK_TRUE(store.discardPendingPresence(3U, mutationRevision));
    CHECK_EQ(5U, mutationRevision);
    CHECK_TRUE(store.load(loaded));
    CHECK_EQ(4U, loaded.deliveryRevision);
    CHECK_EQ(2U, loaded.activePresence.version);
    CHECK_EQ(0U, loaded.pendingPresence.version);

    blinker::GatewayRevocationTombstone tombstone;
    std::memcpy(
        tombstone.operationId,
        next.operationId,
        sizeof(tombstone.operationId));
    tombstone.topologyVersion = 7U;
    fill(tombstone.grantDigest, sizeof(tombstone.grantDigest), 0x81U);
    fill(tombstone.receiptDigest, sizeof(tombstone.receiptDigest), 0x82U);
    tombstone.receiptSize = 12U;
    fill(tombstone.receipt, tombstone.receiptSize, 0x83U);
    const size_t writesBeforeRevocation = memory.replaceCalls;
    uint32_t revocationRevision = 0U;
    CHECK_TRUE(store.commitRevocation(
        loaded.deliveryRevision, tombstone, revocationRevision));
    CHECK_EQ(loaded.storageRevision + 1U, revocationRevision);
    CHECK_EQ(writesBeforeRevocation + 1U, memory.replaceCalls);

    // The secret-bearing record is gone, while the exact child receipt
    // remains replayable across a Hub restart.
    CHECK_EQ(blinker::ErrorCode::StateConflict, store.load(loaded).code());
    blinker::GatewayRevocationTombstone loadedTombstone;
    CHECK_TRUE(store.loadRevocation(loadedTombstone));
    CHECK_EQ(revocationRevision, loadedTombstone.storageRevision);
    CHECK_EQ(tombstone.topologyVersion, loadedTombstone.topologyVersion);
    CHECK_EQ(tombstone.receiptSize, loadedTombstone.receiptSize);
    CHECK_TRUE(equal(
        blinker::ByteView(tombstone.receipt, tombstone.receiptSize),
        blinker::ByteView(
            loadedTombstone.receipt,
            loadedTombstone.receiptSize)));

    CHECK_TRUE(store.commitRevocation(
        loaded.deliveryRevision, tombstone, revocationRevision));
    CHECK_EQ(writesBeforeRevocation + 1U, memory.replaceCalls);
    blinker::GatewayRevocationTombstone conflictingTombstone = tombstone;
    conflictingTombstone.receipt[0] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             store.commitRevocation(
                 loaded.deliveryRevision,
                 conflictingTombstone,
                 revocationRevision).code());
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             store.commit(first, outcome).code());

    uint8_t wrongReceiptDigest[
        blinker::gateway::kAccessMaterialDigestSize];
    fill(wrongReceiptDigest, sizeof(wrongReceiptDigest), 0x84U);
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             store.clearRevocation(
                 blinker::ByteView(
                     tombstone.operationId,
                     sizeof(tombstone.operationId)),
                 tombstone.topologyVersion,
                 blinker::ByteView(
                     wrongReceiptDigest,
                     sizeof(wrongReceiptDigest))).code());
    CHECK_TRUE(store.clearRevocation(
        blinker::ByteView(
            tombstone.operationId,
            sizeof(tombstone.operationId)),
        tombstone.topologyVersion,
        blinker::ByteView(
            tombstone.receiptDigest,
            sizeof(tombstone.receiptDigest))));
    CHECK_TRUE(store.clearRevocation(
        blinker::ByteView(
            tombstone.operationId,
            sizeof(tombstone.operationId)),
        tombstone.topologyVersion,
        blinker::ByteView(
            tombstone.receiptDigest,
            sizeof(tombstone.receiptDigest))));

    // A stale finalization can never clear a subsequently installed record.
    CHECK_TRUE(store.commit(first, outcome));
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             store.clearRevocation(
                 blinker::ByteView(
                     tombstone.operationId,
                     sizeof(tombstone.operationId)),
                 tombstone.topologyVersion,
                 blinker::ByteView(
                     tombstone.receiptDigest,
                     sizeof(tombstone.receiptDigest))).code());

    memory.bytes[blinker::GatewayAccessStore::serializedSize - 1U] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::InvalidEncoding, store.load(loaded).code());
    memory.bytes[blinker::GatewayAccessStore::serializedSize - 1U] ^= 0x01U;
    CHECK_TRUE(store.clear());
    CHECK_TRUE(store.clear());

    MemoryBlob plain(blinker::StorageProtection::PlainFlash);
    blinker::GatewayAccessStore plainStore(plain);
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             plainStore.commit(first, outcome).code());
    CHECK_EQ(0U, plain.replaceCalls);

    blinker::clearGatewayAccessRecord(first);
    blinker::clearGatewayAccessRecord(rewrapped);
    blinker::clearGatewayAccessRecord(conflict);
    blinker::clearGatewayAccessRecord(rotated);
    blinker::clearGatewayAccessRecord(stale);
    blinker::clearGatewayAccessRecord(another);
    blinker::clearGatewayAccessRecord(next);
    blinker::clearGatewayAccessRecord(loaded);
    blinker::clearGatewayRevocationTombstone(tombstone);
    blinker::clearGatewayRevocationTombstone(loadedTombstone);
    blinker::clearGatewayRevocationTombstone(conflictingTombstone);
    blinker::secureZero(blinker::MutableByteSpan(
        wrongReceiptDigest, sizeof(wrongReceiptDigest)));
}

struct SealedEnvelope {
    uint8_t encoded[
        blinker::gateway::kGatewayAccessEnvelopeMaximumEncodedSize];
    size_t size;

    SealedEnvelope() : encoded(), size(0U) {}
};

SealedEnvelope seal(
    FixedKeySource& keySource,
    DeterministicAead& crypto,
    uint8_t operationValue,
    uint8_t nonceValue,
    uint8_t secretValue,
    uint32_t credentialVersion,
    uint32_t presenceVersion = 1U,
    uint8_t presenceValue = 0x55U,
    const char* childId = "child-1",
    uint8_t instanceValue = 0x22U) {
    uint8_t operation[blinker::gateway::kOperationIdSize];
    uint8_t instance[blinker::gateway::kDeviceInstanceIdSize];
    uint8_t controller[blinker::gateway::kControllerIdSize];
    uint8_t nonce[blinker::gateway::kEnvelopeNonceSize];
    fill(operation, sizeof(operation), operationValue);
    fill(instance, sizeof(instance), instanceValue);
    fill(controller, sizeof(controller), 0x33U);
    fill(nonce, sizeof(nonce), nonceValue);

    blinker::gateway::GatewayAccessEnvelopeView envelope;
    envelope.operationId = blinker::ByteView(operation, sizeof(operation));
    envelope.environmentId = blinker::StringView("prod-cn");
    envelope.tenantId = blinker::StringView("tenant-edge");
    envelope.edgeHubLogicalDeviceId = blinker::StringView("edge-hub-1");
    envelope.deviceKeyVersion = 7U;
    envelope.childLogicalDeviceId = blinker::StringView(childId, std::strlen(childId));
    envelope.childDeviceInstanceId = blinker::ByteView(instance, sizeof(instance));
    envelope.accessEpoch = 9U;
    envelope.controllerId = blinker::ByteView(controller, sizeof(controller));
    envelope.credentialVersion = credentialVersion;
    envelope.permissions = blinker::gateway::kGatewayPermissions;
    envelope.presenceKeyVersion = presenceVersion;
    envelope.expiresAtUnixSeconds = 2000000000ULL;
    envelope.nonce = blinker::ByteView(nonce, sizeof(nonce));

    uint8_t aad[blinker::gateway::kGatewayAccessAadMaximumEncodedSize] = {};
    size_t aadSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayAccessAad(
        envelope, blinker::MutableByteSpan(aad, sizeof(aad)), aadSize));
    blinker::GatewayAccessWrapContext context;
    context.environmentId = envelope.environmentId;
    context.tenantId = envelope.tenantId;
    context.edgeHubLogicalDeviceId = envelope.edgeHubLogicalDeviceId;
    context.deviceKeyVersion = envelope.deviceKeyVersion;
    uint8_t wrapKey[blinker::kAes256KeySize] = {};
    CHECK_TRUE(blinker::deriveGatewayAccessWrapKey(
        keySource.key, context,
        blinker::MutableByteSpan(wrapKey, sizeof(wrapKey))));
    uint8_t material[blinker::gateway::kGatewayAccessMaterialSize];
    fill(material, blinker::gateway::kGatewaySecretSize, secretValue);
    fill(
        material + blinker::gateway::kGatewaySecretSize,
        blinker::gateway::kGatewayPresenceKeySize,
        presenceValue);
    uint8_t sealed[
        blinker::gateway::kGatewayAccessMaterialSize +
        blinker::gateway::kEnvelopeTagSize] = {};
    size_t sealedSize = 0U;
    CHECK_TRUE(crypto.aes256GcmEncrypt(
        blinker::ByteView(wrapKey, sizeof(wrapKey)), envelope.nonce,
        blinker::ByteView(aad, aadSize),
        blinker::ByteView(material, sizeof(material)),
        blinker::MutableByteSpan(sealed, sizeof(sealed)), sealedSize));
    CHECK_EQ(sizeof(sealed), sealedSize);
    envelope.ciphertext = blinker::ByteView(
        sealed, blinker::gateway::kGatewayAccessMaterialSize);
    envelope.tag = blinker::ByteView(
        sealed + blinker::gateway::kGatewayAccessMaterialSize,
        blinker::gateway::kEnvelopeTagSize);
    SealedEnvelope output;
    CHECK_TRUE(blinker::gateway::encodeGatewayAccessEnvelope(
        envelope, blinker::MutableByteSpan(output.encoded,
                                            sizeof(output.encoded)),
        output.size));
    blinker::secureZero(blinker::MutableByteSpan(aad, sizeof(aad)));
    blinker::secureZero(blinker::MutableByteSpan(wrapKey, sizeof(wrapKey)));
    blinker::secureZero(blinker::MutableByteSpan(material, sizeof(material)));
    blinker::secureZero(blinker::MutableByteSpan(sealed, sizeof(sealed)));
    return output;
}

struct DirectoryChild {
    blinker::GatewayChildRecords records;
    blinker::GatewayChildBinding binding;
    blinker::GatewayManagementDeliveryMux& delivery;
    DirectoryChild(MemoryBlob& active, MemoryBlob& journal, FixedKeySource& key,
                   DeterministicAead& crypto, FixedClock& clock)
        : records(active, journal), binding(records, key, crypto, clock), delivery(binding.delivery) {}
};

void testChildDirectory() {
    using namespace blinker;
    MemoryBlob aStore, aJournal, bStore, bJournal;
    FixedKeySource keys; FixedClock clock; DeterministicAead crypto;
    const auto frameA = seal(keys, crypto, 0x11, 0x44, 0x43, 1);
    const auto frameB = seal(keys, crypto, 0x12, 0x45, 0x44, 1, 1, 0x55, "child-2", 0x23);
    const auto frameC = seal(keys, crypto, 0x13, 0x46, 0x45, 1, 1, 0x55, "child-3", 0x24);
    GatewayAccessDeliveryContext context; context.deviceKeyVersion = 7;
    uint8_t ack[gateway::kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize] = {};
    size_t ackSize = 0;
    std::vector<uint8_t> originalAck;
    {
        DirectoryChild a(aStore, aJournal, keys, crypto, clock), b(bStore, bJournal, keys, crypto, clock);
        GatewayChildDirectoryEntry entries[] = {{a.records, a.delivery}, {b.records, b.delivery}};
        GatewayChildDirectory directory(entries, 2);
        const auto send = [&](const SealedEnvelope& frame) {
            return directory.process(context, ByteView(frame.encoded, frame.size), MutableByteSpan(ack, sizeof(ack)), ackSize);
        };
        GatewayChildDirectoryEntry aliasedEmpty[] = {{a.records, a.delivery}, {a.records, a.delivery}};
        GatewayChildDirectory aliased(aliasedEmpty, 2);
        CHECK_EQ(ErrorCode::StateConflict, aliased.process(context, ByteView(frameA.encoded, frameA.size),
            MutableByteSpan(ack, sizeof(ack)), ackSize).code()); CHECK_EQ(0U, aStore.replaceCalls);
        // Canonical identity chooses a slot, but bad AEAD/context/time cannot
        // allocate it or create an ACK. An empty cached slot stays available.
        auto corrupt = frameA; corrupt.encoded[corrupt.size - 1] ^= 1;
        CHECK_EQ(ErrorCode::AuthenticationRequired, send(corrupt).code());
        CHECK_EQ(0U, ackSize); CHECK_EQ(0U, aStore.replaceCalls); CHECK_EQ(0U, bStore.replaceCalls);
        context.deviceKeyVersion = 8; CHECK_EQ(ErrorCode::AuthenticationRequired, send(frameA).code());
        context.deviceKeyVersion = 7; clock.now = 2000000000ULL;
        CHECK_EQ(ErrorCode::AuthenticationRequired, send(frameA).code()); clock.now = 1700000000ULL;
        CHECK_TRUE(send(frameA)); originalAck.assign(ack, ack + ackSize);
        CHECK_TRUE(send(frameB)); CHECK_EQ(1U, aStore.replaceCalls); CHECK_EQ(1U, bStore.replaceCalls);
        CHECK_TRUE(send(frameA)); CHECK_TRUE(equal(ByteView(ack, ackSize), ByteView(originalAck.data(), originalAck.size())));
        CHECK_EQ(1U, aStore.replaceCalls); CHECK_EQ(ErrorCode::CapacityExceeded, send(frameC).code());
        CHECK_EQ(0U, ackSize);
        for (uint8_t byte : ack) CHECK_EQ(0U, byte);
        // Logical id, instance and operation collision must never select first.
        CHECK_EQ(ErrorCode::StateConflict, send(seal(keys, crypto, 0x31, 0x47, 0x43, 1, 1, 0x55, "child-1", 0x23)).code());
        CHECK_EQ(ErrorCode::StateConflict, send(seal(keys, crypto, 0x11, 0x47, 0x43, 1, 1, 0x55, "child-2", 0x23)).code());
        GatewayChildDirectoryEntry duplicate[] = {{a.records, a.delivery}, {a.records, a.delivery}};
        GatewayChildDirectory ambiguous(duplicate, 2);
        CHECK_EQ(ErrorCode::StateConflict, ambiguous.process(context, ByteView(frameA.encoded, frameA.size),
            MutableByteSpan(ack, sizeof(ack)), ackSize).code());
        // Selection becomes read-free after warming. Ordinary protocol
        // processing still reads and authenticates the actual durable record.
        const GatewayChildIdentity* identity = nullptr;
        CHECK_TRUE(a.records.identity(identity));
        const unsigned reads = aStore.loadCalls + aJournal.loadCalls;
        for (unsigned i = 0; i != 100; ++i) CHECK_TRUE(a.records.identity(identity));
        CHECK_EQ(reads, aStore.loadCalls + aJournal.loadCalls);
        CHECK_EQ(GatewayChildRecordState::Active, identity->state);
        // Failed writes invalidate as well: the old record is re-read, never
        // replaced by optimistic candidate metadata.
        aStore.failNextReplace = true;
        CHECK_EQ(ErrorCode::WouldBlock, send(seal(keys, crypto, 0x21, 0x48, 0x43, 1, 2, 0x56)).code());
        CHECK_TRUE(a.records.identity(identity)); CHECK_TRUE(aStore.loadCalls + aJournal.loadCalls > reads);
        CHECK_EQ(0x11U, identity->operation[0]);
        // Presence staging/activation use existing store methods, not a
        // directory-specific write path; every one invalidates the projection.
        CHECK_TRUE(send(seal(keys, crypto, 0x21, 0x49, 0x43, 1, 2, 0x56)));
        CHECK_TRUE(a.records.identity(identity)); CHECK_EQ(0x21U, identity->operation[0]);
        uint32_t revision = 0; CHECK_TRUE(a.records.access.activatePendingPresence(2, revision));
        CHECK_TRUE(a.records.identity(identity));
        CHECK_TRUE(a.records.access.clear()); CHECK_TRUE(send(frameA));
        originalAck.assign(ack, ack + ackSize);
    }
    {
        // Recreate RAM from unchanged blobs; reverse enumeration to prove
        // identity is durable and not the array index/address of a context.
        DirectoryChild a(aStore, aJournal, keys, crypto, clock), b(bStore, bJournal, keys, crypto, clock);
        GatewayChildDirectoryEntry entries[] = {{b.records, b.delivery}, {a.records, a.delivery}};
        GatewayChildDirectory directory(entries, 2);
        CHECK_TRUE(directory.process(context, ByteView(frameA.encoded, frameA.size), MutableByteSpan(ack, sizeof(ack)), ackSize));
        CHECK_TRUE(equal(ByteView(ack, ackSize), ByteView(originalAck.data(), originalAck.size())));
        const auto bWrites = bStore.replaceCalls;
        // Tombstone capacity cannot be reused before exact finalization.
        GatewayRevocationTombstone tombstone;
        fill(tombstone.operationId, sizeof(tombstone.operationId), 0x11);
        fill(tombstone.grantDigest, sizeof(tombstone.grantDigest), 0x31);
        fill(tombstone.receiptDigest, sizeof(tombstone.receiptDigest), 0x32);
        tombstone.topologyVersion = 2; tombstone.receiptSize = 1; tombstone.receipt[0] = 1;
        uint32_t revision = 0;
        CHECK_TRUE(a.records.access.commitRevocation(1, tombstone, revision));
        const GatewayChildIdentity* identity = nullptr; CHECK_TRUE(a.records.identity(identity));
        CHECK_EQ(GatewayChildRecordState::Revoked, identity->state);
        CHECK_EQ(ErrorCode::CapacityExceeded, directory.process(context, ByteView(frameC.encoded, frameC.size),
            MutableByteSpan(ack, sizeof(ack)), ackSize).code());
        CHECK_TRUE(a.records.access.clearRevocation(ByteView(tombstone.operationId, sizeof(tombstone.operationId)),
            2, ByteView(tombstone.receiptDigest, sizeof(tombstone.receiptDigest))));
        CHECK_TRUE(directory.process(context, ByteView(frameC.encoded, frameC.size), MutableByteSpan(ack, sizeof(ack)), ackSize));
        CHECK_EQ(bWrites, bStore.replaceCalls);
        // Corruption / backend lifecycle is explicitly invalidated and is
        // never mistaken for unused capacity. Other child storage is untouched.
        aStore.bytes[0] ^= 1; directory.invalidate();
        CHECK_EQ(ErrorCode::InvalidEncoding, directory.process(context, ByteView(frameB.encoded, frameB.size),
            MutableByteSpan(ack, sizeof(ack)), ackSize).code());
        CHECK_EQ(bWrites, bStore.replaceCalls);
    }
}

void testRenewalReservesDirectorySlot() {
    using namespace blinker;
    MemoryBlob active, journal;
    FixedKeySource keys; FixedClock clock; DeterministicAead crypto;
    DirectoryChild child(active, journal, keys, crypto, clock);
    GatewayChildDirectoryEntry entry{child.records, child.delivery}; GatewayChildDirectory directory(&entry, 1);
    const GatewayChildIdentity* identity = nullptr; CHECK_TRUE(child.records.identity(identity));
    CHECK_EQ(GatewayChildRecordState::Empty, identity->state);
    GatewayCredentialRenewalRecord pending;
    fill(pending.renewalOperationId, sizeof(pending.renewalOperationId), 0x11);
    fill(pending.topologyOperationId, sizeof(pending.topologyOperationId), 0x12);
    fill(pending.childDeviceInstanceId, sizeof(pending.childDeviceInstanceId), 0x22);
    fill(pending.controllerId, sizeof(pending.controllerId), 0x33);
    fill(pending.secret, sizeof(pending.secret), 0x43);
    fill(pending.accessMaterialDigest, sizeof(pending.accessMaterialDigest), 0x44);
    pending.accessEpoch = 9; pending.expectedCredentialVersion = 1; pending.credentialVersion = 2;
    pending.presenceKeyVersion = 1; pending.expiresAtUnixSeconds = 2000000000ULL;
    GatewayCredentialRenewalCommitOutcome outcome; CHECK_TRUE(child.records.renewal.commit(pending, outcome));
    CHECK_TRUE(child.records.identity(identity)); CHECK_EQ(GatewayChildRecordState::RenewalOnly, identity->state);
    const auto frame = seal(keys, crypto, 0x12, 0x44, 0x43, 1);
    GatewayAccessDeliveryContext context; context.deviceKeyVersion = 7;
    uint8_t ack[gateway::kGatewayAccessDeliveryAckMaximumEncodedSize] = {}; size_t ackSize = 0;
    CHECK_EQ(ErrorCode::CapacityExceeded, directory.process(context, ByteView(frame.encoded, frame.size),
        MutableByteSpan(ack, sizeof(ack)), ackSize).code()); CHECK_EQ(0U, active.replaceCalls);
    CHECK_TRUE(child.records.renewal.cancel(ByteView(pending.renewalOperationId, sizeof(pending.renewalOperationId)),
        ByteView(pending.topologyOperationId, sizeof(pending.topologyOperationId)),
        ByteView(pending.accessMaterialDigest, sizeof(pending.accessMaterialDigest))));
    CHECK_TRUE(directory.process(context, ByteView(frame.encoded, frame.size), MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_TRUE(child.records.identity(identity)); CHECK_EQ(GatewayChildRecordState::Active, identity->state);
    CHECK_TRUE(!identity->hasRenewal);
    CHECK_TRUE(child.records.renewal.commit(pending, outcome));
    const auto replacement = seal(keys, crypto, 0x13, 0x45, 0x43, 1, 2, 0x56);
    CHECK_EQ(ErrorCode::StateConflict, directory.process(context, ByteView(replacement.encoded, replacement.size),
        MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    // Replaying the same already-committed initial material does not replace
    // a topology or mutate the pending renewal journal.
    const auto activeWrites = active.replaceCalls, journalWrites = journal.replaceCalls;
    CHECK_TRUE(directory.process(context, ByteView(frame.encoded, frame.size), MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(activeWrites, active.replaceCalls); CHECK_EQ(journalWrites, journal.replaceCalls);
    clearGatewayCredentialRenewalRecord(pending);
    MemoryBlob plain(StorageProtection::PlainFlash), protectedJournal;
    GatewayChildRecords unprotected(plain, protectedJournal);
    CHECK_EQ(ErrorCode::AuthenticationRequired, unprotected.identity(identity).code()); CHECK_TRUE(identity == nullptr);
}

void testDeliveryProcessor() {
    MemoryBlob memory;
    blinker::GatewayAccessStore store(memory);
    FixedKeySource keys;
    FixedClock clock;
    DeterministicAead crypto;
    blinker::GatewayAccessDeliveryProcessor processor(
        store, keys, crypto, clock);
    blinker::GatewayAccessDeliveryContext context;
    context.environmentId = blinker::StringView("prod-cn");
    context.tenantId = blinker::StringView("tenant-edge");
    context.edgeHubLogicalDeviceId = blinker::StringView("edge-hub-1");
    context.deviceKeyVersion = 7U;

    SealedEnvelope first = seal(keys, crypto, 0x11U, 0x44U, 0x43U, 1U);
    uint8_t ack[blinker::gateway::kGatewayAccessDeliveryAckMaximumEncodedSize] = {};
    size_t ackSize = 0U;
    // A rejected first delivery cannot create durable access or expose an ACK.
    first.encoded[first.size - 1U] ^= 1U;
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             processor.process(context, blinker::ByteView(first.encoded, first.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    CHECK_EQ(0U, ackSize);
    CHECK_EQ(0U, memory.replaceCalls);
    CHECK_TRUE(!memory.present);
    first.encoded[first.size - 1U] ^= 1U;
    clock.now = 2000000000ULL;
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             processor.process(context, blinker::ByteView(first.encoded, first.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    CHECK_EQ(0U, ackSize);
    CHECK_EQ(0U, memory.replaceCalls);
    CHECK_TRUE(!memory.present);
    clock.now = 1700000000ULL;
    CHECK_TRUE(processor.process(
        context, blinker::ByteView(first.encoded, first.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(1U, memory.replaceCalls);
    blinker::gateway::GatewayAccessDeliveryAckView decodedAck;
    CHECK_TRUE(blinker::gateway::decodeGatewayAccessDeliveryAck(
        blinker::ByteView(ack, ackSize), decodedAck));
    CHECK_EQ(1U, decodedAck.storageRevision);
    uint8_t firstAck[sizeof(ack)] = {};
    std::memcpy(firstAck, ack, ackSize);
    const size_t firstAckSize = ackSize;

    CHECK_TRUE(processor.process(
        context, blinker::ByteView(first.encoded, first.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(1U, memory.replaceCalls);
    CHECK_TRUE(equal(blinker::ByteView(firstAck, firstAckSize),
                     blinker::ByteView(ack, ackSize)));

    SealedEnvelope rewrapped = seal(keys, crypto, 0x11U, 0x45U, 0x43U, 1U);
    CHECK_TRUE(processor.process(
        context, blinker::ByteView(rewrapped.encoded, rewrapped.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(2U, memory.replaceCalls);
    CHECK_TRUE(equal(blinker::ByteView(firstAck, firstAckSize),
                     blinker::ByteView(ack, ackSize)));

    SealedEnvelope conflict = seal(keys, crypto, 0x11U, 0x46U, 0x44U, 1U);
    CHECK_EQ(blinker::ErrorCode::SequenceConflict,
             processor.process(
                 context, blinker::ByteView(conflict.encoded, conflict.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    CHECK_EQ(0U, ackSize);
    CHECK_EQ(2U, memory.replaceCalls);

    SealedEnvelope rotated = seal(
        keys, crypto, 0x12U, 0x47U, 0x43U, 1U, 2U, 0x56U);
    CHECK_TRUE(processor.process(
        context, blinker::ByteView(rotated.encoded, rotated.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(3U, memory.replaceCalls);
    CHECK_TRUE(blinker::gateway::decodeGatewayAccessDeliveryAck(
        blinker::ByteView(ack, ackSize), decodedAck));
    CHECK_EQ(2U, decodedAck.storageRevision);
    uint8_t rotatedAck[sizeof(ack)] = {};
    std::memcpy(rotatedAck, ack, ackSize);
    const size_t rotatedAckSize = ackSize;

    uint32_t activationRevision = 0U;
    CHECK_TRUE(store.activatePendingPresence(2U, activationRevision));
    CHECK_EQ(3U, activationRevision);
    CHECK_EQ(4U, memory.replaceCalls);

    // Reboot/reopen after local activation: exact redelivery returns the
    // original durable ACK. The local storage revision is not ACK identity.
    blinker::GatewayAccessStore reopenedStore(memory);
    blinker::GatewayAccessDeliveryProcessor reopened(
        reopenedStore, keys, crypto, clock);
    CHECK_TRUE(reopened.process(
        context, blinker::ByteView(rotated.encoded, rotated.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_EQ(4U, memory.replaceCalls);
    CHECK_TRUE(equal(blinker::ByteView(rotatedAck, rotatedAckSize),
                     blinker::ByteView(ack, ackSize)));

    // Base DeviceKey sessions do not allocate tenant/logical-id copies. The
    // unique current DeviceKey still authenticates those KDF/AAD fields.
    blinker::GatewayAccessDeliveryContext keyBoundOnly;
    keyBoundOnly.deviceKeyVersion = 7U;
    CHECK_TRUE(reopened.process(
        keyBoundOnly, blinker::ByteView(rotated.encoded, rotated.size),
        blinker::MutableByteSpan(ack, sizeof(ack)), ackSize));
    CHECK_TRUE(equal(blinker::ByteView(rotatedAck, rotatedAckSize),
                     blinker::ByteView(ack, ackSize)));

    SealedEnvelope tampered = rotated;
    tampered.encoded[tampered.size - 1U] ^= 0x01U;
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             reopened.process(
                 context, blinker::ByteView(tampered.encoded, tampered.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    CHECK_EQ(4U, memory.replaceCalls);

    blinker::GatewayAccessDeliveryContext wrong = context;
    wrong.tenantId = blinker::StringView("another-tenant");
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             reopened.process(
                 wrong, blinker::ByteView(rotated.encoded, rotated.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());

    clock.synchronized = false;
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             reopened.process(
                 context, blinker::ByteView(rotated.encoded, rotated.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    clock.synchronized = true;
    clock.now = 2000000000ULL;
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             reopened.process(
                 context, blinker::ByteView(rotated.encoded, rotated.size),
                 blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).code());
    CHECK_EQ(0U, ackSize);
    CHECK_EQ(4U, memory.replaceCalls);
    for (size_t index = 0U; index < sizeof(ack); ++index) CHECK_EQ(0U, ack[index]);

    blinker::secureZero(blinker::MutableByteSpan(ack, sizeof(ack)));
    blinker::secureZero(blinker::MutableByteSpan(firstAck, sizeof(firstAck)));
    blinker::secureZero(blinker::MutableByteSpan(rotatedAck, sizeof(rotatedAck)));
    blinker::secureZero(blinker::MutableByteSpan(first.encoded, sizeof(first.encoded)));
    blinker::secureZero(blinker::MutableByteSpan(rewrapped.encoded, sizeof(rewrapped.encoded)));
    blinker::secureZero(blinker::MutableByteSpan(conflict.encoded, sizeof(conflict.encoded)));
    blinker::secureZero(blinker::MutableByteSpan(rotated.encoded, sizeof(rotated.encoded)));
    blinker::secureZero(blinker::MutableByteSpan(tampered.encoded, sizeof(tampered.encoded)));
}

blinker::SessionCredentials mqttCredentials() {
    blinker::SessionCredentials value;
    value.broker = blinker::BrokerKind::Blinker;
    value.host = blinker::StringView("broker.local");
    value.port = 8883U;
    value.security = blinker::MqttSecurity::Tls;
    value.clientId = blinker::StringView("edge-client-1");
    value.username = blinker::StringView("edge-user");
    value.password = blinker::StringView("edge-password");
    value.publishTopic = blinker::StringView("/device/edge/up");
    value.subscribeTopic = blinker::StringView("/device/edge/down");
    return value;
}

blinker::GatewayAccessDeliveryContext deliveryContext() {
    blinker::GatewayAccessDeliveryContext value;
    value.environmentId = blinker::StringView("prod-cn");
    value.tenantId = blinker::StringView("tenant-edge");
    value.edgeHubLogicalDeviceId = blinker::StringView("edge-hub-1");
    value.deviceKeyVersion = 7U;
    return value;
}

void testManagementClient() {
    MemoryBlob memory;
    blinker::GatewayAccessStore store(memory);
    FixedKeySource keys;
    FixedClock clock;
    DeterministicAead crypto;
    blinker::GatewayAccessDeliveryProcessor processor(
        store, keys, crypto, clock);
    FakeMqtt mqtt;
    blinker::MqttFrameTransport transport(mqtt, clock);
    blinker::SessionCredentials credentials = mqttCredentials();
    CHECK_TRUE(transport.configure(credentials));
    NoopProofControl proof;
    blinker::GatewayCloudMux cloud(transport);
    blinker::GatewayManagementClient management(cloud, processor, proof);
    CHECK_TRUE(management.configure(credentials, deliveryContext()));
    CHECK_TRUE(transport.start());
    transport.poll(1000U);
    CHECK_EQ(blinker::TransportState::Online, transport.state());
    CHECK_EQ(1U, mqtt.subscribeCalls);

    management.poll();
    CHECK_TRUE(management.subscribed());
    CHECK_EQ(2U, mqtt.subscribeCalls);
    CHECK_EQ(1U, mqtt.lastSubscribeQos);
    CHECK_TRUE(mqtt.lastSubscribeTopic ==
               "/edge-hub/edge-client-1/management/down");

    // A product scheduler may not poll this client while MQTT is in Backoff.
    // It must still renew the non-persistent subscription after reconnect.
    clock.monotonic = 5000U;
    mqtt.connectedValue = false;
    transport.poll(1000U);
    CHECK_EQ(blinker::TransportState::Backoff, transport.state());
    clock.monotonic = 10000U;
    transport.poll(1000U);
    CHECK_EQ(blinker::TransportState::Online, transport.state());
    CHECK_EQ(2U, transport.connectionGeneration());
    CHECK_TRUE(!management.subscribed());
    CHECK_EQ(3U, mqtt.subscribeCalls);
    management.poll();
    CHECK_TRUE(management.subscribed());
    CHECK_EQ(4U, mqtt.subscribeCalls);
    CHECK_TRUE(mqtt.lastSubscribeTopic ==
               "/edge-hub/edge-client-1/management/down");

    SealedEnvelope first = seal(keys, crypto, 0x21U, 0x51U, 0x71U, 1U);
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(first.encoded, first.size), 1U, false);
    CHECK_TRUE(management.hasPendingEnvelope());
    mqtt.failPublish = true;
    management.poll();
    CHECK_TRUE(!management.hasPendingEnvelope());
    CHECK_TRUE(management.hasPendingAck());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, management.lastError());
    CHECK_EQ(1U, memory.replaceCalls);
    CHECK_EQ(0U, mqtt.publishCalls);

    management.poll();
    CHECK_TRUE(!management.hasPendingAck());
    CHECK_EQ(blinker::ErrorCode::Ok, management.lastError());
    CHECK_EQ(1U, mqtt.publishCalls);
    CHECK_EQ(0U, mqtt.lastPublishQos);
    CHECK_TRUE(!mqtt.lastRetained);
    CHECK_TRUE(mqtt.lastPublishTopic ==
               "/edge-hub/edge-client-1/management/up");
    blinker::gateway::GatewayAccessDeliveryAckView ack;
    CHECK_TRUE(blinker::gateway::decodeGatewayAccessDeliveryAck(
        blinker::ByteView(&mqtt.lastPayload[0], mqtt.lastPayload.size()), ack));
    CHECK_EQ(1U, ack.storageRevision);

    uint8_t proofNonce[blinker::gateway::kProofNonceSize];
    std::memset(proofNonce, 0x31, sizeof(proofNonce));
    blinker::gateway::GatewayAccessEnvelopeView firstEnvelope;
    CHECK_TRUE(blinker::gateway::decodeGatewayAccessEnvelope(
        blinker::ByteView(first.encoded, first.size), firstEnvelope));
    blinker::gateway::GatewayProofCommandView proofCommand;
    proofCommand.operationId = firstEnvelope.operationId;
    proofCommand.topologyVersion = 3U;
    proofCommand.childLogicalDeviceId = firstEnvelope.childLogicalDeviceId;
    proofCommand.childDeviceInstanceId = firstEnvelope.childDeviceInstanceId;
    proofCommand.accessEpoch = firstEnvelope.accessEpoch;
    proofCommand.gatewayCredentialVersion = firstEnvelope.credentialVersion;
    proofCommand.presenceKeyVersion = firstEnvelope.presenceKeyVersion;
    proofCommand.accessMaterialDigest = blinker::ByteView(
        ack.accessMaterialDigest.data, ack.accessMaterialDigest.size);
    proofCommand.accessStorageRevision = ack.storageRevision;
    proofCommand.proofNonce = blinker::ByteView(
        proofNonce, sizeof(proofNonce));
    proofCommand.expiresAtUnixSeconds = clock.now + 60U;
    uint8_t proofBytes[
        blinker::gateway::kGatewayProofCommandMaximumEncodedSize];
    size_t proofSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayProofCommand(
        proofCommand, blinker::MutableByteSpan(
            proofBytes, sizeof(proofBytes)), proofSize));
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(proofBytes, proofSize), 1U, false);
    management.poll();
    CHECK_EQ(1U, proof.commands);
    CHECK_EQ(2U, mqtt.publishCalls);
    blinker::gateway::GatewayProofResultView proofResult;
    CHECK_TRUE(blinker::gateway::decodeGatewayProofResult(
        blinker::ByteView(&mqtt.lastPayload[0], mqtt.lastPayload.size()),
        proofResult));
    CHECK_EQ(blinker::gateway::GatewayProofStatus::Secure,
             proofResult.status);

    // Rotating the cloud SessionCredential only changes MQTT topics and
    // subscriptions. It must not retire the already-proved BLE topology.
    blinker::SessionCredentials rotatedCredentials = credentials;
    rotatedCredentials.clientId = blinker::StringView("edge-client-2");
    CHECK_TRUE(management.configure(
        rotatedCredentials, deliveryContext()));
    CHECK_EQ(0U, proof.resetCalls);
    const char rotatedTopic[] =
        "/edge-hub/edge-client-2/management/down";
    const blinker::StringView reboundTopic = management.subscribeTopic();
    CHECK_EQ(sizeof(rotatedTopic) - 1U, reboundTopic.size);
    CHECK_TRUE(std::memcmp(
        reboundTopic.data, rotatedTopic, reboundTopic.size) == 0);
    management.reset();
    CHECK_EQ(1U, proof.resetCalls);
    CHECK_TRUE(management.configure(credentials, deliveryContext()));
    // Retained and wrong-QoS management packets never reach flash.
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(first.encoded, first.size), 1U, true);
    management.poll();
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(first.encoded, first.size), 0U, false);
    management.poll();
    CHECK_EQ(1U, memory.replaceCalls);
    CHECK_EQ(2U, mqtt.publishCalls);

    blinker::SessionCredentials plain = credentials;
    plain.security = blinker::MqttSecurity::PlainTcp;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
             management.configure(plain, deliveryContext()).code());

    blinker::secureZero(blinker::MutableByteSpan(
        first.encoded, sizeof(first.encoded)));
}

void testPermitJoinRelayClient() {
    MemoryBlob memory;
    blinker::GatewayAccessStore store(memory);
    FixedKeySource keys;
    FixedClock clock;
    DeterministicAead crypto;
    blinker::GatewayAccessDeliveryProcessor processor(
        store, keys, crypto, clock);
    FakeMqtt mqtt;
    blinker::MqttFrameTransport transport(mqtt, clock);
    const blinker::SessionCredentials credentials = mqttCredentials();
    CHECK_TRUE(transport.configure(credentials));
    blinker::GatewayCloudMux cloud(transport);
    FakePermitJoinRelayAdapter adapter;
    blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
    blinker::GatewayManagementClient management(
        cloud, processor, coordinator);
    blinker::GatewayPermitJoinRelayClient relay(
        cloud, management, coordinator, adapter, clock);
    CHECK_TRUE(management.configure(credentials, deliveryContext()));
    CHECK_TRUE(transport.start());
    transport.poll(1000U);
    management.poll();
    CHECK_TRUE(management.subscribed());

    uint8_t operation[blinker::gateway::kOperationIdSize];
    fill(operation, sizeof(operation), 0x31U);
    blinker::gateway::GatewayPermitJoinCommandView open;
    open.operationId = blinker::ByteView(operation, sizeof(operation));
    open.adapterId =
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    open.action = blinker::gateway::GatewayPermitJoinAction::Open;
    open.expiresAtUnixSeconds = clock.now + 60U;
    uint8_t commandBytes[
        blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize];
    size_t commandSize = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
        open, blinker::MutableByteSpan(
                  commandBytes, sizeof(commandBytes)),
        commandSize));
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(commandBytes, commandSize), 1U, false);
    management.poll();
    CHECK_TRUE(coordinator.windowOpen());
    CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Ready,
             adapter.stateValue);

    relay.poll();
    blinker::gateway::PermitJoinCandidateSnapshotView snapshot;
    CHECK_TRUE(blinker::gateway::decodePermitJoinCandidateSnapshot(
        blinker::ByteView(
            &mqtt.lastPayload[0], mqtt.lastPayload.size()),
        snapshot));
    CHECK_EQ(1U, snapshot.revision);
    CHECK_EQ(1U, snapshot.candidateCount);
    CHECK_TRUE(equal(
        snapshot.candidates[0].token,
        blinker::ByteView(adapter.candidate.token,
                          adapter.candidate.tokenSize)));

    blinker::gateway::PermitJoinSelectCommandView select;
    select.operationId = blinker::ByteView(operation, sizeof(operation));
    select.candidateToken = blinker::ByteView(
        adapter.candidate.token, adapter.candidate.tokenSize);
    uint8_t selectBytes[
        blinker::gateway::kPermitJoinSelectCommandMaximumEncodedSize];
    size_t selectSize = 0U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinSelectCommand(
        select, blinker::MutableByteSpan(
                    selectBytes, sizeof(selectBytes)),
        selectSize));
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(selectBytes, selectSize), 1U, false);
    CHECK_TRUE(!management.hasPendingEnvelope());
    management.poll();
    CHECK_EQ(blinker::ErrorCode::Ok, management.lastError());
    relay.poll();
    blinker::gateway::PermitJoinSelectResultView selectResult;
    CHECK_TRUE(blinker::gateway::decodePermitJoinSelectResult(
        blinker::ByteView(
            &mqtt.lastPayload[0], mqtt.lastPayload.size()),
        selectResult));
    CHECK_EQ(blinker::gateway::PermitJoinSelectStatus::Connecting,
             selectResult.status);
    CHECK_EQ(0U, selectResult.maxPacketSize);

    adapter.connectedValue = true;
    relay.poll();
    CHECK_TRUE(blinker::gateway::decodePermitJoinSelectResult(
        blinker::ByteView(
            &mqtt.lastPayload[0], mqtt.lastPayload.size()),
        selectResult));
    CHECK_EQ(blinker::gateway::PermitJoinSelectStatus::Connected,
             selectResult.status);
    CHECK_EQ(20U, selectResult.maxPacketSize);

    const uint8_t downPacket[] = {0x01U, 0x02U, 0x03U};
    const uint8_t downPacket2[] = {0x04U, 0x05U};
    blinker::gateway::PermitJoinRelayBatchView down;
    down.operationId = blinker::ByteView(operation, sizeof(operation));
    down.direction = blinker::gateway::PermitJoinRelayDirection::Down;
    down.sequence = 1U;
    down.packets[0] = blinker::ByteView(downPacket, sizeof(downPacket));
    down.packets[1] = blinker::ByteView(downPacket2, sizeof(downPacket2));
    down.packetCount = 2U;
    uint8_t relayBytes[
        blinker::gateway::kPermitJoinRelayBatchMaximumEncodedSize];
    size_t relaySize = 0U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinRelayBatch(
        down, blinker::MutableByteSpan(
                  relayBytes, sizeof(relayBytes)),
        relaySize));
    const uint32_t publishBeforeBlockedDown = mqtt.publishCalls;
    adapter.downWouldBlock = true;
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(relayBytes, relaySize), 1U, false);
    CHECK_TRUE(!management.hasPendingEnvelope());
    management.poll();
    CHECK_EQ(blinker::ErrorCode::Ok, management.lastError());
    relay.poll();
    CHECK_EQ(publishBeforeBlockedDown, mqtt.publishCalls);
    CHECK_EQ(0U, adapter.downWrites);
    adapter.downWouldBlock = false;
    adapter.blockAfterNextWrite = true;
    relay.poll();
    CHECK_EQ(publishBeforeBlockedDown, mqtt.publishCalls);
    CHECK_EQ(1U, adapter.downWrites);
    adapter.downWouldBlock = false;
    relay.poll();
    blinker::gateway::PermitJoinRelayAckView downAck;
    CHECK_TRUE(blinker::gateway::decodePermitJoinRelayAck(
        blinker::ByteView(
            &mqtt.lastPayload[0], mqtt.lastPayload.size()),
        downAck));
    CHECK_EQ(blinker::gateway::PermitJoinRelayDirection::Down,
             downAck.direction);
    CHECK_EQ(1U, downAck.sequence);
    CHECK_EQ(blinker::gateway::PermitJoinRelayAckStatus::Accepted,
             downAck.status);
    CHECK_EQ(2U, adapter.downWrites);
    CHECK_EQ(sizeof(downPacket2), adapter.downSize);

    // An application-level retry is acknowledged idempotently and is not
    // written to the southbound link twice.
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(relayBytes, relaySize), 1U, false);
    relay.poll();
    CHECK_EQ(2U, adapter.downWrites);

    const uint8_t upPacket[] = {0x11U, 0x22U, 0x33U, 0x44U};
    adapter.queueUp(upPacket, sizeof(upPacket));
    relay.poll();
    clock.monotonic += 20U;
    relay.poll();
    blinker::gateway::PermitJoinRelayBatchView up;
    CHECK_TRUE(blinker::gateway::decodePermitJoinRelayBatch(
        blinker::ByteView(
            &mqtt.lastPayload[0], mqtt.lastPayload.size()),
        up));
    CHECK_EQ(blinker::gateway::PermitJoinRelayDirection::Up,
             up.direction);
    CHECK_EQ(1U, up.sequence);
    CHECK_EQ(static_cast<size_t>(1U), up.packetCount);
    CHECK_TRUE(equal(
        up.packets[0], blinker::ByteView(upPacket, sizeof(upPacket))));
    const std::vector<uint8_t> firstUp = mqtt.lastPayload;
    const uint32_t firstUpPublish = mqtt.publishCalls;
    clock.monotonic += 499U;
    relay.poll();
    CHECK_EQ(firstUpPublish, mqtt.publishCalls);
    clock.monotonic += 1U;
    relay.poll();
    CHECK_EQ(firstUpPublish + 1U, mqtt.publishCalls);
    CHECK_TRUE(firstUp == mqtt.lastPayload);

    blinker::gateway::PermitJoinRelayAckView upAck;
    upAck.operationId = blinker::ByteView(operation, sizeof(operation));
    upAck.direction = blinker::gateway::PermitJoinRelayDirection::Up;
    upAck.sequence = 1U;
    upAck.status = blinker::gateway::PermitJoinRelayAckStatus::Accepted;
    size_t ackSize = 0U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinRelayAck(
        upAck, blinker::MutableByteSpan(
                   relayBytes, sizeof(relayBytes)),
        ackSize));
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(relayBytes, ackSize), 1U, false);
    CHECK_TRUE(!management.hasPendingEnvelope());
    management.poll();
    CHECK_EQ(blinker::ErrorCode::Ok, management.lastError());
    relay.poll();
    CHECK_EQ(firstUpPublish + 1U, mqtt.publishCalls);
    CHECK_EQ(blinker::ErrorCode::Ok, relay.lastError());

    const auto deliverAck = [&](blinker::gateway::PermitJoinRelayAckView value) {
        size_t written = 0U;
        CHECK_TRUE(blinker::gateway::encodePermitJoinRelayAck(
            value, blinker::MutableByteSpan(relayBytes, sizeof(relayBytes)), written));
        mqtt.deliver("/edge-hub/edge-client-1/management/down",
            blinker::ByteView(relayBytes, written), 1U, false);
        relay.poll();
    };
    deliverAck(upAck);
    CHECK_EQ(blinker::ErrorCode::Ok, relay.lastError());
    CHECK_TRUE(!relay.waitingUpstreamAck());
    CHECK_EQ(firstUpPublish + 1U, mqtt.publishCalls);

    adapter.queueUp(upPacket, sizeof(upPacket));
    relay.poll();
    clock.monotonic += 20U;
    relay.poll();
    const std::vector<uint8_t> secondUp = mqtt.lastPayload;
    CHECK_TRUE(relay.waitingUpstreamAck());
    CHECK_TRUE(blinker::gateway::decodePermitJoinRelayBatch(
        blinker::ByteView(secondUp.data(), secondUp.size()), up));
    CHECK_EQ(2U, up.sequence);
    const uint32_t beforeLateAck = mqtt.publishCalls;
    deliverAck(upAck);
    CHECK_EQ(blinker::ErrorCode::Ok, relay.lastError());
    CHECK_TRUE(relay.waitingUpstreamAck());
    CHECK_EQ(beforeLateAck, mqtt.publishCalls);

    uint8_t wrongOperation[blinker::gateway::kOperationIdSize];
    fill(wrongOperation, sizeof(wrongOperation), 0x55U);
    for (unsigned int variant = 0U; variant < 4U; ++variant) {
        blinker::gateway::PermitJoinRelayAckView wrong = upAck;
        if (variant == 0U) wrong.operationId = blinker::ByteView(wrongOperation, sizeof(wrongOperation));
        if (variant == 1U) wrong.direction = blinker::gateway::PermitJoinRelayDirection::Down;
        if (variant == 2U) wrong.sequence = 3U;
        if (variant == 3U) wrong.status = blinker::gateway::PermitJoinRelayAckStatus::Rejected;
        deliverAck(wrong);
        CHECK_TRUE(relay.lastError() != blinker::ErrorCode::Ok);
        CHECK_TRUE(relay.waitingUpstreamAck());
        CHECK_EQ(beforeLateAck, mqtt.publishCalls);
    }
    clock.monotonic += 500U;
    relay.poll();
    CHECK_TRUE(secondUp == mqtt.lastPayload); // Invalid/old ACKs never clear batch 2.
    upAck.sequence = 2U;
    upAck.status = blinker::gateway::PermitJoinRelayAckStatus::Rejected;
    deliverAck(upAck);
    CHECK_EQ(blinker::ErrorCode::ProtocolError, relay.lastError());
    CHECK_TRUE(!relay.waitingUpstreamAck());
    deliverAck(upAck); // Exact rejection is also terminal/idempotent, not success.
    CHECK_EQ(blinker::ErrorCode::ProtocolError, relay.lastError());
    upAck.status = blinker::gateway::PermitJoinRelayAckStatus::Accepted;
    deliverAck(upAck);
    CHECK_EQ(blinker::ErrorCode::SequenceConflict, relay.lastError());
    CHECK_TRUE(!relay.waitingUpstreamAck());
    relay.reset();
    relay.poll();
    CHECK_TRUE(!coordinator.windowOpen());
    CHECK_TRUE(!relay.active());
    upAck.sequence = 1U;
    deliverAck(upAck);
    CHECK_EQ(blinker::ErrorCode::Ok, relay.lastError());
    CHECK_TRUE(!relay.waitingUpstreamAck()); // Cleared operation has no previous ACK.
    management.poll(); // Publish the retained rejection before accepting another command.
    const uint32_t publishAfterAckMatrix = mqtt.publishCalls;

    // A corrupted MQTT payload is still rejected, not hidden as relay
    // traffic. Broker-side buffer ownership must fix its source instead.
    adapter.stateValue = blinker::GatewayPermitJoinAdapterState::Closing;
    const uint32_t pollsBeforeInvalid = adapter.polls;
    const uint8_t clearedPayload[23] = {};
    mqtt.deliver(
        "/edge-hub/edge-client-1/management/down",
        blinker::ByteView(clearedPayload, sizeof(clearedPayload)), 1U, false);
    CHECK_TRUE(management.hasPendingEnvelope());
    management.poll();
    CHECK_EQ(blinker::ErrorCode::InvalidEncoding, management.lastError());
    CHECK_EQ(pollsBeforeInvalid + 1U, adapter.polls);
    CHECK_TRUE(!coordinator.ownsChildSession()); // Invalid traffic did not starve cleanup.
    CHECK_EQ(0U, memory.replaceCalls);
    CHECK_EQ(publishAfterAckMatrix, mqtt.publishCalls);

    blinker::secureZero(blinker::MutableByteSpan(
        operation, sizeof(operation)));
    blinker::secureZero(blinker::MutableByteSpan(
        commandBytes, sizeof(commandBytes)));
    blinker::secureZero(blinker::MutableByteSpan(
        selectBytes, sizeof(selectBytes)));
    blinker::secureZero(blinker::MutableByteSpan(
        relayBytes, sizeof(relayBytes)));
}

// Exercises real coordinator / management / relay ordering, including the
// scheduler gap where only relay is polled while MQTT is not Online.
class PermitRelayFixture {
public:
    PermitRelayFixture()
        : store(memory), processor(store, keys, crypto, clock),
          transport(mqtt, clock), cloud(transport), coordinator(adapter, clock),
          management(cloud, processor, coordinator),
          relay(cloud, management, coordinator, adapter, clock), operation() {
        const blinker::SessionCredentials credentials = mqttCredentials();
        CHECK_TRUE(transport.configure(credentials));
        CHECK_TRUE(management.configure(credentials, deliveryContext()));
        CHECK_TRUE(transport.start());
        transport.poll(1000U);
        management.poll();
        fill(operation, sizeof(operation), 0x61U);
        window.action = blinker::gateway::GatewayPermitJoinAction::Open;
        window.adapterId = adapter.adapterId();
        window.operationId = blinker::ByteView(operation, sizeof(operation));
        window.expiresAtUnixSeconds = clock.now + 60U;
        command();
        relay.poll();
    }

    void command() {
        uint8_t bytes[blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize];
        size_t size = 0U;
        CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
            window, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(bytes, size)));
        management.poll();
    }
    void deliver(blinker::ByteView bytes) {
        const blinker::StringView topic = management.subscribeTopic();
        const std::string name(topic.data, topic.size);
        mqtt.deliver(name.c_str(), bytes, 1U, false);
        relay.poll();
    }
    void select(bool connected = true) {
        blinker::gateway::PermitJoinSelectCommandView value;
        value.operationId = window.operationId;
        value.candidateToken = blinker::ByteView(adapter.candidate.token, adapter.candidate.tokenSize);
        uint8_t bytes[blinker::gateway::kPermitJoinSelectCommandMaximumEncodedSize];
        size_t size = 0U;
        CHECK_TRUE(blinker::gateway::encodePermitJoinSelectCommand(
            value, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        deliver(blinker::ByteView(bytes, size));
        adapter.connectedValue = connected;
        relay.poll();
    }
    void down(uint32_t sequence = 1U, size_t count = 2U) {
        const uint8_t packet[] = {0x11U, 0x22U};
        blinker::gateway::PermitJoinRelayBatchView value;
        value.operationId = window.operationId;
        value.direction = blinker::gateway::PermitJoinRelayDirection::Down;
        value.sequence = sequence;
        value.packetCount = count;
        for (size_t i = 0U; i < count; ++i) value.packets[i] = blinker::ByteView(packet, sizeof(packet));
        uint8_t bytes[blinker::gateway::kPermitJoinRelayBatchMaximumEncodedSize];
        size_t size = 0U;
        CHECK_TRUE(blinker::gateway::encodePermitJoinRelayBatch(
            value, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        deliver(blinker::ByteView(bytes, size));
    }
    void up() {
        const uint8_t packet[] = {0x33U, 0x44U};
        adapter.queueUp(packet, sizeof(packet));
        relay.poll();
        clock.monotonic += 20U;
        relay.poll();
    }
    void ack(uint32_t sequence) {
        blinker::gateway::PermitJoinRelayAckView value;
        value.operationId = window.operationId;
        value.direction = blinker::gateway::PermitJoinRelayDirection::Up;
        value.sequence = sequence;
        value.status = blinker::gateway::PermitJoinRelayAckStatus::Accepted;
        uint8_t bytes[blinker::gateway::kPermitJoinRelayAckMaximumEncodedSize];
        size_t size = 0U;
        CHECK_TRUE(blinker::gateway::encodePermitJoinRelayAck(
            value, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        deliver(blinker::ByteView(bytes, size));
    }
    void offline() {
        transport.stop();
        relay.poll(); // Production CloudConnecting bypasses management.poll().
    }
    void reconnect(bool rotate) {
        blinker::SessionCredentials credentials = mqttCredentials();
        if (rotate) credentials.clientId = blinker::StringView("edge-client-2");
        CHECK_TRUE(transport.configure(credentials));
        CHECK_TRUE(transport.start());
        transport.poll(1000U);
        CHECK_TRUE(!management.subscribed());
        if (rotate) CHECK_TRUE(management.configure(credentials, deliveryContext()));
        const uint32_t before = mqtt.publishCalls;
        relay.poll(); // Online but the new management subscription is not ready.
        CHECK_EQ(before, mqtt.publishCalls);
        management.poll();
        CHECK_TRUE(management.subscribed());
    }
    blinker::ByteView last() const {
        return blinker::ByteView(mqtt.lastPayload.data(), mqtt.lastPayload.size());
    }

    MemoryBlob memory;
    blinker::GatewayAccessStore store;
    FixedKeySource keys;
    FixedClock clock;
    DeterministicAead crypto;
    blinker::GatewayAccessDeliveryProcessor processor;
    FakeMqtt mqtt;
    blinker::MqttFrameTransport transport;
    blinker::GatewayCloudMux cloud;
    FakePermitJoinRelayAdapter adapter;
    blinker::GatewayPermitJoinCoordinator coordinator;
    blinker::GatewayManagementClient management;
    blinker::GatewayPermitJoinRelayClient relay;
    uint8_t operation[blinker::gateway::kOperationIdSize];
    blinker::gateway::GatewayPermitJoinCommandView window;
};

void testPermitRelayCloudContinuity() {
    for (unsigned int rotate = 0U; rotate < 2U; ++rotate) {
        for (unsigned int phase = 0U; phase < 5U; ++phase) {
            PermitRelayFixture f;
            // Revision > 1 must survive too; sequence checks alone miss a
            // cleared scan/selection that aliases an older Broker snapshot.
            ++f.adapter.candidate.signalQuality;
            f.relay.poll();
            blinker::gateway::PermitJoinCandidateSnapshotView snapshot;
            CHECK_TRUE(blinker::gateway::decodePermitJoinCandidateSnapshot(f.last(), snapshot));
            CHECK_EQ(2U, snapshot.revision);
            if (phase != 3U) f.select(phase != 4U);
            if (phase == 1U) f.adapter.blockAfterNextWrite = true;
            if (phase <= 1U) f.down();
            if (phase == 2U) { f.up(); f.ack(1U); f.up(); }
            const std::vector<uint8_t> pending = f.mqtt.lastPayload;
            const uint32_t writes = f.adapter.downWrites;
            const uint32_t publishes = f.mqtt.publishCalls;
            f.offline();
            CHECK_TRUE(f.coordinator.windowOpen());
            CHECK_TRUE(f.relay.active());
            CHECK_EQ(writes, f.relay.downFragmentCount());
            CHECK_EQ(phase == 2U, f.relay.waitingUpstreamAck());
            CHECK_EQ(publishes, f.mqtt.publishCalls);
            f.reconnect(rotate != 0U);
            if (phase == 0U) {
                f.down(); // Already committed batch: ACK, no southbound replay.
                CHECK_EQ(2U, f.adapter.downWrites);
                f.down(2U, 14U);
                CHECK_EQ(16U, f.adapter.downWrites);
                f.down(3U, 1U);
                CHECK_EQ(16U, f.adapter.downWrites); // Cloud gap did not refill rate credit.
                f.clock.monotonic += 1000U;
                f.relay.poll();
                CHECK_EQ(17U, f.adapter.downWrites);
            } else if (phase == 1U) {
                f.adapter.downWouldBlock = false;
                f.relay.poll();
                CHECK_EQ(2U, f.adapter.downWrites); // Only the second packet.
                f.down();
                CHECK_EQ(2U, f.adapter.downWrites);
            } else if (phase == 2U) {
                f.ack(1U); // Late ACK must not clear the pending second batch.
                CHECK_TRUE(f.relay.waitingUpstreamAck());
                f.clock.monotonic += 500U;
                f.relay.poll();
                CHECK_TRUE(pending == f.mqtt.lastPayload);
                f.ack(2U);
                CHECK_TRUE(!f.relay.waitingUpstreamAck());
                f.up();
                blinker::gateway::PermitJoinRelayBatchView up;
                CHECK_TRUE(blinker::gateway::decodePermitJoinRelayBatch(f.last(), up));
                CHECK_EQ(3U, up.sequence);
                f.ack(3U);
            } else if (phase == 4U) {
                f.adapter.connectedValue = true;
                f.relay.poll();
                blinker::gateway::PermitJoinSelectResultView selection;
                CHECK_TRUE(blinker::gateway::decodePermitJoinSelectResult(f.last(), selection));
                CHECK_EQ(blinker::gateway::PermitJoinSelectStatus::Connected, selection.status);
            }
            f.clock.monotonic += 2000U;
            f.relay.poll();
            CHECK_TRUE(blinker::gateway::decodePermitJoinCandidateSnapshot(f.last(), snapshot));
            CHECK_EQ(2U, snapshot.revision);
            CHECK_EQ(0U, f.memory.replaceCalls);
        }
    }
}

void testPermitRelayCloudInvalidation() {
    for (unsigned int fault = 0U; fault < 10U; ++fault) {
        PermitRelayFixture f;
        f.select();
        f.adapter.blockAfterNextWrite = true;
        f.down();
        f.up();
        CHECK_EQ(1U, f.adapter.downWrites);
        f.offline();
        blinker::GatewayAccessDeliveryContext context = deliveryContext();
        if (fault == 0U) f.clock.now = f.window.expiresAtUnixSeconds;
        if (fault == 1U) f.clock.synchronized = false;
        if (fault == 2U) f.adapter.stateValue = blinker::GatewayPermitJoinAdapterState::Fault;
        if (fault == 3U) ++context.deviceKeyVersion;
        if (fault == 4U) context.tenantId = blinker::StringView("other-tenant");
        if (fault == 7U) context.environmentId = blinker::StringView("other-environment");
        if (fault == 8U) context.edgeHubLogicalDeviceId = blinker::StringView("other-hub");
        if (fault == 9U) ++context.deviceKeyVersion;
        if (fault == 3U || fault == 4U || fault >= 7U) {
            CHECK_TRUE(f.management.configure(mqttCredentials(), context));
        }
        if (fault == 9U) { // ABA rebind between polls is not authority continuity.
            CHECK_TRUE(f.management.configure(mqttCredentials(), deliveryContext()));
        }
        if (fault == 5U) {
            blinker::SessionCredentials invalid = mqttCredentials();
            invalid.security = blinker::MqttSecurity::PlainTcp;
            CHECK_TRUE(!f.management.configure(invalid, context));
        }
        if (fault == 6U) f.relay.reset();
        f.relay.poll();
        CHECK_TRUE(!f.coordinator.windowOpen());
        CHECK_TRUE(!f.relay.active());
        CHECK_TRUE(!f.relay.waitingUpstreamAck());
        CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Stopped, f.adapter.state());
        blinker::gateway::GatewayPermitJoinResultView terminal;
        CHECK_TRUE(blinker::gateway::decodeGatewayPermitJoinResult(f.coordinator.pendingResult(), terminal));
        const auto status = fault == 0U ? blinker::gateway::GatewayPermitJoinStatus::Expired
                                      : blinker::gateway::GatewayPermitJoinStatus::Rejected;
        CHECK_EQ(status, terminal.status);
        CHECK_TRUE(equal(f.window.operationId, terminal.operationId));
        f.clock.synchronized = true;
        CHECK_TRUE(f.management.configure(mqttCredentials(), deliveryContext()));
        f.reconnect(false);
        f.command(); // Same Open cannot resurrect the rejected / expired window.
        CHECK_TRUE(blinker::gateway::decodeGatewayPermitJoinResult(f.last(), terminal));
        CHECK_EQ(status, terminal.status);
        CHECK_TRUE(!f.coordinator.windowOpen());
        CHECK_EQ(1U, f.adapter.downWrites);
        ++f.operation[0];
        f.window.expiresAtUnixSeconds = f.clock.now + 60U;
        f.command();
        f.relay.poll();
        f.adapter.downWouldBlock = false;
        f.select();
        f.down();
        CHECK_EQ(3U, f.adapter.downWrites); // Only a fresh operation starts at 1.
        CHECK_EQ(0U, f.memory.replaceCalls);
    }
}

void testPermitJoinContracts() {
    uint8_t operation[blinker::gateway::kOperationIdSize];
    for (size_t index = 0U; index < sizeof(operation); ++index) {
        operation[index] = static_cast<uint8_t>(0x10U + index);
    }
    const uint8_t commandGolden[] = {
        0x86U, 0x01U, 0x05U, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x01U, 0x01U, 0x1BU,
        0x00U, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
    };
    const uint8_t resultGolden[] = {
        0x86U, 0x01U, 0x06U, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x01U, 0x01U, 0x1BU,
        0x00U, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
    };

    blinker::gateway::GatewayPermitJoinCommandView command;
    command.operationId = blinker::ByteView(operation, sizeof(operation));
    command.adapterId =
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    command.action = blinker::gateway::GatewayPermitJoinAction::Open;
    command.expiresAtUnixSeconds = 9007199254740991ULL;
    uint8_t encoded[
        blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize] = {};
    size_t written = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    CHECK_EQ(sizeof(commandGolden), written);
    CHECK_TRUE(equal(
        blinker::ByteView(encoded, written),
        blinker::ByteView(commandGolden, sizeof(commandGolden))));

    blinker::gateway::GatewayPermitJoinCommandView decodedCommand;
    CHECK_TRUE(blinker::gateway::decodeGatewayPermitJoinCommand(
        blinker::ByteView(encoded, written), decodedCommand));
    CHECK_EQ(command.adapterId, decodedCommand.adapterId);
    CHECK_EQ(command.action, decodedCommand.action);
    CHECK_EQ(command.expiresAtUnixSeconds,
             decodedCommand.expiresAtUnixSeconds);

    command.adapterId = UINT16_MAX;
    command.action = blinker::gateway::GatewayPermitJoinAction::Close;
    command.expiresAtUnixSeconds = 9007199254740991ULL;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    CHECK_EQ(
        blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize,
        written);
    command.adapterId = 0U;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
             blinker::gateway::encodeGatewayPermitJoinCommand(
                 command,
                 blinker::MutableByteSpan(encoded, sizeof(encoded)),
                 written).code());
    CHECK_TRUE(!blinker::gateway::decodeGatewayPermitJoinCommand(
        blinker::ByteView(commandGolden, sizeof(commandGolden) - 1U),
        decodedCommand));

    blinker::gateway::GatewayPermitJoinResultView result;
    result.operationId = blinker::ByteView(operation, sizeof(operation));
    result.adapterId =
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    result.status = blinker::gateway::GatewayPermitJoinStatus::Ready;
    result.expiresAtUnixSeconds = 9007199254740991ULL;
    uint8_t resultBytes[
        blinker::gateway::kGatewayPermitJoinResultMaximumEncodedSize] = {};
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinResult(
        result,
        blinker::MutableByteSpan(resultBytes, sizeof(resultBytes)),
        written));
    CHECK_EQ(sizeof(resultGolden), written);
    CHECK_TRUE(equal(
        blinker::ByteView(resultBytes, written),
        blinker::ByteView(resultGolden, sizeof(resultGolden))));
    blinker::gateway::GatewayPermitJoinResultView decodedResult;
    CHECK_TRUE(blinker::gateway::decodeGatewayPermitJoinResult(
        blinker::ByteView(resultBytes, written), decodedResult));
    CHECK_EQ(result.status, decodedResult.status);

    result.adapterId = UINT16_MAX;
    result.status = blinker::gateway::GatewayPermitJoinStatus::Unsupported;
    result.expiresAtUnixSeconds = 9007199254740991ULL;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinResult(
        result,
        blinker::MutableByteSpan(resultBytes, sizeof(resultBytes)),
        written));
    CHECK_EQ(
        blinker::gateway::kGatewayPermitJoinResultMaximumEncodedSize,
        written);

    blinker::secureZero(blinker::MutableByteSpan(operation, sizeof(operation)));
    blinker::secureZero(blinker::MutableByteSpan(encoded, sizeof(encoded)));
    blinker::secureZero(
        blinker::MutableByteSpan(resultBytes, sizeof(resultBytes)));
}

void testRevocationContracts() {
    uint8_t operation[blinker::gateway::kOperationIdSize];
    uint8_t instance[blinker::gateway::kDeviceInstanceIdSize];
    uint8_t controller[blinker::gateway::kControllerIdSize];
    uint8_t nonce[blinker::gateway::kProofNonceSize];
    uint8_t digest[blinker::gateway::kAccessMaterialDigestSize];
    fill(operation, sizeof(operation), 0x11U);
    fill(instance, sizeof(instance), 0x22U);
    fill(controller, sizeof(controller), 0x33U);
    fill(nonce, sizeof(nonce), 0x44U);
    fill(digest, sizeof(digest), 0x55U);

    blinker::gateway::GatewayRevocationCommandView command;
    command.operationId = blinker::ByteView(operation, sizeof(operation));
    command.topologyVersion = 7U;
    command.childDeviceInstanceId =
        blinker::ByteView(instance, sizeof(instance));
    command.accessEpoch = 9U;
    command.controllerId = blinker::ByteView(controller, sizeof(controller));
    command.gatewayCredentialVersion = 3U;
    command.accessStorageRevision = 2U;
    command.phase = blinker::gateway::GatewayRevocationPhase::Prepare;
    command.expiresAtUnixSeconds = 2000000060ULL;
    uint8_t encoded[
        blinker::gateway::kGatewayRevocationCommandMaximumEncodedSize];
    size_t written = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    blinker::gateway::GatewayRevocationCommandView decodedCommand;
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationCommand(
        blinker::ByteView(encoded, written), decodedCommand));
    CHECK_EQ(blinker::gateway::GatewayRevocationPhase::Prepare,
             decodedCommand.phase);

    uint8_t maximumGrant[
        blinker::gateway::kGatewayRevocationGrantMaximumSize];
    fill(maximumGrant, sizeof(maximumGrant), 0x66U);
    command.topologyVersion = UINT32_MAX;
    command.accessEpoch = UINT32_MAX;
    command.gatewayCredentialVersion = UINT32_MAX;
    command.accessStorageRevision = UINT32_MAX;
    command.phase = blinker::gateway::GatewayRevocationPhase::Apply;
    command.grant = blinker::ByteView(maximumGrant, sizeof(maximumGrant));
    command.expiresAtUnixSeconds = UINT64_MAX;
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    CHECK_EQ(
        blinker::gateway::kGatewayRevocationCommandMaximumEncodedSize,
        written);

    command.phase = blinker::gateway::GatewayRevocationPhase::Finalize;
    command.grant = blinker::ByteView();
    command.receiptDigest = blinker::ByteView(digest, sizeof(digest));
    command.expiresAtUnixSeconds = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    command.phase = blinker::gateway::GatewayRevocationPhase::Supersede;
    command.accessStorageRevision = 0U;
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
        command, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        written));
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationCommand(
        blinker::ByteView(encoded, written), decodedCommand));
    CHECK_EQ(blinker::gateway::GatewayRevocationPhase::Supersede,
             decodedCommand.phase);
    CHECK_EQ(0U, decodedCommand.accessStorageRevision);
    command.expiresAtUnixSeconds = 1U;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
             blinker::gateway::encodeGatewayRevocationCommand(
                 command,
                 blinker::MutableByteSpan(encoded, sizeof(encoded)),
                 written).code());

    blinker::gateway::GatewayRevocationResultView result;
    result.operationId = blinker::ByteView(operation, sizeof(operation));
    result.topologyVersion = 7U;
    result.phase = blinker::gateway::GatewayRevocationPhase::Prepare;
    result.status = blinker::gateway::GatewayRevocationStatus::NonceReady;
    result.payload = blinker::ByteView(nonce, sizeof(nonce));
    uint8_t resultBytes[
        blinker::gateway::kGatewayRevocationResultMaximumEncodedSize];
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationResult(
        result, blinker::MutableByteSpan(
            resultBytes, sizeof(resultBytes)), written));
    blinker::gateway::GatewayRevocationResultView decodedResult;
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        blinker::ByteView(resultBytes, written), decodedResult));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::NonceReady,
             decodedResult.status);

    uint8_t maximumReceipt[
        blinker::gateway::kGatewayRevocationReceiptMaximumSize];
    fill(maximumReceipt, sizeof(maximumReceipt), 0x77U);
    result.topologyVersion = UINT32_MAX;
    result.phase = blinker::gateway::GatewayRevocationPhase::Apply;
    result.status = blinker::gateway::GatewayRevocationStatus::Revoked;
    result.payload = blinker::ByteView(
        maximumReceipt, sizeof(maximumReceipt));
    result.receiptDigest = blinker::ByteView(digest, sizeof(digest));
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationResult(
        result, blinker::MutableByteSpan(
            resultBytes, sizeof(resultBytes)), written));
    CHECK_EQ(
        blinker::gateway::kGatewayRevocationResultMaximumEncodedSize,
        written);

    result.phase = blinker::gateway::GatewayRevocationPhase::Supersede;
    result.status = blinker::gateway::GatewayRevocationStatus::Retired;
    result.payload = blinker::ByteView();
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationResult(
        result, blinker::MutableByteSpan(
            resultBytes, sizeof(resultBytes)), written));
    CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(
        blinker::ByteView(resultBytes, written), decodedResult));
    CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Retired,
             decodedResult.status);
    result.phase = blinker::gateway::GatewayRevocationPhase::Finalize;
    result.status = blinker::gateway::GatewayRevocationStatus::Finalized;
    result.payload = blinker::ByteView();
    CHECK_TRUE(blinker::gateway::encodeGatewayRevocationResult(
        result, blinker::MutableByteSpan(
            resultBytes, sizeof(resultBytes)), written));

    blinker::secureZero(blinker::MutableByteSpan(operation, sizeof(operation)));
    blinker::secureZero(blinker::MutableByteSpan(instance, sizeof(instance)));
    blinker::secureZero(blinker::MutableByteSpan(controller, sizeof(controller)));
    blinker::secureZero(blinker::MutableByteSpan(nonce, sizeof(nonce)));
    blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    blinker::secureZero(blinker::MutableByteSpan(encoded, sizeof(encoded)));
    blinker::secureZero(blinker::MutableByteSpan(
        resultBytes, sizeof(resultBytes)));
    blinker::secureZero(blinker::MutableByteSpan(
        maximumGrant, sizeof(maximumGrant)));
    blinker::secureZero(blinker::MutableByteSpan(
        maximumReceipt, sizeof(maximumReceipt)));
}

} // namespace

int main() {
    testProtocolMaximums();
    testStore();
    testDeliveryProcessor();
    testChildDirectory();
    testRenewalReservesDirectorySlot();
    testManagementClient();
    testPermitJoinRelayClient();
    testPermitRelayCloudContinuity();
    testPermitRelayCloudInvalidation();
    testPermitJoinContracts();
    testRevocationContracts();
    std::cout << "Gateway access store/delivery tests passed" << std::endl;
    return 0;
}
