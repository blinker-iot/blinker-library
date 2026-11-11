#include <BlinkerV2/runtime/GatewayPresenceMatcher.h>
#include <BlinkerV2/protocol/ble/Mode.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(value)                                                     \
    do {                                                                      \
        if (!(value)) {                                                       \
            std::cerr << "check failed at line " << __LINE__ << std::endl;   \
            std::abort();                                                     \
        }                                                                     \
    } while (false)

void fill(uint8_t* bytes, size_t size, uint8_t seed) {
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = static_cast<uint8_t>(seed + index);
    }
}

blinker::GatewayAccessRecord accessRecord() {
    blinker::GatewayAccessRecord access;
    fill(
        access.childDeviceInstanceId,
        sizeof(access.childDeviceInstanceId),
        0x11U);
    access.accessEpoch = 7U;
    access.activePresence.version = 3U;
    fill(
        access.activePresence.key,
        sizeof(access.activePresence.key),
        0x31U);
    access.pendingPresence.version = 4U;
    fill(
        access.pendingPresence.key,
        sizeof(access.pendingPresence.key),
        0x51U);
    return access;
}

blinker::ByteView advertisementFor(
    const blinker::GatewayAccessRecord& access,
    const blinker::GatewayPresenceLocator& presence,
    uint8_t (&storage)[blinker::ble::kModeServiceDataSize]) {
    blinker::DeviceInstanceId child;
    std::memcpy(
        child.bytes,
        access.childDeviceInstanceId,
        sizeof(child.bytes));
    const uint8_t nonce[blinker::ble::kPresenceNonceSize] = {
        0x61U, 0x62U, 0x63U, 0x64U
    };
    uint8_t locator[blinker::ble::kPresenceLocatorSize] = {};
    CHECK_TRUE(blinker::ble::derivePresenceLocator(
        blinker::ByteView(presence.key, sizeof(presence.key)),
        child,
        access.accessEpoch,
        presence.version,
        blinker::ByteView(nonce, sizeof(nonce)),
        blinker::MutableByteSpan(locator, sizeof(locator))));
    const blinker::ble::ModeProfile profile =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        profile,
        blinker::MutableByteSpan(storage, sizeof(storage)),
        encoded));
    return encoded;
}

void testActiveAndPending() {
    blinker::GatewayAccessRecord access = accessRecord();
    blinker::GatewayPresenceMatcher matcher;
    CHECK_TRUE(matcher.configure(access));

    uint8_t bytes[blinker::ble::kModeServiceDataSize] = {};
    blinker::BleCentralAdvertisement advertisement;
    advertisement.modeServiceData = advertisementFor(
        access, access.activePresence, bytes);
    CHECK_TRUE(matcher.match(advertisement));
    CHECK_TRUE(!matcher.matchedPending());
    CHECK_TRUE(
        matcher.matchedPresenceVersion() ==
        access.activePresence.version);

    advertisement.modeServiceData = advertisementFor(
        access, access.pendingPresence, bytes);
    CHECK_TRUE(blinker::GatewayPresenceMatcher::matchThunk(
        &matcher, advertisement));
    CHECK_TRUE(matcher.matchedPending());
    CHECK_TRUE(
        matcher.matchedPresenceVersion() ==
        access.pendingPresence.version);
}

void testRejectsWrongOrMalformedPresence() {
    blinker::GatewayAccessRecord access = accessRecord();
    blinker::GatewayPresenceMatcher matcher;
    CHECK_TRUE(matcher.configure(access));

    blinker::GatewayAccessRecord other = access;
    other.childDeviceInstanceId[0] ^= 0x01U;
    uint8_t bytes[blinker::ble::kModeServiceDataSize] = {};
    blinker::BleCentralAdvertisement advertisement;
    advertisement.modeServiceData = advertisementFor(
        other, other.activePresence, bytes);
    CHECK_TRUE(!matcher.match(advertisement));
    CHECK_TRUE(matcher.matchedPresenceVersion() == 0U);

    const blinker::ble::ModeProfile legacy =
        blinker::ble::makeDirectModeProfile();
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        legacy,
        blinker::MutableByteSpan(bytes, sizeof(bytes)),
        encoded));
    advertisement.modeServiceData = encoded;
    CHECK_TRUE(!matcher.match(advertisement));

    advertisement.modeServiceData = blinker::ByteView(bytes, 5U);
    CHECK_TRUE(!matcher.match(advertisement));
}

void testConfigurationValidation() {
    blinker::GatewayPresenceMatcher matcher;
    blinker::GatewayAccessRecord access = accessRecord();
    access.pendingPresence.version = access.activePresence.version;
    CHECK_TRUE(
        matcher.configure(access).code() ==
        blinker::ErrorCode::InvalidArgument);
    CHECK_TRUE(!matcher.configured());

    access = accessRecord();
    std::memset(
        access.activePresence.key,
        0,
        sizeof(access.activePresence.key));
    CHECK_TRUE(
        matcher.configure(access).code() ==
        blinker::ErrorCode::InvalidArgument);
    CHECK_TRUE(!matcher.configured());

    access = accessRecord();
    access.pendingPresence = blinker::GatewayPresenceLocator();
    CHECK_TRUE(matcher.configure(access));
}

} // namespace

int main() {
    testActiveAndPending();
    testRejectsWrongOrMalformedPresence();
    testConfigurationValidation();
    std::cout << "Gateway presence matcher tests passed" << std::endl;
    return 0;
}
