#include "Mode.h"

#include "Gatt.h"

#include <string.h>

namespace blinker {
namespace ble {

namespace {

bool allZero(const uint8_t* bytes, size_t size) {
    uint8_t aggregate = 0U;
    for (size_t index = 0U; index < size; ++index) {
        aggregate = static_cast<uint8_t>(aggregate | bytes[index]);
    }
    return aggregate == 0U;
}

ModeProfile makeProvisioningProfile(
    ByteView setupSessionLocator,
    bool acceptsWifiConfig,
    uint16_t noiseCapability,
    uint16_t applicationCapability) {
    ModeProfile profile;
    profile.mode = ApplicationMode::Provisioning;
    profile.wireVersion = kProvisioningWireVersion;
    const uint16_t wifiCapability = acceptsWifiConfig
        ? static_cast<uint16_t>(ModeCapabilityWifiConfigV1)
        : 0U;
    profile.capabilities = static_cast<uint16_t>(
        ModeCapabilityFragmentedRecords |
        noiseCapability |
        applicationCapability |
        wifiCapability);
    if (setupSessionLocator.data != nullptr &&
        setupSessionLocator.size == 8U) {
        memcpy(
            profile.setupSessionLocator,
            setupSessionLocator.data,
            8U);
    }
    return profile;
}

} // namespace

ModeProfile makeDirectModeProfile() {
    ModeProfile profile;
    profile.mode = ApplicationMode::Direct;
    profile.wireVersion = kDirectBbp2WireVersion;
    profile.capabilities = static_cast<uint16_t>(
        ModeCapabilityFragmentedRecords | ModeCapabilityDirectBbp2);
    return profile;
}

ModeProfile makeProvisioningModeProfile(
    ByteView setupSessionLocator,
    bool acceptsWifiConfig) {
    return makeProvisioningProfile(
        setupSessionLocator,
        acceptsWifiConfig,
        static_cast<uint16_t>(ModeCapabilityNoiseNn),
        static_cast<uint16_t>(ModeCapabilityEnrollmentV1));
}

ModeProfile makePskProvisioningModeProfile(
    ByteView setupSessionLocator,
    bool acceptsWifiConfig) {
    return makeProvisioningProfile(
        setupSessionLocator,
        acceptsWifiConfig,
        static_cast<uint16_t>(ModeCapabilityNoiseNnPsk0),
        static_cast<uint16_t>(ModeCapabilityEnrollmentV1));
}

Result provisioningNoisePattern(
    const ModeProfile& profile,
    noise::NoiseNnPattern& pattern) {
    pattern = noise::NoiseNnPattern::Nn;
    if (profile.mode != ApplicationMode::Provisioning) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const bool nn =
        (profile.capabilities & ModeCapabilityNoiseNn) != 0U;
    const bool psk0 =
        (profile.capabilities & ModeCapabilityNoiseNnPsk0) != 0U;
    if (nn == psk0) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    pattern = psk0 ? noise::NoiseNnPattern::NnPsk0
                   : noise::NoiseNnPattern::Nn;
    return Result::success();
}

Result validateModeProfile(const ModeProfile& profile) {
    if ((profile.capabilities & ~kKnownModeCapabilities) != 0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    const bool setupSessionIsZero = allZero(
        profile.setupSessionLocator,
        sizeof(profile.setupSessionLocator));
    if (profile.mode == ApplicationMode::Provisioning) {
        const bool enrollment =
            (profile.capabilities & ModeCapabilityEnrollmentV1) != 0U;
        const uint16_t required =
            static_cast<uint16_t>(ModeCapabilityFragmentedRecords);
        noise::NoiseNnPattern pattern;
        if (profile.wireVersion != kProvisioningWireVersion ||
            (profile.capabilities & required) != required ||
            (profile.capabilities & ModeCapabilityDirectBbp2) != 0U ||
            !enrollment ||
            setupSessionIsZero) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        return provisioningNoisePattern(profile, pattern);
    }
    if (profile.mode == ApplicationMode::Direct) {
        const uint16_t required = static_cast<uint16_t>(
            ModeCapabilityFragmentedRecords |
            ModeCapabilityDirectBbp2);
        const uint16_t provisioningOnly = static_cast<uint16_t>(
            ModeCapabilityNoiseNn |
            ModeCapabilityNoiseNnPsk0 |
            ModeCapabilityEnrollmentV1 |
            ModeCapabilityWifiConfigV1);
        if (profile.wireVersion != kDirectBbp2WireVersion ||
            (profile.capabilities & required) != required ||
            (profile.capabilities & provisioningOnly) != 0U ||
            !setupSessionIsZero) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        return Result::success();
    }
    return Result::failure(ErrorCode::InvalidArgument);
}

bool modeProfilesEqual(
    const ModeProfile& left,
    const ModeProfile& right) {
    return left.mode == right.mode &&
           left.wireVersion == right.wireVersion &&
           left.capabilities == right.capabilities &&
           memcmp(
               left.setupSessionLocator,
               right.setupSessionLocator,
               sizeof(left.setupSessionLocator)) == 0;
}

Result encodeModeServiceData(
    const ModeProfile& profile,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateModeProfile(profile);
    if (!result) return result;
    if (output.data == nullptr || output.size < kModeServiceDataSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    output.data[0] = kModeAdvertisingFormatVersion;
    output.data[1] = static_cast<uint8_t>(profile.mode);
    output.data[2] = profile.wireVersion;
    output.data[3] = static_cast<uint8_t>(profile.capabilities);
    output.data[4] = static_cast<uint8_t>(profile.capabilities >> 8U);
    memcpy(output.data + 5U, profile.setupSessionLocator, 8U);
    encoded = ByteView(output.data, kModeServiceDataSize);
    return Result::success();
}

Result decodeModeServiceData(ByteView encoded, ModeProfile& profile) {
    profile = ModeProfile();
    if (encoded.data == nullptr || encoded.size != kModeServiceDataSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (encoded.data[0] != kModeAdvertisingFormatVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    profile.mode = static_cast<ApplicationMode>(encoded.data[1]);
    profile.wireVersion = encoded.data[2];
    profile.capabilities = static_cast<uint16_t>(
        encoded.data[3] |
        (static_cast<uint16_t>(encoded.data[4]) << 8U));
    memcpy(profile.setupSessionLocator, encoded.data + 5U, 8U);
    Result result = validateModeProfile(profile);
    if (!result) profile = ModeProfile();
    return result;
}

Result encodeModeScanResponse(
    const ModeProfile& profile,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr || output.size < kModeScanResponseSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView serviceData;
    Result result = encodeModeServiceData(
        profile,
        MutableByteSpan(output.data + 18U, output.size - 18U),
        serviceData);
    if (!result) return result;
    output.data[0] = 30U;
    output.data[1] = 0x21U;
    memcpy(output.data + 2U, kServiceUuidLittleEndian, 16U);
    encoded = ByteView(output.data, kModeScanResponseSize);
    return Result::success();
}

} // namespace ble
} // namespace blinker
