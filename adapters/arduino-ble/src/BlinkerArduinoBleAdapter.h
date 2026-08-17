#ifndef BLINKER_ARDUINO_BLE_ADAPTER_H
#define BLINKER_ARDUINO_BLE_ADAPTER_H

#include <ArduinoBLE.h>
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
    bool requireEncryption;

    ArduinoBleLinkConfig()
        : deviceName("BlinkerV2"),
          serviceUuid(ble::kServiceUuid),
          receiveUuid(ble::kReceiveUuid),
          transmitUuid(ble::kTransmitUuid),
          maxPacketSize(20),
          requireEncryption(false) {}
};

// ArduinoBLE's public BLE.paired() reports whether any known peer is
// encrypted. A product that requires link encryption must therefore provide
// a platform probe which reports security for the exact connected central.
class IArduinoBleSecuritySource {
public:
    virtual ~IArduinoBleSecuritySource() {}
    virtual void connected(const BLEDevice& central) = 0;
    virtual void disconnected() = 0;
    virtual bool encrypted() const = 0;
    virtual bool bonded() const = 0;
};

class ArduinoBleLink : public IBleModeLink {
public:
    explicit ArduinoBleLink(
        const ArduinoBleLinkConfig& config = ArduinoBleLinkConfig());
    ArduinoBleLink(
        IArduinoBleSecuritySource& security,
        const ArduinoBleLinkConfig& config);

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    BleLinkState state() const override;
    ErrorCode lastError() const override;
    size_t sessionCount() const override;
    Result sessionAt(size_t index, BleSessionInfo& session) const override;
    Result sendPacket(uint32_t sessionId, ByteView packet) override;
    void setPacketReceiver(BlePacketReceiver receiver, void* context) override;
    void setSessionHandlers(
        BleSessionHandler connected,
        BleSessionHandler disconnected,
        void* context) override;
    Result configureBleProfile(const ble::ModeProfile& profile) override;
    ble::ModeProfile bleProfile() const override { return profile_; }

private:
    ArduinoBleLink(
        IArduinoBleSecuritySource* security,
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
    IArduinoBleSecuritySource* security_;
    BLEDevice central_;
    BleSessionInfo session_;
    BlePacketReceiver packetReceiver_;
    void* packetContext_;
    BleSessionHandler connectedHandler_;
    BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    uint32_t nextSessionId_;
    bool connectPending_;
    bool disconnectPending_;
    bool sessionAnnounced_;
    BleLinkState state_;
    ErrorCode lastError_;
    static ArduinoBleLink* active_;
};

} // namespace blinker

#endif
