#ifndef BLINKER_ESP32_NIMBLE_ADAPTER_H
#define BLINKER_ESP32_NIMBLE_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32NimBleAdapter requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Gatt.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>

#ifndef BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE
#define BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE 244
#endif

#ifndef BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH
#define BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH 4
#endif

namespace blinker {

struct Esp32NimBleLinkConfig {
    const char* deviceName;
    const char* serviceUuid;
    const char* receiveUuid;
    const char* transmitUuid;
    uint16_t preferredMtu;
    uint8_t maxRxPacketsPerPoll;
    bool bonding;

    Esp32NimBleLinkConfig()
        : deviceName("BlinkerV2"),
          serviceUuid(ble::kServiceUuid),
          receiveUuid(ble::kReceiveUuid),
          transmitUuid(ble::kTransmitUuid),
          preferredMtu(247),
          maxRxPacketsPerPoll(4),
          bonding(true) {}
};

// The adapter intentionally admits one active peer. NimBLE may be configured
// with more controller connections, but Device V2 only promotes a connection
// to a session after poll() observes its bounded callback event.
class Esp32NimBleLink : public IBleModeLink,
                        private NimBLEServerCallbacks,
                        private NimBLECharacteristicCallbacks {
public:
    explicit Esp32NimBleLink(
        const Esp32NimBleLinkConfig& config = Esp32NimBleLinkConfig());

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

    Result setSessionSecurity(
        uint32_t sessionId,
        bool encrypted,
        bool bonded,
        bool authenticated);

private:
    struct QueuedPacket {
        uint16_t connectionHandle;
        uint16_t size;
        uint8_t data[BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE];

        QueuedPacket() : connectionHandle(BLE_HS_CONN_HANDLE_NONE), size(0), data() {}
    };

    void onConnect(
        NimBLEServer* server,
        NimBLEConnInfo& connection) override;
    void onDisconnect(
        NimBLEServer* server,
        NimBLEConnInfo& connection,
        int reason) override;
    void onMTUChange(
        uint16_t mtu,
        NimBLEConnInfo& connection) override;
    void onAuthenticationComplete(
        NimBLEConnInfo& connection) override;
    void onWrite(
        NimBLECharacteristic* characteristic,
        NimBLEConnInfo& connection) override;
    void onSubscribe(
        NimBLECharacteristic* characteristic,
        NimBLEConnInfo& connection,
        uint16_t subscription) override;

    void processPendingEvents();
    void drainPackets();
    void clearPacketQueueLocked();
    uint32_t nextSessionId();
    static uint16_t packetSizeForMtu(uint16_t mtu);
    bool validConfig() const;

    Esp32NimBleLinkConfig config_;
    ble::ModeProfile profile_;
    NimBLEServer* server_;
    NimBLEService* service_;
    NimBLECharacteristic* receive_;
    NimBLECharacteristic* transmit_;
    NimBLEAdvertising* advertising_;
    BleSessionInfo session_;
    BlePacketReceiver packetReceiver_;
    void* packetContext_;
    BleSessionHandler connectedHandler_;
    BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    QueuedPacket packets_[BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH];
    uint8_t packetHead_;
    uint8_t packetTail_;
    uint8_t packetCount_;
    uint16_t connectionHandle_;
    uint16_t pendingConnectHandle_;
    uint16_t pendingDisconnectHandle_;
    uint16_t pendingMtuHandle_;
    uint16_t pendingSubscribeHandle_;
    uint16_t pendingSecurityHandle_;
    uint16_t pendingPacketSize_;
    uint32_t nextSessionId_;
    bool pendingNotifyEnabled_;
    bool pendingEncrypted_;
    bool pendingBonded_;
    bool connectPending_;
    bool disconnectPending_;
    bool mtuPending_;
    bool subscribePending_;
    bool securityPending_;
    bool sessionAnnounced_;
    BleLinkState state_;
    ErrorCode lastError_;
    portMUX_TYPE lock_;
};

} // namespace blinker

#endif
