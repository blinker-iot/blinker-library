#ifndef BLINKER_MODEL_ENDPOINT_STATE_STORE_H
#define BLINKER_MODEL_ENDPOINT_STATE_STORE_H

#include "EndpointRegistry.h"
#include "EndpointValue.h"
#include "../protocol/bbp2/KeyedBody.h"

namespace blinker {

// Compact mutable metadata for one endpoint value. All encoded CBOR values
// live back-to-back in one caller-owned arena; slots therefore do not retain
// one pointer or one worst-case buffer per field.
struct EndpointStateSlot {
    uint16_t offset;
    uint16_t valueSize;
    cbor::Type valueType;
    bool initialized;

    EndpointStateSlot()
        : offset(0),
          valueSize(0),
          valueType(cbor::Type::Invalid),
          initialized(false) {}
};

// Retains the authoritative endpoint state without heap allocation. Apply is
// transactional: every field and the final packed-arena size are preflighted,
// then all replaced values are removed before new bytes are appended. Thus a
// growth followed by a shrink succeeds whenever the final state fits, while a
// failed multi-endpoint update leaves all values intact.
class EndpointStateStore {
public:
    EndpointStateStore(
        const EndpointRegistry& registry,
        EndpointStateSlot* slots,
        size_t slotCount,
        MutableByteSpan arena);

    Result validateStorage() const;
    bool overlaps(MutableByteSpan buffer) const;
    size_t usedBytes() const { return usedBytes_; }
    size_t capacityBytes() const { return arena_.size; }
    size_t slotCapacity() const { return slotCount_; }

    Result applyKeyed(
        ByteView values,
        uint8_t requiredAnyAccess,
        bool& changed);
    Result applyById(
        ByteView values,
        uint8_t requiredAnyAccess,
        bool& changed);
    Result validateKeyed(
        ByteView values,
        uint8_t requiredAnyAccess,
        bool& changed);
    Result validateById(
        ByteView values,
        uint8_t requiredAnyAccess,
        bool& changed);

    Result encodeSnapshot(bbp2::KeyedBodyWriter& writer) const;
    Result encodeSelection(
        bbp2::KeyedBodyWriter& writer,
        ByteView selectedFields,
        size_t selectedCount) const;
    // Encodes one complete StatePage body directly into output. cursor and
    // nextCursor are zero-based Field Manifest ordinals; ID-mode keys are the
    // corresponding one-based field IDs. Uninitialized fields are covered by
    // the cursor range but omitted from values.
    Result encodeSnapshotPage(
        uint32_t revision,
        uint16_t cursor,
        bool idMode,
        MutableByteSpan output,
        ByteView& encoded,
        uint16_t& nextCursor,
        uint16_t& totalFields,
        const cbor::Limits& limits = cbor::Limits()) const;
    // encoded borrows the packed arena and is invalidated by any successful
    // state mutation, not only by an update of the requested endpoint.
    Result value(
        const EndpointDescriptor& endpoint,
        ByteView& encoded,
        cbor::Type& type) const;
    Result valueById(
        uint16_t endpointId,
        ByteView& encoded,
        cbor::Type& type) const;
    size_t initializedCount() const;
    bool initializedAt(size_t index) const {
        return slots_ != nullptr && index < registry_.size() &&
               index < slotCount_ && slots_[index].initialized;
    }

private:
    struct ApplyContext {
        EndpointStateStore* store;
        uint8_t requiredAnyAccess;
        bool commit;
        bool changed;
        size_t resultingSize;

        ApplyContext()
            : store(nullptr),
              requiredAnyAccess(AccessNone),
              commit(false),
              changed(false),
              resultingSize(0) {}
    };

    static Result keyedApplyThunk(
        void* context,
        StringView endpointKey,
        ByteView encodedValue,
        cbor::Type valueType);
    static Result idApplyThunk(
        void* context,
        uint16_t endpointId,
        ByteView encodedValue,
        cbor::Type valueType);
    static Result keyedEraseThunk(
        void* context,
        StringView endpointKey,
        ByteView encodedValue,
        cbor::Type valueType);
    static Result idEraseThunk(
        void* context,
        uint16_t endpointId,
        ByteView encodedValue,
        cbor::Type valueType);
    Result applyValue(
        const EndpointDescriptor& endpoint,
        ByteView encodedValue,
        cbor::Type valueType,
        ApplyContext& context);
    Result eraseValue(const EndpointDescriptor& endpoint);
    Result eraseIndex(size_t index);
    Result writeSnapshotPageBody(
        cbor::Writer& writer,
        uint32_t revision,
        uint16_t cursor,
        uint16_t nextCursor,
        uint16_t totalFields,
        bool idMode,
        const cbor::Limits& limits) const;
    Result nextCanonicalIndex(
        size_t begin,
        size_t end,
        ByteView selectedFields,
        bool selectedOnly,
        StringView previousKey,
        bool hasPreviousKey,
        size_t& index) const;
    Result indexOf(
        const EndpointDescriptor& endpoint,
        size_t& index) const;

    const EndpointRegistry& registry_;
    EndpointStateSlot* slots_;
    size_t slotCount_;
    MutableByteSpan arena_;
    size_t usedBytes_;
};

} // namespace blinker

#endif
