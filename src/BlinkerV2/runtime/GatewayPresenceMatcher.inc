#include "GatewayPresenceMatcher.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../protocol/ble/Mode.h"

namespace blinker {

namespace {

bool gatewayPresenceNonZero(const uint8_t* bytes, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | bytes[index]);
    }
    return combined != 0U;
}

bool validGatewayPresenceLocator(
    const GatewayPresenceLocator& locator,
    bool required) {
    const bool keyPresent = gatewayPresenceNonZero(
        locator.key, sizeof(locator.key));
    return required ? locator.version != 0U && keyPresent
                    : (locator.version == 0U) == !keyPresent;
}

} // namespace

GatewayPresenceMatcher::GatewayPresenceMatcher()
    : child_(), active_(), pending_(), accessEpoch_(0U),
      matchedVersion_(0U), configured_(false), matchedPending_(false) {}

GatewayPresenceMatcher::~GatewayPresenceMatcher() {
    clear();
}

Result GatewayPresenceMatcher::configure(
    const GatewayAccessRecord& access) {
    clear();
    DeviceInstanceId child;
    memcpy(
        child.bytes,
        access.childDeviceInstanceId,
        sizeof(child.bytes));
    const bool pendingEmpty = access.pendingPresence.version == 0U;
    if (!isValidDeviceInstanceId(child) || access.accessEpoch == 0U ||
        !validGatewayPresenceLocator(access.activePresence, true) ||
        !validGatewayPresenceLocator(access.pendingPresence, false) ||
        (!pendingEmpty &&
         access.pendingPresence.version <= access.activePresence.version)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    child_ = child;
    active_ = access.activePresence;
    pending_ = access.pendingPresence;
    accessEpoch_ = access.accessEpoch;
    configured_ = true;
    return Result::success();
}

void GatewayPresenceMatcher::clear() {
    secureZero(MutableByteSpan(child_.bytes, sizeof(child_.bytes)));
    secureZero(MutableByteSpan(active_.key, sizeof(active_.key)));
    secureZero(MutableByteSpan(pending_.key, sizeof(pending_.key)));
    active_.version = 0U;
    pending_.version = 0U;
    accessEpoch_ = 0U;
    matchedVersion_ = 0U;
    configured_ = false;
    matchedPending_ = false;
}

bool GatewayPresenceMatcher::matchLocator(
    const GatewayPresenceLocator& locator,
    ByteView advertisedLocator) const {
    if (locator.version == 0U) return false;
    bool matches = false;
    return ble::verifyPresenceLocator(
               ByteView(locator.key, sizeof(locator.key)),
               child_, accessEpoch_, locator.version,
               advertisedLocator, matches) &&
           matches;
}

bool GatewayPresenceMatcher::match(
    const BleCentralAdvertisement& advertisement) {
    matchedVersion_ = 0U;
    matchedPending_ = false;
    if (!configured_) return false;

    ble::ModeProfile profile;
    if (!ble::decodeModeServiceData(
            advertisement.modeServiceData, profile) ||
        profile.mode != ble::ApplicationMode::Direct ||
        profile.wireVersion != ble::kAuthorizedDirectBbp2WireVersion ||
        (profile.capabilities &
         ble::ModeCapabilityAuthorizedPresenceV1) == 0U) {
        return false;
    }
    const ByteView locator(
        profile.modeLocator, sizeof(profile.modeLocator));
    if (matchLocator(pending_, locator)) {
        matchedVersion_ = pending_.version;
        matchedPending_ = true;
        return true;
    }
    if (matchLocator(active_, locator)) {
        matchedVersion_ = active_.version;
        return true;
    }
    return false;
}

bool GatewayPresenceMatcher::matchThunk(
    void* context,
    const BleCentralAdvertisement& advertisement) {
    GatewayPresenceMatcher* matcher =
        static_cast<GatewayPresenceMatcher*>(context);
    return matcher != nullptr && matcher->match(advertisement);
}

} // namespace blinker
