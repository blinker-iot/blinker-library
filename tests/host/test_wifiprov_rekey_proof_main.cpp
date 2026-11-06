#include <BlinkerV2/security/WifiprovRekeyProof.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvRekeyApply.h>
#include <BlinkerV2/provisioning/WifiprovRekeyEndpoint.h>
#include <BlinkerV2/provisioning/WifiprovRekeyJournal.h>
#include <BlinkerV2/runtime/TaskRootOwnerGate.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

using namespace blinker;
using namespace blinker::security;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::cerr << "WiFiProv rekey check failed at line " << __LINE__ << ": " << #condition << '\n'; \
        std::exit(1); \
    } \
} while (false)

std::vector<uint8_t> hex(const char* input) {
    std::vector<uint8_t> bytes;
    for (; *input != '\0'; input += 2U) {
        const char pair[] = {input[0], input[1], '\0'};
        bytes.push_back(static_cast<uint8_t>(std::strtoul(pair, nullptr, 16)));
    }
    return bytes;
}

ByteView view(const std::vector<uint8_t>& value) { return ByteView(value.data(), value.size()); }

bool same(ByteView first, ByteView second) {
    return first.size == second.size && std::memcmp(first.data, second.data, first.size) == 0;
}

void fill(uint8_t* output, size_t size, uint8_t start) {
    for (size_t i = 0U; i < size; ++i) output[i] = static_cast<uint8_t>(start + i);
}

// Same synthetic bytes as Gateway test-device-v2-wifiprov-rekey-proof.js.
struct Fixture {
    DeviceKey oldKey;
    DeviceKey newKey;
    WifiprovRekeyBinding binding;
    std::vector<uint8_t> proof = hex("18fdbd40feaed63861c522255279a3e71dbc6f732d74313212b675a0fb491527");
    std::vector<uint8_t> receipt = hex("3f7b9b1533b87deb9b435d32c2628374a16a9e37d92e8b7aa8bbfe5853144789");
    std::vector<uint8_t> wrapKey = hex("ddb1d414b36b639571a8cf19faa4a7f545c9d10c6b962316b219bb8549c7d141");
    std::vector<uint8_t> iv = hex("404142434445464748494a4b");
    std::vector<uint8_t> box = hex("e758aa31d13a9ec9ca8cb4e74517d3e110a9f1c5f3f82aadc445d8adac883dde5a4d75f584b5da61836ca373d2327caf");
    std::vector<uint8_t> aad;

    Fixture() {
        fill(oldKey.bytes, sizeof(oldKey.bytes), 1U);
        fill(newKey.bytes, sizeof(newKey.bytes), 33U);
        WifiprovRekeyOldBinding& previous = binding.previous;
        fill(previous.operationId, sizeof(previous.operationId), 16U);
        fill(previous.challenge, sizeof(previous.challenge), 32U);
        fill(previous.instance.bytes, sizeof(previous.instance.bytes), 48U);
        previous.accessEpoch = 0x01020304U;
        previous.fromVersion = 0x10203040U;
        const auto oldLocator = hex("42e3b10bd1fdb72f963710d6053fc845");
        const auto newLocator = hex("993914c5e39e1379987843dff3cf24fa");
        std::memcpy(previous.oldLocator.bytes, oldLocator.data(), oldLocator.size());
        std::memcpy(binding.newLocator.bytes, newLocator.data(), newLocator.size());
        const uint8_t wrapDomain[] = "blinker.wifiprov-rekey.wrap.v1";
        aad.assign(wrapDomain, wrapDomain + sizeof(wrapDomain));
        const auto encoded = hex("101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f"
            "303132333435363738393a3b3c3d3e3f010203041020304042e3b10bd1fdb72f963710d6053fc845"
            "993914c5e39e1379987843dff3cf24fa");
        aad.insert(aad.end(), encoded.begin(), encoded.end());
    }
};

// Gateway tests exercise actual GCM with the same vector. This adapter pins
// exact platform inputs and can model a faulty provider without SDK deps.
class FixtureGcm final : public IX25519AesGcmCryptoProvider {
public:
    enum class Mode { Normal, WrongKey, ShortWrite };
    explicit FixtureGcm(const Fixture& fixture) : fixture_(fixture), calls(0U), mode(Mode::Normal) {}

    Result x25519PublicKey(ByteView, MutableByteSpan) override { return unsupported(); }
    Result x25519(ByteView, ByteView, MutableByteSpan) override { return unsupported(); }
    Result aes256GcmEncrypt(ByteView, ByteView, ByteView, ByteView, MutableByteSpan, size_t&) override {
        return unsupported();
    }

