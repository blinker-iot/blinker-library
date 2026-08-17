#include "BleFrameTransport.h"

namespace blinker {

BleFrameTransport::BleFrameTransport(
    IBleLink& link,
    IClock& clock,
    MutableByteSpan rxStorage,
    MutableByteSpan txStorage,
    MutableByteSpan packetScratch,
    const BleFrameTransportConfig& config)
    : records_(
          link,
          clock,
          frameFormat(),
          rxStorage,
          txStorage,
          packetScratch,
          recordConfig(config)) {}

Result BleFrameTransport::start() {
    return records_.start();
}

void BleFrameTransport::stop() {
    records_.stop();
}

void BleFrameTransport::poll(uint32_t budgetMicros) {
    records_.poll(budgetMicros);
}

TransportState BleFrameTransport::state() const {
    return records_.state();
}

TransportCapabilities BleFrameTransport::capabilities() const {
    TransportCapabilities result;
    result.maxFrameSize = records_.maxRecordSize();
    result.features = TransportFeatureBinary |
                      TransportFeatureLocal |
                      TransportFeatureFragmented;
    return result;
}

Result BleFrameTransport::send(
    ByteView frame,
    const SendTarget& target) {
    return records_.send(frame, target);
}

void BleFrameTransport::setReceiver(
    FrameReceiver receiver,
    void* context) {
    records_.setReceiver(receiver, context);
}

void BleFrameTransport::setSessionHandlers(
    FrameSessionHandler connected,
    FrameSessionHandler disconnected,
    void* context) {
    records_.setSessionHandlers(connected, disconnected, context);
}

BleRecordFormat BleFrameTransport::frameFormat() {
    BleRecordFormat format;
    format.minimumHeaderSize = bbp2::kBaseHeaderSize;
    format.decodeSize = &BleFrameTransport::decodeFrameSize;
    format.validate = &BleFrameTransport::validateFrame;
    return format;
}

BleRecordTransportConfig BleFrameTransport::recordConfig(
    const BleFrameTransportConfig& config) {
    BleRecordTransportConfig result;
    result.maxRecordSize = config.maxFrameSize;
    result.reassemblyTimeoutMillis = config.reassemblyTimeoutMillis;
    result.maxPacketsPerPoll = config.maxPacketsPerPoll;
    return result;
}

Result BleFrameTransport::decodeFrameSize(
    ByteView prefix,
    size_t& frameSize) {
    frameSize = 0U;
    bbp2::FrameHeader header;
    Result result = bbp2::decodeHeader(prefix, header);
    if (!result) return result;
    frameSize = bbp2::encodedFrameSize(header);
    return Result::success();
}

Result BleFrameTransport::validateFrame(ByteView frame) {
    bbp2::FrameView parsed;
    return bbp2::parseFrame(frame, parsed);
}

} // namespace blinker
