#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "BlinkerV2/core/SecureMemory.h"
#include "BlinkerV2/protocol/cbor/Cbor.h"
#include "BlinkerV2/provisioning/TaskManagementGrant.h"
#include "BlinkerV2/provisioning/TaskAuthorityContract.h"
#include "BlinkerV2/provisioning/TaskManagementSession.h"
#include "BlinkerV2/identity/ConfiguredDeviceKeySource.h"
#include "BlinkerV2/identity/DeviceAccessStore.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
using Bytes = std::vector<uint8_t>;
using Properties = std::map<std::string, Bytes>;
ByteView view(const Bytes& bytes) { return ByteView(bytes.data(), bytes.size()); }
uint8_t nibble(char c) {
    CHECK((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    return static_cast<uint8_t>(c <= '9' ? c - '0' : c - 'a' + 10);
}
Properties load(const char* path) {
    std::ifstream input(path);
    CHECK(input);
    Properties values;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t split = line.find('=');
        CHECK(split != std::string::npos && split != 0U);
        const std::string name = line.substr(0U, split), hex = line.substr(split + 1U);
        CHECK(!hex.empty() && hex.size() % 2U == 0U && values.count(name) == 0U);
        Bytes bytes;
        for (size_t i = 0U; i < hex.size(); i += 2U) {
            bytes.push_back(static_cast<uint8_t>((nibble(hex[i]) << 4U) | nibble(hex[i + 1U])));
        }
        values[name] = bytes;
    }
    return values;
}
ByteView get(const Properties& values, const std::string& name) {
    const auto found = values.find(name);
    CHECK(found != values.end());
    return view(found->second);
}
void empty(const TaskManagementGrant& grant) {
    CHECK(grant.serverKeyId == 0U && grant.signature.empty());
    CHECK(grant.binding.deviceInstanceId.empty() && grant.binding.taskScope.empty());
    CHECK(grant.binding.nonce.empty() && grant.binding.requestId.empty() && grant.binding.operationDigest.empty());
}

// Boundary double: exact digest/signature matching, NOT cryptographic proof.
// The independent Node oracle checks these same fixtures with real crypto.
struct FixtureVerifier : IServerSignatureVerifier {
    ByteView digest, signature;
    ServerSignatureAlgorithm algorithm;
    unsigned calls = 0U;
    Result failure;
    Result verifyDigest(uint32_t key, ServerSignatureAlgorithm alg, ByteView hash, ByteView sig) override {
        ++calls;
        if (!failure) return failure;
        return key == UINT32_MAX && alg == algorithm &&
               constantTimeEqual(digest, hash) && constantTimeEqual(signature, sig)
            ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
    }
};

Bytes encode(const TaskManagementGrant& grant) {
    uint8_t output[kTaskManagementGrantMaxSize]; ByteView encoded;
    CHECK(encodeTaskManagementGrant(grant, MutableByteSpan(output, sizeof(output)), encoded));
    return Bytes(encoded.data, encoded.data + encoded.size);
}

// Deliberately bypass field validation to test hostile CBOR at the real decoder.
Bytes raw(const TaskManagementGrant& grant, uint64_t version = 1U,
          uint64_t key = UINT32_MAX, uint64_t algorithm = 1U) {
    uint8_t output[256]; cbor::Writer w(MutableByteSpan(output, sizeof(output)));
    CHECK(w.beginArray(9U)); CHECK(w.writeUnsigned(version));
    CHECK(w.writeBytes(grant.binding.deviceInstanceId)); CHECK(w.writeBytes(grant.binding.taskScope));
    CHECK(w.writeBytes(grant.binding.nonce)); CHECK(w.writeBytes(grant.binding.requestId));
    CHECK(w.writeBytes(grant.binding.operationDigest)); CHECK(w.writeUnsigned(key));
    CHECK(w.writeUnsigned(algorithm)); CHECK(w.writeBytes(grant.signature));
    return Bytes(w.view().data, w.view().data + w.size());
}

void run(const Properties& values, const char* name, ServerSignatureAlgorithm algorithm) {
    TaskManagementGrant input;
    input.binding.deviceInstanceId = get(values, "device_instance_id");
    input.binding.taskScope = get(values, "task_scope");
    input.binding.nonce = get(values, "nonce");
    input.binding.requestId = get(values, "request_id");
    input.binding.operationDigest = get(values, "operation_digest");
    input.serverKeyId = UINT32_MAX; input.signatureAlgorithm = algorithm;
    input.signature = get(values, std::string(name) + ".signature");
    const Bytes encoded = encode(input);
    CHECK(encoded.size() == kTaskManagementGrantMaxSize);
    CHECK(constantTimeEqual(view(encoded), get(values, std::string(name) + ".encoded")));
    uint8_t hash[kTaskGrantDigestSize];
    CHECK(taskManagementGrantDigest(input, MutableByteSpan(hash, sizeof(hash))));
    FixtureVerifier verifier;
    verifier.algorithm = algorithm; verifier.signature = input.signature;
    verifier.digest = get(values, std::string(name) + ".digest");
    CHECK(constantTimeEqual(ByteView(hash, sizeof(hash)), verifier.digest));
    TaskManagementGrant decoded;
    CHECK(verifyTaskManagementGrant(view(encoded), input.binding, verifier, decoded));
    CHECK(verifier.calls == 1U);
    CHECK(decoded.binding.taskScope.data >= encoded.data() &&
          decoded.binding.taskScope.data < encoded.data() + encoded.size());
    CHECK(encode(decoded) == encoded);

    // Each expected binding field is mandatory and an exact match, including
    // digest and requestId; no untrusted zero/empty bootstrap wildcard.
    ByteView TaskGrantBinding::* fields[] = {
        &TaskGrantBinding::deviceInstanceId, &TaskGrantBinding::taskScope,
        &TaskGrantBinding::nonce, &TaskGrantBinding::requestId, &TaskGrantBinding::operationDigest};
    for (auto field : fields) {
        TaskGrantBinding expected = input.binding;
        const ByteView original = expected.*field;
        Bytes changed(original.data, original.data + original.size); changed[0] ^= 1U;
        expected.*field = view(changed);
        const unsigned calls = verifier.calls;
        CHECK(verifyTaskManagementGrant(view(encoded), expected, verifier, decoded).code() == ErrorCode::AuthenticationRequired);
        empty(decoded); CHECK(verifier.calls == calls);
        expected.*field = ByteView();
        CHECK(verifyTaskManagementGrant(view(encoded), expected, verifier, decoded).code() == ErrorCode::InvalidArgument);
        empty(decoded); CHECK(verifier.calls == calls);
    }
    for (size_t i = 0U; i < encoded.size(); ++i) {
        decoded = input;
        CHECK(!decodeTaskManagementGrant(ByteView(encoded.data(), i), decoded)); empty(decoded);
        Bytes changed = encoded; changed[i] ^= 1U;
        decoded = input;
        CHECK(!verifyTaskManagementGrant(view(changed), input.binding, verifier, decoded)); empty(decoded);
    }
    // Signature verifier failures propagate; they must not publish decoded data.
    verifier.failure = Result::failure(ErrorCode::NotConfigured);
    CHECK(verifyTaskManagementGrant(view(encoded), input.binding, verifier, decoded).code() == ErrorCode::NotConfigured);
    empty(decoded); verifier.failure = Result::success();
    TaskManagementGrant changed = input;
    changed.serverKeyId = 1U;
    const Bytes unknownKey = encode(changed);
    CHECK(verifyTaskManagementGrant(view(unknownKey), input.binding, verifier, decoded).code() == ErrorCode::AuthenticationRequired);
    empty(decoded);

    // Invalid widths/zero identifiers; output reset includes partial decode.
    const uint8_t zeros[32] = {};
    for (auto field : fields) {
        changed = input; changed.binding.*field = ByteView(zeros, 15U);
        CHECK(!decodeTaskManagementGrant(view(raw(changed)), decoded)); empty(decoded);
        if (field == &TaskGrantBinding::operationDigest) continue; // A hash may be all zero bits.
        changed.binding.*field = ByteView(zeros, 16U);
        CHECK(!decodeTaskManagementGrant(view(raw(changed)), decoded)); empty(decoded);
        TaskGrantBinding expected = input.binding; expected.*field = ByteView(zeros, 16U);
        CHECK(!verifyTaskManagementGrant(view(encoded), expected, verifier, decoded)); empty(decoded);
    }
    for (size_t length : {0U, 63U}) {
        changed = input; changed.signature.size = length;
        CHECK(!decodeTaskManagementGrant(view(raw(changed)), decoded)); empty(decoded);
    }
    CHECK(!decodeTaskManagementGrant(view(raw(input, 2U)), decoded)); empty(decoded);
    CHECK(!decodeTaskManagementGrant(view(raw(input, 1U, 0U)), decoded)); empty(decoded);
    CHECK(!decodeTaskManagementGrant(view(raw(input, 1U, UINT64_C(0x100000000))), decoded)); empty(decoded);
    for (uint64_t alg : {UINT64_C(0), UINT64_C(3), UINT64_C(257)}) {
        CHECK(!decodeTaskManagementGrant(view(raw(input, 1U, 1U, alg)), decoded)); empty(decoded);
    }
    // Use a short keyId so malformed forms remain below the 176-byte cap.
    Bytes shortGrant = raw(input, 1U, 1U, static_cast<uint8_t>(algorithm));
    Bytes changedWire = shortGrant; changedWire.push_back(0U);
    CHECK(!decodeTaskManagementGrant(view(changedWire), decoded)); empty(decoded);
    changedWire = shortGrant; changedWire[1] = 0x18U; changedWire.insert(changedWire.begin() + 2, 1U);
    CHECK(decodeTaskManagementGrant(view(changedWire), decoded).code() == ErrorCode::NonCanonicalEncoding); empty(decoded);
    changedWire = shortGrant; changedWire[2] = 0x58U; changedWire.insert(changedWire.begin() + 3, 16U);
    CHECK(decodeTaskManagementGrant(view(changedWire), decoded).code() == ErrorCode::NonCanonicalEncoding); empty(decoded);
    changedWire = shortGrant; changedWire[0] = 0x9fU; changedWire.push_back(0xffU);
    CHECK(!decodeTaskManagementGrant(view(changedWire), decoded)); empty(decoded);
    changedWire = shortGrant; changedWire[0] = 0x88U;
    CHECK(!decodeTaskManagementGrant(view(changedWire), decoded)); empty(decoded);

    uint8_t output[kTaskManagementGrantMaxSize];
    for (size_t size = 0U; size < sizeof(output); ++size) {
        ByteView result = view(encoded);
        CHECK(!encodeTaskManagementGrant(input, MutableByteSpan(output, size), result)); CHECK(result.empty());
    }
    ByteView result = view(encoded);
    changed = input; changed.serverKeyId = 0U;
    CHECK(!encodeTaskManagementGrant(changed, MutableByteSpan(output, sizeof(output)), result)); CHECK(result.empty());
    memset(hash, 0xa5, sizeof(hash));
    CHECK(!taskManagementGrantDigest(changed, MutableByteSpan(hash, sizeof(hash))));
    CHECK(constantTimeEqual(ByteView(hash, sizeof(hash)), ByteView(zeros, sizeof(zeros))));
    CHECK(!taskManagementGrantDigest(input, MutableByteSpan(hash, sizeof(hash) - 1U)));
    // Hashing is independent of the signature and does not require its storage.
    changed = input; changed.signature = ByteView();
    CHECK(taskManagementGrantDigest(changed, MutableByteSpan(hash, sizeof(hash))));
    CHECK(constantTimeEqual(ByteView(hash, sizeof(hash)), verifier.digest));
}
struct MemoryBlob : IAtomicBlobStore {
    Bytes data;
    unsigned writes = 0U;
    bool failLoad = false;
    Result load(MutableByteSpan out, size_t& written) override {
        written = 0U;
        if (failLoad) return Result::failure(ErrorCode::InternalError);
        if (data.empty()) return Result::failure(ErrorCode::NotFound);
        if (out.size < data.size()) return Result::failure(ErrorCode::BufferTooSmall);
        memcpy(out.data, data.data(), data.size()); written = data.size(); return Result::success();
    }
    Result replace(ByteView value) override { ++writes; data.assign(value.data, value.data + value.size); return Result::success(); }
    Result clear() override { ++writes; data.clear(); return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};

struct UncheckedControllerSource : IControllerCredentialSource {
    ControllerCredential value;
    Result loadActive(ByteView, ControllerCredentialDomain, uint32_t, ControllerCredential& output) override {
        output = value; return Result::success();
    }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};

DeviceInstanceId instance(const Properties& values) {
    DeviceInstanceId result; const ByteView bytes = get(values, "device_instance_id");
    CHECK(bytes.size == sizeof(result.bytes)); memcpy(result.bytes, bytes.data, bytes.size); return result;
}

void authorityRoots(const Properties& values) {
    DeviceKey key;
    for (size_t i = 0U; i < sizeof(key.bytes); ++i) key.bytes[i] = static_cast<uint8_t>(65U + i);
    ControllerCredential admin;
    admin.domain = ControllerCredentialDomain::PlatformAccess;
    admin.ownershipGeneration = UINT32_MAX; admin.credentialVersion = 1U; admin.permissions = kControllerPermissionAll;
    for (size_t i = 0U; i < sizeof(admin.controllerId); ++i) admin.controllerId[i] = static_cast<uint8_t>(17U + i);
    for (size_t i = 0U; i < sizeof(admin.secret); ++i) admin.secret[i] = static_cast<uint8_t>(97U + i);
    MemoryBlob blob; DeviceAccessStore store(blob);
    CHECK(store.bootstrap(key, UINT32_MAX, admin));
    const DeviceInstanceId device = instance(values);
    TaskRootSources sources;
    uint8_t digest[32]; const MutableByteSpan out(digest, sizeof(digest));
    const unsigned writes = blob.writes;
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        sources.deviceKey = mode == 1U ? nullptr : &store;
        sources.controllers = mode == 0U ? nullptr : &store;
        sources.adminControllerId = mode == 0U ? ByteView() : admin.id();
        sources.accessEpoch = mode == 0U ? 0U : UINT32_MAX;
        CHECK(taskAuthorityRootDigest(device, sources, out));
        const char* name = mode == 0U ? "authority.wifi" : (mode == 1U ? "authority.ble" : "authority.mixed");
        CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, name)));
        TaskManagementChallenge context;
        memcpy(context.authority.deviceInstanceId, device.bytes, 16U);
        memcpy(context.authority.rootDigest, digest, 32U);
        context.authority.taskScope[0] = 1U; context.nonce[0] = 2U;
        context.requestId[0] = 3U; context.operationDigest[0] = 4U;
        TaskManagementResult result, confirmed;
        uint8_t wire[kTaskManagementResultMaxSize]; ByteView encoded;
        for (TaskResultKey role : {TaskResultKey::DeviceKey, TaskResultKey::Admin}) {
            const bool supported = role == TaskResultKey::DeviceKey ? mode != 1U : mode != 0U;
            CHECK(encodeTaskManagementReceipt(device, sources, role, context, result,
                MutableByteSpan(wire, sizeof(wire)), encoded).ok() == supported);
            if (!supported) { CHECK(encoded.empty()); continue; }
            const ByteView secret = role == TaskResultKey::DeviceKey ? ByteView(key.bytes, 32U) : admin.authenticationSecret();
            CHECK(verifyTaskManagementResult(encoded, context, role, secret, confirmed));
            CHECK(!verifyTaskManagementResult(encoded, context, role, ByteView(digest, 32U), confirmed));
            auto stale = context; stale.authority.rootDigest[0] ^= 1U;
            CHECK(!encodeTaskManagementReceipt(device, sources, role, stale, result,
                MutableByteSpan(wire, sizeof(wire)), encoded) && encoded.empty());
        }
    }
    CHECK(blob.writes == writes); // Digest never changes storage.
    // Shared install/rotation/revocation do not change the owner root fingerprint.
    ControllerCredential shared = admin; shared.controllerId[0] ^= 0x80U;
    shared.permissions = kControllerPermissionObserve | kControllerPermissionControl;
    CHECK(store.installVerified(shared));
    CHECK(taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "authority.mixed")));
    shared.credentialVersion = 2U; shared.secret[0] ^= 1U;
    CHECK(store.rotateVerified(1U, shared)); CHECK(taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "authority.mixed")));
    sources.adminControllerId = shared.id();
    CHECK(taskAuthorityRootDigest(device, sources, out).code() == ErrorCode::AuthenticationRequired);
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
    sources.adminControllerId = admin.id();
    CHECK(store.revokeVerified(shared.id(), shared.domain, UINT32_MAX, 2U));
    CHECK(taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "authority.mixed")));
    key.bytes[0] ^= 128U; CHECK(store.replace(key));
    CHECK(taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "authority.rotated")));
    admin.credentialVersion = 2U; admin.secret[31] ^= 1U;
    CHECK(store.rotateVerified(1U, admin)); CHECK(taskAuthorityRootDigest(device, sources, out));
    CHECK(!constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "authority.rotated")));
    // No fallback from combined to BLE-only when DeviceKey disappears.
    CHECK(store.clear()); CHECK(!taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
    sources.deviceKey = nullptr; CHECK(taskAuthorityRootDigest(device, sources, out));
    sources.accessEpoch = 1U; CHECK(!taskAuthorityRootDigest(device, sources, out));
    sources.accessEpoch = UINT32_MAX; blob.failLoad = true;
    CHECK(taskAuthorityRootDigest(device, sources, out).code() == ErrorCode::InternalError);
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
    // Even an injected source claiming success cannot bypass role/selector checks.
    UncheckedControllerSource unchecked; sources.controllers = &unchecked;
    for (unsigned invalid = 0U; invalid < 7U; ++invalid) {
        unchecked.value = admin;
        switch (invalid) {
        case 0: unchecked.value.domain = ControllerCredentialDomain::Local; unchecked.value.ownershipGeneration = 0U; break;
        case 1: unchecked.value.controllerId[15] ^= 1U; break;
        case 2: unchecked.value.ownershipGeneration = 1U; break;
        case 3: unchecked.value.permissions = kControllerPermissionControl; break;
        case 4: memset(unchecked.value.secret, 0, sizeof(unchecked.value.secret)); break;
        case 5: unchecked.value.credentialVersion = 0U; break;
        default: unchecked.value.suite = static_cast<ControllerCredentialSuite>(2U); break;
        }
        memset(digest, 0xa5, sizeof(digest));
        CHECK(!taskAuthorityRootDigest(device, sources, out));
        CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
    }
    CHECK(!taskAuthorityRootDigest(DeviceInstanceId(), sources, out));
    CHECK(!taskAuthorityRootDigest(device, TaskRootSources(), out));
    // Manual WiFi uses the original configured key source, not a fake Admin.
    char encodedKey[43]; StringView encoded;
    CHECK(encodeDeviceKey(key, MutableCharSpan(encodedKey, sizeof(encodedKey)), encoded));
    ConfiguredDeviceKeySource configured; CHECK(configured.configure(encoded));
    sources = TaskRootSources(); sources.deviceKey = &configured;
    CHECK(taskAuthorityRootDigest(device, sources, out));
    const Bytes original(digest, digest + sizeof(digest));
    auto other = device; other.bytes[15] ^= 1U;
    CHECK(taskAuthorityRootDigest(other, sources, out)); CHECK(!constantTimeEqual(view(original), ByteView(digest, sizeof(digest))));
    configured.clear(); CHECK(!taskAuthorityRootDigest(device, sources, out));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
}

