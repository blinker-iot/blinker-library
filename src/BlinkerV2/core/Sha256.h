#ifndef BLINKER_CORE_SHA256_H
#define BLINKER_CORE_SHA256_H

#include "Result.h"
#include "Span.h"

namespace blinker {

static const size_t kSha256Size = 32;

class Sha256 {
public:
    Sha256();
    ~Sha256();
    Sha256(const Sha256&) = delete;
    Sha256& operator=(const Sha256&) = delete;

    void reset();
    Result update(ByteView input);
    Result finish(MutableByteSpan output);

private:
    void transform(const uint8_t block[64]);

    uint32_t state_[8];
    uint8_t block_[64];
    uint64_t totalBytes_;
    size_t blockSize_;
    bool finished_;
};

Result sha256(ByteView input, MutableByteSpan output);

} // namespace blinker

#endif
