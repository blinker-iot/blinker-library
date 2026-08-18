#include "Client.h"

#include <stdint.h>

#include "PatchBuilder.h"

namespace blinker {

namespace {

bool overlaps(MutableByteSpan first, MutableByteSpan second) {
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

} // namespace

Client::Client(
    const ClientBuffers& buffers,
    const DeviceRuntimeConfig& config)
    : transports_(),
      registry_(buffers.endpointTable),
      endpoints_(
          *this,
          registry_,
          buffers.endpointSlots,
          buffers.endpointCapacity),
      stateStore_(
          registry_,
          buffers.stateSlots,
          buffers.stateSlotCount,
          buffers.stateArena),
      runtime_(transports_, registry_, buffers.transmitBuffer, config),
      transmitBuffer_(buffers.transmitBuffer),
      patchBuffer_(buffers.patchBuffer),
      prepared_(false),
      started_(false),
      patchBufferBusy_(false),
      stateWriteHandler_(nullptr),
      stateWriteContext_(nullptr),
      stateApplyActive_(false) {
    runtime_.setCommandHandler(&Client::commandThunk, this);
    runtime_.setStateEncoder(&Client::stateEncoderThunk, this);
    runtime_.setStatePageEncoder(&Client::statePageEncoderThunk, this);
    runtime_.setStateApplyTransactionHandler(
        &Client::stateApplyTransactionThunk, this);
}

Result Client::addTransport(
    IFrameTransport& transport,
    TransportLifecyclePolicy lifecycle) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    return transports_.addTransport(transport, lifecycle);
}

Result Client::removeTransport(IFrameTransport& transport) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    return transports_.removeTransport(transport);
}

Result Client::prepare() {
    if (prepared_) return Result::success();
    Result result = endpoints_.validateStorage();
    if (!result) return result;
    if (patchBuffer_.data == nullptr || patchBuffer_.size == 0U ||
        transmitBuffer_.data == nullptr || transmitBuffer_.size == 0U ||
        overlaps(patchBuffer_, transmitBuffer_) ||
        stateStore_.overlaps(patchBuffer_) ||
        stateStore_.overlaps(transmitBuffer_)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    result = stateStore_.validateStorage();
    if (!result) return result;
    result = registry_.seal();
    if (result) prepared_ = true;
    return result;
}

Result Client::begin() {
    if (started_) return Result::failure(ErrorCode::AlreadyExists);
    Result result = prepare();
    if (!result) return result;
    result = runtime_.start();
    if (result) started_ = true;
    return result;
}

void Client::end() {
    if (!started_) return;
    runtime_.stop();
    started_ = false;
}

void Client::poll(uint32_t totalBudgetMicros) {
    runtime_.poll(totalBudgetMicros);
}

ClientStatus Client::status() const {
    ClientStatus result;
    result.prepared = prepared_;
    result.started = started_;
    result.transportCount = transports_.size();
    for (size_t index = 0; index < transports_.size(); ++index) {
        IFrameTransport* transport = transports_.at(index);
        if (transport == nullptr) continue;
        const TransportState state = transport->state();
        if (state == TransportState::Online) {
            ++result.onlineCount;
            const uint16_t features = transport->capabilities().features;
            if ((features & TransportFeatureLocal) != 0U) {
                result.localReady = true;
            }
            if ((features & TransportFeatureCloud) != 0U) {
                result.cloudReady = true;
            }
        } else if (state == TransportState::Starting) {
            ++result.startingCount;
        } else if (state == TransportState::Backoff) {
            ++result.backoffCount;
        } else if (state == TransportState::Error) {
            ++result.errorCount;
        }
    }
    return result;
}

Result Client::transportStatus(
    size_t index,
    TransportState& state,
    TransportCapabilities& capabilities) const {
    IFrameTransport* transport = transports_.at(index);
    if (transport == nullptr) return Result::failure(ErrorCode::NotFound);
    state = transport->state();
    capabilities = transport->capabilities();
    return Result::success();
}

Result Client::setAuthorizationProvider(IAuthorizationProvider* provider) {
    return runtime_.setAuthorizationProvider(provider);
}

Result Client::setControllerControlEndpoint(
    IControllerControlEndpoint* endpoint) {
    return runtime_.setControllerControlEndpoint(endpoint);
}

Result Client::setReliableOutbox(ReliableOutbox* outbox) {
    return runtime_.setReliableOutbox(outbox);
}

Result Client::setStateWriteHandler(
    StateWriteTransactionHandler handler,
    void* context) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    stateWriteHandler_ = handler;
    stateWriteContext_ = context;
    return Result::success();
}

