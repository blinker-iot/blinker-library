#ifndef BLINKER_V2_ARDUINO_PORTS_BLE_LINK_H
#define BLINKER_V2_ARDUINO_PORTS_BLE_LINK_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <BlinkerV2/interface/IBleConnectionSecuritySource.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Gatt.h>
#include <BlinkerV2/protocol/ble/Fragment.h>

namespace blinker {

struct ArduinoBleLinkConfig {
    const char* deviceName;
    const char* serviceUuid;
    const char* receiveUuid;
    const char* transmitUuid;
    uint16_t maxPacketSize;
    uint32_t sessionReadyTimeoutMillis;
    bool requireEncryption;

    ArduinoBleLinkConfig()
        : deviceName("BlinkerV2"),
          serviceUuid(ble::kServiceUuid),
          receiveUuid(ble::kReceiveUuid),
          transmitUuid(ble::kTransmitUuid),
          maxPacketSize(20),
          sessionReadyTimeoutMillis(15000U),
          requireEncryption(false) {}
};

class ArduinoBleLink : public IBleModeLink {
public:
    explicit ArduinoBleLink(
        const ArduinoBleLinkConfig& config = ArduinoBleLinkConfig());
    ArduinoBleLink(
        IBleConnectionSecuritySource& security,
        const ArduinoBleLinkConfig& config);

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    BleLinkState state() const override;
    ErrorCode lastError() const override;
    size_t sessionCount() const override;
    Result sessionAt(size_t index, BleSessionInfo& session) const override;
    Result sendPacket(uint32_t sessionId, ByteView packet) override;
    Result disconnectSession(uint32_t sessionId) override;
    void setPacketReceiver(BlePacketReceiver receiver, void* context) override;
    void setSessionHandlers(
        BleSessionHandler connected,
        BleSessionHandler disconnected,
        void* context) override;
    Result configureBleProfile(const ble::ModeProfile& profile) override;
    ble::ModeProfile bleProfile() const override { return profile_; }

private:
    ArduinoBleLink(
        IBleConnectionSecuritySource* security,
        const ArduinoBleLinkConfig& config);
    static void connectedThunk(BLEDevice device);
    static void disconnectedThunk(BLEDevice device);
    void onConnected(BLEDevice device);
    void onDisconnected(BLEDevice device);
    void updateConnection();
    uint32_t nextSessionId();
    bool validConfig() const;

    ArduinoBleLinkConfig config_;
    ble::ModeProfile profile_;
    uint8_t modeScanResponse_[ble::kModeScanResponseSize];
    BLEService service_;
    BLECharacteristic receive_;
    BLECharacteristic transmit_;
    IBleConnectionSecuritySource* security_;
    BLEDevice central_;
    BleSessionInfo session_;
    BlePacketReceiver packetReceiver_;
    void* packetContext_;
    BleSessionHandler connectedHandler_;
    BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    uint32_t nextSessionId_;
    uint32_t connectedAtMillis_;
    bool connectPending_;
    bool disconnectPending_;
    bool sessionAnnounced_;
    BleLinkState state_;
    ErrorCode lastError_;
    static ArduinoBleLink*& active();
};

} // namespace blinker

#include "ArduinoBleLink.ipp"

#endif
