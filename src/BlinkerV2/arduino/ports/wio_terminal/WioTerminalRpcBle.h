#ifndef BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_RPC_BLE_H
#define BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_RPC_BLE_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalRpcBle requires a Wio Terminal target"
#endif

#include <Arduino.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/protocol/ble/Gatt.h>

#include <rpcBLEDevice.h>
#include <BLE2902.h>

namespace blinker {

struct WioTerminalRpcBleConfig {
    const char* deviceName;
    uint32_t sessionReadyTimeoutMillis;
    uint8_t maxRxPacketsPerPoll;

    WioTerminalRpcBleConfig()
        : deviceName(ble::kLocalName),
          sessionReadyTimeoutMillis(15000U),
          maxRxPacketsPerPoll(4U) {}
};

class WioTerminalRpcBleLink final : public IBleModeLink,
                                    private BLEServerCallbacks,
                                    private BLECharacteristicCallbacks {
public:
    enum : size_t {
        maximumPacketSize = 20U,
        // The 384-byte enrollment payload plus Noise/record overhead needs
        // at most 26 minimum-MTU fragments. rpcBLE can deliver that burst
        // before the SAMD loop drains it, so retain one complete record.
        maximumQueuedPackets = 32U
    };

    explicit WioTerminalRpcBleLink(
        const WioTerminalRpcBleConfig& config =
            WioTerminalRpcBleConfig());

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    BleLinkState state() const override { return state_; }
    ErrorCode lastError() const override { return lastError_; }
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
    struct QueuedPacket {
        uint8_t size;
        uint8_t data[maximumPacketSize];

        QueuedPacket() : size(0U), data() {}
    };

    void onConnect(BLEServer* server) override;
    void onDisconnect(BLEServer* server) override;
    void onWrite(BLECharacteristic* characteristic) override;
    void onStatus(
        BLECharacteristic* characteristic,
        Status status,
        uint32_t code) override;
    Result configureAdvertising();
    void advanceAdvertising();
    Result requestDisconnect();
    void reconcileConnection();
    void processConnection();
    void updateSessionReadiness();
    void drainPackets();
    void clearPackets();
    uint32_t nextSessionId();
    bool validConfig() const;

    WioTerminalRpcBleConfig config_;
    ble::ModeProfile profile_;
    BLEServer* server_;
    BLEService* service_;
    BLECharacteristic* receive_;
    BLECharacteristic* transmit_;
    BLE2902* notifyDescriptor_;
    BleSessionInfo session_;
    BlePacketReceiver packetReceiver_;
    void* packetContext_;
    BleSessionHandler connectedHandler_;
    BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    QueuedPacket packets_[maximumQueuedPackets];
    uint8_t packetHead_;
    uint8_t packetTail_;
    uint8_t packetCount_;
    uint16_t pendingConnectionId_;
    uint16_t connectionId_;
    uint32_t nextSessionId_;
    uint32_t connectedAtMillis_;
    uint32_t nextConnectionReconcileAt_;
    bool connectPending_;
    bool disconnectPending_;
    bool sessionAnnounced_;
    bool gattStarted_;
    bool advertisingConfigured_;
    uint32_t advertisingDeadlineAt_;
    uint32_t nextAdvertisingAttemptAt_;
    volatile bool rxObserved_;
    volatile bool notifySucceeded_;
    BleLinkState state_;
    ErrorCode lastError_;
    BLEFreeRTOS::Semaphore lock_;

    WioTerminalRpcBleLink(const WioTerminalRpcBleLink&);
    WioTerminalRpcBleLink& operator=(const WioTerminalRpcBleLink&);
};

} // namespace blinker

#include "WioTerminalRpcBle.ipp"

#endif
