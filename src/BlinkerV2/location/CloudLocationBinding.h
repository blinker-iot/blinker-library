#ifndef BLINKER_CLOUD_LOCATION_BINDING_H
#define BLINKER_CLOUD_LOCATION_BINDING_H

#include "LocationCodec.h"
#include "LocationSnapshot.h"
#include "../api/Device.h"

namespace blinker {

// Optional, caller-owned Cloud-only sampler. It never initializes State,
// starts GNSS hardware or owns a timer/transport. The Snapshot must outlive it,
// and this binding must outlive the Device to which it is attached.
class CloudLocationBinding final : public CloudTelemetrySampleProvider {
public:
    explicit CloudLocationBinding(LocationSnapshot& snapshot)
        : snapshot_(snapshot), endpointId_(0U), encoded_(), size_(0U) {}

    Result attach(Device& device, StringView key) { return attach(device.client(), key); }

    Result attach(Client& client, StringView key) {
        if (endpointId_ != 0U) return Result::failure(ErrorCode::AlreadyExists);
        const EndpointHandle endpoint = client.endpoints().find(key);
        if (!endpoint.valid()) return Result::failure(endpoint.error());
        endpointId_ = endpoint.id();
        const Result result = client.attachCloudSampleProvider(*this);
        if (!result) endpointId_ = 0U;
        return result;
    }

    uint16_t endpointId() const override { return endpointId_; }

    Result prepare(uint32_t nowMillis) override {
        size_ = 0U;
        LocationSample sample;
        Result result = snapshot_.read(nowMillis, sample);
        ByteView encoded;
        if (result) result = location::encodeSample(sample, MutableByteSpan(encoded_, sizeof(encoded_)), encoded);
        if (result) size_ = encoded.size;
        return result;
    }

    Result write(bbp2::IdBodyWriter& writer) const override {
        return endpointId_ != 0U && size_ != 0U
            ? writer.writeEncoded(endpointId_, ByteView(encoded_, size_))
            : Result::failure(ErrorCode::NotFound);
    }

private:
    LocationSnapshot& snapshot_;
    uint16_t endpointId_;
    uint8_t encoded_[location::kMaximumEncodedSampleSize];
    size_t size_;
    CloudLocationBinding(const CloudLocationBinding&);
    CloudLocationBinding& operator=(const CloudLocationBinding&);
};

} // namespace blinker

#endif
