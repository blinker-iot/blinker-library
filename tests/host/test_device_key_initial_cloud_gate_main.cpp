#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/identity/DeviceKeyStore.h>
#include <BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.h>
#include <BlinkerV2/runtime/TaskRootOwnerGate.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

// Only the atomic persistence boundary is substituted. Root decoding, Admin,
// Key mutation and the provisioning wire all use the actual production code.
class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    MemoryBlob() : bytes(), size(0U), present(false), loads(0U), writes(0U),
                   failLoad(0U), failAfterWrite(false) {}
    blinker::Result load(blinker::MutableByteSpan output, size_t& written) override {
        written = 0U;
        ++loads;
        if (loads == failLoad) return blinker::Result::failure(blinker::ErrorCode::InvalidEncoding);
        if (!present) return blinker::Result::failure(blinker::ErrorCode::NotFound);
        if (output.data == nullptr || output.size < size) return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView input) override {
        if (input.data == nullptr || input.size > sizeof(bytes)) return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
        ++writes;
        std::memcpy(bytes, input.data, input.size);
        size = input.size;
        present = true;
        // Models an uncertain commit whose same handle exposes candidate bytes.
        // Reading them is not evidence that the commit became durable.
        if (failAfterWrite) {
            failAfterWrite = false;
            return blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
        return blinker::Result::success();
    }
    blinker::Result clear() override {
        ++writes;
        std::memset(bytes, 0, sizeof(bytes));
        present = false;
        size = 0U;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override { return blinker::StorageProtection::PlainFlash; }

    uint8_t bytes[blinker::DeviceAccessStore::serializedSize];
    size_t size;
    bool present;
    uint32_t loads;
    uint32_t writes;
    uint32_t failLoad;
    bool failAfterWrite;
};

const char first[] = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
const char second[] = "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA";

blinker::DeviceInstanceId instance() {
    blinker::DeviceInstanceId value;
    std::memset(value.bytes, 0x44, sizeof(value.bytes));
    return value;
}

blinker::ControllerCredential admin() {
    blinker::ControllerCredential value;
    value.domain = blinker::ControllerCredentialDomain::PlatformAccess;
    value.ownershipGeneration = 7U;
    value.credentialVersion = 1U;
    value.permissions = blinker::kControllerPermissionAll;
    std::memset(value.controllerId, 0x20, sizeof(value.controllerId));
    std::memset(value.secret, 0x60, sizeof(value.secret));
    return value;
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

void installRequest(const char* key, uint8_t* output) {
    output[0] = blinker::kDeviceKeyProvisioningVersion;
    output[1] = static_cast<uint8_t>(blinker::DeviceKeyProvisioningOperation::Install);
    std::memcpy(output + 2U, key, blinker::kDeviceKeyTextSize);
}

void bootstrapRequest(uint8_t* output) {
    output[0] = blinker::kDeviceKeyProvisioningVersion;
    output[1] = static_cast<uint8_t>(blinker::DeviceKeyProvisioningOperation::Bootstrap);
    std::memcpy(output + 2U, first, blinker::kDeviceKeyTextSize);
    size_t offset = 2U + blinker::kDeviceKeyTextSize;
    const blinker::ControllerCredential value = admin();
    writeU32(output + offset, value.ownershipGeneration);
    offset += 4U;
    std::memcpy(output + offset, value.controllerId, blinker::kControllerIdSize);
    offset += blinker::kControllerIdSize;
    writeU32(output + offset, value.credentialVersion);
    offset += 4U;
    std::memcpy(output + offset, value.secret, blinker::kControllerCredentialSecretSize);
}

template <size_t Size>
blinker::DeviceKeyProvisioningStatus send(blinker::DeviceKeyProvisioningEndpoint& endpoint,
                                          const uint8_t (&request)[Size]) {
    uint8_t response[blinker::kDeviceKeyProvisioningMaxResponseSize] = {};
    size_t written = 0U;
    check(endpoint.handle(blinker::ByteView(request, Size),
          blinker::MutableByteSpan(response, sizeof(response)), written).ok(), "endpoint transport failure");
    check(written == blinker::kDeviceKeyProvisioningStatusResponseSize, "unexpected status size");
    check(response[0] == blinker::kDeviceKeyProvisioningVersion && response[1] == request[1], "unrelated response");
    return static_cast<blinker::DeviceKeyProvisioningStatus>(response[2]);
}

void testAdminOnlyLegacyInstallIsReadOnlyRejected() {
    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    check(access.bootstrapAccess(7U, admin()).ok(), "admin fixture bootstrap failed");
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
    uint8_t before[sizeof(memory.bytes)] = {};
    std::memcpy(before, memory.bytes, sizeof(before));
    const uint32_t writes = memory.writes;
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(first, request);
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::AccessRequired,
          "legacy Install did not report AccessRequired for an Admin root without Key");
    check(!endpoint.installed() && memory.writes == writes &&
          std::memcmp(before, memory.bytes, sizeof(before)) == 0, "first-Key rejection changed root");
    blinker::DeviceKey missing;
    check(access.load(missing).code() == blinker::ErrorCode::NotFound, "rejected first Key appeared installed");
}

void testInstalledFlagCannotAdmitKeyAfterClear() {
    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
    uint8_t bootstrap[blinker::kDeviceKeyProvisioningBootstrapRequestSize] = {};
    bootstrapRequest(bootstrap);
    check(send(endpoint, bootstrap) == blinker::DeviceKeyProvisioningStatus::Success && endpoint.installed(), "bootstrap fixture failed");
    check(access.clear().ok(), "clear fixture failed");
    const uint32_t writes = memory.writes;
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(first, request);
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::AccessRequired,
          "installed flag bypassed the actual no-Key root guard");
    check(memory.writes == writes, "installed flag guard wrote root");
}

