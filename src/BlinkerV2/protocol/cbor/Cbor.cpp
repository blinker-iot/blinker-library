#include "Cbor.h"

#include <limits.h>
#include <string.h>

#include "../../core/Utf8.h"

namespace blinker {
namespace cbor {

namespace {

bool isFiniteFloat32(float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000UL) != 0x7F800000UL;
}

bool isFiniteFloat64(double value) {
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & UINT64_C(0x7FF0000000000000)) !=
           UINT64_C(0x7FF0000000000000);
}

uint64_t readBigEndian(const uint8_t* data, size_t size) {
    uint64_t value = 0;
    for (size_t index = 0; index < size; ++index) {
        value = (value << 8U) | data[index];
    }
    return value;
}

} // namespace

Result validate(ByteView encoded, const Limits& limits) {
    Reader reader(encoded, limits);
    Result result = reader.skipValue();
    if (!result) {
        return result;
    }
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Writer::Writer(MutableByteSpan output)
    : output_(output), sink_(nullptr), sinkContext_(nullptr), offset_(0) {}

Writer::Writer(WriteSink sink, void* context)
    : output_(), sink_(sink), sinkContext_(context), offset_(0) {}

Result Writer::append(const uint8_t* data, size_t size) {
    if ((size != 0U && data == nullptr) ||
        (sink_ == nullptr && output_.size != 0U && output_.data == nullptr) ||
        (sink_ != nullptr && sinkContext_ == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (size > SIZE_MAX - offset_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (sink_ != nullptr) {
        Result result = sink_(sinkContext_, ByteView(data, size));
        if (!result) return result;
        offset_ += size;
        return Result::success();
    }
    if (size > output_.size - offset_) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (size != 0U) {
        // Encoded sub-values may already live later in the same output
        // buffer (for example while adding a protocol envelope). memmove
        // keeps that zero-allocation composition safe.
        memmove(output_.data + offset_, data, size);
    }
    offset_ += size;
    return Result::success();
}

Result Writer::writeHead(uint8_t majorType, uint64_t value) {
    uint8_t bytes[9];
    size_t size = 1;
    if (value < 24U) {
        bytes[0] = static_cast<uint8_t>((majorType << 5U) | value);
    } else if (value <= UINT8_MAX) {
        bytes[0] = static_cast<uint8_t>((majorType << 5U) | 24U);
        bytes[1] = static_cast<uint8_t>(value);
        size = 2;
    } else if (value <= UINT16_MAX) {
        bytes[0] = static_cast<uint8_t>((majorType << 5U) | 25U);
        bytes[1] = static_cast<uint8_t>(value >> 8U);
        bytes[2] = static_cast<uint8_t>(value);
        size = 3;
    } else if (value <= UINT32_MAX) {
        bytes[0] = static_cast<uint8_t>((majorType << 5U) | 26U);
        for (size_t index = 0; index < 4; ++index) {
            bytes[1 + index] =
                static_cast<uint8_t>(value >> (24U - index * 8U));
        }
        size = 5;
    } else {
        bytes[0] = static_cast<uint8_t>((majorType << 5U) | 27U);
        for (size_t index = 0; index < 8; ++index) {
            bytes[1 + index] =
                static_cast<uint8_t>(value >> (56U - index * 8U));
        }
        size = 9;
    }
    return append(bytes, size);
}

Result Writer::writeUnsigned(uint64_t value) {
    return writeHead(0, value);
}

Result Writer::writeInt(int64_t value) {
    if (value >= 0) {
        return writeHead(0, static_cast<uint64_t>(value));
    }
    return writeHead(1, static_cast<uint64_t>(-(value + 1)));
}

Result Writer::writeBool(bool value) {
    const uint8_t encoded = value ? 0xF5U : 0xF4U;
    return append(&encoded, 1);
}

Result Writer::writeNull() {
    const uint8_t encoded = 0xF6U;
    return append(&encoded, 1);
}

Result Writer::writeFloat32(float value) {
    if (!isFiniteFloat32(value)) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    uint8_t encoded[5] = {
        0xFAU,
        static_cast<uint8_t>(bits >> 24U),
        static_cast<uint8_t>(bits >> 16U),
        static_cast<uint8_t>(bits >> 8U),
        static_cast<uint8_t>(bits)
    };
    return append(encoded, sizeof(encoded));
}

Result Writer::writeFloat64(double value) {
    if (!isFiniteFloat64(value)) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    uint8_t encoded[9];
    encoded[0] = 0xFBU;
    for (size_t index = 0; index < 8; ++index) {
        encoded[1 + index] =
            static_cast<uint8_t>(bits >> (56U - index * 8U));
    }
    return append(encoded, sizeof(encoded));
}

Result Writer::writeBytes(ByteView value) {
    if (value.size != 0U && value.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = writeHead(2, value.size);
    return result ? append(value.data, value.size) : result;
}

Result Writer::writeText(StringView value) {
    if (!isValidUtf8(value)) {
        return Result::failure(ErrorCode::InvalidUtf8);
    }
    Result result = writeHead(3, value.size);
    return result
               ? append(reinterpret_cast<const uint8_t*>(value.data), value.size)
               : result;
}

Result Writer::beginArray(size_t itemCount) {
    return writeHead(4, itemCount);
}

Result Writer::beginMap(size_t pairCount) {
    return writeHead(5, pairCount);
}

Result Writer::writeEncodedValue(ByteView value, const Limits& limits) {
    Result result = validate(value, limits);
    return result ? append(value.data, value.size) : result;
}

Reader::Reader(ByteView input, const Limits& limits)
    : input_(input), limits_(limits), offset_(0) {}

Result Reader::decodeHead(size_t offset, Head& head) const {
    if (offset >= input_.size || input_.data == nullptr) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    const uint8_t initial = input_.data[offset];
    head.majorType = initial >> 5U;
    head.additional = initial & 0x1FU;
    head.argument = 0;
    head.size = 1;

    if (head.additional < 24U) {
        head.argument = head.additional;
        return Result::success();
    }
    if (head.additional == 31U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (head.additional > 27U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const size_t argumentSize = static_cast<size_t>(1U) <<
                                (head.additional - 24U);
    if (argumentSize > input_.size - offset - 1U) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    head.argument = readBigEndian(input_.data + offset + 1U, argumentSize);
    head.size += argumentSize;

    if (head.majorType != 7U &&
        ((head.additional == 24U && head.argument < 24U) ||
         (head.additional == 25U && head.argument <= UINT8_MAX) ||
         (head.additional == 26U && head.argument <= UINT16_MAX) ||
         (head.additional == 27U && head.argument <= UINT32_MAX))) {
        return Result::failure(ErrorCode::NonCanonicalEncoding);
    }
    return Result::success();
}

Type Reader::typeFromHead(const Head& head) {
    switch (head.majorType) {
        case 0: return Type::Unsigned;
        case 1: return Type::Negative;
        case 2: return Type::Bytes;
        case 3: return Type::Text;
        case 4: return Type::Array;
        case 5: return Type::Map;
        case 7:
            if (head.additional == 20U || head.additional == 21U) {
                return Type::Boolean;
            }
            if (head.additional == 22U) {
                return Type::Null;
            }
            if (head.additional == 26U || head.additional == 27U) {
                return Type::Float;
            }
            return Type::Invalid;
        default: return Type::Invalid;
    }
}

Result Reader::peekType(Type& type) const {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result) {
        return result;
    }
    type = typeFromHead(head);
    return type == Type::Invalid
               ? Result::failure(ErrorCode::UnsupportedFeature)
               : Result::success();
}

Result Reader::readUnsigned(uint64_t& value) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != 0U) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    value = head.argument;
    offset_ += head.size;
    return Result::success();
}

Result Reader::readInt(int64_t& value) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || (head.majorType != 0U && head.majorType != 1U)) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    if (head.majorType == 0U) {
        if (head.argument > static_cast<uint64_t>(INT64_MAX)) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        value = static_cast<int64_t>(head.argument);
    } else {
        if (head.argument > static_cast<uint64_t>(INT64_MAX)) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        value = -static_cast<int64_t>(head.argument) - 1;
    }
    offset_ += head.size;
    return Result::success();
}

Result Reader::readBool(bool& value) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != 7U ||
        (head.additional != 20U && head.additional != 21U)) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    value = head.additional == 21U;
    offset_ += head.size;
    return Result::success();
}

