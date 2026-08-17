#ifndef BLINKER_PROTOCOL_BBP2_KEYED_BODY_H
#define BLINKER_PROTOCOL_BBP2_KEYED_BODY_H

#include "../../core/Result.h"
#include "../../core/Span.h"
#include "../cbor/Cbor.h"

#ifndef BLINKER_BBP2_MAX_KEYED_FIELDS
#define BLINKER_BBP2_MAX_KEYED_FIELDS 32
#endif

namespace blinker {
namespace bbp2 {

typedef Result (*KeyedValueVisitor)(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType);

typedef Result (*IdValueVisitor)(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType);

Result visitKeyedBody(
    ByteView encoded,
    KeyedValueVisitor visitor,
    void* context,
    const cbor::Limits& limits = cbor::Limits());

Result visitIdBody(
    ByteView encoded,
    IdValueVisitor visitor,
    void* context,
    const cbor::Limits& limits = cbor::Limits());

class KeyedBodyWriter {
public:
    explicit KeyedBodyWriter(
        MutableByteSpan output,
        const cbor::Limits& limits = cbor::Limits());

    Result begin(size_t fieldCount);
    Result writeBool(StringView key, bool value);
    Result writeNull(StringView key);
    Result writeInt(StringView key, int64_t value);
    Result writeUnsigned(StringView key, uint64_t value);
    Result writeFloat32(StringView key, float value);
    Result writeFloat64(StringView key, double value);
    Result writeText(StringView key, StringView value);
    Result writeBytes(StringView key, ByteView value);
    Result writeEncoded(
        StringView key,
        ByteView value,
        const cbor::Limits& limits = cbor::Limits());
    Result finish(ByteView& encoded);

private:
    Result writeKey(StringView key);

    cbor::Writer writer_;
    cbor::Limits limits_;
    StringView keys_[BLINKER_BBP2_MAX_KEYED_FIELDS];
    size_t expectedFields_;
    size_t writtenFields_;
    bool begun_;
    bool failed_;
};

class IdBodyWriter {
public:
    explicit IdBodyWriter(
        MutableByteSpan output,
        const cbor::Limits& limits = cbor::Limits());

    Result begin(size_t fieldCount);
    Result writeBool(uint16_t id, bool value);
    Result writeNull(uint16_t id);
    Result writeInt(uint16_t id, int64_t value);
    Result writeUnsigned(uint16_t id, uint64_t value);
    Result writeFloat32(uint16_t id, float value);
    Result writeFloat64(uint16_t id, double value);
    Result writeText(uint16_t id, StringView value);
    Result writeBytes(uint16_t id, ByteView value);
    Result writeEncoded(
        uint16_t id,
        ByteView value,
        const cbor::Limits& limits = cbor::Limits());
    Result finish(ByteView& encoded);

private:
    Result writeId(uint16_t id);

    cbor::Writer writer_;
    cbor::Limits limits_;
    uint16_t ids_[BLINKER_BBP2_MAX_KEYED_FIELDS];
    size_t expectedFields_;
    size_t writtenFields_;
    bool begun_;
    bool failed_;
};

} // namespace bbp2
} // namespace blinker

#endif
