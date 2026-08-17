#ifndef BLINKER_PROVISIONING_BLELOCALPROVISIONINGENDPOINT_H
#define BLINKER_PROVISIONING_BLELOCALPROVISIONINGENDPOINT_H

#include "BleNoiseProvisioningChannel.h"
#include "../interface/ILocalProvisioningApplication.h"

namespace blinker {

struct BleLocalProvisioningMetrics {
    uint32_t requests;
    uint32_t responses;
    uint32_t backpressureEvents;
    uint32_t ignoredForeignSessionEvents;

    BleLocalProvisioningMetrics()
        : requests(0U),
          responses(0U),
          backpressureEvents(0U),
          ignoredForeignSessionEvents(0U) {}
};

// Provisioning endpoint owned by BleModeCoordinator. BleNoiseModePreparation
// arms the channel and begins the application session before start(). This
// endpoint binds both sides, preserves an exact response across TX
// backpressure, and turns active-session disconnects/faults into an explicit
// restart requirement. All retained storage is caller owned.
class BleLocalProvisioningEndpoint : public ITransportLifecycle {
public:
    BleLocalProvisioningEndpoint(
        BleNoiseProvisioningChannel& channel,
        ILocalProvisioningApplication& application,
        MutableByteSpan operationWorkspace,
        MutableByteSpan responseStorage);
    ~BleLocalProvisioningEndpoint();
    BleLocalProvisioningEndpoint(
        const BleLocalProvisioningEndpoint&) = delete;
    BleLocalProvisioningEndpoint& operator=(
        const BleLocalProvisioningEndpoint&) = delete;

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override;

    bool responsePending() const { return pendingResponse_; }
    bool recoveryRequired() const { return recoveryRequired_; }
    ErrorCode lastError() const { return lastError_; }
    uint32_t activeSessionId() const { return activeSessionId_; }
    const BleLocalProvisioningMetrics& metrics() const { return metrics_; }

private:
    static void payloadThunk(
        void* context,
        BleNoisePayloadKind kind,
        ByteView payload,
        const RxContext& rx);
    static void connectedThunk(void* context, const RxContext& rx);
    static void disconnectedThunk(void* context, const RxContext& rx);
    static void faultThunk(
        void* context,
        ErrorCode error,
        const RxContext& rx);

    void onPayload(
        BleNoisePayloadKind kind,
        ByteView payload,
        const RxContext& rx);
    void onConnected(const RxContext& rx);
    void onDisconnected(const RxContext& rx);
    void onFault(ErrorCode error, const RxContext& rx);
    Result sendResponse(BleNoisePayloadKind kind, ByteView response);
    void flushPending();
    void enterRecovery(ErrorCode error);
    void clearPending();
    void wipeWorkspaces();
    bool validConfiguration() const;

    BleNoiseProvisioningChannel& channel_;
    ILocalProvisioningApplication& application_;
    MutableByteSpan operationWorkspace_;
    MutableByteSpan responseStorage_;
    BleLocalProvisioningMetrics metrics_;
    size_t pendingResponseSize_;
    uint32_t activeSessionId_;
    ErrorCode lastError_;
    BleNoisePayloadKind pendingKind_;
    bool started_;
    bool pendingResponse_;
    bool recoveryRequired_;
};

} // namespace blinker

#endif
