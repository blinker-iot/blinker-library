#ifndef BLINKER_OFFICIAL_BLE_PRODUCT_H
#define BLINKER_OFFICIAL_BLE_PRODUCT_H

#include <BlinkerV2/interface/IProductLifecycle.h>
#include <BlinkerV2/core/ResourceProfile.h>
#include <BlinkerV2/provisioning/BleNoiseProvisioningChannel.h>
#include <BlinkerV2/transport/BleFrameTransport.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Provisioning and Direct never run together on one BLE link, so their
// record/ATT buffers alias instead of reserving two independent workspaces.
template <
    typename Platform,
    size_t OperationWorkspaceSize,
    size_t ResponseWorkspaceSize>
class BleWorkspace {
public:
    enum : size_t {
        directRxSize =
            BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_SESSIONS,
        directTxSize =
            BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_TX_FRAMES,
        sharedRxSize = directRxSize > kBleNoiseRecordSize
                           ? directRxSize
                           : kBleNoiseRecordSize,
        sharedTxSize = directTxSize > kBleNoiseRecordSize
                           ? directTxSize
                           : kBleNoiseRecordSize,
        packetSize = Platform::maximumBlePacketSize
    };

    MutableByteSpan noiseRx() {
        return MutableByteSpan(recordRx_, kBleNoiseRecordSize);
    }
    MutableByteSpan noiseTx() {
        return MutableByteSpan(recordTx_, kBleNoiseRecordSize);
    }
    MutableByteSpan directRx() {
        return MutableByteSpan(recordRx_, sizeof(recordRx_));
    }
    MutableByteSpan directTx() {
        return MutableByteSpan(recordTx_, sizeof(recordTx_));
    }
    MutableByteSpan packet() {
        return MutableByteSpan(packet_, sizeof(packet_));
    }
    MutableByteSpan plaintext() {
        return MutableByteSpan(plaintext_, sizeof(plaintext_));
    }
    MutableByteSpan operation() {
        return MutableByteSpan(operation_, sizeof(operation_));
    }
    MutableByteSpan response() {
        return MutableByteSpan(response_, sizeof(response_));
    }

private:
    uint8_t recordRx_[sharedRxSize];
    uint8_t recordTx_[sharedTxSize];
    uint8_t packet_[packetSize];
    uint8_t plaintext_[kBleNoiseMaxTransportPayloadSize];
    uint8_t operation_[OperationWorkspaceSize];
    uint8_t response_[ResponseWorkspaceSize];
};

inline BleFrameTransportConfig directBleTransportConfig() {
    BleFrameTransportConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

class ILocalBleComposition : public IProductLifecycle {
public:
    virtual Result openPairingMode() = 0;
    virtual Result cancelPairingMode() = 0;
    virtual Result confirmPhysicalPresence() = 0;
};

// Internal package/resource seams. The public BLE facade will bind one
// canonical contract after the remaining provisioning decision is frozen.
IProductLifecycle& esp32PlatformBleLifecycle();
ILocalBleComposition& esp32LocalBleLifecycle();
IProductLifecycle& renesasUnoPlatformBleLifecycle();
ILocalBleComposition& renesasUnoLocalBleLifecycle();

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