void testRootAndKeyReadFailuresCannotBecomeMissingKeyOrReplaySuccess() {
    for (uint32_t failingRead = 1U; failingRead <= 2U; ++failingRead) {
        MemoryBlob memory;
        blinker::DeviceAccessStore access(memory);
        const blinker::DeviceInstanceId id = instance();
        blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
        uint8_t bootstrap[blinker::kDeviceKeyProvisioningBootstrapRequestSize] = {};
        bootstrapRequest(bootstrap);
        check(send(endpoint, bootstrap) == blinker::DeviceKeyProvisioningStatus::Success, "read-failure fixture failed");
        const uint32_t writes = memory.writes;
        memory.failLoad = memory.loads + failingRead;
        uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
        installRequest(first, request);
        check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::StorageFailure,
              "failed root/Key read was accepted through installed replay");
        check(memory.writes == writes, "read failure changed root");
    }
}

void testShortBufferAndMixedStoreNeverReadOrWrite() {
    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    check(access.bootstrapAccess(7U, admin()).ok(), "short-buffer fixture failed");
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(first, request);
    const uint32_t writes = memory.writes, loads = memory.loads;
    uint8_t response[2U] = {};
    size_t written = 99U;
    check(endpoint.handle(blinker::ByteView(request, sizeof(request)),
          blinker::MutableByteSpan(response, sizeof(response)), written).code() == blinker::ErrorCode::BufferTooSmall,
          "short response buffer was accepted");
    check(written == 0U && memory.writes == writes && memory.loads == loads, "short response touched Store");

    MemoryBlob separate;
    blinker::DeviceKeyStore keys(separate);
    blinker::DeviceKeyProvisioningEndpoint mixed(id, keys, &access);
    check(send(mixed, request) == blinker::DeviceKeyProvisioningStatus::StorageFailure, "mixed Store composition was accepted");
    check(memory.writes == writes && memory.loads == loads && separate.loads == 0U && separate.writes == 0U,
          "mixed Store composition touched persistence");
}

void testPureKeyInstallAndSessionRotationStayCompatible() {
    MemoryBlob memory;
    blinker::DeviceKeyStore keys(memory);
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, keys);
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(first, request);
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::Success, "pure Key first Install changed");
    const uint32_t writes = memory.writes;
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::Success && memory.writes == writes,
          "pure Key exact replay changed");
    installRequest(second, request);
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::Conflict && memory.writes == writes,
          "pure Key same-session conflict changed");
    endpoint.reset();
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::Success && memory.writes == writes + 1U,
          "pure Key explicit new-session rotation changed");
}

