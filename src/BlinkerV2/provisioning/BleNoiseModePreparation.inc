#include "BleNoiseModePreparation.h"

#include "../core/SecureMemory.h"

namespace blinker {

Result BleNoiseModePreparation::prepareBleMode(
    const ble::ModeProfile& profile,
    bool rollback) {
    (void)rollback;

    // Preparation is fail-closed even when called outside the coordinator.
    channel_.cancel();
    session_.endSession();
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    if (profile.mode == ble::ApplicationMode::Direct) {
        return Result::success();
    }
    if (profile.mode != ble::ApplicationMode::Provisioning) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    noise::NoiseNnPattern pattern;
    result = ble::provisioningNoisePattern(profile, pattern);
    if (result && pattern != channel_.pattern()) {
        result = Result::failure(ErrorCode::NotConfigured);
    }
    if (result && pattern == noise::NoiseNnPattern::Nn) {
        result = channel_.arm(ByteView());
    } else if (result) {
        result = armPsk(profile);
    }
    if (result) {
        result = session_.beginSession(ByteView(
            profile.modeLocator,
            kSetupSessionLocatorSize));
    }
    if (!result) {
        channel_.cancel();
        session_.endSession();
    }
    return result;
}

Result BleNoiseModePreparation::armPsk(
    const ble::ModeProfile& profile) {
    if (provider_ == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    uint8_t psk[kSetupPskSize] = {};
    size_t written = 0U;
    Result result = provider_->loadSetupPsk(
        ByteView(
            profile.modeLocator,
            kSetupSessionLocatorSize),
        MutableByteSpan(psk, sizeof(psk)),
        written);
    if (result && written != sizeof(psk)) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (result) result = channel_.arm(ByteView(psk, sizeof(psk)));
    secureZero(MutableByteSpan(psk, sizeof(psk)));
    return result;
}

} // namespace blinker
