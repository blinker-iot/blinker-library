#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.h>

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

class MemoryBlobStore final : public blinker::IAtomicBlobStore {
public:
    MemoryBlobStore()
        : bytes_(), size_(0U), present_(false), failReplace_(false) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present_) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size_) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, bytes_, size_);
        written = size_;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        if (failReplace_) {
            failReplace_ = false;
            return blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
        if (value.data == nullptr || value.size > sizeof(bytes_)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(bytes_, value.data, value.size);
        size_ = value.size;
        present_ = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        std::memset(bytes_, 0, sizeof(bytes_));
        size_ = 0U;
        present_ = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }

    void failNextReplace() { failReplace_ = true; }
    void corrupt() {
        if (present_ && size_ > 8U) bytes_[8U] ^= 1U;
    }

private:
    uint8_t bytes_[blinker::DeviceAccessStore::serializedSize];
    size_t size_;
    bool present_;
    bool failReplace_;
};

class MemoryDeviceKeyStore final : public blinker::IDeviceKeyStore {
public:
    MemoryDeviceKeyStore()
        : key_(), present_(false), failReplace_(false), corrupt_(false) {}

    blinker::Result load(blinker::DeviceKey& output) override {
        if (corrupt_) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidEncoding);
        }
        if (!present_) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = key_;
        return blinker::Result::success();
    }

    blinker::Result replace(const blinker::DeviceKey& key) override {
        if (failReplace_) {
            failReplace_ = false;
            return blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
        const blinker::Result result = blinker::validateDeviceKey(key);
        if (!result) return result;
        key_ = key;
        present_ = true;
        corrupt_ = false;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        blinker::clearDeviceKey(key_);
        present_ = false;
        corrupt_ = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }

    void failNextReplace() { failReplace_ = true; }
    void corrupt() { corrupt_ = true; }

private:
    blinker::DeviceKey key_;
    bool present_;
    bool failReplace_;
    bool corrupt_;
};

uint8_t status(const uint8_t* response, size_t written) {
    check(
        written >= blinker::kDeviceKeyProvisioningStatusResponseSize,
        "provisioning response was truncated");
    return response[2];
}