void testInstalledExactReplayCannotClearUnknownRootCommit() {
    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    blinker::TaskRootOwnerGate gate;
    access.setChangeObserver(&gate);
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
    uint8_t bootstrap[blinker::kDeviceKeyProvisioningBootstrapRequestSize] = {};
    bootstrapRequest(bootstrap);
    check(send(endpoint, bootstrap) == blinker::DeviceKeyProvisioningStatus::Success && endpoint.installed(),
          "unknown replay fixture Bootstrap failed");
    blinker::DeviceKey changed;
    check(blinker::decodeDeviceKey(blinker::StringView(second, sizeof(second) - 1U), changed).ok(),
          "unknown replay fixture Key failed");
    memory.failAfterWrite = true;
    check(!access.replace(changed), "uncertain root commit appeared successful");
    check(gate.uncertain() && gate.idle(), "unknown root commit did not poison the real owner gate");
    blinker::DeviceKey visible;
    check(access.load(visible).ok() && blinker::sameDeviceKey(changed, visible),
          "uncertain handle did not expose the same candidate view");
    blinker::clearDeviceKey(visible);
    blinker::clearDeviceKey(changed);
    const uint32_t writes = memory.writes;
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(second, request);
    check(send(endpoint, request) == blinker::DeviceKeyProvisioningStatus::StorageFailure,
          "installed exact replay accepted an uncertain same-handle Key view");
    check(memory.writes == writes && gate.uncertain() && gate.idle(),
          "installed exact replay wrote storage or cleared owner uncertainty");
}

void testBootstrapAndExistingCloudKeyRemainCompatible() {
    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    const blinker::DeviceInstanceId id = instance();
    blinker::DeviceKeyProvisioningEndpoint endpoint(id, access, &access);
    uint8_t install[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    installRequest(first, install);
    check(send(endpoint, install) == blinker::DeviceKeyProvisioningStatus::AccessRequired && memory.writes == 0U,
          "empty combined root accepted key-only Install");
    uint8_t bootstrap[blinker::kDeviceKeyProvisioningBootstrapRequestSize] = {};
    bootstrapRequest(bootstrap);
    check(send(endpoint, bootstrap) == blinker::DeviceKeyProvisioningStatus::Success, "empty-root Bootstrap changed");
    const uint32_t writes = memory.writes;
    endpoint.reset();
    check(send(endpoint, bootstrap) == blinker::DeviceKeyProvisioningStatus::Success && memory.writes == writes,
          "exact Bootstrap replay changed");
    installRequest(second, install);
    check(send(endpoint, install) == blinker::DeviceKeyProvisioningStatus::Conflict && memory.writes == writes,
          "combined Key same-session conflict became a rotation");
    endpoint.reset();
    installRequest(first, install);
    check(send(endpoint, install) == blinker::DeviceKeyProvisioningStatus::Success && memory.writes == writes,
          "existing combined Key exact replay changed");
    endpoint.reset();
    installRequest(second, install);
    check(send(endpoint, install) == blinker::DeviceKeyProvisioningStatus::Success && memory.writes == writes + 1U,
          "existing combined Key refresh changed");
    blinker::ControllerCredential loaded;
    const blinker::ControllerCredential expected = admin();
    check(access.loadActive(expected.id(), expected.domain, 7U, loaded).ok() &&
          blinker::sameControllerCredential(expected, loaded), "Key refresh changed existing Admin");
}

} // namespace

int main() {
    testAdminOnlyLegacyInstallIsReadOnlyRejected();
    testInstalledFlagCannotAdmitKeyAfterClear();
    testRootAndKeyReadFailuresCannotBecomeMissingKeyOrReplaySuccess();
    testShortBufferAndMixedStoreNeverReadOrWrite();
    testPureKeyInstallAndSessionRotationStayCompatible();
    testInstalledExactReplayCannotClearUnknownRootCommit();
    testBootstrapAndExistingCloudKeyRemainCompatible();
    std::cout << "Initial Cloud Key legacy endpoint gate: 7 finite groups passed" << std::endl;
    return 0;
}