void empty(const TaskAuthorityChange& change) {
    CHECK(change.requestId.empty() && change.deviceInstanceId.empty() && change.taskScope.empty() &&
        change.previousRoot.empty() && change.currentRoot.empty());
}

void authorityChanges(const Properties& values, const char* operation) {
    const std::string prefix = std::string("authority.") + operation;
    const ByteView wire = get(values, prefix + ".encoded");
    TaskAuthorityChange change; CHECK(decodeTaskAuthorityChange(wire, change));
    uint8_t buffer[256], digest[32]; ByteView encoded;
    CHECK(encodeTaskAuthorityChange(change, MutableByteSpan(buffer, sizeof(buffer)), encoded));
    CHECK(constantTimeEqual(encoded, wire));
    CHECK(taskAuthorityChangeDigest(change, MutableByteSpan(digest, sizeof(digest))));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, prefix + ".digest")));
    FixtureVerifier verifier;
    verifier.algorithm = ServerSignatureAlgorithm::Ed25519;
    verifier.digest = get(values, prefix + ".grant_digest"); verifier.signature = get(values, prefix + ".signature");
    const ByteView grantWire = get(values, prefix + ".grant"), nonce = get(values, "nonce");
    const DeviceInstanceId device = instance(values);
    TaskManagementGrant verified;
    CHECK(verifyTaskAuthorityChangeGrant(grantWire, change, nonce, device, change.currentRoot, verifier, verified));
    CHECK(verifier.calls == 1U);
    // Pre-signature checks use independently observed actual roots/instance.
    auto other = device; other.bytes[15] ^= 1U;
    CHECK(!verifyTaskAuthorityChangeGrant(grantWire, change, nonce, other, change.currentRoot, verifier, verified));
    empty(verified); CHECK(verifier.calls == 1U);
    CHECK(!verifyTaskAuthorityChangeGrant(grantWire, change, nonce, device, get(values, "operation_digest"), verifier, verified));
    empty(verified); CHECK(verifier.calls == 1U);
    CHECK(!verifyTaskAuthorityChangeGrant(grantWire, change, ByteView(), device, change.currentRoot, verifier, verified));
    empty(verified);
    for (size_t length = 0U; length < wire.size; ++length) {
        TaskAuthorityChange decoded = change;
        CHECK(!decodeTaskAuthorityChange(ByteView(wire.data, length), decoded)); empty(decoded);
        ByteView out = wire;
        CHECK(!encodeTaskAuthorityChange(change, MutableByteSpan(buffer, length), out)); CHECK(out.empty());
    }
    for (size_t i = 0U; i < wire.size; ++i) {
        Bytes altered(wire.data, wire.data + wire.size); altered[i] ^= 1U;
        TaskAuthorityChange decoded;
        if (!decodeTaskAuthorityChange(view(altered), decoded)) { empty(decoded); continue; }
        CHECK(!verifyTaskAuthorityChangeGrant(grantWire, decoded, nonce, device, change.currentRoot, verifier, verified));
        empty(verified);
    }
    Bytes malformed(wire.data, wire.data + wire.size);
    malformed[1] = 0x18U; malformed.insert(malformed.begin() + 2U, 1U);
    TaskAuthorityChange decoded;
    CHECK(!decodeTaskAuthorityChange(view(malformed), decoded)); empty(decoded);
    malformed.assign(wire.data, wire.data + wire.size); malformed.push_back(0U);
    CHECK(!decodeTaskAuthorityChange(view(malformed), decoded)); empty(decoded);
    auto invalid = change; invalid.previousRoot = change.currentRoot; // No-op is not a new root transition.
    CHECK(!encodeTaskAuthorityChange(invalid, MutableByteSpan(buffer, sizeof(buffer)), encoded)); CHECK(encoded.empty());
    CHECK(!taskAuthorityChangeDigest(invalid, MutableByteSpan(digest, sizeof(digest))));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(DeviceKey().bytes, 32U)));
    const uint8_t zeros[32] = {};
    ByteView TaskAuthorityChange::* fields[] = {&TaskAuthorityChange::requestId, &TaskAuthorityChange::deviceInstanceId,
        &TaskAuthorityChange::taskScope, &TaskAuthorityChange::previousRoot, &TaskAuthorityChange::currentRoot};
    for (auto field : fields) {
        invalid = change;
        invalid.*field = ByteView(zeros, field == &TaskAuthorityChange::previousRoot || field == &TaskAuthorityChange::currentRoot ? 32U : 16U);
        CHECK(!encodeTaskAuthorityChange(invalid, MutableByteSpan(buffer, sizeof(buffer)), encoded)); CHECK(encoded.empty());
        invalid.*field = ByteView(zeros, 15U);
        CHECK(!taskAuthorityChangeDigest(invalid, MutableByteSpan(digest, sizeof(digest))));
    }
    verifier.failure = Result::failure(ErrorCode::NotConfigured);
    CHECK(verifyTaskAuthorityChangeGrant(grantWire, change, nonce, device, change.currentRoot, verifier, verified).code() == ErrorCode::NotConfigured);
    empty(verified);
}
void typedPrepare(const Properties& values) {
    TaskMutation request;
    memcpy(request.requestId, get(values, "request_id").data, 16U);
    request.expectedRevision = UINT32_MAX; request.taskId = UINT32_MAX;
    request.kind = TaskMutationKind::Replace;
    auto& definition = request.definition;
    definition.enabled = true; definition.timing.durationMillis = 1000U;
    memcpy(definition.key, "power", 5U); definition.keyLength = 5U;
    definition.value[0] = 0xF5U; definition.valueLength = 1U;
    uint8_t buffer[256], digest[32]; ByteView encoded;
    CHECK(encodeTaskMutationPrepare(request, MutableByteSpan(buffer, sizeof(buffer)), encoded));
    CHECK(constantTimeEqual(encoded, get(values, "prepare.mutation.encoded")));
    CHECK(taskMutationPrepareDigest(request, get(values, "authority.mixed"), MutableByteSpan(digest, sizeof(digest))));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "prepare.mutation.digest")));
    CHECK(taskReadPrepareDigest(get(values, "request_id"), 0U, UINT32_MAX,
        get(values, "authority.mixed"), MutableByteSpan(digest, sizeof(digest))));
    CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "prepare.read.digest")));
    for (const std::string name : {"metadata", "target"}) {
        TaskReadPrepare read; read.catalog = true;
        memcpy(read.requestId, get(values, "request_id").data, 16U);
        read.index = name == "metadata" ? UINT32_MAX : 1U;
        if (read.index != UINT32_MAX) memcpy(read.catalogFingerprint, get(values, "catalog.fingerprint").data, 32U);
        CHECK(encodeTaskReadPrepare(read, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        CHECK(constantTimeEqual(encoded, get(values, "catalog." + name + ".prepare")));
        CHECK(taskReadPrepareDigest(read, get(values, "authority.mixed"), MutableByteSpan(digest, sizeof(digest))));
        CHECK(constantTimeEqual(ByteView(digest, sizeof(digest)), get(values, "catalog." + name + ".digest")));
        for (size_t n = 0U; n < encoded.size; ++n) {
            TaskReadPrepare invalid;
            CHECK(!decodeTaskReadPrepare(encoded.subspan(0U, n), invalid));
            CHECK(invalid.requestId[0] == 0U && !invalid.catalog);
        }
    }
}

void empty(const TaskMutation& value) {
    CHECK(value.definition.keyLength == 0U && value.definition.valueLength == 0U && value.taskId == 0U);
    for (uint8_t byte : value.requestId) CHECK(byte == 0U);
}
void empty(const TaskManagementResult& value) {
    CHECK(!value.hasRecord && value.count == 0U && value.tableRevision == 0U && value.mutation.taskId == 0U);
    CHECK(value.record.keyLength == 0U && value.record.valueLength == 0U);
}
void strictPrepare(const Properties& values) {
    const ByteView fixture = get(values, "prepare.mutation.encoded");
    TaskMutation decoded;
    CHECK(decodeTaskMutationPrepare(fixture, decoded));
    uint8_t buffer[256]; ByteView encoded;
    for (TaskMutationKind kind : {TaskMutationKind::Create, TaskMutationKind::Replace, TaskMutationKind::Remove,
            TaskMutationKind::SetEnabled, TaskMutationKind::Start}) {
        TaskMutation request = decoded;
        request.kind = kind;
        if (kind == TaskMutationKind::Create) request.taskId = 0U;
        if (kind != TaskMutationKind::Create && kind != TaskMutationKind::Replace) request.definition = TaskSnapshotRecord();
        request.enabled = kind == TaskMutationKind::SetEnabled;
        CHECK(encodeTaskMutationPrepare(request, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        Bytes owned(encoded.data, encoded.data + encoded.size);
        TaskMutation roundtrip;
        CHECK(decodeTaskMutationPrepare(view(owned), roundtrip));
        owned.assign(owned.size(), 0U); // Decoder returns owned key/scalar, not RX views.
        CHECK(encodeTaskMutationPrepare(roundtrip, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        for (size_t size = 0U; size < encoded.size; ++size) {
            TaskMutation bad = roundtrip;
            CHECK(!decodeTaskMutationPrepare(encoded.subspan(0U, size), bad)); empty(bad);
        }
        Bytes bad(encoded.data, encoded.data + encoded.size); bad.push_back(0U);
        CHECK(!decodeTaskMutationPrepare(view(bad), roundtrip)); empty(roundtrip);
        bad.assign(encoded.data, encoded.data + encoded.size);
        bad[1] = 0x18U; bad.insert(bad.begin() + 2U, 1U); // Overlong version integer.
        CHECK(!decodeTaskMutationPrepare(view(bad), roundtrip)); empty(roundtrip);
        bad.assign(encoded.data, encoded.data + encoded.size); bad[0] = 0x9FU;
        CHECK(!decodeTaskMutationPrepare(view(bad), roundtrip)); empty(roundtrip);
        ByteView small;
        CHECK(!encodeTaskMutationPrepare(request, MutableByteSpan(buffer, 1U), small)); CHECK(small.empty());
    }
    // Arbitrary mutations may remain valid, but every accepted value must
    // roundtrip EXACTLY: no truncation, ignored fields or hidden wire data.
    for (size_t i = 0U; i < fixture.size; ++i) {
        Bytes altered(fixture.data, fixture.data + fixture.size); altered[i] ^= 0x80U;
        if (!decodeTaskMutationPrepare(view(altered), decoded)) { empty(decoded); continue; }
        CHECK(encodeTaskMutationPrepare(decoded, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        CHECK(constantTimeEqual(encoded, view(altered)));
    }
    // Explicit u32 overflow, unknown operation, zero request and forbidden
    // definition on a non-definition operation, all otherwise canonical CBOR.
    for (unsigned failure = 0U; failure < 5U; ++failure) {
        cbor::Writer w(MutableByteSpan(buffer, sizeof(buffer)));
        CHECK(w.beginArray(7U)); CHECK(w.writeUnsigned(1U));
        uint8_t zeros[16] = {};
        CHECK(w.writeBytes(failure == 0U ? ByteView(zeros, 16U) : get(values, "request_id")));
        CHECK(w.writeUnsigned(failure == 1U ? 0x100000000ULL : 1U));
        CHECK(w.writeUnsigned(failure == 2U ? 256U : 4U));
        CHECK(w.writeUnsigned(failure == 3U ? 0x100000000ULL : 1U));
        CHECK(w.writeBool(false));
        if (failure == 4U) CHECK(w.beginArray(0U)); else CHECK(w.writeNull());
        CHECK(!decodeTaskMutationPrepare(w.view(), decoded)); empty(decoded);
    }
    TaskReadPrepare read;
    CHECK(decodeTaskReadPrepare(get(values, "prepare.read.encoded"), read));
    CHECK(encodeTaskReadPrepare(read, MutableByteSpan(buffer, sizeof(buffer)), encoded));
    CHECK(constantTimeEqual(encoded, get(values, "prepare.read.encoded")));
    for (size_t size = 0U; size < encoded.size; ++size) {
        TaskReadPrepare bad = read;
        CHECK(!decodeTaskReadPrepare(encoded.subspan(0U, size), bad)); CHECK(bad.requestId[0] == 0U);
    }
    Bytes bad(encoded.data, encoded.data + encoded.size); bad[19] = 1U; // Metadata revision must be zero.
    CHECK(!decodeTaskReadPrepare(view(bad), read)); CHECK(read.requestId[0] == 0U);
}

TaskManagementChallenge challengeFixture(const Properties& values) {
    TaskManagementChallenge value;
    memcpy(value.authority.deviceInstanceId, get(values, "device_instance_id").data, 16U);
    memcpy(value.authority.taskScope, get(values, "task_scope").data, 16U);
    memcpy(value.authority.rootDigest, get(values, "authority.mixed").data, 32U);
    memcpy(value.nonce, get(values, "nonce").data, 16U); memcpy(value.requestId, get(values, "request_id").data, 16U);
    memcpy(value.operationDigest, get(values, "operation_digest").data, 32U);
    return value;
}
void authenticatedResults(const Properties& values) {
    const TaskManagementChallenge challenge = challengeFixture(values);
    uint8_t buffer[256]; ByteView encoded;
    CHECK(encodeTaskManagementChallenge(challenge, MutableByteSpan(buffer, sizeof(buffer)), encoded));
    CHECK(constantTimeEqual(encoded, get(values, "challenge.encoded")) && encoded.size == kTaskManagementChallengeMaxSize);
    TaskManagementChallenge parsed;
    CHECK(decodeTaskManagementChallenge(encoded, parsed)); CHECK(parsed.authority == challenge.authority);
    for (size_t size = 0U; size < encoded.size; ++size) {
        CHECK(!decodeTaskManagementChallenge(encoded.subspan(0U, size), parsed)); CHECK(!parsed.authority.valid());
    }
    Bytes bad(encoded.data, encoded.data + encoded.size); bad[1] = 2U;
    CHECK(!decodeTaskManagementChallenge(view(bad), parsed)); CHECK(!parsed.authority.valid());
    bad.assign(encoded.data, encoded.data + encoded.size); bad.push_back(0U);
    CHECK(!decodeTaskManagementChallenge(view(bad), parsed));
    uint8_t key[32]; for (size_t i = 0U; i < sizeof(key); ++i) key[i] = static_cast<uint8_t>(0xA0U + i);
    const ByteView secret(key, sizeof(key));
    for (TaskResultKey role : {TaskResultKey::DeviceKey, TaskResultKey::Admin}) {
        for (TaskManagementResultKind kind : {TaskManagementResultKind::Authority, TaskManagementResultKind::Mutation,
                TaskManagementResultKind::Metadata, TaskManagementResultKind::Record, TaskManagementResultKind::Retired,
                TaskManagementResultKind::Catalog, TaskManagementResultKind::Target, TaskManagementResultKind::Rejected}) {
            TaskManagementResult value;
            value.kind = kind; value.tableRevision = 9U; value.count = 2U;
            if (kind == TaskManagementResultKind::Rejected) value.rejection = TaskMutationRejection::WriteBudget;
            if (kind == TaskManagementResultKind::Mutation) {
                value.mutation.taskId = 3U; value.mutation.tableRevision = 9U; value.mutation.replayed = true;
            } else if (kind == TaskManagementResultKind::Record) {
                TaskMutation request; CHECK(decodeTaskMutationPrepare(get(values, "prepare.mutation.encoded"), request));
                value.record = request.definition; value.hasRecord = true; value.index = 1U;
                value.record.taskId = 3U; value.record.definitionRevision = 8U;
                value.record.startSequence = 7U; value.record.progress = TaskProgress::Completed;
            }
            if (kind == TaskManagementResultKind::Retired) {
                value.count = 0U; memcpy(value.retirementId, get(values, "retirement_id").data, 16U);
            }
            const bool catalog = kind == TaskManagementResultKind::Catalog || kind == TaskManagementResultKind::Target;
            if (catalog) {
                value.catalog.capacity = 2U; value.catalog.total = 2U;
                memcpy(value.catalog.fingerprint, get(values, "catalog.fingerprint").data, 32U);
                if (kind == TaskManagementResultKind::Target) {
                    value.index = 1U; value.catalog.keyLength = 5U; memcpy(value.catalog.key, "level", 5U);
                    value.catalog.valueType = ValueType::UnsignedInteger;
                }
            }
            CHECK(encodeTaskManagementResult(challenge, value, role, secret, MutableByteSpan(buffer, sizeof(buffer)), encoded));
            const std::string prefix = role == TaskResultKey::DeviceKey ? "result.device." : "result.admin.";
            const std::string name = kind == TaskManagementResultKind::Rejected
                ? (role == TaskResultKey::DeviceKey ? "rejection.device.1" : "rejection.admin.1")
                : catalog ? std::string("catalog.") + (kind == TaskManagementResultKind::Catalog ? "metadata." : "target.") +
                (role == TaskResultKey::DeviceKey ? "device.result" : "admin.result") : prefix + std::to_string(static_cast<unsigned>(kind));
            CHECK(constantTimeEqual(encoded, get(values, name)));
            const Bytes wire(encoded.data, encoded.data + encoded.size);
            TaskManagementResult decoded;
            CHECK(verifyTaskManagementResult(view(wire), challenge, role, secret, decoded));
            CHECK(decoded.kind == kind && decoded.tableRevision == value.tableRevision && decoded.count == value.count);
            if (kind == TaskManagementResultKind::Rejected) {
                CHECK(decoded.rejection == TaskMutationRejection::WriteBudget);
                value.rejection = TaskMutationRejection::StartCooldown;
                CHECK(encodeTaskManagementResult(challenge, value, role, secret, MutableByteSpan(buffer, sizeof(buffer)), encoded));
                CHECK(constantTimeEqual(encoded, get(values, role == TaskResultKey::DeviceKey ? "rejection.device.2" : "rejection.admin.2")));
                for (unsigned reason : {0U, 3U, 255U}) {
                    value.rejection = static_cast<TaskMutationRejection>(reason);
                    CHECK(!encodeTaskManagementResult(challenge, value, role, secret, MutableByteSpan(buffer, sizeof(buffer)), encoded));
                }
                value.rejection = TaskMutationRejection::WriteBudget;
            }
            for (size_t i = 0U; i < wire.size(); ++i) {
                Bytes altered = wire; altered[i] ^= 1U;
                CHECK(!verifyTaskManagementResult(view(altered), challenge, role, secret, decoded)); empty(decoded);
            }
            for (size_t size = 0U; size < wire.size(); ++size) {
                CHECK(!verifyTaskManagementResult(ByteView(wire.data(), size), challenge, role, secret, decoded)); empty(decoded);
            }
            for (unsigned field = 0U; field < 6U; ++field) {
                auto stale = challenge;
                uint8_t* fields[] = {stale.authority.deviceInstanceId, stale.authority.taskScope, stale.authority.rootDigest,
                    stale.nonce, stale.requestId, stale.operationDigest};
                fields[field][0] ^= 1U;
                CHECK(!verifyTaskManagementResult(view(wire), stale, role, secret, decoded)); empty(decoded);
            }
            CHECK(!verifyTaskManagementResult(view(wire), challenge,
                role == TaskResultKey::DeviceKey ? TaskResultKey::Admin : TaskResultKey::DeviceKey, secret, decoded)); empty(decoded);
            key[0] ^= 1U;
            CHECK(!verifyTaskManagementResult(view(wire), challenge, role, secret, decoded)); empty(decoded);
            key[0] ^= 1U;
            Bytes altered = wire; altered.push_back(0U);
            CHECK(!verifyTaskManagementResult(view(altered), challenge, role, secret, decoded)); empty(decoded);
            altered = wire; altered[1] = 0x18U; altered.insert(altered.begin() + 2U, 1U);
            CHECK(!verifyTaskManagementResult(view(altered), challenge, role, secret, decoded)); empty(decoded);
            ByteView small;
            CHECK(!encodeTaskManagementResult(challenge, value, role, secret, MutableByteSpan(buffer, 1U), small)); CHECK(small.empty());
            value.count = 11U;
            CHECK(!encodeTaskManagementResult(challenge, value, role, secret, MutableByteSpan(buffer, sizeof(buffer)), small)); CHECK(small.empty());
        }
    }
}
void retirementPrepare(const Properties& values) {
    const ByteView golden = get(values, "prepare.retirement.encoded");
    TaskRetirementPrepare request;
    CHECK(decodeTaskRetirementPrepare(golden, request));
    uint8_t buffer[128], digest[32]; ByteView encoded;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(buffer, sizeof(buffer)), encoded));
    CHECK(constantTimeEqual(encoded, golden) && encoded.size == kTaskRetirementPrepareMaxSize);
    for (bool query : {false, true}) {
        CHECK(taskRetirementPrepareDigest(request, query, MutableByteSpan(digest, sizeof(digest))));
        CHECK(constantTimeEqual(ByteView(digest, 32U), get(values, query ? "prepare.retirement-read.digest" : "prepare.retire.digest")));
    }
    for (size_t size = 0U; size < golden.size; ++size) {
        CHECK(!decodeTaskRetirementPrepare(golden.subspan(0U, size), request));
        CHECK(!request.authority.valid() && request.requestId[0] == 0U);
    }
    Bytes changed(golden.data, golden.data + golden.size); changed.push_back(0U);
    CHECK(!decodeTaskRetirementPrepare(view(changed), request));
    changed.assign(golden.data, golden.data + golden.size); changed[1] = 0x18U; changed.insert(changed.begin() + 2U, 1U);
    CHECK(!decodeTaskRetirementPrepare(view(changed), request));
    for (size_t offset : {3U, 20U, 37U, 55U}) {
        changed.assign(golden.data, golden.data + golden.size);
        memset(changed.data() + offset, 0, offset == 55U ? 32U : 16U);
        CHECK(!decodeTaskRetirementPrepare(view(changed), request));
    }
}
} // namespace

int main(int argc, char** argv) {
    CHECK(argc == 2);
    const Properties values = load(argv[1]);
    run(values, "ed25519", ServerSignatureAlgorithm::Ed25519);
    run(values, "p256", ServerSignatureAlgorithm::EcdsaP256Sha256Raw);
    authorityRoots(values);
    authorityChanges(values, "initialize"); authorityChanges(values, "rebind");
    typedPrepare(values);
    strictPrepare(values); authenticatedResults(values);
    retirementPrepare(values);
    std::cout << "task grant + real root sources/authority change tests passed (injected signature boundary)\n";
}
