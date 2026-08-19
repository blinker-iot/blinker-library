#include <ArduinoBLE.h>
#include <utility/ATT.h>

namespace blinker {

namespace {

int hexNibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool decodeAddress(StringView text, uint8_t output[6]) {
    if (text.data == nullptr || text.size != 17U) return false;
    for (size_t index = 0U; index < 6U; ++index) {
        const size_t offset = index * 3U;
        if (index != 5U && text.data[offset + 2U] != ':') return false;
        const int high = hexNibble(text.data[offset]);
        const int low = hexNibble(text.data[offset + 1U]);
        if (high < 0 || low < 0) return false;
        output[5U - index] = static_cast<uint8_t>((high << 4U) | low);
    }
    return true;
}

} // namespace

inline void RenesasUnoBleSecuritySource::connected(StringView peerAddress) {
    connectionHandle_ = 0xffffU;
    uint8_t address[6] = {};
    if (!decodeAddress(peerAddress, address)) return;

    for (uint8_t addressType = 0U; addressType <= 1U; ++addressType) {
        const uint16_t handle = ATT.connectionHandle(addressType, address);
        if (handle != 0xffffU) {
            connectionHandle_ = handle;
            return;
        }
    }
}

inline bool RenesasUnoBleSecuritySource::encrypted() const {
    return connectionHandle_ != 0xffffU && ATT.paired(connectionHandle_);
}

} // namespace blinker
