#ifndef BLINKER_API_ENDPOINTHANDLE_H
#define BLINKER_API_ENDPOINTHANDLE_H

#include "../model/EndpointRegistry.h"
#include "../protocol/cbor/Cbor.h"
#include "../transport/IFrameTransport.h"

namespace blinker {

class Client;
class EndpointCatalog;
class PatchBuilder;
class EndpointHandle;

class EndpointValueView {
public:
    EndpointValueView()
        : encoded_(), type_(cbor::Type::Invalid) {}
    EndpointValueView(ByteView encoded, cbor::Type type)
        : encoded_(encoded), type_(type) {}

    cbor::Type type() const { return type_; }
    ByteView encoded() const { return encoded_; }

    Result asBool(bool& value) const;
    Result asInt(int64_t& value) const;
    Result asUnsigned(uint64_t& value) const;
    Result asFloat(double& value) const;
    Result asText(StringView& value) const;
    Result asBytes(ByteView& value) const;
    Result asNull() const;

private:
    ByteView encoded_;
    cbor::Type type_;
};

typedef Result (*EndpointCommandCallback)(
    void* context,
    const EndpointHandle& endpoint,
    const EndpointValueView& value,
    const RxContext& rx);

// Compact mutable state supplied by ClientStorage. Field keys, types,
// constraints and access metadata live in the application-owned immutable
// EndpointDescriptor table and are never copied here.
struct EndpointSlot {
    EndpointCommandCallback command;
    void* commandContext;

    EndpointSlot()
        : command(nullptr), commandContext(nullptr) {}
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(EndpointSlot) <= 32U,
    "EndpointSlot must remain within the 32-byte MCU metadata budget");
#endif

class EndpointHandle {
public:
    EndpointHandle()
        : client_(nullptr),
          registry_(nullptr),
          slot_(nullptr),
          id_(0),
          error_(ErrorCode::NotFound) {}

    bool valid() const { return error_ == ErrorCode::Ok && slot_ != nullptr; }
    ErrorCode error() const { return error_; }
    StringView key() const;
    EndpointKind kind() const;
    uint16_t id() const { return valid() ? id_ : 0U; }

    Result onCommand(EndpointCommandCallback callback, void* context) const;

    // set() commits bounded authoritative state after Client::prepare().
    // When the data plane is started it also broadcasts a PATCH; offline state
    // remains available for the next revision-consistent snapshot.
    Result set(bool value) const;
    Result set(float value) const;
    Result set(double value) const;
    Result setInt(int64_t value) const;
    Result setUnsigned(uint64_t value) const;
    Result setText(StringView value) const;
    Result setBytes(ByteView value) const;
    Result setEncoded(ByteView value) const;
    // emit() publishes an EVENT and never mutates the authoritative state
    // store. It is valid only for an EndpointKind::Event descriptor.
    Result emit(bool value) const;
    Result emit(float value) const;
    Result emit(double value) const;
    Result emitInt(int64_t value) const;
    Result emitUnsigned(uint64_t value) const;
    Result emitText(StringView value) const;
    Result emitBytes(ByteView value) const;
    Result emitNull() const;
    // The returned view borrows the packed state arena. Any state mutation may
    // compact that arena, so decode/copy the view before calling set(),
    // StateUpdate::commit(), poll(), or any other path that can apply state.
    Result current(EndpointValueView& value) const;

private:
    EndpointHandle(
        Client* client,
        const EndpointRegistry* registry,
        EndpointSlot* slot,
        uint16_t id,
        ErrorCode error)
        : client_(client),
          registry_(registry),
          slot_(slot),
          id_(id),
          error_(error) {}

    Client* client_;
    const EndpointRegistry* registry_;
    EndpointSlot* slot_;
    uint16_t id_;
    ErrorCode error_;

    friend class Client;
    friend class EndpointCatalog;
    friend class PatchBuilder;
};

class EndpointCatalog {
public:
    EndpointCatalog(
        Client& client,
        EndpointRegistry& registry,
        EndpointSlot* slots,
        size_t capacity);

    EndpointHandle find(StringView key);

    template <size_t N>
    EndpointHandle find(const char (&key)[N]) {
        return find(StringView(key, N - 1U));
    }

private:
    Result validateStorage() const;
    EndpointHandle fromSlot(
        EndpointSlot& slot,
        uint16_t id,
        ErrorCode error);
    EndpointSlot* slotFor(const EndpointDescriptor& descriptor);

    Client& client_;
    EndpointRegistry& registry_;
    EndpointSlot* slots_;
    size_t capacity_;

    friend class Client;
};

} // namespace blinker

#endif