    Result aes256GcmDecrypt(ByteView key, ByteView iv, ByteView aad, ByteView box,
        MutableByteSpan output, size_t& written) override {
        ++calls;
        CHECK(output.size == kDeviceKeySize && output.data != nullptr);
        std::memcpy(output.data, fixture_.newKey.bytes, output.size); // Deliberate partial leak on failure.
        written = output.size;
        if (!same(key, view(fixture_.wrapKey)) || !same(iv, view(fixture_.iv)) ||
            !same(aad, view(fixture_.aad)) || !same(box, view(fixture_.box)))
            return Result::failure(ErrorCode::AuthenticationRequired);
        if (mode == Mode::WrongKey) output.data[0] ^= 0x80U;
        if (mode == Mode::ShortWrite) --written;
        return Result::success();
    }

private:
    static Result unsupported() { return Result::failure(ErrorCode::UnsupportedFeature); }
    const Fixture& fixture_;

public:
    size_t calls;
    Mode mode;
};

void checkVectorAndProofBinding(const Fixture& fixture) {
    WifiprovRekeyMac proof;
    WifiprovRekeyMac receipt;
    CHECK(createWifiprovRekeyChallengeProof(fixture.oldKey, fixture.binding.previous, proof));
    CHECK(same(ByteView(proof.bytes, sizeof(proof.bytes)), view(fixture.proof)));
    CHECK(createWifiprovRekeyInstalledReceipt(fixture.newKey, fixture.binding, receipt));
    CHECK(same(ByteView(receipt.bytes, sizeof(receipt.bytes)), view(fixture.receipt)));

    std::array<WifiprovRekeyBinding, 5U> changed = {{ fixture.binding, fixture.binding,
        fixture.binding, fixture.binding, fixture.binding }};
    changed[0].previous.operationId[0] ^= 0x80U;
    changed[1].previous.challenge[0] ^= 0x80U;
    changed[2].previous.instance.bytes[0] ^= 0x80U;
    ++changed[3].previous.accessEpoch;
    ++changed[4].previous.fromVersion;
    for (const auto& binding : changed) {
        CHECK(createWifiprovRekeyChallengeProof(fixture.oldKey, binding.previous, proof));
        CHECK(!same(ByteView(proof.bytes, sizeof(proof.bytes)), view(fixture.proof)));
        CHECK(createWifiprovRekeyInstalledReceipt(fixture.newKey, binding, receipt));
        CHECK(!same(ByteView(receipt.bytes, sizeof(receipt.bytes)), view(fixture.receipt)));
    }

    const WifiprovRekeyMac zero;
    CHECK(createWifiprovRekeyChallengeProof(fixture.newKey, fixture.binding.previous, proof).code()
        == ErrorCode::AuthenticationRequired);
    CHECK(same(ByteView(proof.bytes, sizeof(proof.bytes)), ByteView(zero.bytes, sizeof(zero.bytes))));
    CHECK(createWifiprovRekeyInstalledReceipt(fixture.oldKey, fixture.binding, receipt).code()
        == ErrorCode::AuthenticationRequired);
    CHECK(same(ByteView(receipt.bytes, sizeof(receipt.bytes)), ByteView(zero.bytes, sizeof(zero.bytes))));
    auto invalid = fixture.binding.previous;
    invalid.fromVersion = UINT32_MAX;
    CHECK(createWifiprovRekeyChallengeProof(fixture.oldKey, invalid, proof).code() == ErrorCode::InvalidArgument);
    invalid = fixture.binding.previous;
    std::memset(invalid.challenge, 0, sizeof(invalid.challenge));
    CHECK(createWifiprovRekeyChallengeProof(fixture.oldKey, invalid, proof).code() == ErrorCode::InvalidArgument);
}

