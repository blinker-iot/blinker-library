#ifndef BLINKER_IDENTITY_P256DEVELOPERIDENTITYRECORD_H
#define BLINKER_IDENTITY_P256DEVELOPERIDENTITYRECORD_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../interface/IP256CryptoProvider.h"

namespace blinker {

enum : size_t {
    kP256DeveloperIdentityRecordSize = 72U
};

Result encodeP256DeveloperIdentityRecord(
    ByteView privateKey,
    MutableByteSpan output,
    ByteView& encoded);

Result decodeP256DeveloperIdentityRecord(
    ByteView encoded,
    MutableByteSpan privateKey);

} // namespace blinker

#endif
