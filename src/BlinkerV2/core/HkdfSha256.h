#ifndef BLINKER_CORE_HKDFSHA256_H
#define BLINKER_CORE_HKDFSHA256_H

#include "HmacSha256.h"

namespace blinker {

// RFC 5869 HKDF-SHA256. Empty salt is accepted and means 32 zero bytes.
// Output is limited by RFC 5869 to 255 SHA-256 blocks.
Result hkdfSha256(
    ByteView salt,
    ByteView inputKeyMaterial,
    ByteView info,
    MutableByteSpan output);

} // namespace blinker

#endif
