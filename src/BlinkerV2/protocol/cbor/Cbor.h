#ifndef BLINKER_PROTOCOL_CBOR_CBOR_H
#define BLINKER_PROTOCOL_CBOR_CBOR_H

#include "../../core/Result.h"
#include "../../core/Span.h"

namespace blinker {
namespace cbor {

enum class Type : uint8_t {
    Unsigned = 0,
    Negative = 1,
    Bytes = 2,
    Text = 3,
    Array = 4,
    Map = 5,
    Boolean = 7,
    Null = 8,
    Float = 9,
    Invalid = 0xFF
};

struct Limits {
    size_t maxTextLength;
    size_t maxByteStringLength;
    size_t maxContainerItems;
    uint8_t maxDepth;

    Limits()
        : maxTextLength(256),
          maxByteStringLength(4096),
          maxContainerItems(32),
          maxDepth(6) {}
};

Result validate(ByteView encoded, const Limits& limits = Limits());

// A zero-allocation output sink used when canonical CBOR must be hashed or
// measured without first materializing the complete value in RAM.
typedef Result (*WriteSink)(void* context, ByteView bytes);

class Writer {
public:
    explicit Writer(MutableByteSpan output);
    Writer(WriteSink sink, void* context);

    Result writeUnsigned(uint64_t value);
    Result writeInt(int64_t value);
    Result writeBool(bool value);
    Result writeNull();
    Result writeFloat32(float value);
    Result writeFloat64(double value);
    Result writeBytes(ByteView value);
    Result writeText(StringView value);
    Result beginArray(size_t itemCount);
    Result beginMap(size_t pairCount);
    Result writeEncodedValue(ByteView value, const Limits& limits);

    size_t size() const { return offset_; }
    ByteView view() const {
        return sink_ == nullptr
                   ? ByteView(output_.data, offset_)
                   : ByteView();
    }

private:
    Result writeHead(uint8_t majorType, uint64_t value);
    Result append(const uint8_t* data, size_t size);

    MutableByteSpan output_;
    WriteSink sink_;
    void* sinkContext_;
    size_t offset_;
};

class Reader {
public:
    explicit Reader(ByteView input, const Limits& limits = Limits());

    Result peekType(Type& type) const;
    Result readUnsigned(uint64_t& value);
    Result readInt(int64_t& value);
    Result readBool(bool& value);
    Result readNull();
    Result readFloat(double& value);
    Result readBytes(ByteView& value);
    Result readText(StringView& value);
    Result readArraySize(size_t& itemCount);
    Result readMapSize(size_t& pairCount);
    Result captureValue(ByteView& encoded, Type* type = nullptr);
    Result skipValue();

    bool finished() const { return offset_ == input_.size; }
    size_t remaining() const { return input_.size - offset_; }

private:
    struct Head {
        uint8_t majorType;
        uint8_t additional;
        uint64_t argument;
        size_t size;
    };

    Result decodeHead(size_t offset, Head& head) const;
    Result readSized(uint8_t majorType, size_t maximum, ByteView& value);
    Result readContainer(uint8_t majorType, size_t& itemCount);
    Result skipValueAt(size_t& cursor, uint8_t depth, Type* type) const;
    static Type typeFromHead(const Head& head);

    ByteView input_;
    Limits limits_;
    size_t offset_;
};

} // namespace cbor
} // namespace blinker

#endif
