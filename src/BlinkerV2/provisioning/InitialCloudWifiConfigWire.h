#ifndef BLINKER_PROVISIONING_INITIALCLOUDWIFICONFIGWIRE_H
#define BLINKER_PROVISIONING_INITIALCLOUDWIFICONFIGWIRE_H

#include "WifiCredential.h"

namespace blinker {

// Preflight only the pinned Android WiFiProv's three prov-config requests.
// The SDK still decodes them and encodes its replies. Its Set decoder copies
// protobuf bytes to C strings, which loses embedded-NUL/field-length facts.
// This bounded subset rejects those ambiguities before entering that decoder.
inline Result validateInitialCloudWifiConfigWire(ByteView frame) {
    // The installed ESP32-S3 config decoder rejects 64-byte raw PSKs.
    constexpr size_t maxSdkPassphrase = 63U;
    if (frame.data == nullptr || frame.size < 2U || frame.size > 8U + kWifiSsidMaxSize + maxSdkPassphrase)
        return Result::failure(ErrorCode::InvalidEncoding);
    const uint8_t* bytes = frame.data;
    if ((frame.size == 2U && bytes[0] == 0x52U && bytes[1] == 0U) ||
        (frame.size == 4U && bytes[0] == 0x08U && bytes[1] == 4U && bytes[2] == 0x72U && bytes[3] == 0U))
        return Result::success();
    if (frame.size < 7U || bytes[0] != 0x08U || bytes[1] != 2U || bytes[2] != 0x62U ||
        bytes[3] != frame.size - 4U || bytes[4] != 0x0AU || bytes[5] == 0U ||
        bytes[5] > kWifiSsidMaxSize || bytes[5] > frame.size - 6U)
        return Result::failure(ErrorCode::InvalidEncoding);
    WifiNetworkConfig candidate;
    candidate.ssid = ByteView(bytes + 6U, bytes[5]);
    const size_t passwordTag = 6U + candidate.ssid.size;
    if (passwordTag != frame.size) {
        if (frame.size - passwordTag < 3U || bytes[passwordTag] != 0x12U ||
            bytes[passwordTag + 1U] != frame.size - passwordTag - 2U ||
            bytes[passwordTag + 1U] > maxSdkPassphrase)
            return Result::failure(ErrorCode::InvalidEncoding);
        candidate.authentication = WifiAuthentication::Wpa2Personal;
        candidate.credential = ByteView(bytes + passwordTag + 2U, bytes[passwordTag + 1U]);
    }
    return validateWifiNetworkConfig(candidate);
}

} // namespace blinker
#endif
