#include "ConfiguredDeviceKeySource.h"

namespace blinker {

ConfiguredDeviceKeySource::ConfiguredDeviceKeySource()
    : key_(), configured_(false) {}

ConfiguredDeviceKeySource::~ConfiguredDeviceKeySource() {
    clear();
}

Result ConfiguredDeviceKeySource::configure(StringView encoded) {
    DeviceKey decoded;
    const Result result = decodeDeviceKey(encoded, decoded);
    if (result) {
        clearDeviceKey(key_);
        key_ = decoded;
        configured_ = true;
    }
    clearDeviceKey(decoded);
    return result;
}

Result ConfiguredDeviceKeySource::load(DeviceKey& output) {
    if (!configured_) return Result::failure(ErrorCode::NotFound);
    output = key_;
    return Result::success();
}

void ConfiguredDeviceKeySource::clear() {
    clearDeviceKey(key_);
    configured_ = false;
}

} // namespace blinker