Result Reader::readNull() {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != 7U || head.additional != 22U) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    offset_ += head.size;
    return Result::success();
}

Result Reader::readFloat(double& value) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != 7U ||
        (head.additional != 26U && head.additional != 27U)) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    if (head.additional == 26U) {
        const uint32_t bits = static_cast<uint32_t>(head.argument);
        float decoded = 0;
        memcpy(&decoded, &bits, sizeof(decoded));
        if (!isFiniteFloat32(decoded)) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        value = decoded;
    } else {
        const uint64_t bits = head.argument;
        double decoded = 0;
        memcpy(&decoded, &bits, sizeof(decoded));
        if (!isFiniteFloat64(decoded)) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        value = decoded;
    }
    offset_ += head.size;
    return Result::success();
}

Result Reader::readSized(uint8_t majorType, size_t maximum, ByteView& value) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != majorType) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    if (head.argument > maximum || head.argument > SIZE_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    const size_t size = static_cast<size_t>(head.argument);
    if (size > input_.size - offset_ - head.size) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    value = ByteView(input_.data + offset_ + head.size, size);
    offset_ += head.size + size;
    return Result::success();
}

Result Reader::readBytes(ByteView& value) {
    return readSized(2, limits_.maxByteStringLength, value);
}

Result Reader::readText(StringView& value) {
    ByteView bytes;
    const size_t originalOffset = offset_;
    Result result = readSized(3, limits_.maxTextLength, bytes);
    if (!result) {
        return result;
    }
    StringView text(reinterpret_cast<const char*>(bytes.data), bytes.size);
    if (!isValidUtf8(text)) {
        offset_ = originalOffset;
        return Result::failure(ErrorCode::InvalidUtf8);
    }
    value = text;
    return Result::success();
}

