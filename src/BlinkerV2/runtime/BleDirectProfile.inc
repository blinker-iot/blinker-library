#include "BleDirectProfile.h"

#include "../core/SecureMemory.h"

#include <string.h>

namespace blinker {

BleDirectProfileProvider::BleDirectProfileProvider(
    const DeviceInstanceId& deviceInstance,
    IDeviceAccessStore& access,
    IRandom& random,
    IClock& clock,
    uint32_t rotationIntervalMillis)
    : deviceInstance_(deviceInstance),
      access_(access),
      random_(random),
      clock_(clock),
      rotationIntervalMillis_(rotationIntervalMillis),
      generatedAtMillis_(0U),
      observedSessionRevision_(0U),
      previousNonce_(),
      authorizedProfile_(0U),
      profileReady_(0U),
      hasPreviousNonce_(0U),
      refreshRequested_(0U) {}

Result BleDirectProfileProvider::make(
    uint32_t sessionRevision,
    ble::ModeProfile& profile) {
    profile = ble::makeDirectModeProfile();
    Result result = tryMakeAuthorized(profile);
    if (!result) {
        profile = ble::ModeProfile();
        return result;
    }
    generatedAtMillis_ = clock_.monotonicMillis();
    observedSessionRevision_ = sessionRevision;
    authorizedProfile_ =
        profile.wireVersion == ble::kAuthorizedDirectBbp2WireVersion
            ? 1U
            : 0U;
    profileReady_ = 1U;
    refreshRequested_ = 0U;
    return Result::success();
}

bool BleDirectProfileProvider::refreshDue(
    size_t sessionCount,
    uint32_t sessionRevision) const {
    if (profileReady_ == 0U || sessionCount != 0U) {
        return false;
    }
    if (refreshRequested_ != 0U) return true;
    if (sessionRevision != observedSessionRevision_) return true;
    return authorizedProfile_ != 0U &&
           rotationIntervalMillis_ != 0U &&
           static_cast<uint32_t>(
               clock_.monotonicMillis() - generatedAtMillis_) >=
               rotationIntervalMillis_;
}

void BleDirectProfileProvider::requestRefresh() {
    if (profileReady_ != 0U) refreshRequested_ = 1U;
}

void BleDirectProfileProvider::reset() {
    secureZero(MutableByteSpan(previousNonce_, sizeof(previousNonce_)));
    generatedAtMillis_ = 0U;
    observedSessionRevision_ = 0U;
    authorizedProfile_ = 0U;
    profileReady_ = 0U;
    hasPreviousNonce_ = 0U;
    refreshRequested_ = 0U;
}

Result BleDirectProfileProvider::tryMakeAuthorized(
    ble::ModeProfile& profile) {
    uint32_t accessEpoch = 0U;
    Result result = access_.loadAccessEpoch(accessEpoch);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    if (!result) return result;

    DevicePresenceKey presence;
    result = access_.loadPresence(accessEpoch, presence);
    if (!result && (result.code() == ErrorCode::NotFound ||
                    result.code() == ErrorCode::UnsupportedFeature)) {
        clearDevicePresenceKey(presence);
        return Result::success();
    }
    if (!result) {
        clearDevicePresenceKey(presence);
        return result;
    }

    uint8_t nonce[ble::kPresenceNonceSize] = {};
    uint8_t locator[ble::kPresenceLocatorSize] = {};
    const ByteView previous = hasPreviousNonce_ != 0U
                                  ? ByteView(
                                        previousNonce_,
                                        sizeof(previousNonce_))
                                  : ByteView();
    result = ble::generatePresenceNonce(
        random_, previous, MutableByteSpan(nonce, sizeof(nonce)));
    if (result) {
        result = ble::derivePresenceLocator(
            ByteView(presence.bytes, sizeof(presence.bytes)),
            deviceInstance_,
            accessEpoch,
            presence.version,
            ByteView(nonce, sizeof(nonce)),
            MutableByteSpan(locator, sizeof(locator)));
    }
    if (result) {
        const ble::ModeProfile candidate =
            ble::makeAuthorizedDirectModeProfile(
                ByteView(locator, sizeof(locator)));
        result = ble::validateModeProfile(candidate);
        if (result) {
            profile = candidate;
            memcpy(previousNonce_, nonce, sizeof(previousNonce_));
            hasPreviousNonce_ = 1U;
        }
    }

    clearDevicePresenceKey(presence);
    secureZero(MutableByteSpan(nonce, sizeof(nonce)));
    secureZero(MutableByteSpan(locator, sizeof(locator)));
    return result;
}

} // namespace blinker
