#include "KeyedBody.h"

#include <string.h>

namespace blinker {
namespace bbp2 {

namespace {

bool equal(StringView first, StringView second) {
    return first.size == second.size &&
           (first.size == 0U ||
            memcmp(first.data, second.data, first.size) == 0);
}

} // namespace

Result visitKeyedBody(
    ByteView encoded,
    KeyedValueVisitor visitor,
    void* context,
    const cbor::Limits& limits) {
    if (visitor == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    if (count > BLINKER_BBP2_MAX_KEYED_FIELDS) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    StringView seen[BLINKER_BBP2_MAX_KEYED_FIELDS];
    for (size_t index = 0; index < count; ++index) {
        StringView key;
        result = reader.readText(key);
        if (!result) return result;
        if (key.empty()) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        for (size_t prior = 0; prior < index; ++prior) {
            if (equal(seen[prior], key)) {
                return Result::failure(ErrorCode::DuplicateField);
            }
        }
        seen[index] = key;

        ByteView value;
        cbor::Type type = cbor::Type::Invalid;
        result = reader.captureValue(value, &type);
        if (!result) return result;
        result = visitor(context, key, value, type);
        if (!result) return result;
    }
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result visitIdBody(
    ByteView encoded,
    IdValueVisitor visitor,
    void* context,
    const cbor::Limits& limits) {
    if (visitor == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    if (count > BLINKER_BBP2_MAX_KEYED_FIELDS) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    uint16_t seen[BLINKER_BBP2_MAX_KEYED_FIELDS] = {};
    for (size_t index = 0; index < count; ++index) {
        uint64_t rawId = 0;
        result = reader.readUnsigned(rawId);
        if (!result) return result;
        if (rawId == 0U || rawId > UINT16_MAX) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        const uint16_t id = static_cast<uint16_t>(rawId);
        for (size_t prior = 0; prior < index; ++prior) {
            if (seen[prior] == id) {
                return Result::failure(ErrorCode::DuplicateField);
            }
        }
        seen[index] = id;
        ByteView value;
        cbor::Type type = cbor::Type::Invalid;
        result = reader.captureValue(value, &type);
        if (!result) return result;
        result = visitor(context, id, value, type);
        if (!result) return result;
    }
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

KeyedBodyWriter::KeyedBodyWriter(
    MutableByteSpan output,
    const cbor::Limits& limits)
    : writer_(output),
      limits_(limits),
      keys_(),
      expectedFields_(0),
      writtenFields_(0),
      begun_(false),
      failed_(false) {}

Result KeyedBodyWriter::begin(size_t fieldCount) {
    if (begun_) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (limits_.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (fieldCount > BLINKER_BBP2_MAX_KEYED_FIELDS ||
        fieldCount > limits_.maxContainerItems) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    Result result = writer_.beginMap(fieldCount);
    if (!result) return result;
    expectedFields_ = fieldCount;
    begun_ = true;
    return Result::success();
}

Result KeyedBodyWriter::writeKey(StringView key) {
    if (failed_) return Result::failure(ErrorCode::ProtocolError);
    if (!begun_) return Result::failure(ErrorCode::NotConfigured);
    if (writtenFields_ >= expectedFields_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (key.empty()) return Result::failure(ErrorCode::InvalidArgument);
    if (key.size > limits_.maxTextLength) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    for (size_t index = 0; index < writtenFields_; ++index) {
        if (equal(keys_[index], key)) {
            return Result::failure(ErrorCode::DuplicateField);
        }
    }
    Result result = writer_.writeText(key);
    if (!result) return result;
    keys_[writtenFields_] = key;
    return Result::success();
}

Result KeyedBodyWriter::writeBool(StringView key, bool value) {
    Result result = writeKey(key);
    if (result) result = writer_.writeBool(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeNull(StringView key) {
    Result result = writeKey(key);
    if (result) result = writer_.writeNull();
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeInt(StringView key, int64_t value) {
    Result result = writeKey(key);
    if (result) result = writer_.writeInt(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeUnsigned(StringView key, uint64_t value) {
    Result result = writeKey(key);
    if (result) result = writer_.writeUnsigned(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeFloat32(StringView key, float value) {
    Result result = writeKey(key);
    if (result) result = writer_.writeFloat32(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeFloat64(StringView key, double value) {
    Result result = writeKey(key);
    if (result) result = writer_.writeFloat64(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeText(StringView key, StringView value) {
    Result result = writeKey(key);
    if (result && value.size > limits_.maxTextLength) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) result = writer_.writeText(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeBytes(StringView key, ByteView value) {
    Result result = writeKey(key);
    if (result && value.size > limits_.maxByteStringLength) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) result = writer_.writeBytes(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::writeEncoded(
    StringView key,
    ByteView value,
    const cbor::Limits& limits) {
    Result result = writeKey(key);
    cbor::Limits nestedLimits = limits;
    const uint8_t availableDepth = limits_.maxDepth == 0U
                                       ? 0U
                                       : static_cast<uint8_t>(limits_.maxDepth - 1U);
    if (nestedLimits.maxDepth == 0U ||
        nestedLimits.maxDepth > availableDepth) {
        nestedLimits.maxDepth = availableDepth;
    }
    if (nestedLimits.maxTextLength > limits_.maxTextLength) {
        nestedLimits.maxTextLength = limits_.maxTextLength;
    }
    if (nestedLimits.maxByteStringLength > limits_.maxByteStringLength) {
        nestedLimits.maxByteStringLength = limits_.maxByteStringLength;
    }
    if (nestedLimits.maxContainerItems > limits_.maxContainerItems) {
        nestedLimits.maxContainerItems = limits_.maxContainerItems;
    }
    if (result) result = writer_.writeEncodedValue(value, nestedLimits);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result KeyedBodyWriter::finish(ByteView& encoded) {
    if (!begun_ || failed_ || writtenFields_ != expectedFields_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    encoded = writer_.view();
    return Result::success();
}

IdBodyWriter::IdBodyWriter(
    MutableByteSpan output,
    const cbor::Limits& limits)
    : writer_(output),
      limits_(limits),
      ids_(),
      expectedFields_(0),
      writtenFields_(0),
      begun_(false),
      failed_(false) {}

Result IdBodyWriter::begin(size_t fieldCount) {
    if (begun_) return Result::failure(ErrorCode::AlreadyExists);
    if (limits_.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (fieldCount > BLINKER_BBP2_MAX_KEYED_FIELDS ||
        fieldCount > limits_.maxContainerItems) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    Result result = writer_.beginMap(fieldCount);
    if (!result) return result;
    expectedFields_ = fieldCount;
    begun_ = true;
    return Result::success();
}

Result IdBodyWriter::writeId(uint16_t id) {
    if (failed_) return Result::failure(ErrorCode::ProtocolError);
    if (!begun_) return Result::failure(ErrorCode::NotConfigured);
    if (writtenFields_ >= expectedFields_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (id == 0U) return Result::failure(ErrorCode::InvalidArgument);
    for (size_t index = 0; index < writtenFields_; ++index) {
        if (ids_[index] == id) {
            return Result::failure(ErrorCode::DuplicateField);
        }
    }
    Result result = writer_.writeUnsigned(id);
    if (!result) return result;
    ids_[writtenFields_] = id;
    return Result::success();
}

Result IdBodyWriter::writeBool(uint16_t id, bool value) {
    Result result = writeId(id);
    if (result) result = writer_.writeBool(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeNull(uint16_t id) {
    Result result = writeId(id);
    if (result) result = writer_.writeNull();
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeInt(uint16_t id, int64_t value) {
    Result result = writeId(id);
    if (result) result = writer_.writeInt(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeUnsigned(uint16_t id, uint64_t value) {
    Result result = writeId(id);
    if (result) result = writer_.writeUnsigned(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeFloat32(uint16_t id, float value) {
    Result result = writeId(id);
    if (result) result = writer_.writeFloat32(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeFloat64(uint16_t id, double value) {
    Result result = writeId(id);
    if (result) result = writer_.writeFloat64(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeText(uint16_t id, StringView value) {
    Result result = writeId(id);
    if (result && value.size > limits_.maxTextLength) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) result = writer_.writeText(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeBytes(uint16_t id, ByteView value) {
    Result result = writeId(id);
    if (result && value.size > limits_.maxByteStringLength) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) result = writer_.writeBytes(value);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::writeEncoded(
    uint16_t id,
    ByteView value,
    const cbor::Limits& limits) {
    Result result = writeId(id);
    cbor::Limits nestedLimits = limits;
    const uint8_t availableDepth = limits_.maxDepth == 0U
                                       ? 0U
                                       : static_cast<uint8_t>(limits_.maxDepth - 1U);
    if (nestedLimits.maxDepth == 0U ||
        nestedLimits.maxDepth > availableDepth) {
        nestedLimits.maxDepth = availableDepth;
    }
    if (nestedLimits.maxTextLength > limits_.maxTextLength) {
        nestedLimits.maxTextLength = limits_.maxTextLength;
    }
    if (nestedLimits.maxByteStringLength > limits_.maxByteStringLength) {
        nestedLimits.maxByteStringLength = limits_.maxByteStringLength;
    }
    if (nestedLimits.maxContainerItems > limits_.maxContainerItems) {
        nestedLimits.maxContainerItems = limits_.maxContainerItems;
    }
    if (result) result = writer_.writeEncodedValue(value, nestedLimits);
    if (!result) failed_ = true;
    if (result) ++writtenFields_;
    return result;
}

Result IdBodyWriter::finish(ByteView& encoded) {
    if (!begun_ || failed_ || writtenFields_ != expectedFields_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    encoded = writer_.view();
    return Result::success();
}

} // namespace bbp2
} // namespace blinker
