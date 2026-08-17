#ifndef BLINKER_PROTOCOL_NOISE_LOCALSECURERECORD_H
#define BLINKER_PROTOCOL_NOISE_LOCALSECURERECORD_H

#include "../../core/Result.h"
#include "../../core/Span.h"
#include "NoiseSuite.h"

namespace blinker {
namespace noise {

static const uint8_t kLocalSecureRecordMagic = 0xB3U;
static const uint8_t kLocalSecureRecordVersion = 1U;
static const uint8_t kLocalSecureRecordVersionShift = 4U;
static const uint8_t kLocalSecureRecordVersionMask = 0xF0U;
static const uint8_t kLocalSecureRecordTypeMask = 0x0FU;
static const size_t kLocalSecureRecordHeaderSize = 4U;

// Product limit, intentionally much smaller than Noise's general 65535-byte
// ceiling. It covers Wi-Fi setup, paged discovery and Enrollment v1 messages
// while bounding MCU reassembly storage.
static const size_t kLocalSecureRecordMaxBodySize = 1024U;

enum class LocalSecureRecordType : uint8_t {
    InitiatorHandshake = 1U,
    ResponderHandshake = 2U,
    Transport = 3U
};

struct LocalSecureRecordView {
    LocalSecureRecordType type;
    ByteView body;

    LocalSecureRecordView()
        : type(LocalSecureRecordType::InitiatorHandshake), body() {}
};

Result encodeLocalSecureRecord(
    LocalSecureRecordType type,
    ByteView body,
    MutableByteSpan output,
    ByteView& encoded);
// Decodes and validates the fixed header without requiring the complete body.
// BLE record reassembly uses this after the first fragment to bound storage.
Result decodeLocalSecureRecordSize(
    ByteView prefix,
    size_t& recordSize);
Result parseLocalSecureRecord(
    ByteView encoded,
    LocalSecureRecordView& record);

// Fixed prologue binds the Blinker setup context, record version and suite.
// There is no unauthenticated suite negotiation in v1.
ByteView localSecureNoisePrologue(NoiseNnPattern pattern);

} // namespace noise
} // namespace blinker

#endif
