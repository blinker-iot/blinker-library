#ifndef BLINKER_SECURITY_P256SIGNATURE_H
#define BLINKER_SECURITY_P256SIGNATURE_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../interface/IP256DigestVerifier.h"

namespace blinker {

bool isValidP256Scalar(ByteView scalar);
bool isCanonicalP256PublicKey(ByteView publicKey);
bool isCanonicalP256Signature(ByteView signature);

// Converts a valid raw r||s signature to the unique low-S form in place.
Result normalizeP256Signature(MutableByteSpan signature);

} // namespace blinker

#endif
