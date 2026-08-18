#ifndef BLINKER_IDENTITY_CONFIGUREDDEVICEKEYSOURCE_H
#define BLINKER_IDENTITY_CONFIGUREDDEVICEKEYSOURCE_H

#include "../interface/IDeviceKeySource.h"

namespace blinker {

// Allocation-free DeviceKey source for the beginner/manual path. configure()
// decodes the 43-character public representation once; session refreshes use
// only the validated 32-byte value. The source does not own persistent
// storage and therefore conservatively reports PlainFlash protection.
class ConfiguredDeviceKeySource final : public IDeviceKeySource {
public:
    ConfiguredDeviceKeySource();
    ~ConfiguredDeviceKeySource() override;

    Result configure(StringView encoded);
    Result load(DeviceKey& output) override;
    StorageProtection protection() const override {
        return StorageProtection::PlainFlash;
    }

    bool configured() const { return configured_; }
    void clear();

private:
    DeviceKey key_;
    bool configured_;

    ConfiguredDeviceKeySource(const ConfiguredDeviceKeySource&);
    ConfiguredDeviceKeySource& operator=(
        const ConfiguredDeviceKeySource&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(ConfiguredDeviceKeySource) <= 40U,
    "ConfiguredDeviceKeySource exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
