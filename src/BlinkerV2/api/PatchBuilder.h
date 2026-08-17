#ifndef BLINKER_API_PATCHBUILDER_H
#define BLINKER_API_PATCHBUILDER_H

#include "EndpointHandle.h"
#include "../protocol/bbp2/KeyedBody.h"

namespace blinker {

// Builds a keyed PATCH body directly into caller-owned memory. finish()
// validates endpoint membership, type, access and constraints before the body
// can be handed to Runtime.
class PatchBuilder {
public:
    PatchBuilder(
        const EndpointRegistry& registry,
        MutableByteSpan output,
        const cbor::Limits& limits = cbor::Limits(),
        uint8_t requiredAccess =
            static_cast<uint8_t>(AccessRead | AccessNotify));

    Result begin(size_t fieldCount);
    Result write(const EndpointHandle& endpoint, bool value);
    Result write(const EndpointHandle& endpoint, int64_t value);
    Result write(const EndpointHandle& endpoint, uint64_t value);
    Result write(const EndpointHandle& endpoint, float value);
    Result write(const EndpointHandle& endpoint, double value);
    Result writeText(const EndpointHandle& endpoint, StringView value);
    Result writeBytes(const EndpointHandle& endpoint, ByteView value);
    Result writeNull(const EndpointHandle& endpoint);
    Result writeEncoded(const EndpointHandle& endpoint, ByteView value);
    Result finish(ByteView& encoded);

private:
    static Result validateValue(
        void* context,
        StringView endpointKey,
        ByteView encodedValue,
        cbor::Type valueType);
    Result validateHandle(
        const EndpointHandle& endpoint,
        ValueType expected) const;
    Result validateHandle(const EndpointHandle& endpoint) const;

    const EndpointRegistry& registry_;
    bbp2::KeyedBodyWriter writer_;
    cbor::Limits limits_;
    uint8_t requiredAccess_;
};

} // namespace blinker

#endif
