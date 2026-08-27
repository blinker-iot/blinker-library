#ifndef BLINKER_TRANSPORT_TRANSPORTHUB_H
#define BLINKER_TRANSPORT_TRANSPORTHUB_H

#include "IFrameTransport.h"
#include "../core/ResourceProfile.h"

namespace blinker {

enum class TransportLifecyclePolicy : uint8_t {
    Managed = 0U,
    External
};

class TransportHub {
public:
    TransportHub();

    Result addTransport(
        IFrameTransport& transport,
        TransportLifecyclePolicy lifecycle =
            TransportLifecyclePolicy::Managed);
    Result removeTransport(IFrameTransport& transport);

    Result startAll();
    void stopAll();
    void poll(uint32_t totalBudgetMicros);

    Result send(ByteView frame, const SendTarget& target);
    Result broadcast(ByteView frame);
    Result disconnectSession(uint8_t transportId, uint32_t sessionId);

    void setReceiver(FrameReceiver receiver, void* context);
    void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context);

    size_t size() const { return count_; }
    IFrameTransport* at(size_t index) const;

private:
    struct ReceiverContext {
        TransportHub* hub;
        uint8_t transportId;

        ReceiverContext() : hub(nullptr), transportId(0) {}
    };

    static void receiveThunk(
        void* context,
        ByteView frame,
        const RxContext& rx);
    static void connectedThunk(void* context, const RxContext& rx);
    static void disconnectedThunk(void* context, const RxContext& rx);

    void receive(
        uint8_t transportId,
        ByteView frame,
        const RxContext& rx);
    void sessionConnected(uint8_t transportId, const RxContext& rx);
    void sessionDisconnected(uint8_t transportId, const RxContext& rx);

    IFrameTransport* transports_[BLINKER_MAX_TRANSPORTS];
    TransportLifecyclePolicy lifecycles_[BLINKER_MAX_TRANSPORTS];
    ReceiverContext receiverContexts_[BLINKER_MAX_TRANSPORTS];
    size_t count_;
    size_t nextPoll_;
    FrameReceiver receiver_;
    void* receiverContext_;
    FrameSessionHandler sessionConnected_;
    FrameSessionHandler sessionDisconnected_;
    void* sessionContext_;
};

} // namespace blinker

#endif
