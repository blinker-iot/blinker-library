#include "EndpointStateStore.h"

#include <stdint.h>
#include <string.h>

namespace blinker {

namespace {

bool spansOverlap(MutableByteSpan first, MutableByteSpan second) {
    if (first.data == nullptr || second.data == nullptr ||
        first.size == 0U || second.size == 0U) {
        return false;
    }
    const uintptr_t firstBegin = reinterpret_cast<uintptr_t>(first.data);
    const uintptr_t secondBegin = reinterpret_cast<uintptr_t>(second.data);
    if (first.size > UINTPTR_MAX - firstBegin ||
        second.size > UINTPTR_MAX - secondBegin) {
        return true;
    }
    const uintptr_t firstEnd = firstBegin + first.size;
    const uintptr_t secondEnd = secondBegin + second.size;
    return firstBegin < secondEnd && secondBegin < firstEnd;
}

bool equalValue(
    const EndpointStateSlot& slot,
    ByteView arena,
    ByteView value) {
    return slot.initialized && slot.valueSize == value.size &&
           (value.size == 0U ||
            memcmp(arena.data + slot.offset, value.data, value.size) == 0);
}

Result discardEncoded(void*, ByteView) {
    return Result::success();
}

} // namespace

EndpointStateStore::EndpointStateStore(
    const EndpointRegistry& registry,
    EndpointStateSlot* slots,
    size_t slotCount,
    MutableByteSpan arena)
    : registry_(registry),
      slots_(slots),
      slotCount_(slotCount),
      arena_(arena),
      usedBytes_(0) {
    const size_t count = registry_.size() < slotCount_
                             ? registry_.size()
                             : slotCount_;
    for (size_t index = 0; slots_ != nullptr && index < count; ++index) {
        if (!slots_[index].initialized) continue;
        const size_t end =
            static_cast<size_t>(slots_[index].offset) +
            slots_[index].valueSize;
        if (end > usedBytes_) usedBytes_ = end;
    }
}

Result EndpointStateStore::validateStorage() const {
    if (slots_ == nullptr || slotCount_ < registry_.size() ||
        arena_.data == nullptr || arena_.size == 0U ||
        arena_.size > UINT16_MAX || usedBytes_ > arena_.size) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    for (size_t index = 0; index < registry_.size(); ++index) {
        const EndpointStateSlot& slot = slots_[index];
        const size_t end =
            static_cast<size_t>(slot.offset) + slot.valueSize;
        if ((!slot.initialized &&
             (slot.offset != 0U || slot.valueSize != 0U ||
              slot.valueType != cbor::Type::Invalid)) ||
            (slot.initialized &&
             (slot.valueSize == 0U || end > usedBytes_ ||
              slot.valueType == cbor::Type::Invalid))) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (slot.initialized) {
            const EndpointDescriptor* endpoint = registry_.at(index);
            if (endpoint == nullptr) {
                return Result::failure(ErrorCode::InternalError);
            }
            Result result = validateEndpointValue(
                *endpoint,
                ByteView(arena_.data + slot.offset, slot.valueSize),
                slot.valueType,
                static_cast<uint8_t>(AccessRead | AccessNotify));
            if (!result) return result;
            for (size_t prior = 0; prior < index; ++prior) {
                const EndpointStateSlot& priorSlot = slots_[prior];
                if (!priorSlot.initialized) continue;
                const size_t priorEnd =
                    static_cast<size_t>(priorSlot.offset) +
                    priorSlot.valueSize;
                if (slot.offset < priorEnd &&
                    priorSlot.offset < end) {
                    return Result::failure(ErrorCode::NotConfigured);
                }
            }
        }
    }
    return Result::success();
}

bool EndpointStateStore::overlaps(MutableByteSpan buffer) const {
    return spansOverlap(arena_, buffer);
}

Result EndpointStateStore::indexOf(
    const EndpointDescriptor& endpoint,
    size_t& index) const {
    index = 0U;
    for (; index < registry_.size(); ++index) {
        if (registry_.at(index) == &endpoint) return Result::success();
    }
    return Result::failure(ErrorCode::NotFound);
}

Result EndpointStateStore::applyValue(
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    ApplyContext& context) {
    Result result = validateEndpointValue(
        endpoint,
        encodedValue,
        valueType,
        context.requiredAnyAccess);
    if (!result) return result;

    size_t index = 0U;
    result = indexOf(endpoint, index);
    if (!result) return result;
    if (index >= slotCount_ || slots_ == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (encodedValue.size == 0U || encodedValue.size > UINT16_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    EndpointStateSlot& slot = slots_[index];

    const bool valueChanged =
        !equalValue(
            slot,
            ByteView(arena_.data, usedBytes_),
            encodedValue) ||
        slot.valueType != valueType;
    context.changed = context.changed || valueChanged;
    if (!context.commit) {
        const size_t previousSize = slot.initialized ? slot.valueSize : 0U;
        if (previousSize > context.resultingSize) {
            return Result::failure(ErrorCode::InternalError);
        }
        context.resultingSize -= previousSize;
        if (encodedValue.size > SIZE_MAX - context.resultingSize) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        context.resultingSize += encodedValue.size;
        return Result::success();
    }
    if (context.commit && valueChanged) {
        if (encodedValue.size > arena_.size - usedBytes_) {
            return Result::failure(ErrorCode::InternalError);
        }
        memmove(arena_.data + usedBytes_, encodedValue.data, encodedValue.size);
        slot.offset = static_cast<uint16_t>(usedBytes_);
        slot.valueSize = static_cast<uint16_t>(encodedValue.size);
        slot.valueType = valueType;
        slot.initialized = true;
        usedBytes_ += encodedValue.size;
    }
    return Result::success();
}

Result EndpointStateStore::eraseIndex(size_t index) {
    if (slots_ == nullptr || index >= registry_.size() ||
        index >= slotCount_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    EndpointStateSlot& slot = slots_[index];
    if (!slot.initialized) return Result::success();
    const size_t begin = slot.offset;
    const size_t end = begin + slot.valueSize;
    if (end > usedBytes_) {
        return Result::failure(ErrorCode::InternalError);
    }
    const size_t removed = slot.valueSize;
    if (end < usedBytes_) {
        memmove(arena_.data + begin, arena_.data + end, usedBytes_ - end);
    }
    for (size_t other = 0; other < registry_.size(); ++other) {
        if (other == index || !slots_[other].initialized) continue;
        if (slots_[other].offset > begin) {
            slots_[other].offset = static_cast<uint16_t>(
                slots_[other].offset - removed);
        }
    }
    usedBytes_ -= removed;
    slot = EndpointStateSlot();
    return Result::success();
}

Result EndpointStateStore::eraseValue(
    const EndpointDescriptor& endpoint) {
    size_t index = 0U;
    Result result = indexOf(endpoint, index);
    return result ? eraseIndex(index) : result;
}

Result EndpointStateStore::keyedApplyThunk(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    ApplyContext* apply = static_cast<ApplyContext*>(context);
    if (apply == nullptr || apply->store == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    const EndpointDescriptor* endpoint =
        apply->store->registry_.find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return apply->store->applyValue(
        *endpoint, encodedValue, valueType, *apply);
}

Result EndpointStateStore::idApplyThunk(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    ApplyContext* apply = static_cast<ApplyContext*>(context);
    if (apply == nullptr || apply->store == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    const EndpointDescriptor* endpoint =
        apply->store->registry_.findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return apply->store->applyValue(
        *endpoint, encodedValue, valueType, *apply);
}

Result EndpointStateStore::keyedEraseThunk(
    void* context,
    StringView endpointKey,
    ByteView,
    cbor::Type) {
    EndpointStateStore* store =
        static_cast<EndpointStateStore*>(context);
    if (store == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    const EndpointDescriptor* endpoint = store->registry_.find(endpointKey);
    return endpoint != nullptr
               ? store->eraseValue(*endpoint)
               : Result::failure(ErrorCode::InternalError);
}

Result EndpointStateStore::idEraseThunk(
    void* context,
    uint16_t endpointId,
    ByteView,
    cbor::Type) {
    EndpointStateStore* store =
        static_cast<EndpointStateStore*>(context);
    if (store == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    const EndpointDescriptor* endpoint =
        store->registry_.findById(endpointId);
    return endpoint != nullptr
               ? store->eraseValue(*endpoint)
               : Result::failure(ErrorCode::InternalError);
}

Result EndpointStateStore::applyKeyed(
    ByteView values,
    uint8_t requiredAnyAccess,
    bool& changed) {
    Result result = validateKeyed(values, requiredAnyAccess, changed);
    if (!result || !changed) return result;

    result = bbp2::visitKeyedBody(
        values, &EndpointStateStore::keyedEraseThunk, this);
    if (!result) return Result::failure(ErrorCode::InternalError);

    ApplyContext commit;
    commit.store = this;
    commit.requiredAnyAccess = requiredAnyAccess;
    commit.commit = true;
    result = bbp2::visitKeyedBody(
        values, &EndpointStateStore::keyedApplyThunk, &commit);
    changed = result && commit.changed;
    return result;
}

Result EndpointStateStore::applyById(
    ByteView values,
    uint8_t requiredAnyAccess,
    bool& changed) {
    Result result = validateById(values, requiredAnyAccess, changed);
    if (!result || !changed) return result;

    result = bbp2::visitIdBody(
        values, &EndpointStateStore::idEraseThunk, this);
    if (!result) return Result::failure(ErrorCode::InternalError);

    ApplyContext commit;
    commit.store = this;
    commit.requiredAnyAccess = requiredAnyAccess;
    commit.commit = true;
    result = bbp2::visitIdBody(
        values, &EndpointStateStore::idApplyThunk, &commit);
    changed = result && commit.changed;
    return result;
}

Result EndpointStateStore::validateKeyed(
    ByteView values,
    uint8_t requiredAnyAccess,
    bool& changed) {
    changed = false;
    if (requiredAnyAccess == AccessNone ||
        (requiredAnyAccess & ~kKnownPropertyAccess) != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (overlaps(MutableByteSpan(
            const_cast<uint8_t*>(values.data), values.size))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ApplyContext preflight;
    preflight.store = this;
    preflight.requiredAnyAccess = requiredAnyAccess;
    preflight.resultingSize = usedBytes_;
    Result result = bbp2::visitKeyedBody(
        values, &EndpointStateStore::keyedApplyThunk, &preflight);
    if (result && preflight.resultingSize > arena_.size) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    changed = result && preflight.changed;
    return result;
}

Result EndpointStateStore::validateById(
    ByteView values,
    uint8_t requiredAnyAccess,
    bool& changed) {
    changed = false;
    if (requiredAnyAccess == AccessNone ||
        (requiredAnyAccess & ~kKnownPropertyAccess) != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (overlaps(MutableByteSpan(
            const_cast<uint8_t*>(values.data), values.size))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ApplyContext preflight;
    preflight.store = this;
    preflight.requiredAnyAccess = requiredAnyAccess;
    preflight.resultingSize = usedBytes_;
    Result result = bbp2::visitIdBody(
        values, &EndpointStateStore::idApplyThunk, &preflight);
    if (result && preflight.resultingSize > arena_.size) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    changed = result && preflight.changed;
    return result;
}

size_t EndpointStateStore::initializedCount() const {
    if (slots_ == nullptr) return 0U;
    size_t count = 0U;
    const size_t limit =
        registry_.size() < slotCount_ ? registry_.size() : slotCount_;
    for (size_t index = 0; index < limit; ++index) {
        if (slots_[index].initialized) ++count;
    }
    return count;
}

Result EndpointStateStore::encodeSnapshot(
    bbp2::KeyedBodyWriter& writer) const {
    Result result = writer.begin(initializedCount());
    for (size_t index = 0; result && index < registry_.size(); ++index) {
        if (index >= slotCount_ || slots_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        const EndpointStateSlot& slot = slots_[index];
        if (!slot.initialized) continue;
        const EndpointDescriptor* endpoint = registry_.at(index);
        const size_t end =
            static_cast<size_t>(slot.offset) + slot.valueSize;
        if (endpoint == nullptr || end > usedBytes_) {
            return Result::failure(ErrorCode::InternalError);
        }
        result = writer.writeEncoded(
            endpoint->key,
            ByteView(arena_.data + slot.offset, slot.valueSize));
    }
    return result;
}

Result EndpointStateStore::writeSnapshotPageBody(
    cbor::Writer& writer,
    uint32_t revision,
    uint16_t cursor,
    uint16_t nextCursor,
    uint16_t totalFields,
    bool idMode,
    const cbor::Limits& limits) const {
    if (cursor > nextCursor || nextCursor > totalFields ||
        totalFields != registry_.size() || slots_ == nullptr ||
        slotCount_ < registry_.size()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxDepth < 2U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < 5U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    size_t valueCount = 0U;
    for (uint16_t index = cursor; index < nextCursor; ++index) {
        if (slots_[index].initialized) ++valueCount;
    }
    if (valueCount > limits.maxContainerItems) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    Result result = writer.beginMap(5U);
    if (result) result = writer.writeUnsigned(0U);
    if (result) result = writer.writeUnsigned(revision);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeUnsigned(cursor);
    if (result) result = writer.writeUnsigned(2U);
    if (result) result = writer.writeUnsigned(nextCursor);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeUnsigned(totalFields);
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.beginMap(valueCount);

    cbor::Limits valueLimits = limits;
    --valueLimits.maxDepth;
    for (uint16_t index = cursor;
         result && index < nextCursor;
         ++index) {
        const EndpointStateSlot& slot = slots_[index];
        if (!slot.initialized) continue;
        const EndpointDescriptor* endpoint = registry_.at(index);
        const size_t end =
            static_cast<size_t>(slot.offset) + slot.valueSize;
        if (endpoint == nullptr || end > usedBytes_) {
            return Result::failure(ErrorCode::InternalError);
        }
        if (idMode) {
            result = writer.writeUnsigned(static_cast<uint16_t>(index + 1U));
        } else if (endpoint->key.empty() ||
                   endpoint->key.size > limits.maxTextLength) {
            return Result::failure(ErrorCode::CapacityExceeded);
        } else {
            result = writer.writeText(endpoint->key);
        }
        if (result) {
            result = writer.writeEncodedValue(
                ByteView(arena_.data + slot.offset, slot.valueSize),
                valueLimits);
        }
    }
    return result;
}

Result EndpointStateStore::encodeSnapshotPage(
    uint32_t revision,
    uint16_t cursor,
    bool idMode,
    MutableByteSpan output,
    ByteView& encoded,
    uint16_t& nextCursor,
    uint16_t& totalFields,
    const cbor::Limits& limits) const {
    encoded = ByteView();
    nextCursor = cursor;
    totalFields = 0U;
    if (registry_.size() > UINT16_MAX ||
        (output.size != 0U && output.data == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    totalFields = static_cast<uint16_t>(registry_.size());
    if (cursor > totalFields) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }

    bool found = false;
    Result candidateError = Result::success();
    uint8_t countContext = 0U;
    size_t candidateValueCount = 0U;
    for (uint16_t candidate = cursor;; ++candidate) {
        if (candidate != cursor && slots_[candidate - 1U].initialized) {
            ++candidateValueCount;
        }
        // maxContainerItems is a per-page limit. Reaching it selects the
        // previous complete field boundary; it must not abort the snapshot.
        if (candidateValueCount > limits.maxContainerItems) break;
        cbor::Writer counter(&discardEncoded, &countContext);
        Result result = writeSnapshotPageBody(
            counter,
            revision,
            cursor,
            candidate,
            totalFields,
            idMode,
            limits);
        if (!result) {
            candidateError = result;
            break;
        }
        if (counter.size() > output.size) break;
        found = true;
        nextCursor = candidate;
        if (candidate == totalFields) break;
    }
    if (!candidateError) return candidateError;
    if (!found || (cursor < totalFields && nextCursor == cursor)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    cbor::Writer writer(output);
    Result result = writeSnapshotPageBody(
        writer,
        revision,
        cursor,
        nextCursor,
        totalFields,
        idMode,
        limits);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result EndpointStateStore::value(
    const EndpointDescriptor& endpoint,
    ByteView& encoded,
    cbor::Type& type) const {
    encoded = ByteView();
    type = cbor::Type::Invalid;
    size_t index = 0U;
    Result result = indexOf(endpoint, index);
    if (!result) return result;
    if (slots_ == nullptr || index >= slotCount_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    const EndpointStateSlot& slot = slots_[index];
    if (!slot.initialized) return Result::failure(ErrorCode::NotFound);
    const size_t end =
        static_cast<size_t>(slot.offset) + slot.valueSize;
    if (end > usedBytes_) {
        return Result::failure(ErrorCode::InternalError);
    }
    encoded = ByteView(arena_.data + slot.offset, slot.valueSize);
    type = slot.valueType;
    return Result::success();
}

Result EndpointStateStore::valueById(
    uint16_t endpointId,
    ByteView& encoded,
    cbor::Type& type) const {
    const EndpointDescriptor* endpoint = registry_.findById(endpointId);
    return endpoint != nullptr
               ? value(*endpoint, encoded, type)
               : Result::failure(ErrorCode::NotFound);
}

} // namespace blinker
