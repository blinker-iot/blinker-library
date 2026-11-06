#ifndef BLINKER_TRANSPORT_MANAGEDMQTTTRANSPORT_H
#define BLINKER_TRANSPORT_MANAGEDMQTTTRANSPORT_H

#include "../interface/ISessionProvider.h"
#include "MqttFrameTransport.h"

namespace blinker {

// Re-evaluated after servicing the current socket; no Task dependency or
// retained callback. The caller owns its existing bounded exchange window.
typedef bool (*SessionRefreshGate)(void* context);

// Wraps the control-plane and MQTT data-plane as one transport. This lets a
// TransportHub start BLE/local transports immediately while session exchange
// is in progress. Authentication remains entirely inside ISessionProvider.
class ManagedMqttTransport : public IFrameTransport {
public:
    ManagedMqttTransport(
        ISessionProvider& sessionProvider,
        MqttFrameTransport& mqttTransport);

    void setNetworkAvailable(bool available);
    bool networkAvailable() const { return networkAvailable_; }
    ErrorCode lastError() const;

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    void poll(uint32_t budgetMicros, SessionRefreshGate gate, void* context);
    TransportState state() const override;
    TransportCapabilities capabilities() const override;
    Result send(ByteView frame, const SendTarget& target) override;
    void setReceiver(FrameReceiver receiver, void* context) override;
    void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context) override;

private:
    bool canHoldCurrentSession() const;
    Result applyCredentials();

    ISessionProvider& sessionProvider_;
    MqttFrameTransport& mqttTransport_;
    uint32_t appliedGeneration_;
    bool started_;
    bool networkAvailable_;
    bool refreshRequested_;
    ErrorCode bindingError_;
};

} // namespace blinker

#endif