Result Reader::readContainer(uint8_t majorType, size_t& itemCount) {
    Head head;
    Result result = decodeHead(offset_, head);
    if (!result || head.majorType != majorType) {
        return result ? Result::failure(ErrorCode::InvalidEncoding) : result;
    }
    if (limits_.maxDepth == 0U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (head.argument > limits_.maxContainerItems || head.argument > SIZE_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    itemCount = static_cast<size_t>(head.argument);
    offset_ += head.size;
    return Result::success();
}

Result Reader::readArraySize(size_t& itemCount) {
    return readContainer(4, itemCount);
}

Result Reader::readMapSize(size_t& pairCount) {
    return readContainer(5, pairCount);
}

Result Reader::skipValueAt(size_t& cursor, uint8_t depth, Type* type) const {
    Head head;
    Result result = decodeHead(cursor, head);
    if (!result) {
        return result;
    }
    const Type decodedType = typeFromHead(head);
    if (decodedType == Type::Invalid) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (type != nullptr) {
        *type = decodedType;
    }
    cursor += head.size;

    if (head.majorType == 2U || head.majorType == 3U) {
        const size_t maximum = head.majorType == 2U
                                   ? limits_.maxByteStringLength
                                   : limits_.maxTextLength;
        if (head.argument > maximum || head.argument > SIZE_MAX) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        const size_t size = static_cast<size_t>(head.argument);
        if (size > input_.size - cursor) {
            return Result::failure(ErrorCode::TruncatedInput);
        }
        if (head.majorType == 3U &&
            !isValidUtf8(StringView(
                reinterpret_cast<const char*>(input_.data + cursor), size))) {
            return Result::failure(ErrorCode::InvalidUtf8);
        }
        cursor += size;
        return Result::success();
    }

    if (head.majorType == 4U || head.majorType == 5U) {
        if (depth == 0U || depth > limits_.maxDepth) {
            return Result::failure(ErrorCode::NestingTooDeep);
        }
        if (head.argument > limits_.maxContainerItems ||
            head.argument > SIZE_MAX) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        size_t childCount = static_cast<size_t>(head.argument);
        if (head.majorType == 5U) {
            if (childCount > SIZE_MAX / 2U) {
                return Result::failure(ErrorCode::CapacityExceeded);
            }
            childCount *= 2U;
        }
        for (size_t index = 0; index < childCount; ++index) {
            result = skipValueAt(cursor, static_cast<uint8_t>(depth + 1U), nullptr);
            if (!result) {
                return result;
            }
        }
        return Result::success();
    }

    if (head.majorType == 7U &&
        (head.additional == 26U || head.additional == 27U)) {
        if (head.additional == 26U) {
            const uint32_t bits = static_cast<uint32_t>(head.argument);
            float decoded = 0;
            memcpy(&decoded, &bits, sizeof(decoded));
            if (!isFiniteFloat32(decoded)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
        } else {
            const uint64_t bits = head.argument;
            double decoded = 0;
            memcpy(&decoded, &bits, sizeof(decoded));
            if (!isFiniteFloat64(decoded)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
        }
    }
    return Result::success();
}

Result Reader::captureValue(ByteView& encoded, Type* type) {
    const size_t start = offset_;
    size_t cursor = offset_;
    Result result = skipValueAt(cursor, 1, type);
    if (!result) {
        return result;
    }
    encoded = ByteView(input_.data + start, cursor - start);
    offset_ = cursor;
    return Result::success();
}

Result Reader::skipValue() {
    ByteView ignored;
    return captureValue(ignored, nullptr);
}

} // namespace cbor
} // namespace blinker
