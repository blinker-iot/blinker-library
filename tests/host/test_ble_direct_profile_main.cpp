#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/runtime/BleDirectProfile.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "BLE Direct profile check failed at line "           \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(EXIT_FAILURE);                                            \
        }                                                                       \
    } while (false)

class MemoryStore final : public blinker::IAtomicBlobStore {
public:
    MemoryStore() : data_(), size_(0U), present_(false) {}

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
        std::memcpy(output.data, data_, size_);
        written = size_;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        if (value.data == nullptr || value.size > sizeof(data_)) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memset(data_, 0, sizeof(data_));
        std::memcpy(data_, value.data, value.size);
        size_ = value.size;
        present_ = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        std::memset(data_, 0, sizeof(data_));
        size_ = 0U;
        present_ = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

private:
    uint8_t data_[blinker::DeviceAccessStore::serializedSize];
    size_t size_;
    bool present_;
};

class TestClock final : public blinker::IClock {
public:
    TestClock() : now(0U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    uint32_t now;
};

class TestRandom final : public blinker::IRandom {
public:
    TestRandom() : next_(1U) {}
    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (output.data == nullptr || output.empty()) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = static_cast<uint8_t>(next_ + index);
        }
        ++next_;
        return blinker::Result::success();
    }

private:
    uint8_t next_;
};

blinker::ControllerCredential controller(uint32_t epoch) {
    blinker::ControllerCredential value;
    value.domain = blinker::ControllerCredentialDomain::PlatformAccess;
    value.ownershipGeneration = epoch;
    value.credentialVersion = 1U;
    value.permissions = blinker::kControllerPermissionAll;
    for (size_t index = 0U; index < sizeof(value.controllerId); ++index) {
        value.controllerId[index] = static_cast<uint8_t>(0x20U + index);
    }
    for (size_t index = 0U; index < sizeof(value.secret); ++index) {
        value.secret[index] = static_cast<uint8_t>(0x40U + index);
    }
    return value;
}

blinker::DevicePresenceKey presence(uint32_t version, uint8_t base) {
    blinker::DevicePresenceKey value;
    value.version = version;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        value.bytes[index] = static_cast<uint8_t>(base + index);
    }
    return value;
}

blinker::DeviceInstanceId deviceInstance() {
    blinker::DeviceInstanceId value;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        value.bytes[index] = static_cast<uint8_t>(0x10U + index);
    }
    return value;
}

void testFailClosedFallbackAndAuthorizedRefresh() {
    MemoryStore memory;
    blinker::DeviceAccessStore access(memory);
    TestRandom random;
    TestClock clock;
    const blinker::DeviceInstanceId instance = deviceInstance();

    blinker::BleDirectProfileProvider profiles(
        instance, access, random, clock, 1000U);
    blinker::ble::ModeProfile profile;
    CHECK_TRUE(profiles.make(0U, profile));
    CHECK_TRUE(profile.wireVersion == blinker::ble::kDirectBbp2WireVersion);
    CHECK_TRUE(!profiles.refreshDue(0U, 0U));
    CHECK_TRUE(profiles.refreshDue(0U, 1U));
    CHECK_TRUE(profiles.make(1U, profile));
    CHECK_TRUE(profile.wireVersion == blinker::ble::kDirectBbp2WireVersion);
    clock.now = 5000U;
    CHECK_TRUE(!profiles.refreshDue(0U, 1U));

    const uint32_t epoch = 7U;
    const blinker::DevicePresenceKey first = presence(1U, 0x80U);
    CHECK_TRUE(access.bootstrapAccessWithPresence(
        epoch, controller(epoch), first));

    CHECK_TRUE(profiles.refreshDue(0U, 2U));
    CHECK_TRUE(profiles.make(2U, profile));
    CHECK_TRUE(
        profile.wireVersion ==
        blinker::ble::kAuthorizedDirectBbp2WireVersion);
    CHECK_TRUE(
        (profile.capabilities &
         blinker::ble::ModeCapabilityAuthorizedPresenceV1) != 0U);
    bool matches = false;
    CHECK_TRUE(blinker::ble::verifyPresenceLocator(
        blinker::ByteView(first.bytes, sizeof(first.bytes)),
        instance,
        epoch,
        first.version,
        blinker::ByteView(profile.modeLocator, sizeof(profile.modeLocator)),
        matches));
    CHECK_TRUE(matches);

    CHECK_TRUE(!profiles.refreshDue(1U, 3U));
    CHECK_TRUE(profiles.refreshDue(0U, 3U));
    const blinker::ble::ModeProfile beforeDisconnect = profile;
    CHECK_TRUE(profiles.make(3U, profile));
    CHECK_TRUE(std::memcmp(
        beforeDisconnect.modeLocator,
        profile.modeLocator,
        sizeof(profile.modeLocator)) != 0);
    CHECK_TRUE(!profiles.refreshDue(0U, 3U));

    profiles.requestRefresh();
    CHECK_TRUE(!profiles.refreshDue(1U, 3U));
    CHECK_TRUE(profiles.refreshDue(0U, 3U));
    CHECK_TRUE(profiles.make(3U, profile));
    CHECK_TRUE(!profiles.refreshDue(0U, 3U));

    clock.now = 5999U;
    CHECK_TRUE(!profiles.refreshDue(0U, 3U));
    clock.now = 6000U;
    CHECK_TRUE(profiles.refreshDue(0U, 3U));

    const blinker::DevicePresenceKey second = presence(2U, 0x90U);
    CHECK_TRUE(access.replacePresenceVerified(epoch, 1U, second));
    CHECK_TRUE(profiles.make(4U, profile));
    CHECK_TRUE(blinker::ble::verifyPresenceLocator(
        blinker::ByteView(second.bytes, sizeof(second.bytes)),
        instance,
        epoch,
        second.version,
        blinker::ByteView(profile.modeLocator, sizeof(profile.modeLocator)),
        matches));
    CHECK_TRUE(matches);

    profiles.reset();
    CHECK_TRUE(!profiles.refreshDue(0U, 8U));
}

} // namespace

int main() {
    testFailClosedFallbackAndAuthorizedRefresh();
    std::cout << "BLE Direct profile provider checks passed" << std::endl;
    return 0;
}
