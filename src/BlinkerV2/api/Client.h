#ifndef BLINKER_API_CLIENT_H
#define BLINKER_API_CLIENT_H

#include "EndpointHandle.h"
#include "../model/EndpointStateStore.h"
#include "../runtime/DeviceRuntime.h"

namespace blinker {

class PatchBuilder;

// Called only after the complete incoming PATCH Apply transaction has passed
// schema, access, constraint and state-capacity preflight. The application
// must accept all values as one transaction or return an error. The encoded
// view is borrowed for the callback duration.
typedef Result (*StateWriteTransactionHandler)(
    void* context,
    ByteView values,
    bool idMode,
    const RxContext& rx);

struct ClientBuffers {
    EndpointSlot* endpointSlots;
    EndpointDescriptorTable endpointTable;
    size_t endpointCount;
    size_t endpointCapacity;
    MutableByteSpan transmitBuffer;
    MutableByteSpan patchBuffer;
    EndpointStateSlot* stateSlots;
    size_t stateSlotCount;
    MutableByteSpan stateArena;

    ClientBuffers()
        : endpointSlots(nullptr),
          endpointTable(),
          endpointCount(0),
          endpointCapacity(0),
          transmitBuffer(),
          patchBuffer(),
          stateSlots(nullptr),
          stateSlotCount(0),
          stateArena() {}
};

// Snapshot of service readiness, intentionally separating a locally
// available transport (for example BLE advertising/GATT ready) from a cloud
// data-plane transport. HTTP credentials being ready does not make cloudReady
// true; only an Online transport with TransportFeatureCloud does.
struct ClientStatus {
    size_t transportCount;
    size_t onlineCount;
    size_t startingCount;
    size_t backoffCount;
    size_t errorCount;
    bool prepared;
    bool started;
    bool localReady;
    bool cloudReady;

    ClientStatus()
        : transportCount(0),
          onlineCount(0),
          startingCount(0),
          backoffCount(0),
          errorCount(0),
          prepared(false),
          started(false),
          localReady(false),
          cloudReady(false) {}
};

// StateArenaSize is the total retained canonical-CBOR capacity shared by all
// fields; it is deliberately not multiplied by EndpointCapacity. Budget it as
// the maximum simultaneously retained values (for example bool=1 B,
// float32=5 B, float64/worst integer=9 B, plus CBOR head and content for
// text/bytes/containers).
template <
    size_t EndpointCapacity,
    size_t FrameSize,
    size_t PatchSize,
    size_t StateArenaSize>
class ClientStorage {
public:
    ClientStorage()
        : endpointSlots_(),
          frame_(),
          patch_(),
          stateSlots_(),
          stateArena_() {
        static_assert(EndpointCapacity > 0U, "EndpointCapacity must be positive");
        static_assert(FrameSize >= bbp2::kBaseHeaderSize, "FrameSize is too small");
        static_assert(PatchSize > 0U, "PatchSize must be positive");
        static_assert(StateArenaSize > 0U, "StateArenaSize must be positive");
        static_assert(
            StateArenaSize <= UINT16_MAX,
            "StateArenaSize exceeds compact state-offset range");
    }

    ClientBuffers buffers(
        const EndpointDescriptor* descriptors,
        size_t count) {
        return buffers(
            EndpointDescriptorTable(
                descriptors,
                &ClientStorage::contiguousDescriptorAt,
                count));
    }

    ClientBuffers buffers(const EndpointDescriptorTable& descriptors) {
        ClientBuffers result;
        result.endpointSlots = endpointSlots_;
        result.endpointTable = descriptors;
        result.endpointCount = descriptors.count;
        result.endpointCapacity = EndpointCapacity;
        result.transmitBuffer = MutableByteSpan(frame_, sizeof(frame_));
        result.patchBuffer = MutableByteSpan(patch_, sizeof(patch_));
        result.stateSlots = stateSlots_;
        result.stateSlotCount = EndpointCapacity;
        result.stateArena = MutableByteSpan(stateArena_, sizeof(stateArena_));
        return result;
    }

    template <size_t N>
    ClientBuffers buffers(const EndpointDescriptor (&descriptors)[N]) {
        static_assert(
            N <= EndpointCapacity,
            "descriptor table exceeds ClientStorage endpoint capacity");
        return buffers(descriptors, N);
    }

private:
    static const EndpointDescriptor* contiguousDescriptorAt(
        const void* context,
        size_t index) {
        const EndpointDescriptor* descriptors =
            static_cast<const EndpointDescriptor*>(context);
        return descriptors != nullptr ? &descriptors[index] : nullptr;
    }