void Client::setEventHandler(EventHandler handler, void* context) {
    runtime_.setEventHandler(handler, context);
}

Result Client::commandThunk(
    void* context,
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    const RxContext& rx) {
    Client* client = static_cast<Client*>(context);
    return client != nullptr
               ? client->dispatchCommand(
                     endpoint, encodedValue, valueType, rx)
               : Result::failure(ErrorCode::InvalidArgument);
}

Result Client::stateEncoderThunk(
    void* context,
    bbp2::KeyedBodyWriter& writer) {
    Client* client = static_cast<Client*>(context);
    return client != nullptr
               ? client->stateStore_.encodeSnapshot(writer)
               : Result::failure(ErrorCode::InvalidArgument);
}

Result Client::statePageEncoderThunk(
    void* context,
    uint32_t revision,
    uint16_t cursor,
    bool idMode,
    MutableByteSpan output,
    ByteView& encoded,
    uint16_t& nextCursor,
    uint16_t& totalFields) {
    Client* client = static_cast<Client*>(context);
    return client != nullptr
               ? client->stateStore_.encodeSnapshotPage(
                     revision,
                     cursor,
                     idMode,
                     output,
                     encoded,
                     nextCursor,
                     totalFields)
               : Result::failure(ErrorCode::InvalidArgument);
}

