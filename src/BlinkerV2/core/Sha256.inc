#include "Sha256.h"

#include <string.h>

#include "SecureMemory.h"

namespace blinker {

namespace {

const uint32_t kRoundConstants[64] = {
    0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL,
    0x3956C25BUL, 0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL,
    0xD807AA98UL, 0x12835B01UL, 0x243185BEUL, 0x550C7DC3UL,
    0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL, 0xC19BF174UL,
    0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL,
    0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL,
    0x983E5152UL, 0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL,
    0xC6E00BF3UL, 0xD5A79147UL, 0x06CA6351UL, 0x14292967UL,
    0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL, 0x53380D13UL,
    0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
    0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL,
    0xD192E819UL, 0xD6990624UL, 0xF40E3585UL, 0x106AA070UL,
    0x19A4C116UL, 0x1E376C08UL, 0x2748774CUL, 0x34B0BCB5UL,
    0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL, 0x682E6FF3UL,
    0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL,
    0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL
};

uint32_t rotateRight(uint32_t value, uint8_t bits) {
    return (value >> bits) | (value << (32U - bits));
}

uint32_t readU32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24U) |
           (static_cast<uint32_t>(data[1]) << 16U) |
           (static_cast<uint32_t>(data[2]) << 8U) |
           static_cast<uint32_t>(data[3]);
}

void writeU32(uint8_t* data, uint32_t value) {
    data[0] = static_cast<uint8_t>(value >> 24U);
    data[1] = static_cast<uint8_t>(value >> 16U);
    data[2] = static_cast<uint8_t>(value >> 8U);
    data[3] = static_cast<uint8_t>(value);
}

} // namespace

Sha256::Sha256()
    : state_(), block_(), totalBytes_(0), blockSize_(0), finished_(false) {
    reset();
}

Sha256::~Sha256() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(state_),
        sizeof(state_)));
    secureZero(MutableByteSpan(block_, sizeof(block_)));
    totalBytes_ = 0;
    blockSize_ = 0;
    finished_ = true;
}

void Sha256::reset() {
    state_[0] = 0x6A09E667UL;
    state_[1] = 0xBB67AE85UL;
    state_[2] = 0x3C6EF372UL;
    state_[3] = 0xA54FF53AUL;
    state_[4] = 0x510E527FUL;
    state_[5] = 0x9B05688CUL;
    state_[6] = 0x1F83D9ABUL;
    state_[7] = 0x5BE0CD19UL;
    memset(block_, 0, sizeof(block_));
    totalBytes_ = 0;
    blockSize_ = 0;
    finished_ = false;
}

Result Sha256::update(ByteView input) {
    if (finished_) return Result::failure(ErrorCode::ProtocolError);
    if (input.size != 0U && input.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (input.size > UINT64_MAX / 8U - totalBytes_) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    totalBytes_ += input.size;
    size_t offset = 0;
    while (offset < input.size) {
        const size_t available = sizeof(block_) - blockSize_;
        const size_t remaining = input.size - offset;
        const size_t copied = remaining < available ? remaining : available;
        memcpy(block_ + blockSize_, input.data + offset, copied);
        blockSize_ += copied;
        offset += copied;
        if (blockSize_ == sizeof(block_)) {
            transform(block_);
            secureZero(MutableByteSpan(block_, sizeof(block_)));
            blockSize_ = 0;
        }
    }
    return Result::success();
}

Result Sha256::finish(MutableByteSpan output) {
    if (finished_) return Result::failure(ErrorCode::ProtocolError);
    if (output.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size < kSha256Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    const uint64_t totalBits = totalBytes_ * 8U;
    block_[blockSize_++] = 0x80;
    if (blockSize_ > 56U) {
        memset(block_ + blockSize_, 0, sizeof(block_) - blockSize_);
        transform(block_);
        secureZero(MutableByteSpan(block_, sizeof(block_)));
        blockSize_ = 0;
    }
    memset(block_ + blockSize_, 0, 56U - blockSize_);
    for (uint8_t index = 0; index < 8U; ++index) {
        block_[63U - index] = static_cast<uint8_t>(
            totalBits >> (static_cast<uint8_t>(index * 8U)));
    }
    transform(block_);
    for (uint8_t index = 0; index < 8U; ++index) {
        writeU32(output.data + static_cast<size_t>(index) * 4U, state_[index]);
    }
    finished_ = true;
    secureZero(MutableByteSpan(block_, sizeof(block_)));
    blockSize_ = 0;
    return Result::success();
}

void Sha256::transform(const uint8_t block[64]) {
    uint32_t words[64];
    for (uint8_t index = 0; index < 16U; ++index) {
        words[index] = readU32(block + static_cast<size_t>(index) * 4U);
    }
    for (uint8_t index = 16U; index < 64U; ++index) {
        const uint32_t first = words[index - 15U];
        const uint32_t second = words[index - 2U];
        const uint32_t sigma0 = rotateRight(first, 7U) ^
                                rotateRight(first, 18U) ^ (first >> 3U);
        const uint32_t sigma1 = rotateRight(second, 17U) ^
                                rotateRight(second, 19U) ^ (second >> 10U);
        words[index] = words[index - 16U] + sigma0 +
                       words[index - 7U] + sigma1;
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];
    uint32_t f = state_[5];
    uint32_t g = state_[6];
    uint32_t h = state_[7];
    for (uint8_t index = 0; index < 64U; ++index) {
        const uint32_t sum1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^
                              rotateRight(e, 25U);
        const uint32_t choose = (e & f) ^ ((~e) & g);
        const uint32_t temporary1 = h + sum1 + choose +
                                    kRoundConstants[index] + words[index];
        const uint32_t sum0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^
                              rotateRight(a, 22U);
        const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t temporary2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(words),
        sizeof(words)));
}

Result sha256(ByteView input, MutableByteSpan output) {
    Sha256 hash;
    Result result = hash.update(input);
    if (!result) return result;
    return hash.finish(output);
}

} // namespace blinker
