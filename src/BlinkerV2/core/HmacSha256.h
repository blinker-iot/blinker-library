#ifndef BLINKER_CORE_HMACSHA256_H
#define BLINKER_CORE_HMACSHA256_H

#include "Sha256.h"

namespace blinker {

class HmacSha256 {
public:
    HmacSha256();
    ~HmacSha256();
    HmacSha256(const HmacSha256&) = delete;
    HmacSha256& operator=(const HmacSha256&) = delete;

    Result begin(ByteView key);
    Result update(ByteView input);
    Result finish(MutableByteSpan output);
    void clear();

private:
    Sha256 inner_;
    uint8_t outerPad_[64];
    bool initialized_;
    bool finished_;
};

Result hmacSha256(
    ByteView key,
    ByteView input,
    MutableByteSpan output);

} // namespace blinker

#endif