void checkOpen(const Fixture& fixture) {
    FixtureGcm crypto(fixture);
    DeviceKey candidate;
    CHECK(openWifiprovRekeyCandidate(fixture.oldKey, fixture.binding, view(fixture.iv), view(fixture.box), crypto, candidate));
    CHECK(sameDeviceKey(candidate, fixture.newKey));
    CHECK(crypto.calls == 1U);

    const auto expectRejected = [&](const WifiprovRekeyBinding& binding, ByteView iv, ByteView box, ErrorCode code) {
        candidate = fixture.newKey;
        CHECK(openWifiprovRekeyCandidate(fixture.oldKey, binding, iv, box, crypto, candidate).code() == code);
        const DeviceKey empty;
        CHECK(same(ByteView(candidate.bytes, sizeof(candidate.bytes)), ByteView(empty.bytes, sizeof(empty.bytes))));
    };
    const ByteView iv = view(fixture.iv), box = view(fixture.box);
    auto altered = fixture.binding;
    altered.newLocator.bytes[0] ^= 0x80U;
    expectRejected(altered, iv, box, ErrorCode::AuthenticationRequired);
    altered = fixture.binding;
    altered.previous.operationId[0] ^= 0x80U;
    expectRejected(altered, iv, box, ErrorCode::AuthenticationRequired);
    auto tampered = fixture.iv;
    tampered[0] ^= 0x80U;
    expectRejected(fixture.binding, view(tampered), box, ErrorCode::AuthenticationRequired);
    tampered = fixture.box;
    tampered.front() ^= 0x80U;
    expectRejected(fixture.binding, iv, view(tampered), ErrorCode::AuthenticationRequired);
    tampered = fixture.box;
    tampered.back() ^= 0x80U;
    expectRejected(fixture.binding, iv, view(tampered), ErrorCode::AuthenticationRequired);
    crypto.mode = FixtureGcm::Mode::WrongKey;
    expectRejected(fixture.binding, iv, box, ErrorCode::AuthenticationRequired);
    crypto.mode = FixtureGcm::Mode::ShortWrite;
    expectRejected(fixture.binding, iv, box, ErrorCode::InvalidEncoding);
    CHECK(crypto.calls == 8U);

    altered = fixture.binding;
    altered.newLocator = altered.previous.oldLocator;
    expectRejected(altered, iv, box, ErrorCode::InvalidArgument);
    altered = fixture.binding;
    altered.previous.oldLocator.bytes[0] ^= 0x80U;
    expectRejected(altered, iv, box, ErrorCode::AuthenticationRequired);
    expectRejected(fixture.binding, iv, box.subspan(0U, box.size - 1U), ErrorCode::InvalidArgument);
    const uint8_t zeroIv[kAesGcmNonceSize] = {};
    expectRejected(fixture.binding, ByteView(zeroIv, sizeof(zeroIv)), box, ErrorCode::InvalidArgument);
    CHECK(crypto.calls == 8U);

    DeviceKey aliased = fixture.oldKey;
    CHECK(openWifiprovRekeyCandidate(aliased, fixture.binding, iv, box, crypto, aliased).code() == ErrorCode::InvalidArgument);
    CHECK(sameDeviceKey(aliased, fixture.oldKey));
}

class RootWriter final : public IAtomicBlobStore {
public:
    enum class Mode { Normal, UnknownUncommitted, UnknownCommitted };
    std::vector<uint8_t> live;
    std::vector<uint8_t> durable;
    Mode mode = Mode::Normal;
    const bool* reserved = nullptr;
    size_t writes = 0U;

    Result load(MutableByteSpan output, size_t& written) override {
        written = 0U;
        if (live.empty()) return Result::failure(ErrorCode::NotFound);
        CHECK(output.data != nullptr && output.size >= live.size());
        std::memcpy(output.data, live.data(), live.size());
        written = live.size();
        return Result::success();
    }
    Result replace(ByteView input) override {
        CHECK(reserved == nullptr || *reserved);
        ++writes;
        live.assign(input.data, input.data + input.size);
        if (mode != Mode::UnknownUncommitted) durable = live;
        return mode == Mode::Normal ? Result::success() : Result::failure(ErrorCode::InternalError);
    }
    Result clear() override { return Result::failure(ErrorCode::UnsupportedFeature); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};

class DurableReader final : public IAtomicBlobStore {
public:
    explicit DurableReader(const RootWriter& root) : root_(root) {}
    Result load(MutableByteSpan output, size_t& written) override {
        const auto& bytes = root_.durable;
        CHECK(output.data != nullptr && output.size >= bytes.size());
        std::memcpy(output.data, bytes.data(), bytes.size());
        written = bytes.size();
        return Result::success();
    }
    Result replace(ByteView) override { CHECK(false); return Result::failure(ErrorCode::UnsupportedFeature); }
    Result clear() override { CHECK(false); return Result::failure(ErrorCode::UnsupportedFeature); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }

private:
    const RootWriter& root_;
};

// Simulated guard preserves its journal independently of the endpoint object
// and distinguishes the visible write-handle state from the durable root.
class ExactGuard final : public IWifiprovRekeyOwnerGuard {
public:
    enum class Readback { Durable, Missing, SameHandle };

    ExactGuard(const Fixture& fixture, RootWriter& root, TaskRootOwnerGate& fence)
        : fixture_(fixture), root_(root), fence_(fence), reader_(root) {
        fill(admin.controllerId, sizeof(admin.controllerId), 0x71U);
        admin.version = 5U;
    }