    EndpointSlot endpointSlots_[EndpointCapacity];
    uint8_t frame_[FrameSize];
    uint8_t patch_[PatchSize];
    EndpointStateSlot stateSlots_[EndpointCapacity];
    uint8_t stateArena_[StateArenaSize];
};

// Canonical device API. It owns only the fixed-capacity registry, transport
// hub and Runtime state. Platform services, native clients, adapters and all
// backing buffers remain application-owned.
class Client {
public:
    Client(
        const ClientBuffers& buffers,
        const DeviceRuntimeConfig& config = DeviceRuntimeConfig());

    EndpointCatalog& endpoints() { return endpoints_; }
    const EndpointRegistry& endpointRegistry() const { return registry_; }

    Result addTransport(
        IFrameTransport& transport,
        TransportLifecyclePolicy lifecycle =
            TransportLifecyclePolicy::Managed);
    Result removeTransport(IFrameTransport& transport);
    // Validates and seals the model/storage without starting transports.
    // Property state may be committed after prepare(); Events still require
    // the data plane to be started with begin().
    Result prepare();
    Result begin();
    void end();
    void poll(uint32_t totalBudgetMicros);
    ClientStatus status() const;
    Result transportStatus(
        size_t index,
        TransportState& state,
        TransportCapabilities& capabilities) const;

    Result setAuthorizationProvider(IAuthorizationProvider* provider);
    Result setReliableOutbox(ReliableOutbox* outbox);
    Result setStateWriteHandler(
        StateWriteTransactionHandler handler,
        void* context);
    void setEventHandler(EventHandler handler, void* context);

    const DeviceRuntime& runtime() const { return runtime_; }

private:
    static Result commandThunk(
        void* context,
        const EndpointDescriptor& endpoint,
        ByteView encodedValue,
        cbor::Type valueType,
        const RxContext& rx);
    static Result stateEncoderThunk(
        void* context,
        bbp2::KeyedBodyWriter& writer);
    static Result statePageEncoderThunk(
        void* context,
        uint32_t revision,
        uint16_t cursor,
        bool idMode,
        MutableByteSpan output,
        ByteView& encoded,
        uint16_t& nextCursor,
        uint16_t& totalFields);
    static Result stateApplyTransactionThunk(
        void* context,
        ByteView values,
        bool idMode,
        const RxContext& rx,
        bool& changed);
    Result dispatchCommand(
        const EndpointDescriptor& endpoint,
        ByteView encodedValue,
        cbor::Type valueType,
        const RxContext& rx);

    Result publish(const EndpointHandle& endpoint, bool value);
    Result publish(const EndpointHandle& endpoint, int64_t value);
    Result publish(const EndpointHandle& endpoint, uint64_t value);
    Result publish(const EndpointHandle& endpoint, float value);
    Result publish(const EndpointHandle& endpoint, double value);
    Result publishText(const EndpointHandle& endpoint, StringView value);
    Result publishBytes(const EndpointHandle& endpoint, ByteView value);
    Result publishEncoded(const EndpointHandle& endpoint, ByteView value);
    Result emit(const EndpointHandle& endpoint, bool value);
    Result emit(const EndpointHandle& endpoint, int64_t value);
    Result emit(const EndpointHandle& endpoint, uint64_t value);
    Result emit(const EndpointHandle& endpoint, float value);
    Result emit(const EndpointHandle& endpoint, double value);
    Result emitText(const EndpointHandle& endpoint, StringView value);
    Result emitBytes(const EndpointHandle& endpoint, ByteView value);
    Result emitNull(const EndpointHandle& endpoint);
    Result beginStatePatch(PatchBuilder& builder) const;
    Result beginEventPatch(PatchBuilder& builder) const;
    Result finishStatePatch(PatchBuilder& builder);
    Result finishEventPatch(PatchBuilder& builder);
    Result commitState(ByteView values);
    Result currentState(
        const EndpointHandle& endpoint,
        ByteView& encoded,
        cbor::Type& type) const;
    EndpointHandle handleById(uint16_t id);
    Result acquirePatchBuffer();
    void releasePatchBuffer();

    TransportHub transports_;
    EndpointRegistry registry_;
    EndpointCatalog endpoints_;
    EndpointStateStore stateStore_;
    DeviceRuntime runtime_;
    MutableByteSpan transmitBuffer_;
    MutableByteSpan patchBuffer_;
    bool prepared_;
    bool started_;
    bool patchBufferBusy_;
    StateWriteTransactionHandler stateWriteHandler_;
    void* stateWriteContext_;
    bool stateApplyActive_;

    Client(const Client&);
    Client& operator=(const Client&);

    friend class EndpointHandle;
    friend class StateUpdate;
};

} // namespace blinker

#endif
