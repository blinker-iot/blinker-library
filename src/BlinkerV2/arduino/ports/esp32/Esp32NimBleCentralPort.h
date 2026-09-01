#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLECENTRALPORT_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLECENTRALPORT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32NimBleCentralPort requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <BlinkerV2/interface/IBleCentralPort.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/protocol/ble/Gatt.h>
#include <BlinkerV2/protocol/ble/Mode.h>

#include "Esp32NimBleRuntime.h"

#if !defined(CONFIG_BT_NIMBLE_ENABLED) || !CONFIG_BT_NIMBLE_ENABLED
#error "Blinker BLE Central requires ESP-IDF NimBLE"
#endif
#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL) || \
    !CONFIG_BT_NIMBLE_ROLE_CENTRAL || \
    !defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER) || \
    !CONFIG_BT_NIMBLE_ROLE_OBSERVER
#error "Blinker BLE Central requires NimBLE Central and Observer roles"
#endif

#include <freertos/FreeRTOS.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>

#ifndef BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE
#define BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE 20
#endif

#ifndef BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH
#define BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH 4
#endif

#ifndef BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH
#define BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH 4
#endif

namespace blinker {

struct Esp32NimBleCentralPortConfig {
    uint32_t hostReadyTimeoutMillis;
    uint32_t hostStopTimeoutMillis;
    uint32_t nativeConnectTimeoutMillis;
    uint8_t maxScanResultsPerPoll;
    uint8_t maxRxPacketsPerPoll;

    Esp32NimBleCentralPortConfig()
        : hostReadyTimeoutMillis(2000U),
          hostStopTimeoutMillis(1000U),
          nativeConnectTimeoutMillis(12000U),
          maxScanResultsPerPoll(4U),
          maxRxPacketsPerPoll(4U) {}
};

// One outbound NimBLE connection with fixed-size application queues. The IDF
// host still owns its internal heap. Native addresses and GATT handles never
// leave this Port; logical identity is selected only by the caller's
// authorized service-data matcher.
class Esp32NimBleCentralPort final : public IBleCentralPort {
public:
    explicit Esp32NimBleCentralPort(
        const Esp32NimBleCentralPortConfig& config =
            Esp32NimBleCentralPortConfig());
    ~Esp32NimBleCentralPort() override { stop(); }

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    BleCentralPortState state() const override { return state_; }
    ErrorCode lastError() const override { return lastError_; }
    Result connect(const BleCentralConnectRequest& request) override;
    Result cancel(uint32_t attemptId) override;
    Result connection(BleCentralConnectionInfo& output) const override;
    Result sendPacket(uint32_t attemptId, ByteView packet) override;
    void setPacketReceiver(
        BleCentralPacketReceiver receiver,
        void* context) override;
    void setConnectionHandlers(
        BleCentralConnectionHandler connected,
        BleCentralConnectionHandler disconnected,
        void* context) override;

private:
    struct ScanCandidate {
        ble_addr_t address;
        int16_t rssi;
        uint8_t serviceData[ble::kModeServiceDataSize];

        ScanCandidate() : address(), rssi(0), serviceData() {}
    };

    struct QueuedPacket {
        uint16_t connectionHandle;
        uint8_t size;
        uint8_t data[BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE];

        QueuedPacket()
            : connectionHandle(BLE_HS_CONN_HANDLE_NONE), size(0U), data() {}
    };

    static Esp32NimBleCentralPort*& activePort();
    static void hostTask(void* context);
    static void onHostReset(int reason);
    static void onHostSync();
    static int onGapEvent(ble_gap_event* event, void* context);
    static int onService(
        uint16_t connectionHandle,
        const ble_gatt_error* error,
        const ble_gatt_svc* service,
        void* context);
    static int onReceiveCharacteristic(
        uint16_t connectionHandle,
        const ble_gatt_error* error,
        const ble_gatt_chr* characteristic,
        void* context);
    static int onTransmitCharacteristic(
        uint16_t connectionHandle,
        const ble_gatt_error* error,
        const ble_gatt_chr* characteristic,
        void* context);
    static int onDescriptor(
        uint16_t connectionHandle,
        const ble_gatt_error* error,
        uint16_t characteristicValueHandle,
        const ble_gatt_dsc* descriptor,
        void* context);
    static int onSubscribe(
        uint16_t connectionHandle,
        const ble_gatt_error* error,
        ble_gatt_attr* attribute,
        void* context);

    void initializeUuids();
    bool validConfig() const;
    Result startScan();
    void processScanCandidates();
    void processNativeEvents();
    void drainPackets();
    void beginNativeConnect(const ScanCandidate& candidate);
    void beginDiscovery(uint16_t connectionHandle);
    void completeAttempt(ErrorCode error);
    void markHostResult(ErrorCode error);
    void markDiscoveryResult(ErrorCode error);
    int handleGapEvent(const ble_gap_event& event);
    bool queueCandidate(const ble_gap_disc_desc& discovery);
    bool queuePacket(
        uint16_t connectionHandle,
        const struct os_mbuf* buffer);
    bool currentNativeConnection(uint16_t connectionHandle) const;
    void clearQueuesLocked();
    void clearAttempt();

    Esp32NimBleCentralPortConfig config_;
    ble_uuid128_t serviceUuid_;
    ble_uuid128_t receiveUuid_;
    ble_uuid128_t transmitUuid_;
    BleCentralConnectRequest request_;
    BleCentralConnectionInfo connection_;
    BleCentralPacketReceiver packetReceiver_;
    void* packetContext_;
    BleCentralConnectionHandler connectedHandler_;
    BleCentralConnectionHandler disconnectedHandler_;
    void* connectionContext_;
    ScanCandidate candidates_[BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH];
    QueuedPacket packets_[BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH];
    uint8_t candidateHead_;
    uint8_t candidateTail_;
    uint8_t candidateCount_;
    uint8_t packetHead_;
    uint8_t packetTail_;
    uint8_t packetCount_;
    uint8_t ownAddressType_;
    uint16_t connectionHandle_;
    uint16_t pendingConnectionHandle_;
    uint16_t pendingDisconnectHandle_;
    uint16_t serviceStartHandle_;
    uint16_t serviceEndHandle_;
    uint16_t receiveHandle_;
    uint16_t transmitHandle_;
    uint16_t cccdHandle_;
    int pendingConnectStatus_;
    ErrorCode pendingHostError_;
    ErrorCode pendingDiscoveryError_;
    bool hostResultPending_;
    bool connectEventPending_;
    bool disconnectEventPending_;
    bool discoveryResultPending_;
    bool securityEventPending_;
    bool pendingEncrypted_;
    bool pendingBonded_;
    bool cancelCompletionPending_;
    bool initialized_;
    bool stopping_;
    BleCentralPortState state_;
    ErrorCode lastError_;
    portMUX_TYPE lock_;

    Esp32NimBleCentralPort(const Esp32NimBleCentralPort&);
    Esp32NimBleCentralPort& operator=(const Esp32NimBleCentralPort&);
};

} // namespace blinker

#include "Esp32NimBleCentralPort.ipp"

#endif