void buildInstall(
    const char* key,
    uint8_t* request,
    size_t capacity) {
    check(
        capacity >= blinker::kDeviceKeyProvisioningInstallRequestSize,
        "test install request capacity");
    request[0] = blinker::kDeviceKeyProvisioningVersion;
    request[1] = static_cast<uint8_t>(
        blinker::DeviceKeyProvisioningOperation::Install);
    std::memcpy(request + 2U, key, blinker::kDeviceKeyTextSize);
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

void buildBootstrap(
    const char* key,
    uint32_t generation,
    uint8_t controllerBase,
    uint32_t credentialVersion,
    uint8_t secretBase,
    uint8_t* request,
    size_t capacity) {
    check(
        capacity >= blinker::kDeviceKeyProvisioningBootstrapRequestSize,
        "test bootstrap request capacity");
    request[0] = blinker::kDeviceKeyProvisioningVersion;
    request[1] = static_cast<uint8_t>(
        blinker::DeviceKeyProvisioningOperation::Bootstrap);
    std::memcpy(request + 2U, key, blinker::kDeviceKeyTextSize);
    size_t offset = 2U + blinker::kDeviceKeyTextSize;
    writeU32(request + offset, generation);
    offset += 4U;
    for (size_t index = 0U; index < blinker::kControllerIdSize; ++index) {
        request[offset + index] = static_cast<uint8_t>(
            controllerBase + index);
    }
    offset += blinker::kControllerIdSize;
    writeU32(request + offset, credentialVersion);
    offset += 4U;
    for (size_t index = 0U;
         index < blinker::kControllerCredentialSecretSize;
         ++index) {
        request[offset + index] = static_cast<uint8_t>(secretBase + index);
    }
}

void testInfoAndStrictWire() {
    MemoryDeviceKeyStore store;
    blinker::DeviceInstanceId instance;
    for (size_t index = 0U; index < sizeof(instance.bytes); ++index) {
        instance.bytes[index] = static_cast<uint8_t>(0x10U + index);
    }
    blinker::DeviceKeyProvisioningEndpoint endpoint(instance, store);
    uint8_t response[blinker::kDeviceKeyProvisioningMaxResponseSize] = {};
    size_t written = 0U;

    const uint8_t info[] = {
        blinker::kDeviceKeyProvisioningVersion,
        static_cast<uint8_t>(
            blinker::DeviceKeyProvisioningOperation::GetInfo)};
    blinker::Result result = endpoint.handle(
        blinker::ByteView(info, sizeof(info)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(result.ok(), "DeviceKey info failed");
    check(
        written == blinker::kDeviceKeyProvisioningInfoResponseSize &&
            status(response, written) == 0U && response[3] == 0U,
        "empty DeviceKey info mismatch");
    check(
        std::memcmp(response + 4U, instance.bytes, sizeof(instance.bytes)) ==
            0,
        "DeviceKey info changed instance id");

    result = endpoint.handle(
        blinker::ByteView(info, sizeof(info)),
        blinker::MutableByteSpan(response, 2U),
        written);
    check(
        result.code() == blinker::ErrorCode::BufferTooSmall && written == 0U,
        "short DeviceKey response buffer accepted");

    const uint8_t shortRequest[] = {1U};
    result = endpoint.handle(
        blinker::ByteView(shortRequest, sizeof(shortRequest)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::Malformed),
        "malformed DeviceKey request status mismatch");

    const uint8_t wrongVersion[] = {1U, 1U};
    result = endpoint.handle(
        blinker::ByteView(wrongVersion, sizeof(wrongVersion)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::UnsupportedVersion),
        "DeviceKey version status mismatch");

    const uint8_t wrongOperation[] = {
        blinker::kDeviceKeyProvisioningVersion, 99U};
    result = endpoint.handle(
        blinker::ByteView(wrongOperation, sizeof(wrongOperation)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::UnsupportedOperation),
        "DeviceKey operation status mismatch");
}

void testInstallRotationAndFailures() {
    static const char first[] =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
    static const char second[] =
        "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA";
    static const char invalid[] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    MemoryDeviceKeyStore store;
    blinker::DeviceInstanceId instance;
    std::memset(instance.bytes, 0x33, sizeof(instance.bytes));
    blinker::DeviceKeyProvisioningEndpoint endpoint(instance, store);
    uint8_t request[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    uint8_t response[blinker::kDeviceKeyProvisioningMaxResponseSize] = {};
    size_t written = 0U;

    buildInstall(first, request, sizeof(request));
    blinker::Result result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, 2U),
        written);
    blinker::DeviceKey absent;
    check(
        result.code() == blinker::ErrorCode::BufferTooSmall &&
            written == 0U && !endpoint.installed() &&
            store.load(absent).code() == blinker::ErrorCode::NotFound,
        "short install response buffer changed DeviceKey storage");
    blinker::clearDeviceKey(absent);

    buildInstall(invalid, request, sizeof(request));
    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() && !endpoint.installed() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::InvalidDeviceKey),
        "invalid DeviceKey install was accepted");

    store.failNextReplace();
    buildInstall(first, request, sizeof(request));
    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() && !endpoint.installed() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::StorageFailure),
        "DeviceKey storage failure was hidden");

    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() && endpoint.installed() &&
            status(response, written) == 0U,
        "valid DeviceKey install failed");

    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() && status(response, written) == 0U,
        "idempotent DeviceKey install failed");

    buildInstall(second, request, sizeof(request));
    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::Conflict),
        "second DeviceKey in one setup session was accepted");

    endpoint.reset();
    result = endpoint.handle(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() && endpoint.installed() &&
            status(response, written) == 0U,
        "new setup session could not rotate DeviceKey");

    blinker::DeviceKey expected;
    blinker::DeviceKey loaded;
    check(
        blinker::decodeDeviceKey(
            blinker::StringView(second, sizeof(second) - 1U),
            expected).ok() &&
            store.load(loaded).ok() &&
            blinker::sameDeviceKey(expected, loaded),
        "rotated DeviceKey was not durable");
    blinker::clearDeviceKey(expected);
    blinker::clearDeviceKey(loaded);

    endpoint.reset();
    store.corrupt();
    const uint8_t info[] = {
        blinker::kDeviceKeyProvisioningVersion,
        static_cast<uint8_t>(
            blinker::DeviceKeyProvisioningOperation::GetInfo)};
    result = endpoint.handle(
        blinker::ByteView(info, sizeof(info)),
        blinker::MutableByteSpan(response, sizeof(response)),
        written);
    check(
        result.ok() &&
            status(response, written) == static_cast<uint8_t>(
                blinker::DeviceKeyProvisioningStatus::StorageFailure),
        "corrupt DeviceKey store was reported as empty");
}

