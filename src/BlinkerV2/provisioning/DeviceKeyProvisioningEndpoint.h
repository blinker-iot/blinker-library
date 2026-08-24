#ifndef BLINKER_PROVISIONING_DEVICEKEYPROVISIONINGENDPOINT_H
#define BLINKER_PROVISIONING_DEVICEKEYPROVISIONINGENDPOINT_H

#include "../identity/DeviceInstanceId.h"
#include "../interface/IDeviceAccessStore.h"
#include "../interface/IDeviceKeyStore.h"

namespace blinker {

enum class DeviceKeyProvisioningOperation : uint8_t {
    GetInfo = 1U,
    Install = 2U,
    Bootstrap = 3U
};

enum class DeviceKeyProvisioningStatus : uint8_t {
    Success = 0U,
    Malformed = 1U,
    UnsupportedVersion = 2U,
    UnsupportedOperation = 3U,
    InvalidDeviceKey = 4U,
    StorageFailure = 5U,
    Conflict = 6U,
    InvalidController = 7U,
    AccessRequired = 8U
};

enum : size_t {
    kDeviceKeyProvisioningVersion = 2U,
    kDeviceKeyProvisioningInfoRequestSize = 2U,
    kDeviceKeyProvisioningInstallRequestSize = 2U + kDeviceKeyTextSize,
    kDeviceKeyProvisioningBootstrapRequestSize =
        2U + kDeviceKeyTextSize + 4U + kControllerIdSize + 4U +
        kControllerCredentialSecretSize,
    kDeviceKeyProvisioningStatusResponseSize = 3U,
    kDeviceKeyProvisioningInfoResponseSize =
        4U + kDeviceInstanceIdSize + 4U,
    kDeviceKeyProvisioningMaxResponseSize =
        kDeviceKeyProvisioningInfoResponseSize
};

enum : uint8_t {
    kDeviceKeyProvisioningHasDeviceKey = 1U << 0U,
    kDeviceKeyProvisioningSupportsAccessBootstrap = 1U << 1U,
    kDeviceKeyProvisioningHasAccessState = 1U << 2U
};

// Transport-neutral endpoint used inside an already protected physical setup
// session. It only exposes the public instance id and installs one DeviceKey.
// Combined cloud/direct products bootstrap DeviceKey + accessEpoch + Admin
// atomically.
class DeviceKeyProvisioningEndpoint {
public:
    DeviceKeyProvisioningEndpoint(
        const DeviceInstanceId& instance,
        IDeviceKeyStore& keys)
        : instance_(instance), keys_(keys), access_(nullptr),
          installed_(false), validComposition_(true) {}
    DeviceKeyProvisioningEndpoint(
        const DeviceInstanceId& instance,
        IDeviceKeyStore& keys,
        IDeviceAccessStore* access)
        : instance_(instance), keys_(keys), access_(access),
          installed_(false), validComposition_(
              access == nullptr ||
              &keys == static_cast<IDeviceKeyStore*>(access)) {}

    Result handle(
        ByteView request,
        MutableByteSpan response,
        size_t& written);

    void reset() { installed_ = false; }
    bool installed() const { return installed_; }

private:
    Result getInfo(MutableByteSpan response, size_t& written);
    Result install(
        ByteView encodedKey,
        MutableByteSpan response,
        size_t& written);
    Result bootstrap(
        ByteView request,
        MutableByteSpan response,
        size_t& written);
    Result commitKey(
        uint8_t operation,
        const DeviceKey& candidate,
        MutableByteSpan response,
        size_t& written);
    static Result writeStatus(
        uint8_t operation,
        DeviceKeyProvisioningStatus status,
        MutableByteSpan response,
        size_t& written);

    const DeviceInstanceId& instance_;
    IDeviceKeyStore& keys_;
    IDeviceAccessStore* access_;
    bool installed_;
    bool validComposition_;

    DeviceKeyProvisioningEndpoint(
        const DeviceKeyProvisioningEndpoint&);
    DeviceKeyProvisioningEndpoint& operator=(
        const DeviceKeyProvisioningEndpoint&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(DeviceKeyProvisioningEndpoint) <= 16U,
    "DeviceKey provisioning endpoint exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