    Result begin(WifiprovRekeyOperation operation, const WifiprovRekeyOldBinding& supplied,
        const DeviceKeyLocator* candidate, WifiprovRekeyAdmin& output) override {
        CHECK(!held);
        ++begins;
        const auto& expected = fixture_.binding.previous;
        if (!authorized || (reserved && operation != WifiprovRekeyOperation::ReadReceipt) ||
            std::memcmp(supplied.operationId, expected.operationId, sizeof(expected.operationId)) != 0 ||
            std::memcmp(supplied.challenge, expected.challenge, sizeof(expected.challenge)) != 0 ||
            !sameDeviceInstanceId(supplied.instance, expected.instance) ||
            supplied.accessEpoch != expected.accessEpoch || supplied.fromVersion != expected.fromVersion ||
            std::memcmp(supplied.oldLocator.bytes, expected.oldLocator.bytes, sizeof(expected.oldLocator.bytes)) != 0)
            return deny();
        if (operation != WifiprovRekeyOperation::Challenge && (candidate == nullptr ||
            std::memcmp(candidate->bytes, fixture_.binding.newLocator.bytes, sizeof(candidate->bytes)) != 0))
            return deny();
        held = true;
        output = admin;
        return Result::success();
    }

    Result reserveAttempt() override {
        CHECK(held && fence_.idle());
        ++reserves;
        if (reserved) return Result::failure(ErrorCode::StateConflict);
        reserved = true;
        return reserveResult;
    }
    Result validateWifiprovRekeyCommit() override {
        CHECK(held && reserved && !fence_.idle());
        ++commits;
        return stillLive ? Result::success() : deny();
    }
    Result openDurableReadback(IAtomicBlobStore*& storage) override {
        CHECK(held);
        ++reads;
        if (!reserved || readback == Readback::Missing) return Result::failure(ErrorCode::NotConfigured);
        storage = readback == Readback::SameHandle ? static_cast<IAtomicBlobStore*>(&root_) : &reader_;
        return Result::success();
    }
    void end() override { CHECK(held); held = false; ++ends; }

    WifiprovRekeyAdmin admin;
    bool authorized = true;
    bool stillLive = true;
    bool held = false;
    bool reserved = false;
    size_t begins = 0U;
    size_t ends = 0U;
    size_t reserves = 0U;
    size_t commits = 0U;
    size_t reads = 0U;
    Readback readback = Readback::Durable;
    Result reserveResult;

private:
    static Result deny() { return Result::failure(ErrorCode::AuthenticationRequired); }
    const Fixture& fixture_;
    RootWriter& root_;
    TaskRootOwnerGate& fence_;
    DurableReader reader_;
};

std::vector<uint8_t> request(const Fixture& fixture, WifiprovRekeyOperation operation) {
    const std::vector<uint8_t>::const_iterator binding = fixture.aad.cend() - kWifiprovRekeyBindingSize;
    const size_t size = operation == WifiprovRekeyOperation::Challenge ? kWifiprovRekeyOldBindingSize : kWifiprovRekeyBindingSize;
    std::vector<uint8_t> wire{2U, static_cast<uint8_t>(operation)};
    wire.insert(wire.end(), binding, binding + size);
    if (operation == WifiprovRekeyOperation::Install) {
        wire.insert(wire.end(), fixture.iv.begin(), fixture.iv.end());
        wire.insert(wire.end(), fixture.box.begin(), fixture.box.end());
    }
    return wire;
}

struct EndpointFixture {
    explicit EndpointFixture(const Fixture& fixture)
        : access(root), guard(fixture, root, fence), crypto(fixture),
          endpoint(fixture.binding.previous.instance, access, crypto, &guard) {
        ControllerCredential admin;
        std::memcpy(admin.controllerId, guard.admin.controllerId, sizeof(admin.controllerId));
        fill(admin.secret, sizeof(admin.secret), 0xC1U);
        admin.domain = ControllerCredentialDomain::PlatformAccess;
        admin.ownershipGeneration = fixture.binding.previous.accessEpoch;
        admin.credentialVersion = guard.admin.version;
        admin.permissions = kControllerPermissionAll;
        CHECK(access.bootstrap(fixture.oldKey, admin.ownershipGeneration, admin));
        root.reserved = &guard.reserved;
        access.setChangeObserver(&fence);
    }