void testAtomicAccessBootstrapAndRefresh() {
    static const char first[] =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
    static const char second[] =
        "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA";
    MemoryBlobStore accessBlob;
    blinker::DeviceAccessStore access(accessBlob);
    blinker::DeviceInstanceId instance;
    std::memset(instance.bytes, 0x44, sizeof(instance.bytes));
    blinker::DeviceKeyProvisioningEndpoint endpoint(
        instance, access, &access);
    uint8_t response[blinker::kDeviceKeyProvisioningMaxResponseSize] = {};
    uint8_t install[blinker::kDeviceKeyProvisioningInstallRequestSize] = {};
    uint8_t bootstrap[
        blinker::kDeviceKeyProvisioningBootstrapRequestSize] = {};
    size_t written = 0U;

    const uint8_t info[] = {
        blinker::kDeviceKeyProvisioningVersion,
        static_cast<uint8_t>(
            blinker::DeviceKeyProvisioningOperation::GetInfo)};
    check(endpoint.handle(
              blinker::ByteView(info, sizeof(info)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok(),
          "access info failed");
    check(
        response[3] ==
            blinker::kDeviceKeyProvisioningSupportsAccessBootstrap,
        "access capabilities mismatch");

    buildInstall(first, install, sizeof(install));
    check(endpoint.handle(
              blinker::ByteView(install, sizeof(install)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok() &&
              status(response, written) == static_cast<uint8_t>(
                  blinker::DeviceKeyProvisioningStatus::AccessRequired),
          "key-only first access was accepted");

    buildBootstrap(first, 7U, 0x20U, 3U, 0x60U,
                   bootstrap, sizeof(bootstrap));
    accessBlob.failNextReplace();
    check(endpoint.handle(
              blinker::ByteView(bootstrap, sizeof(bootstrap)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok() &&
              status(response, written) == static_cast<uint8_t>(
                  blinker::DeviceKeyProvisioningStatus::StorageFailure),
          "atomic access write failure was hidden");

    uint32_t epoch = 0U;
    check(
        access.loadAccessEpoch(epoch).code() == blinker::ErrorCode::NotFound,
        "failed bootstrap left a partial access root");
    blinker::DeviceKey missing;
    check(access.load(missing).code() == blinker::ErrorCode::NotFound,
          "failed DeviceKey write appeared durable");
    blinker::clearDeviceKey(missing);

    check(endpoint.handle(
              blinker::ByteView(bootstrap, sizeof(bootstrap)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok() && status(response, written) == 0U &&
              endpoint.installed(),
          "exact bootstrap recovery failed");
    check(access.loadAccessEpoch(epoch).ok() && epoch == 7U,
          "bootstrap access epoch mismatch");

    endpoint.reset();
    buildInstall(second, install, sizeof(install));
    check(endpoint.handle(
              blinker::ByteView(install, sizeof(install)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok() && status(response, written) == 0U,
          "DeviceKey refresh failed");
    blinker::ControllerCredential controller;
    uint8_t controllerId[blinker::kControllerIdSize];
    for (size_t index = 0U; index < sizeof(controllerId); ++index) {
        controllerId[index] = static_cast<uint8_t>(0x20U + index);
    }
    check(access.loadActive(
              blinker::ByteView(controllerId, sizeof(controllerId)),
            blinker::ControllerCredentialDomain::PlatformAccess,
              7U,
              controller).ok(),
          "DeviceKey refresh removed the controller");

    endpoint.reset();
    buildBootstrap(second, 8U, 0x30U, 1U, 0x70U,
                   bootstrap, sizeof(bootstrap));
    check(endpoint.handle(
              blinker::ByteView(bootstrap, sizeof(bootstrap)),
              blinker::MutableByteSpan(response, sizeof(response)),
              written).ok() &&
              status(response, written) == static_cast<uint8_t>(
                  blinker::DeviceKeyProvisioningStatus::Conflict),
          "second first-access bootstrap replaced ownership");
}

} // namespace

int main() {
    testInfoAndStrictWire();
    testInstallRotationAndFailures();
    testAtomicAccessBootstrapAndRefresh();
    std::cout << "DeviceKey provisioning tests passed" << std::endl;
    return 0;
}
