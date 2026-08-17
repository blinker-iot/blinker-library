#include "WifiCredential.h"

namespace blinker {

namespace {

bool validAuthentication(WifiAuthentication authentication) {
    return static_cast<uint8_t>(authentication) <=
           static_cast<uint8_t>(WifiAuthentication::Wpa2Wpa3Personal);
}

bool containsZero(ByteView value) {
    if (value.data == nullptr) return !value.empty();
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == 0U) return true;
    }
    return false;
}

bool isHex(uint8_t value) {
    return (value >= static_cast<uint8_t>('0') &&
            value <= static_cast<uint8_t>('9')) ||
           (value >= static_cast<uint8_t>('a') &&
            value <= static_cast<uint8_t>('f')) ||
           (value >= static_cast<uint8_t>('A') &&
            value <= static_cast<uint8_t>('F'));
}

bool validPrintableCredential(
    ByteView value,
    size_t minimum,
    size_t maximum) {
    if (value.data == nullptr || value.size < minimum ||
        value.size > maximum) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] < 0x20U || value.data[index] > 0x7EU) {
            return false;
        }
    }
    return true;
}

} // namespace

Result validateWifiNetworkConfig(const WifiNetworkConfig& config) {
    if (config.ssid.data == nullptr || config.ssid.empty() ||
        config.ssid.size > kWifiSsidMaxSize || containsZero(config.ssid) ||
        !validAuthentication(config.authentication)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (config.authentication == WifiAuthentication::Open) {
        return config.credential.empty()
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }
    bool valid = false;
    if (config.authentication == WifiAuthentication::Wpa2Personal) {
        valid = validPrintableCredential(config.credential, 8U, 63U);
        if (!valid && config.credential.data != nullptr &&
            config.credential.size == 64U) {
            valid = true;
            for (size_t index = 0U;
                 index < config.credential.size;
                 ++index) {
                if (!isHex(config.credential.data[index])) {
                    valid = false;
                    break;
                }
            }
        }
    } else if (config.authentication == WifiAuthentication::Wpa3Personal) {
        valid = validPrintableCredential(config.credential, 1U, 63U);
    } else {
        valid = validPrintableCredential(config.credential, 8U, 63U);
    }
    return valid ? Result::success()
                 : Result::failure(ErrorCode::InvalidArgument);
}

} // namespace blinker
