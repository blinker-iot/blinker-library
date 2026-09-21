#ifndef BLINKER_TRANSPORT_WEBSOCKETINGRESSGUARD_H
#define BLINKER_TRANSPORT_WEBSOCKETINGRESSGUARD_H

#include "../../core/Span.h"

namespace blinker {

// Validation tap in front of a platform WS decoder, not a second decoder.
// Never allocates, unmasks or retains payload. In particular, reject 64-bit
// lengths BEFORE a 32-bit SDK narrows them to size_t. No extensions negotiated.
class WebSocketIngressGuard {
public:
    explicit WebSocketIngressGuard(uint16_t maximum) : maximum_(maximum) { reset(); }
    void reset() {
        headerSize_ = 0U; headerNeed_ = 2U; remaining_ = 0U;
        messageSize_ = 0U; fragmented_ = false; failed_ = false;
    }
    bool boundary() const { return headerSize_ == 0U && remaining_ == 0U; }
    bool messageBoundary() const { return boundary() && !fragmented_; }
    bool consume(ByteView input) {
        if (failed_ || (input.data == nullptr && !input.empty())) return false;
        for (size_t i = 0U; i < input.size; ++i) {
            if (remaining_ != 0U) { --remaining_; continue; }
            header_[headerSize_++] = input.data[i];
            if (headerSize_ == 2U) {
                const uint8_t length = header_[1] & 127U;
                if ((header_[0] & 112U) != 0U || (header_[1] & 128U) == 0U || length == 127U)
                    return fail();
                headerNeed_ = length == 126U ? 8U : 6U;
            }
            if (headerSize_ == headerNeed_) {
                if (!acceptHeader()) return fail();
                headerSize_ = 0U; headerNeed_ = 2U;
            }
        }
        return true;
    }
private:
    bool fail() { failed_ = true; return false; }
    bool acceptHeader() {
        const uint8_t opcode = header_[0] & 15U;
        const bool final = (header_[0] & 128U) != 0U;
        const uint16_t size = (header_[1] & 127U) == 126U
            ? static_cast<uint16_t>((static_cast<uint16_t>(header_[2]) << 8U) | header_[3])
            : static_cast<uint16_t>(header_[1] & 127U);
        if ((header_[1] & 127U) == 126U && size < 126U) return false;
        if (opcode >= 8U) {
            if (!final || size > 125U || opcode > 10U || (opcode == 8U && size == 1U)) return false;
        } else {
            if (opcode != 0U && opcode != 2U) return false; // Binary only.
            if ((opcode == 0U) != fragmented_) return false;
            if (size > maximum_ - messageSize_) return false;
            messageSize_ = static_cast<uint16_t>(messageSize_ + size);
            fragmented_ = !final;
            if (final) {
                if (messageSize_ == 0U) return false;
                messageSize_ = 0U;
            }
        }
        remaining_ = size;
        return true;
    }
    const uint16_t maximum_;
    uint16_t remaining_, messageSize_;
    uint8_t header_[8], headerSize_, headerNeed_;
    bool fragmented_, failed_;
};

} // namespace blinker
#endif