    RootWriter root;
    DeviceAccessStore access;
    TaskRootOwnerGate fence;
    ExactGuard guard;
    FixtureGcm crypto;
    WifiprovRekeyEndpoint endpoint;
};

void checkWire(WifiprovRekeyEndpoint& endpoint, const std::vector<uint8_t>& wire, uint8_t status,
    const std::vector<uint8_t>* proof = nullptr) {
    uint8_t response[kWifiprovRekeySuccessResponseSize] = {};
    size_t written = 0U;
    CHECK(endpoint.handle(view(wire), MutableByteSpan(response, sizeof(response)), written));
    CHECK(response[0] == 2U && response[1] == wire[1] && response[2] == status);
    CHECK(written == (proof == nullptr ? 3U : sizeof(response)));
    if (proof != nullptr) {
        CHECK(std::memcmp(response + 3U, wire.data() + 2U, kWifiprovRekeyOperationSize) == 0);
        CHECK(same(ByteView(response + 3U + kWifiprovRekeyOperationSize, kWifiprovRekeyMacSize), view(*proof)));
    }
}

void checkSetupAndWifiMac(const Fixture& fixture) {
    const auto permit = hex("34178a08ee6abc452d3266b07a6a479fd997b43c2e97adaad0340eb21c951d7d");
    CHECK(verifyWifiprovRekeyWifiPermit(fixture.newKey, fixture.binding, view(permit)));
    CHECK(!verifyWifiprovRekeyWifiPermit(fixture.oldKey, fixture.binding, view(permit)));
    CHECK(!verifyWifiprovRekeyWifiPermit(fixture.newKey, fixture.binding, view(fixture.receipt)));
    auto changed = fixture.binding;
    ++changed.previous.fromVersion;
    CHECK(!verifyWifiprovRekeyWifiPermit(fixture.newKey, changed, view(permit)));

    ControllerCredential admin;
    fill(admin.controllerId, sizeof(admin.controllerId), 0x71U);
    fill(admin.secret, sizeof(admin.secret), 0xC1U);
    admin.domain = ControllerCredentialDomain::PlatformAccess;
    admin.ownershipGeneration = fixture.binding.previous.accessEpoch;
    admin.credentialVersion = 5U;
    admin.permissions = kControllerPermissionAll;
    const auto proof = hex("205101e3aa3ac62024e6302fef7fe3398d882b916a23e2f56aa4486b6c18895d");
    std::vector<uint8_t> nonce(kWifiprovRekeyOwnerNonceSize);
    fill(nonce.data(), nonce.size(), 0xA1U);
    CHECK(verifyWifiprovRekeySetupAdminProof(admin, fixture.binding.previous, view(nonce), view(proof)));
    CHECK(!verifyWifiprovRekeySetupAdminProof(admin, changed.previous, view(nonce), view(proof)));
    CHECK(!verifyWifiprovRekeySetupAdminProof(admin, fixture.binding.previous, view(nonce), view(permit)));
    nonce[0] ^= 0x80U;
    CHECK(!verifyWifiprovRekeySetupAdminProof(admin, fixture.binding.previous, view(nonce), view(proof)));
    nonce[0] ^= 0x80U;
    ++admin.credentialVersion;
    CHECK(!verifyWifiprovRekeySetupAdminProof(admin, fixture.binding.previous, view(nonce), view(proof)));
    --admin.credentialVersion;
    admin.permissions = kControllerPermissionObserve | kControllerPermissionControl;
    CHECK(!verifyWifiprovRekeySetupAdminProof(admin, fixture.binding.previous, view(nonce), view(proof)));
    clearControllerCredential(admin);
}

void checkEndpointSingleAttempt(const Fixture& fixture) {
    EndpointFixture test(fixture);
    const auto challenge = request(fixture, WifiprovRekeyOperation::Challenge);
    const auto install = request(fixture, WifiprovRekeyOperation::Install);
    const auto read = request(fixture, WifiprovRekeyOperation::ReadReceipt);
    WifiprovRekeyEndpoint disabled(fixture.binding.previous.instance, test.access, test.crypto);
    checkWire(disabled, challenge, 8U);
    checkWire(disabled, install, 8U);
    checkWire(disabled, read, 8U);
    checkWire(test.endpoint, {2U, 2U}, 3U); // Legacy Install not interpreted here.
    auto invalid = install;
    invalid.push_back(0U);
    checkWire(test.endpoint, invalid, 1U);
    invalid = challenge;
    invalid[34] ^= 0x80U; // Actual physical instance is checked before guard.
    checkWire(test.endpoint, invalid, 8U);
    uint8_t shortResponse[50] = {};
    size_t written = 42U;
    CHECK(test.endpoint.handle(view(install), MutableByteSpan(shortResponse, sizeof(shortResponse)), written).code()
        == ErrorCode::BufferTooSmall);
    CHECK(written == 0U && test.guard.begins == 0U && test.root.writes == 1U);

    test.guard.authorized = false;
    checkWire(test.endpoint, challenge, 8U);
    test.guard.authorized = true;
    invalid = challenge;
    invalid[57] ^= 1U; // Same public instance, wrong version for guarded intent.
    checkWire(test.endpoint, invalid, 8U);
    checkWire(test.endpoint, challenge, 0U, &fixture.proof);
    checkWire(test.endpoint, read, kWifiprovRekeyOutcomeUnknown); // No durable attempt yet.
    invalid = install;
    invalid.back() ^= 1U;
    checkWire(test.endpoint, invalid, 4U);
    CHECK(test.guard.reserves == 0U && test.guard.commits == 0U && test.root.writes == 1U);
    checkWire(test.endpoint, install, 0U, &fixture.receipt);
    CHECK(test.guard.reserves == 1U && test.guard.commits == 1U && test.root.writes == 2U);
    checkWire(test.endpoint, install, 6U);
    invalid = challenge;
    invalid[2] ^= 0x80U;
    checkWire(test.endpoint, invalid, 6U);
    invalid = read;
    invalid[2] ^= 0x80U;
    checkWire(test.endpoint, invalid, 8U);
    checkWire(test.endpoint, read, 0U, &fixture.receipt);
    CHECK(test.guard.reserves == 1U && test.guard.commits == 1U && test.root.writes == 2U && !test.guard.held);
}

void checkEndpointUnknownReadback(const Fixture& fixture) {
    const auto install = request(fixture, WifiprovRekeyOperation::Install);
    const auto read = request(fixture, WifiprovRekeyOperation::ReadReceipt);
    for (const auto mode : {RootWriter::Mode::UnknownCommitted, RootWriter::Mode::UnknownUncommitted}) {
        EndpointFixture test(fixture);
        test.root.mode = mode;
        checkWire(test.endpoint, install, kWifiprovRekeyOutcomeUnknown);
        CHECK(test.guard.reserved && test.guard.commits == 1U && test.guard.reads == 0U && test.fence.uncertain());
        DeviceKey visible;
        CHECK(test.access.load(visible) && sameDeviceKey(visible, fixture.newKey));
        test.guard.readback = ExactGuard::Readback::SameHandle;
        checkWire(test.endpoint, read, kWifiprovRekeyOutcomeUnknown);
        test.guard.readback = ExactGuard::Readback::Durable;
        WifiprovRekeyEndpoint reentered(fixture.binding.previous.instance, test.access, test.crypto, &test.guard);
        checkWire(reentered, install, 8U); // Guard still owns the durable attempt after endpoint recreation.
        auto different = install;
        different[2] ^= 0x80U;
        checkWire(reentered, different, 8U);
        const bool committed = mode == RootWriter::Mode::UnknownCommitted;
        checkWire(reentered, read, committed ? 0U : kWifiprovRekeyOutcomeUnknown, committed ? &fixture.receipt : nullptr);
        CHECK(test.root.writes == 2U && test.guard.reserves == 1U && test.guard.commits == 1U);
    }

    EndpointFixture expired(fixture);
    expired.guard.stillLive = false;
    checkWire(expired.endpoint, install, kWifiprovRekeyOutcomeUnknown);
    CHECK(expired.guard.reserved && expired.guard.commits == 1U && expired.root.writes == 1U && !expired.fence.uncertain());
    checkWire(expired.endpoint, read, kWifiprovRekeyOutcomeUnknown);
    checkWire(expired.endpoint, install, 6U);

    EndpointFixture reserveFailed(fixture);
    reserveFailed.guard.reserveResult = Result::failure(ErrorCode::InternalError);
    checkWire(reserveFailed.endpoint, install, kWifiprovRekeyOutcomeUnknown);
    CHECK(reserveFailed.root.writes == 1U && reserveFailed.guard.commits == 0U);
    checkWire(reserveFailed.endpoint, install, 6U);

    EndpointFixture readbackMissing(fixture);
    readbackMissing.guard.readback = ExactGuard::Readback::Missing;
    checkWire(readbackMissing.endpoint, install, kWifiprovRekeyOutcomeUnknown);
    CHECK(readbackMissing.root.writes == 2U);
    readbackMissing.guard.readback = ExactGuard::Readback::Durable;
    checkWire(readbackMissing.endpoint, read, 0U, &fixture.receipt);
    CHECK(readbackMissing.root.writes == 2U);
}

void checkDurableAttemptJournal(const Fixture& fixture) {
    WifiprovRekeyAdmin admin;
    fill(admin.controllerId, sizeof(admin.controllerId), 0x71U);
    admin.version = 5U;
    RootWriter journalStorage;
    WifiprovRekeyJournal journal(journalStorage);
    WifiprovRekeyJournalPhase phase = WifiprovRekeyJournalPhase::ApplyConsumed;
    CHECK(journal.inspect(fixture.binding, admin, phase).code() == ErrorCode::NotFound);
    CHECK(journal.consumeApply(fixture.binding, admin).code() == ErrorCode::NotFound);
    CHECK(journal.reserveAttempt(fixture.binding, admin));
    CHECK(journalStorage.writes == 1U && journalStorage.durable.size() == WifiprovRekeyJournal::serializedSize);
    CHECK(journal.inspect(fixture.binding, admin, phase) && phase == WifiprovRekeyJournalPhase::AttemptReserved);
    CHECK(journal.complete(fixture.binding, admin, 2U).code() == ErrorCode::StateConflict);
    auto other = fixture.binding;
    other.previous.operationId[0] ^= 1U;
    CHECK(journal.reserveAttempt(fixture.binding, admin).code() == ErrorCode::StateConflict);
    CHECK(journal.reserveAttempt(other, admin).code() == ErrorCode::StateConflict);
    CHECK(journal.consumeApply(other, admin).code() == ErrorCode::StateConflict);
    ++admin.version;
    CHECK(journal.consumeApply(fixture.binding, admin).code() == ErrorCode::StateConflict);
    --admin.version;
    CHECK(journal.consumeApply(fixture.binding, admin));
    CHECK(journal.inspect(fixture.binding, admin, phase) && phase == WifiprovRekeyJournalPhase::ApplyConsumed);
    CHECK(journal.consumeApply(fixture.binding, admin).code() == ErrorCode::StateConflict);
    CHECK(journalStorage.writes == 2U);
    WifiprovRekeyJournalRecord record;
    CHECK(journal.read(record) && record.phase == WifiprovRekeyJournalPhase::ApplyConsumed && record.networkGeneration == 0U);
    CHECK(journal.complete(fixture.binding, admin, 0U).code() == ErrorCode::InvalidArgument);
    CHECK(journal.complete(other, admin, 2U).code() == ErrorCode::StateConflict);
    CHECK(journal.complete(fixture.binding, admin, 2U));
    CHECK(journal.inspect(fixture.binding, admin, phase) && phase == WifiprovRekeyJournalPhase::Complete);
    CHECK(journal.read(record) && record.phase == WifiprovRekeyJournalPhase::Complete && record.networkGeneration == 2U);
    CHECK(journal.complete(fixture.binding, admin, 3U).code() == ErrorCode::StateConflict);
    CHECK(journal.reserveAttempt(other, admin).code() == ErrorCode::StateConflict && journalStorage.writes == 3U);

    RootWriter failed;
    failed.mode = RootWriter::Mode::UnknownCommitted;
    WifiprovRekeyJournal uncertain(failed);
    CHECK(!uncertain.reserveAttempt(fixture.binding, admin));
    CHECK(uncertain.inspect(fixture.binding, admin, phase).code() == ErrorCode::StateConflict);
    CHECK(uncertain.reserveAttempt(other, admin).code() == ErrorCode::StateConflict);
    CHECK(failed.writes == 1U);
    RootWriter reopened;
    reopened.live = failed.durable;
    reopened.durable = failed.durable;
    WifiprovRekeyJournal cold(reopened);
    CHECK(cold.inspect(fixture.binding, admin, phase) && phase == WifiprovRekeyJournalPhase::AttemptReserved);
    CHECK(cold.reserveAttempt(fixture.binding, admin).code() == ErrorCode::StateConflict);
    reopened.live.back() ^= 1U;
    CHECK(cold.consumeApply(fixture.binding, admin).code() == ErrorCode::InvalidEncoding);
    CHECK(reopened.writes == 0U);
}

class FixtureRandom final : public IRandom {
public:
    Result fill(MutableByteSpan output) override {
        CHECK(output.data != nullptr);
        for (size_t i = 0U; i < output.size; ++i) output.data[i] = static_cast<uint8_t>(next + i);
        ++next;
        return Result::success();
    }
    uint8_t next = 0xA1U;
};

void checkNativeSetupAndPermit(const Fixture& fixture, bool reconnect) {
    using Kind = Esp32WiFiProvRekeyEndpointKind;
    EndpointFixture test(fixture);
    RootWriter journalStorage;
    WifiprovRekeyJournal journal(journalStorage);
    DurableReader durableRoot(test.root);
    Esp32WiFiProvSecurity1Owner nativeOwner;
    FixtureRandom random;
    Esp32WiFiProvRekeyAdmin authorization(nativeOwner, fixture.binding.previous.instance, test.access, random);
    Esp32WiFiProvRekeyApply apply(nativeOwner, authorization, test.access, durableRoot, journal);
    int handle = 0;
    constexpr uint32_t session = 1U;
    const uint8_t popByte = 1U;
    const protocomm_security1_params_t pop{&popByte, 1U};
    uint8_t response[Esp32WiFiProvRekeyAdmin::successResponseSize] = {};
    size_t written = 0U;
    const auto output = MutableByteSpan(response, sizeof(response));
    const std::vector<uint8_t> nonceRequest{2U, 8U};
    CHECK(authorization.handle(session, view(nonceRequest), output, written) && written == 3U && response[2] == 8U);
    nativeOwner.arm();
    nativeOwner.opened(&handle, session);
    nativeOwner.handshake(&handle, session, &pop);
    CHECK(nativeOwner.beginRequest(Kind::BlinkerConfig, session));
    nativeOwner.decrypted(&handle, session);
    CHECK(authorization.handle(session, view(nonceRequest), output, written) && written == 19U && response[2] == 0U);
    auto auth = request(fixture, WifiprovRekeyOperation::Challenge);
    auth[1] = 9U;
    auth.insert(auth.end(), test.guard.admin.controllerId, test.guard.admin.controllerId + kControllerIdSize);
    auth.insert(auth.end(), {0U, 0U, 0U, 5U});
    auth.insert(auth.end(), response + 3U, response + sizeof(response));
    const auto authMac = hex("205101e3aa3ac62024e6302fef7fe3398d882b916a23e2f56aa4486b6c18895d");
    auth.insert(auth.end(), authMac.begin(), authMac.end());
    CHECK(!apply.beginInstall(session, fixture.binding)); // Nonce alone is not authorization.
    auth.back() ^= 1U;
    CHECK(authorization.handle(session, view(auth), output, written) && written == 3U && response[2] == 8U);
    auth.back() ^= 1U;
    CHECK(authorization.handle(session, view(auth), output, written) && written == 19U && response[2] == 0U);
    CHECK(authorization.handle(session, view(auth), output, written) && written == 19U && response[2] == 0U);
    CHECK(std::memcmp(response + 3U, fixture.binding.previous.operationId, kWifiprovRekeyOperationSize) == 0);

    CHECK(apply.beginInstall(session, fixture.binding));
    CHECK(journal.reserveAttempt(fixture.binding, test.guard.admin));
    const auto install = request(fixture, WifiprovRekeyOperation::Install);
    checkWire(test.endpoint, install, 0U, &fixture.receipt);
    apply.finishInstall(session, true);

    auto permit = request(fixture, WifiprovRekeyOperation::ReadReceipt);
    permit[1] = 7U;
    const auto permitMac = hex("34178a08ee6abc452d3266b07a6a479fd997b43c2e97adaad0340eb21c951d7d");
    permit.insert(permit.end(), permitMac.begin(), permitMac.end());
    permit.back() ^= 1U;
    CHECK(apply.handle(session, view(permit), output, written) && written == 3U && response[2] == 8U);
    permit.back() ^= 1U;
    CHECK(apply.handle(session, view(permit), output, written) && written == 19U && response[2] == 0U);
    CHECK(apply.handle(session, view(permit), output, written) && written == 3U && response[2] == 8U);
    nativeOwner.endRequest();
    if (reconnect) {
        nativeOwner.closed(&handle, session);
        nativeOwner.opened(&handle, session);
        nativeOwner.handshake(&handle, session, &pop);
    }
    CHECK(nativeOwner.beginRequest(Kind::SdkConfig, session));
    nativeOwner.decrypted(&handle, session);
    CHECK(apply.allowStatus(session));
    CHECK(!apply.beginApply(session));
    CHECK(apply.beginSet(session) != reconnect);
    CHECK(!apply.beginSet(session));
    apply.finishSet(session, true);
    CHECK(apply.beginApply(session) != reconnect);
    CHECK(!apply.beginApply(session));
    nativeOwner.endRequest();
    CHECK(journalStorage.writes == (reconnect ? 1U : 2U));
}

} // namespace

int main() {
    const Fixture fixture;
    checkVectorAndProofBinding(fixture);
    checkOpen(fixture);
    checkEndpointSingleAttempt(fixture);
    checkEndpointUnknownReadback(fixture);
    checkSetupAndWifiMac(fixture);
    checkDurableAttemptJournal(fixture);
    checkNativeSetupAndPermit(fixture, false);
    checkNativeSetupAndPermit(fixture, true);
    std::cout << "WiFiProv rekey proof tests passed\n";
}
