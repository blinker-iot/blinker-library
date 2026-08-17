#ifndef BLINKER_TRANSPORT_BLEFRAMETRANSPORT_H
#define BLINKER_TRANSPORT_BLEFRAMETRANSPORT_H

#include "../protocol/bbp2/Frame.h"
#include "BleRecordTransport.h"

namespace blinker {

struct BleFrameTransportConfig {
    uint16_t maxFrameSize;
    uint32_t reassemblyTimeoutMillis;
    uint8_t maxPacketsPerPoll;

    BleFrameTransportConfig()
        : maxFrameSize(512U),
          reassemblyTimeoutMillis(2000U),
          maxPacketsPerPoll(4U) {}
};

// BBP/2 compatibility wrapper over the protocol-neutral BLE record transport.
// All fragment ordering, timeout, MTU and queue logic lives in one place.
class BleFrameTransport : public IFrameTransport {
public:
    BleFrameTransport(
        IBleLink& link,
        IClock& clock,
        MutableByteSpan rxStorage,
        MutableByteSpan txStorage,
        MutableByteSpan packetScratch,
        const BleFrameTransportConfig& config = BleFrameTransportConfig());

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override;
    TransportCapabilities capabilities() const override;
    Result send(ByteView frame, const SendTarget& target) override;
    void setReceiver(FrameReceiver receiver, void* context) override;
    void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context) override;

    size_t queuedFrameCount() const { return records_.queuedRecordCount(); }
    uint32_t droppedFragmentCount() const {
        return records_.droppedFragmentCount();
    }
    uint32_t completedFrameCount() const {
        return records_.completedRecordCount();
    }

private:
    static BleRecordFormat frameFormat();
    static BleRecordTransportConfig recordConfig(
        const BleFrameTransportConfig& config);
    static Result decodeFrameSize(ByteView prefix, size_t& frameSize);
    static Result validateFrame(ByteView frame);

    BleRecordTransport records_;
};

} // namespace blinker

#endif
