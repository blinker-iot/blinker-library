#include "FactoryCredentialBundle.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'F', 'B', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kSecretOffset = 32U;
const size_t kLogicalIdOffset = 64U;
const size_t kSsidOffset = 128U;
const size_t kWifiCredentialOffset = 160U;
const size_t kCrcOffset = 224U;

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool allZero(const uint8_t* data, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | data[index]);
    }
    return combined == 0U;
}

bool sameWifi(
    const WifiCredentialProfile& first,
    const WifiCredentialProfile& second) {
    return first.generation == second.generation && first.confirmed() &&
           second.confirmed() &&
           first.authentication == second.authentication &&
           first.hidden() == second.hidden() &&
           first.ssidLength == second.ssidLength &&
           first.credentialLength == second.credentialLength &&
           memcmp(first.ssid, second.ssid, first.ssidLength) == 0 &&
           constantTimeEqual(
               ByteView(first.credential, first.credentialLength),
               ByteView(second.credential, second.credentialLength));
}

} // namespace

void clearFactoryCredentialBundle(FactoryCredentialBundle& bundle) {
    clearCloudCredential(bundle.cloud);
    clearWifiCredentialProfile(bundle.wifi);
}

Result FactoryCredentialInstaller::decode(
    ByteView encoded,
    FactoryCredentialBundle& output) {
    clearFactoryCredentialBundle(output);
    if (encoded.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (encoded.size < encodedSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (encoded.size > encodedSize) {
        return Result::failure(ErrorCode::TrailingData);
    }
    if (memcmp(encoded.data, kMagic, sizeof(kMagic)) != 0) {
        return Result::failure(ErrorCode::InvalidMagic);
    }
    if (encoded.data[4] != kSchemaVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (encoded.data[5] != 0U || encoded.data[25] != 0U ||
        encoded.data[26] != 0U || encoded.data[27] != 0U ||
        encoded.data[28] != 0U || encoded.data[29] != 0U ||
        encoded.data[30] != 0U || encoded.data[31] != 0U ||
        (encoded.data[21] & 0xFEU) != 0U) {
        return Result::failure(ErrorCode::InvalidHeader);
    }
    const uint16_t declaredSize = static_cast<uint16_t>(
        (static_cast<uint16_t>(encoded.data[6]) << 8U) |
        static_cast<uint16_t>(encoded.data[7]));
    if (declaredSize != encodedSize ||
        readU32(encoded.data + kCrcOffset) !=
            computeCrc32(ByteView(encoded.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const uint8_t logicalSize = encoded.data[22];
    const uint8_t ssidSize = encoded.data[23];
    const uint8_t credentialSize = encoded.data[24];
    if (logicalSize == 0U || logicalSize > kCloudLogicalDeviceIdCapacity ||
        ssidSize == 0U || ssidSize > kWifiSsidMaxSize ||
        credentialSize > kWifiCredentialMaxSize ||
        !allZero(
            encoded.data + kLogicalIdOffset + logicalSize,
            kCloudLogicalDeviceIdCapacity - logicalSize) ||
        !allZero(
            encoded.data + kSsidOffset + ssidSize,
            kWifiSsidMaxSize - ssidSize) ||
        !allZero(
            encoded.data + kWifiCredentialOffset + credentialSize,
            kWifiCredentialMaxSize - credentialSize)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    FactoryCredentialBundle decoded;
    decoded.cloud.generation = readU32(encoded.data + 8U);
    decoded.cloud.credentialVersion = readU32(encoded.data + 12U);
    decoded.cloud.logicalDeviceIdLength = logicalSize;
    memcpy(
        decoded.cloud.secret,
        encoded.data + kSecretOffset,
        kCloudCredentialSecretSize);
    memcpy(
        decoded.cloud.logicalDeviceId,
        encoded.data + kLogicalIdOffset,
        logicalSize);

    decoded.wifi.generation = readU32(encoded.data + 16U);
    decoded.wifi.authentication =
        static_cast<WifiAuthentication>(encoded.data[20]);
    decoded.wifi.ssidLength = ssidSize;
    decoded.wifi.credentialLength = credentialSize;
    decoded.wifi.setHidden((encoded.data[21] & 0x01U) != 0U);
    decoded.wifi.setConfirmed(true);
    memcpy(decoded.wifi.ssid, encoded.data + kSsidOffset, ssidSize);
    if (credentialSize != 0U) {
        memcpy(
            decoded.wifi.credential,
            encoded.data + kWifiCredentialOffset,
            credentialSize);
    }

    Result result = validateCloudCredential(decoded.cloud);
    if (result) result = validateWifiNetworkConfig(decoded.wifi.networkConfig());
    if (result) output = decoded;
    clearFactoryCredentialBundle(decoded);
    return result;
}

Result FactoryCredentialInstaller::install(
    const FactoryCredentialBundle& bundle,
    CloudCredentialStore& cloudStore,
    WifiCredentialStore& wifiStore) {
    Result result = validateCloudCredential(bundle.cloud);
    if (!result) return result;
    result = validateWifiNetworkConfig(bundle.wifi.networkConfig());
    if (!result || !bundle.wifi.confirmed() || bundle.wifi.generation == 0U) {
        return result ? Result::failure(ErrorCode::InvalidArgument) : result;
    }

    CloudCredential currentCloud;
    const Result cloudResult = cloudStore.load(currentCloud);
    const bool cloudMissing = cloudResult.code() == ErrorCode::NotFound;
    const bool cloudMatches =
        cloudResult && sameCloudCredential(currentCloud, bundle.cloud);
    clearCloudCredential(currentCloud);
    if (!cloudMissing && !cloudMatches) {
        return cloudResult
                   ? Result::failure(ErrorCode::StateConflict)
                   : cloudResult;
    }

    WifiCredentialProfile currentWifi;
    const Result wifiResult = wifiStore.load(currentWifi);
    const bool wifiMissing = wifiResult.code() == ErrorCode::NotFound;
    const bool wifiMatches = wifiResult && sameWifi(currentWifi, bundle.wifi);
    clearWifiCredentialProfile(currentWifi);
    if (!wifiMissing && !wifiMatches) {
        return wifiResult ? Result::failure(ErrorCode::StateConflict)
                          : wifiResult;
    }

    // WiFi is installed first. The cloud credential is the final marker,
    // so an interrupted first run cannot authenticate with partial network
    // state and the exact same bundle can safely resume.
    if (wifiMissing) result = wifiStore.installConfirmed(bundle.wifi);
    if (result && cloudMissing) {
        result = cloudStore.installFresh(bundle.cloud);
    }
    return result;
}

Result FactoryCredentialInstaller::installEncoded(
    ByteView encoded,
    CloudCredentialStore& cloudStore,
    WifiCredentialStore& wifiStore) {
    FactoryCredentialBundle bundle;
    Result result = decode(encoded, bundle);
    if (result) result = install(bundle, cloudStore, wifiStore);
    clearFactoryCredentialBundle(bundle);
    return result;
}

} // namespace blinker
