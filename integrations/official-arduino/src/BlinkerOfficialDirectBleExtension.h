#ifndef BLINKER_OFFICIAL_DIRECT_BLE_EXTENSION_H
#define BLINKER_OFFICIAL_DIRECT_BLE_EXTENSION_H

#include <BlinkerV2Advanced.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Adds the Direct BLE data plane and its authenticated access-group control
// endpoint. Account/share ACLs remain server-side.
template <typename Platform>
class DirectBleExtension {
public:
    DirectBleExtension(
        Platform& platform,
        const DeviceInstanceId& deviceInstance,
        IClock& clock,
        IRandom& random)
        : access_(platform.deviceAccessStore()),
          rx_(),
          tx_(),
          packet_(),
          transport_(
              platform.bleLink(),
              clock,
              MutableByteSpan(rx_, sizeof(rx_)),
              MutableByteSpan(tx_, sizeof(tx_)),
              MutableByteSpan(packet_, sizeof(packet_)),
              transportConfig()),
          sessions_(),
          authorizer_(
              access_,
              random,
              sessions_,
              BLINKER_BLE_MAX_SESSIONS),
          grantVerifier_(platform.serverSignatureVerifier()),
          controlCoordinator_(
              deviceInstance,
              access_,
              access_,
              grantVerifier_),
          controlWorkspace_(),
          controlEndpoint_(
              controlCoordinator_,
              random,
              MutableByteSpan(
                  controlWorkspace_,
                  sizeof(controlWorkspace_))),
          client_(nullptr) {}

    Result attach(Client& client) {
        if (client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        Result result = client.addTransport(transport_);
        if (result) result = client.setAuthorizationProvider(&authorizer_);
        if (result) {
            result = client.setControllerControlEndpoint(&controlEndpoint_);
        }
        if (result) client_ = &client;
        return result;
    }

    IDeviceAccessStore* deviceAccessStore() { return &access_; }

    uint16_t capabilities() const {
        return ProductCapabilityDirectBleData;
    }

private:
    static BleFrameTransportConfig transportConfig() {
        BleFrameTransportConfig config;
        config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
        return config;
    }

    IDeviceAccessStore& access_;
    uint8_t rx_[BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_SESSIONS];
    uint8_t tx_[BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_TX_FRAMES];
    uint8_t packet_[Platform::maximumBlePacketSize];
    BleFrameTransport transport_;
    security::ControllerAuthSession sessions_[BLINKER_BLE_MAX_SESSIONS];
    security::ControllerHmacSha256Authorizer authorizer_;
    ControllerGrantVerifier grantVerifier_;
    ControllerControlCoordinator controlCoordinator_;
    uint8_t controlWorkspace_[kControllerControlWorkspaceSize];
    ControllerControlEndpoint controlEndpoint_;
    Client* client_;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
