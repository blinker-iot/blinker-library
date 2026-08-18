#ifndef BLINKER_RUNTIME_DEVICEKEYSESSIONARENA_H
#define BLINKER_RUNTIME_DEVICEKEYSESSIONARENA_H

#include "../control/DeviceKeySessionContract.h"
#include "../core/SecureMemory.h"

namespace blinker {

// Exact caller-owned scratch required by HttpDeviceKeySessionProvider. The
// buffers intentionally cannot alias, and credentials are wiped with the wire
// scratch when the composition is destroyed or explicitly reset.
class DeviceKeySessionArena {
public:
    DeviceKeySessionArena() : request_(), response_(), credentials_() {}
    ~DeviceKeySessionArena() { clear(); }

    MutableByteSpan requestBuffer() {
        return MutableByteSpan(request_, sizeof(request_));
    }
    MutableByteSpan responseBuffer() {
        return MutableByteSpan(response_, sizeof(response_));
    }
    MutableCharSpan credentialArena() {
        return MutableCharSpan(credentials_, sizeof(credentials_));
    }

    void clear() {
        secureZero(MutableByteSpan(request_, sizeof(request_)));
        secureZero(MutableByteSpan(response_, sizeof(response_)));
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(credentials_),
            sizeof(credentials_)));
    }

private:
    uint8_t request_[kDeviceKeySessionRequestMaxEncodedSize];
    uint8_t response_[kDeviceKeySessionResponseMaxEncodedSize];
    char credentials_[kDeviceKeySessionCredentialArenaSize];

    DeviceKeySessionArena(const DeviceKeySessionArena&);
    DeviceKeySessionArena& operator=(const DeviceKeySessionArena&);
};

static_assert(
    sizeof(DeviceKeySessionArena) == 1260U,
    "DeviceKeySessionArena changed its exact RAM budget");

} // namespace blinker

#endif