Result Client::stateApplyTransactionThunk(
    void* context,
    ByteView values,
    bool idMode,
    const RxContext& rx,
    bool& changed) {
    Client* client = static_cast<Client*>(context);
    if (client == nullptr) {
        changed = false;
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = idMode
                        ? client->stateStore_.validateById(
                              values, AccessWrite, changed)
                        : client->stateStore_.validateKeyed(
                              values, AccessWrite, changed);
    if (!result || !changed) return result;
    if (client->stateWriteHandler_ == nullptr) {
        changed = false;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (client->stateApplyActive_) {
        changed = false;
        return Result::failure(ErrorCode::WouldBlock);
    }

    client->stateApplyActive_ = true;
    result = client->stateWriteHandler_(
        client->stateWriteContext_, values, idMode, rx);
    client->stateApplyActive_ = false;
    if (!result) {
        changed = false;
        return result;
    }

    bool committed = false;
    result = idMode
                 ? client->stateStore_.applyById(
                       values, AccessWrite, committed)
                 : client->stateStore_.applyKeyed(
                       values, AccessWrite, committed);
    if (result && !committed) {
        changed = false;
        return Result::failure(ErrorCode::InternalError);
    }
    changed = result && committed;
    return result;
}

Result Client::dispatchCommand(
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    const RxContext& rx) {
    EndpointSlot* slot = endpoints_.slotFor(endpoint);
    if (slot == nullptr || slot->command == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint16_t id = 0U;
    Result result = registry_.idOf(endpoint.key, id);
    if (!result) return result;
    EndpointHandle handle = endpoints_.fromSlot(*slot, id, ErrorCode::Ok);
    if (!handle.valid()) return Result::failure(handle.error());
    const EndpointValueView value(encodedValue, valueType);
    return slot->command(slot->commandContext, handle, value, rx);
}

Result Client::publish(const EndpointHandle& endpoint, bool value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publish(const EndpointHandle& endpoint, int64_t value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publish(const EndpointHandle& endpoint, uint64_t value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publish(const EndpointHandle& endpoint, float value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publish(const EndpointHandle& endpoint, double value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publishText(
    const EndpointHandle& endpoint,
    StringView value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.writeText(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publishBytes(
    const EndpointHandle& endpoint,
    ByteView value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.writeBytes(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::publishEncoded(
    const EndpointHandle& endpoint,
    ByteView value) {
    PatchBuilder builder(registry_, patchBuffer_);
    Result result = beginStatePatch(builder);
    if (result) result = builder.writeEncoded(endpoint, value);
    return result ? finishStatePatch(builder) : result;
}

Result Client::emit(const EndpointHandle& endpoint, bool value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emit(const EndpointHandle& endpoint, int64_t value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emit(const EndpointHandle& endpoint, uint64_t value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emit(const EndpointHandle& endpoint, float value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emit(const EndpointHandle& endpoint, double value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.write(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emitText(
    const EndpointHandle& endpoint,
    StringView value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.writeText(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emitBytes(
    const EndpointHandle& endpoint,
    ByteView value) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.writeBytes(endpoint, value);
    return result ? finishEventPatch(builder) : result;
}

Result Client::emitNull(const EndpointHandle& endpoint) {
    PatchBuilder builder(
        registry_, patchBuffer_, cbor::Limits(), AccessEvent);
    Result result = beginEventPatch(builder);
    if (result) result = builder.writeNull(endpoint);
    return result ? finishEventPatch(builder) : result;
}

Result Client::beginStatePatch(PatchBuilder& builder) const {
    if (!prepared_) return Result::failure(ErrorCode::NotConfigured);
    if (patchBufferBusy_ || stateApplyActive_) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    return builder.begin(1U);
}

Result Client::beginEventPatch(PatchBuilder& builder) const {
    if (!started_) return Result::failure(ErrorCode::NotConnected);
    if (patchBufferBusy_ || stateApplyActive_) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    return builder.begin(1U);
}

Result Client::finishStatePatch(PatchBuilder& builder) {
    ByteView encoded;
    Result result = builder.finish(encoded);
    return result ? commitState(encoded) : result;
}

Result Client::finishEventPatch(PatchBuilder& builder) {
    ByteView encoded;
    Result result = builder.finish(encoded);
    SendTarget target;
    target.kind = SendTargetKind::Broadcast;
    return result ? runtime_.sendEvent(encoded, target) : result;
}

Result Client::commitState(ByteView values) {
    if (!prepared_) return Result::failure(ErrorCode::NotConfigured);
    if (runtime_.stateRevision() == UINT32_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    bool changed = false;
    Result result = stateStore_.applyKeyed(
        values,
        static_cast<uint8_t>(AccessRead | AccessNotify),
        changed);
    if (!result || !changed) return result;

    uint32_t revision = 0U;
    result = runtime_.advanceStateRevision(revision);
    if (!result) return result;
    return started_
               ? runtime_.fanOutCurrentPatch(values)
               : Result::success();
}

Result Client::currentState(
    const EndpointHandle& endpoint,
    ByteView& encoded,
    cbor::Type& type) const {
    if (!endpoint.valid() || endpoint.registry_ != &registry_ ||
        endpoint.slot_ == nullptr) {
        return Result::failure(
            endpoint.valid() ? ErrorCode::InvalidArgument : endpoint.error());
    }
    const EndpointDescriptor* descriptor = registry_.findById(endpoint.id_);
    return descriptor != nullptr
               ? stateStore_.value(*descriptor, encoded, type)
               : Result::failure(ErrorCode::NotFound);
}

EndpointHandle Client::handleById(uint16_t id) {
    const EndpointDescriptor* descriptor = registry_.findById(id);
    if (descriptor == nullptr) return EndpointHandle();
    EndpointSlot* slot = endpoints_.slotFor(*descriptor);
    return slot != nullptr
               ? endpoints_.fromSlot(*slot, id, ErrorCode::Ok)
               : EndpointHandle();
}

Result Client::acquirePatchBuffer() {
    if (!prepared_) return Result::failure(ErrorCode::NotConfigured);
    if (patchBufferBusy_ || stateApplyActive_) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    patchBufferBusy_ = true;
    return Result::success();
}

void Client::releasePatchBuffer() {
    patchBufferBusy_ = false;
}

} // namespace blinker
