#ifndef BLINKER_MODEL_MANIFEST_H
#define BLINKER_MODEL_MANIFEST_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/cbor/Cbor.h"
#include "EndpointRegistry.h"

namespace blinker {

Result encodeManifest(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());

// Hashes the canonical logical Manifest directly into SHA-256. No complete
// Manifest scratch buffer is required.
Result computeManifestFingerprint(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan fingerprint,
    const cbor::Limits& limits = cbor::Limits());

// Transitional source-compatible overload. scratch is deliberately unused;
// callers should migrate to the overload above.
Result computeManifestFingerprint(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan scratch,
    MutableByteSpan fingerprint,
    const cbor::Limits& limits = cbor::Limits());

// A Manifest page is a canonical map:
// {0:revision, 1:fingerprint32, 2:cursor, 3:nextCursor,
//  4:totalFields, 5:fields[]}.
// cursor and nextCursor are zero-based field offsets. nextCursor ==
// totalFields marks the final page. The fingerprint always describes the
// canonical logical Manifest produced by encodeManifest(), not a page body.
struct ManifestPageView {
    uint32_t revision;
    ByteView fingerprint;
    uint16_t cursor;
    uint16_t nextCursor;
    uint16_t totalFields;
    uint16_t fieldCount;
    ByteView encodedFields;

    ManifestPageView()
        : revision(0),
          fingerprint(),
          cursor(0),
          nextCursor(0),
          totalFields(0),
          fieldCount(0),
          encodedFields() {}
};

Result encodeManifestPage(
    uint32_t revision,
    const EndpointRegistry& registry,
    ByteView fingerprint,
    uint16_t cursor,
    MutableByteSpan output,
    ByteView& encoded,
    uint16_t& nextCursor,
    const cbor::Limits& limits = cbor::Limits());

Result decodeManifestPage(
    ByteView encoded,
    ManifestPageView& page,
    const cbor::Limits& limits = cbor::Limits());

} // namespace blinker

#endif
