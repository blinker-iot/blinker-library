#ifndef BLINKER_MODEL_ENDPOINTVALUE_H
#define BLINKER_MODEL_ENDPOINTVALUE_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/cbor/Cbor.h"
#include "Endpoint.h"

namespace blinker {

Result validateEndpointValue(
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    uint8_t requiredAnyAccess,
    const cbor::Limits& limits = cbor::Limits());

} // namespace blinker

#endif
