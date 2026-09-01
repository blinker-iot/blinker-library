#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLE_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLE_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32NimBleLink requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/protocol/ble/Gatt.h>

#include "Esp32NimBleRuntime.h"

#if !defined(CONFIG_BT_NIMBLE_ENABLED) || !CONFIG_BT_NIMBLE_ENABLED
#error "Blinker BLE requires an Arduino-ESP32 target built with ESP-IDF NimBLE"
#endif

#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include <freertos/FreeRTOS.h>

// BBP/2 BLE v1 fixes the bearer packet at the default ATT payload. An MTU
// change alone is not a protocol negotiation and must not alter fragmentation.
#ifndef BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE
#define BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE 20
#endif

#ifndef BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH
#define BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH 4
#endif

namespace blinker {

struct Esp32NimBleLinkConfig {
    const char* deviceName;
    uint32_t sessionReadyTimeoutMillis;
    uint8_t maxRxPacketsPerPoll;

    Esp32NimBleLinkConfig()
        : deviceName(ble::kLocalName),
          sessionReadyTimeoutMillis(15000U),
          maxRxPacketsPerPoll(4U) {}
};

// ESP32 BLE uses the NimBLE Host shipped with ESP-IDF. Keeping the Port on the
// IDF C API lets WiFiProv and Direct BLE share one implementation and avoids a
// second, symbol-conflicting BLE runtime in the same firmware.
class Esp32NimBleLink : public IBleModeLink {
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
    Result disconnectSession(uint32_t sessionId) override;
    void setPacketReceiver(BlePacketReceiver receiver, void* context) override;
    void setSessionHandlers(
        BleSessionHandler connected,
        BleSessionHandler disconnected,
        void* context) override;
    Result configureBleProfile(const ble::ModeProfile& profile) override;
    ble::ModeProfile bleProfile() const override { return profile_; }

private:
    enum : size_t {
        modeServiceDataSize = 16U + ble::kModeServiceDataSize
    };

    struct QueuedPacket {
        uint16_t connectionHandle;
        uint16_t size;
        uint8_t data[BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE];

        QueuedPacket()
            : connectionHandle(BLE_HS_CONN_HANDLE_NONE), size(0U), data() {}
    };

    static Esp32NimBleLink*& activeLink();
    static void hostTask(void* context);
    static void onHostReset(int reason);
    static void onHostSync();
    static int onGapEvent(ble_gap_event* event, void* context);
    static int onGattAccess(
        uint16_t connectionHandle,
        uint16_t attributeHandle,
        ble_gatt_access_ctxt* context,
        void* owner);

    void initializeUuids();
    void initializeGatt();
    Result startAdvertising();
    void markHostResult(ErrorCode error);
    int handleGapEvent(const ble_gap_event& event);
    int handleGattAccess(
        uint16_t connectionHandle,
        uint16_t attributeHandle,
        ble_gatt_access_ctxt& context);
    bool queuePacket(
        uint16_t connectionHandle,
        const uint8_t* data,
        size_t size);
    void processPendingEvents();
    void drainPackets();
    void clearPacketQueueLocked();
    void resetRuntimeLocked();
    uint32_t nextSessionId();
    bool validConfig() const;

    Esp32NimBleLinkConfig config_;
    ble::ModeProfile profile_;
    ble_uuid128_t serviceUuid_;
    ble_uuid128_t receiveUuid_;
    ble_uuid128_t transmitUuid_;
    ble_gatt_chr_def characteristics_[3];
    ble_gatt_svc_def services_[2];
    uint8_t modeServiceData_[modeServiceDataSize];
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
    uint8_t ownAddressType_;
    uint16_t receiveHandle_;
    uint16_t transmitHandle_;
    uint16_t connectionHandle_;
    uint16_t pendingConnectHandle_;
    uint16_t pendingDisconnectHandle_;
    uint16_t pendingSubscribeHandle_;
    uint16_t pendingSecurityHandle_;
    uint32_t nextSessionId_;
    uint32_t connectedAtMillis_;
    ErrorCode pendingHostError_;
    bool pendingNotifyEnabled_;
    bool pendingEncrypted_;
    bool pendingBonded_;
    bool connectPending_;
    bool disconnectPending_;
    bool subscribePending_;
    bool securityPending_;
    bool hostResultPending_;
    bool hostReady_;
    bool initialized_;
    bool stopping_;
    bool sessionAnnounced_;
    BleLinkState state_;
    ErrorCode lastError_;
    portMUX_TYPE lock_;
};

} // namespace blinker

#include "Esp32NimBle.ipp"

#endif
