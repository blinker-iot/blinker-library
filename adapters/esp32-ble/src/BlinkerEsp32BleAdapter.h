#ifndef BLINKER_ESP32_BLE_ADAPTER_H
#define BLINKER_ESP32_BLE_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32BleAdapter requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Gatt.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <freertos/FreeRTOS.h>

#ifndef BLINKER_ESP32_BLE_MAX_PACKET_SIZE
#define BLINKER_ESP32_BLE_MAX_PACKET_SIZE 244
#endif

#ifndef BLINKER_ESP32_BLE_RX_QUEUE_DEPTH
#define BLINKER_ESP32_BLE_RX_QUEUE_DEPTH 4
#endif

namespace blinker {

struct Esp32BleLinkConfig {
    const char* deviceName;
    const char* serviceUuid;
    const char* receiveUuid;
    const char* transmitUuid;
    uint16_t preferredMtu;
    uint8_t maxRxPacketsPerPoll;
    bool bonding;

    Esp32BleLinkConfig()
        : deviceName("BlinkerV2"),
          serviceUuid(ble::kServiceUuid),
          receiveUuid(ble::kReceiveUuid),
          transmitUuid(ble::kTransmitUuid),
          preferredMtu(247),
          maxRxPacketsPerPoll(4),
          bonding(true) {}
};

// This adapter owns the ESP32 Arduino BLE singleton while it is started.
// It admits one peer and publishes a Device V2 session only after the BLE
// stack reports encryption and the peer subscribes to notifications.
class Esp32BleLink : public IBleModeLink,
                     private BLEServerCallbacks,
                     private BLECharacteristicCallbacks,
                     private BLESecurityCallbacks {
public:
    explicit Esp32BleLink(
        const Esp32BleLinkConfig& config = Esp32BleLinkConfig());

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
    enum : uint16_t { invalidConnectionHandle = 0xFFFFU };

    struct QueuedPacket {
        uint16_t connectionHandle;
        uint16_t size;
        uint8_t data[BLINKER_ESP32_BLE_MAX_PACKET_SIZE];

        QueuedPacket()
            : connectionHandle(invalidConnectionHandle), size(0), data() {}
    };

    void onWrite(BLECharacteristic* characteristic) override;
#if defined(CONFIG_BLUEDROID_ENABLED)
    void onConnect(
        BLEServer* server,
        esp_ble_gatts_cb_param_t* parameter) override;
    void onDisconnect(
        BLEServer* server,
        esp_ble_gatts_cb_param_t* parameter) override;
    void onMtuChanged(
        BLEServer* server,
        esp_ble_gatts_cb_param_t* parameter) override;
    void onWrite(
        BLECharacteristic* characteristic,
        esp_ble_gatts_cb_param_t* parameter) override;
    void onAuthenticationComplete(esp_ble_auth_cmpl_t result) override;
#endif
#if defined(CONFIG_NIMBLE_ENABLED)
    void onConnect(
        BLEServer* server,
        ble_gap_conn_desc* description) override;
    void onDisconnect(
        BLEServer* server,
        ble_gap_conn_desc* description) override;
    void onMtuChanged(
        BLEServer* server,
        ble_gap_conn_desc* description,
        uint16_t mtu) override;
    void onWrite(
        BLECharacteristic* characteristic,
        ble_gap_conn_desc* description) override;
    void onSubscribe(
        BLECharacteristic* characteristic,
        ble_gap_conn_desc* description,
        uint16_t subscription) override;
    void onAuthenticationComplete(ble_gap_conn_desc* description) override;
#endif

    void queueMtu(uint16_t connectionHandle, uint16_t mtu);
    void queuePacket(
        BLECharacteristic* characteristic,
        uint16_t connectionHandle);
    void processPendingEvents();
    void drainPackets();
    void clearPacketQueueLocked();
    void refreshSubscription();
    void announceSessionIfReady();
    uint32_t nextSessionId();
    static uint16_t packetSizeForMtu(uint16_t mtu);
    bool validConfig() const;

    Esp32BleLinkConfig config_;
    ble::ModeProfile profile_;
    BLEServer* server_;
    BLEService* service_;
    BLECharacteristic* receive_;
    BLECharacteristic* transmit_;
    BLE2902* transmitDescriptor_;
    BLEAdvertising* advertising_;
    BleSessionInfo session_;
    BlePacketReceiver packetReceiver_;
    void* packetContext_;
    BleSessionHandler connectedHandler_;
    BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    QueuedPacket packets_[BLINKER_ESP32_BLE_RX_QUEUE_DEPTH];
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
#if defined(CONFIG_BLUEDROID_ENABLED)
    uint8_t pendingRemoteAddress_[ESP_BD_ADDR_LEN];
#endif
    BleLinkState state_;
    ErrorCode lastError_;
    portMUX_TYPE lock_;
};

} // namespace blinker

#endif
