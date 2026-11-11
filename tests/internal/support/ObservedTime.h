#ifndef BLINKER_TEST_OBSERVED_TIME_H
#define BLINKER_TEST_OBSERVED_TIME_H

#include <BlinkerV2/runtime/TimeSyncService.h>
#include <string.h>

namespace blinker_test {
// Optional bounded wire observation. No retained frame/secret or changed reply.
struct TimeExchangeTrace {
    uint16_t requestSequence = 0U, responseSequence = 0U;
    size_t requestBodyBytes = 0U, responseFrameBytes = 0U;
    uint32_t sourceUncertaintyMs = 0U;
    bool correlated = false;
    uint32_t errorResponses = 0U;
    uint32_t sendRejections = 0U;
    unsigned sendError = 0U;
    void requested(uint16_t sequence, blinker::ByteView body) {
        blinker::ByteView nonce;
        requestSequence = sequence; requestBodyBytes = body.size; correlated = false;
        if (blinker::bbp2::decodeTimeRequest(body, nonce)) memcpy(nonce_, nonce.data, sizeof(nonce_));
    }
    void received(const blinker::bbp2::FrameView& frame) {
        blinker::bbp2::TimeResponseView response;
        responseSequence = frame.header.sequence;
        responseFrameBytes = blinker::bbp2::encodedFrameSize(frame.header);
        if (blinker::bbp2::decodeTimeResponse(frame.body, response)) {
            sourceUncertaintyMs = response.uncertaintyMillis;
            correlated = requestSequence == responseSequence &&
                memcmp(nonce_, response.nonce.data, sizeof(nonce_)) == 0;
        }
    }
private:
    uint8_t nonce_[blinker::bbp2::kTimeSyncNonceSize] = {};
};
// Observes the real service only: no substitute clock, protocol reply or grant.
class ObservedTime final : public blinker::ITimeSync {
public:
    explicit ObservedTime(TimeExchangeTrace* trace = nullptr) : trace_(trace) {}
    uint32_t requests = 0U, samples = 0U, sampleAt = 0U, serverSamples = 0U, localSamples = 0U;
    blinker::Result configure(blinker::IClock& tick, blinker::IRandom& random) override {
        return service_.configure(tick, random);
    }
    void reset(bool clear) override { service_.reset(clear); }
    void sendRejected(blinker::ErrorCode error) override {
        if (trace_ != nullptr) { ++trace_->sendRejections; trace_->sendError = static_cast<unsigned>(error); }
        service_.sendRejected(error);
    }
    bool poll(const blinker::TimeSyncPeer& peer) override { return service_.poll(peer); }
    void sessionClosed(uint8_t transport, uint32_t session) override { service_.sessionClosed(transport, session); }
    blinker::Result request(const blinker::TimeSyncPeer& peer, uint16_t sequence,
                           blinker::MutableByteSpan body, size_t& size) override {
        const auto result = service_.request(peer, sequence, body, size);
        if (result) {
            ++requests;
            if (trace_ != nullptr) trace_->requested(sequence, blinker::ByteView(body.data, size));
        }
        return result;
    }
    blinker::Result receive(const blinker::TimeSyncPeer& peer, const blinker::bbp2::FrameView& frame) override {
        const auto result = service_.receive(peer, frame);
        if (result && trace_ != nullptr && frame.header.kind == static_cast<uint8_t>(blinker::bbp2::MessageKind::Error))
            ++trace_->errorResponses; // Real service already checked related sequence/peer.
        if (result && frame.header.kind == static_cast<uint8_t>(blinker::bbp2::MessageKind::TimeResponse)) {
            ++samples; sampleAt = millis();
            if (peer.source == blinker::WallTimeSource::Server) ++serverSamples;
            if (peer.source == blinker::WallTimeSource::Controller) ++localSamples;
            if (trace_ != nullptr) trace_->received(frame);
        }
        return result;
    }
    bool readTime(blinker::WallClockSnapshot& snapshot) override { return service_.readTime(snapshot); }
private:
    blinker::TimeSyncService service_;
    TimeExchangeTrace* trace_;
};
} // namespace blinker_test
#endif
