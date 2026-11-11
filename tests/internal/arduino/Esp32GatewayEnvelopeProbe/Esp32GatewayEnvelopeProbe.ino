// Isolated hardware gate, NOT a product example. No real key/account/network.
// Writes only synthetic fixtures to nvs:bl_n3_env/record. Never erase NVS.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Crypto.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Storage.h>
#include <BlinkerV2/runtime/GatewayAccessDeliveryProcessor.h>
#include <BlinkerV2/core/Sha256.h>
#include <esp_system.h>
#include "EnvelopeVectors.h"

namespace {
using namespace blinker;
using namespace gateway_envelope_probe;

Esp32NvsBlobStoreConfig config() {
    Esp32NvsBlobStoreConfig value;
    value.namespaceName = "bl_n3_env";
    value.maximumSize = GatewayAccessStore::serializedSize;
    return value;
}

// Counts calls to the actual NVS adapter. The protection override is explicitly
// test-only: this does not attest Flash Encryption or close the D4b gate.
class ProbeBlob final : public Esp32NvsAtomicBlobStore {
public:
    ProbeBlob() : Esp32NvsAtomicBlobStore(config()) {}
    StorageProtection protection() const override { return StorageProtection::EncryptedFlash; }
    Result replace(ByteView bytes) override {
        ++attempts;
        if (failNext) { failNext = false; return Result::failure(ErrorCode::WouldBlock); }
        const Result result = Esp32NvsAtomicBlobStore::replace(bytes);
        if (result) ++writes;
        return result;
    }
    uint32_t attempts = 0U, writes = 0U;
    bool failNext = false;
} blob;

class FixtureKey final : public IDeviceKeySource {
public:
    Result load(DeviceKey& output) override {
        memset(output.bytes, changed ? 0x41 : 0x42, sizeof(output.bytes));
        return Result::success();
    }
    StorageProtection protection() const override { return StorageProtection::EncryptedFlash; }
    bool changed = false;
} keys;

// Inject expiry/clock admission without touching the MCU/SNTP/product clock.
class FixtureClock final : public IClock {
public:
    uint32_t monotonicMillis() const override { return millis(); }
    bool unixTime(uint64_t& seconds) const override { seconds = now; return trusted; }
    uint64_t now = 1700000000ULL;
    bool trusted = true;
} clockSource;

Esp32MbedTlsCryptoProvider crypto;
GatewayAccessStore store(blob);
GatewayAccessDeliveryProcessor processor(store, keys, crypto, clockSource);
uint32_t checks = 0U, failedLine = 0U;
uint8_t ack[gateway::kGatewayAccessDeliveryAckMaximumEncodedSize];
uint8_t encoded[gateway::kGatewayAccessEnvelopeMaximumEncodedSize];
size_t ackSize = 0U;

#define REQUIRE(value) do { ++checks; if (!(value)) { failedLine = __LINE__; return false; } } while (false)

GatewayAccessDeliveryContext context() {
    // Base DeviceKey profiles intentionally omit optional text copies. KDF/AAD
    // must still bind those envelope fields even when this context is empty.
    GatewayAccessDeliveryContext value;
    value.deviceKeyVersion = 7U;
    return value;
}

bool sameAck() {
    return ackSize == sizeof(expectedAck) && memcmp(ack, expectedAck, ackSize) == 0;
}

Result process(ByteView envelope, const GatewayAccessDeliveryContext& ctx) {
    memset(ack, 0xa5, sizeof(ack));
    ackSize = 99U;
    return processor.process(ctx, envelope, MutableByteSpan(ack, sizeof(ack)), ackSize);
}

bool digest(uint8_t (&output)[kSha256Size], bool& present) {
    uint8_t bytes[GatewayAccessStore::serializedSize] = {};
    size_t size = 0U;
    const Result result = blob.load(MutableByteSpan(bytes, sizeof(bytes)), size);
    present = result.ok();
    if (!result && result.code() != ErrorCode::NotFound) return false;
    const Result hashed = sha256(ByteView(bytes, size), MutableByteSpan(output, sizeof(output)));
    secureZero(MutableByteSpan(bytes, sizeof(bytes)));
    return hashed.ok();
}

bool rejected(ByteView envelope, const GatewayAccessDeliveryContext& ctx, ErrorCode expected) {
    uint8_t before[kSha256Size], after[kSha256Size];
    bool existed = false, exists = false;
    REQUIRE(digest(before, existed));
    const uint32_t attempts = blob.attempts;
    REQUIRE(process(envelope, ctx).code() == expected);
    REQUIRE(ackSize == 0U);
    for (size_t i = 0U; i < sizeof(ack); ++i) REQUIRE(ack[i] == 0U);
    REQUIRE(blob.attempts == attempts);
    REQUIRE(digest(after, exists));
    REQUIRE(existed == exists && memcmp(before, after, sizeof(before)) == 0);
    return true;
}

bool negativeMatrix() {
    // Re-encode valid canonical fields without re-sealing: tests real KDF/AAD
    // and GCM authentication, not merely malformed-CBOR rejection.
    for (unsigned int field = 0U; field < 15U; ++field) {
        gateway::GatewayAccessEnvelopeView value;
        REQUIRE(gateway::decodeGatewayAccessEnvelope(ByteView(firstEnvelope, sizeof(firstEnvelope)), value));
        uint8_t changed[48U];
        ByteView bytes;
        switch (field) {
        case 0U: value.environmentId = StringView("other-cn"); break;
        case 1U: value.tenantId = StringView("other-tenant"); break;
        case 2U: value.edgeHubLogicalDeviceId = StringView("other-hub"); break;
        case 3U: ++value.deviceKeyVersion; break;
        case 4U: value.childLogicalDeviceId = StringView("other-child"); break;
        case 5U: ++value.accessEpoch; break;
        case 6U: ++value.credentialVersion; break;
        case 7U: ++value.presenceKeyVersion; break;
        case 8U: ++value.expiresAtUnixSeconds; break;
        case 9U: bytes = value.operationId; break;
        case 10U: bytes = value.childDeviceInstanceId; break;
        case 11U: bytes = value.controllerId; break;
        case 12U: bytes = value.nonce; break;
        case 13U: bytes = value.ciphertext; break;
        case 14U: bytes = value.tag; break;
        }
        if (!bytes.empty()) {
            REQUIRE(bytes.size <= sizeof(changed));
            memcpy(changed, bytes.data, bytes.size); changed[0] ^= 1U;
            const ByteView modified(changed, bytes.size);
            switch (field) {
            case 9U: value.operationId = modified; break;
            case 10U: value.childDeviceInstanceId = modified; break;
            case 11U: value.controllerId = modified; break;
            case 12U: value.nonce = modified; break;
            case 13U: value.ciphertext = modified; break;
            case 14U: value.tag = modified; break;
            }
        }
        size_t size = 0U;
        REQUIRE(gateway::encodeGatewayAccessEnvelope(value, MutableByteSpan(encoded, sizeof(encoded)), size));
        REQUIRE(rejected(ByteView(encoded, size), context(), ErrorCode::AuthenticationRequired));
    }
    for (unsigned int field = 0U; field < 4U; ++field) {
        GatewayAccessDeliveryContext wrong = context();
        if (field == 0U) wrong.environmentId = StringView("wrong");
        if (field == 1U) wrong.tenantId = StringView("wrong");
        if (field == 2U) wrong.edgeHubLogicalDeviceId = StringView("wrong");
        if (field == 3U) ++wrong.deviceKeyVersion;
        REQUIRE(rejected(ByteView(firstEnvelope, sizeof(firstEnvelope)), wrong, ErrorCode::AuthenticationRequired));
    }
    keys.changed = true;
    const bool wrongKey = rejected(ByteView(firstEnvelope, sizeof(firstEnvelope)), context(), ErrorCode::AuthenticationRequired);
    keys.changed = false;
    REQUIRE(wrongKey);
    clockSource.trusted = false;
    const bool untrusted = rejected(ByteView(firstEnvelope, sizeof(firstEnvelope)), context(), ErrorCode::AuthenticationRequired);
    clockSource.trusted = true;
    REQUIRE(untrusted);
    clockSource.now = 2000000000ULL;
    const bool expired = rejected(ByteView(firstEnvelope, sizeof(firstEnvelope)), context(), ErrorCode::AuthenticationRequired);
    clockSource.now = 1700000000ULL;
    REQUIRE(expired);
    return true;
}

bool verifyPersisted() {
    // Exact fixture match before any write/cleanup. Unknown records fail closed.
    GatewayAccessRecord record;
    REQUIRE(store.load(record));
    uint8_t expected[kSha256Size];
    REQUIRE(sha256(ByteView(firstEnvelope, sizeof(firstEnvelope)), MutableByteSpan(expected, sizeof(expected))));
    const bool match = record.storageRevision == 1U && record.deliveryRevision == 1U
        && memcmp(record.envelopeDigest, expected, sizeof(expected)) == 0;
    clearGatewayAccessRecord(record);
    REQUIRE(match);
    const uint32_t attempts = blob.attempts;
    REQUIRE(process(ByteView(firstEnvelope, sizeof(firstEnvelope)), context()));
    REQUIRE(sameAck());
    REQUIRE(blob.attempts == attempts);
    return true;
}

bool exercise() {
    GatewayAccessRecord previous;
    const Result existing = store.load(previous);
    clearGatewayAccessRecord(previous);
    REQUIRE(existing || existing.code() == ErrorCode::NotFound);
    if (existing) REQUIRE(verifyPersisted());
    REQUIRE(negativeMatrix());
    if (!existing) {
        blob.failNext = true; // failed persistence cannot produce an ACK
        REQUIRE(process(ByteView(firstEnvelope, sizeof(firstEnvelope)), context()).code() == ErrorCode::WouldBlock);
        REQUIRE(ackSize == 0U && blob.writes == 0U);
    }
    REQUIRE(process(ByteView(firstEnvelope, sizeof(firstEnvelope)), context()));
    REQUIRE(sameAck());
    REQUIRE(verifyPersisted());
    REQUIRE(negativeMatrix());
    const uint32_t writes = blob.writes;
    REQUIRE(process(ByteView(rewrappedEnvelope, sizeof(rewrappedEnvelope)), context()));
    REQUIRE(sameAck() && blob.writes == writes + 1U);
    REQUIRE(rejected(ByteView(conflictingEnvelope, sizeof(conflictingEnvelope)), context(), ErrorCode::SequenceConflict));
    // Restore only the synthetic envelope metadata, so the next real reboot
    // checks the frozen original ACK with zero replace calls.
    REQUIRE(process(ByteView(firstEnvelope, sizeof(firstEnvelope)), context()));
    REQUIRE(sameAck() && blob.writes == writes + 2U);
    return verifyPersisted();
}

void report(char command, bool passed) {
    Serial.printf("envelopeGate command=%c pass=%u checks=%lu failedLine=%lu attempts=%lu writes=%lu ackSame=%u\n",
        command, passed ? 1U : 0U, static_cast<unsigned long>(checks), static_cast<unsigned long>(failedLine),
        static_cast<unsigned long>(blob.attempts), static_cast<unsigned long>(blob.writes), sameAck() ? 1U : 0U);
}

void hubWitness() {
    // Optional, read-only witness for restoring an existing Hub. No other
    // product namespaces or raw credentials are exported.
    nvs_handle_t handle;
    esp_err_t error = nvs_open("bl_eh_access", NVS_READONLY, &handle);
    uint8_t bytes[GatewayAccessStore::serializedSize] = {}, hash[kSha256Size] = {};
    size_t size = sizeof(bytes);
    if (error == ESP_OK) {
        error = nvs_get_blob(handle, "child", bytes, &size);
        nvs_close(handle);
    }
    if (error == ESP_OK && size == sizeof(bytes) &&
        sha256(ByteView(bytes, size), MutableByteSpan(hash, sizeof(hash)))) {
        Serial.printf("hubAccessWitness bytes=%u sha256=", static_cast<unsigned int>(size));
        for (size_t i = 0U; i < sizeof(hash); ++i) Serial.printf("%02x", hash[i]);
        Serial.println();
    } else Serial.printf("hubAccessWitness unavailable=%ld\n", static_cast<long>(error));
    secureZero(MutableByteSpan(bytes, sizeof(bytes)));
    secureZero(MutableByteSpan(hash, sizeof(hash)));
}
} // namespace

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0U);
    delay(200U);
    Serial.printf("envelopeGate ready=%u namespace=bl_n3_env fixtureOnly=1\n", blob.begin() ? 1U : 0U);
}

void loop() {
    if (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());
        checks = failedLine = 0U;
        if (command == 'e') report(command, exercise());
        if (command == 'v') report(command, verifyPersisted());
        if (command == 's') hubWitness();
        if (command == 'x') {
            const bool verified = verifyPersisted();
            report(command, verified && blob.clear()); // exact test key only
        }
        if (command == 'r') { blob.end(); Serial.flush(); delay(100U); esp_restart(); }
    }
    delay(1U);
}
