#include "EndpointHandle.h"

#include <string.h>

#include "Client.h"

namespace blinker {

namespace {

Result finishRead(const cbor::Reader& reader, Result result) {
    if (!result) return result;
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

} // namespace

Result EndpointValueView::asBool(bool& value) const {
    if (type_ != cbor::Type::Boolean) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readBool(value));
}

Result EndpointValueView::asInt(int64_t& value) const {
    if (type_ != cbor::Type::Unsigned && type_ != cbor::Type::Negative) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readInt(value));
}

Result EndpointValueView::asUnsigned(uint64_t& value) const {
    if (type_ != cbor::Type::Unsigned) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readUnsigned(value));
}

Result EndpointValueView::asFloat(double& value) const {
    if (type_ != cbor::Type::Float) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readFloat(value));
}

Result EndpointValueView::asText(StringView& value) const {
    if (type_ != cbor::Type::Text) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readText(value));
}

Result EndpointValueView::asBytes(ByteView& value) const {
    if (type_ != cbor::Type::Bytes) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readBytes(value));
}

Result EndpointValueView::asNull() const {
    if (type_ != cbor::Type::Null) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Reader reader(encoded_);
    return finishRead(reader, reader.readNull());
}

StringView EndpointHandle::key() const {
    const EndpointDescriptor* descriptor =
        valid() && registry_ != nullptr ? registry_->findById(id_) : nullptr;
    return descriptor != nullptr ? descriptor->key : StringView();
}

EndpointKind EndpointHandle::kind() const {
    const EndpointDescriptor* descriptor =
        valid() && registry_ != nullptr ? registry_->findById(id_) : nullptr;
    return descriptor != nullptr ? descriptor->kind : EndpointKind::Invalid;
}

Result EndpointHandle::onCommand(
    EndpointCommandCallback callback,
    void* context) const {
    if (!valid()) return Result::failure(error_);
    const EndpointDescriptor* descriptor = registry_->findById(id_);
    if (descriptor == nullptr ||
        descriptor->telemetryMinimumIntervalMs != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    slot_->callback.command = callback;
    slot_->context = context;
    return Result::success();
}

Result EndpointHandle::onSample(
    EndpointSampleCallback callback,
    void* context) const {
    if (!valid()) return Result::failure(error_);
    const EndpointDescriptor* descriptor = registry_->findById(id_);
    if (descriptor == nullptr ||
        descriptor->telemetryMinimumIntervalMs == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    slot_->callback.sample = callback;
    slot_->context = context;
    return Result::success();
}

Result EndpointHandle::set(bool value) const {
    return valid() && client_ != nullptr
               ? client_->publish(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::set(float value) const {
    return valid() && client_ != nullptr
               ? client_->publish(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::set(double value) const {
    return valid() && client_ != nullptr
               ? client_->publish(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::setInt(int64_t value) const {
    return valid() && client_ != nullptr
               ? client_->publish(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::setUnsigned(uint64_t value) const {
    return valid() && client_ != nullptr
               ? client_->publish(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::setText(StringView value) const {
    return valid() && client_ != nullptr
               ? client_->publishText(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::setBytes(ByteView value) const {
    return valid() && client_ != nullptr
               ? client_->publishBytes(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::setEncoded(ByteView value) const {
    return valid() && client_ != nullptr
               ? client_->publishEncoded(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emit(bool value) const {
    return valid() && client_ != nullptr
               ? client_->emit(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emit(float value) const {
    return valid() && client_ != nullptr
               ? client_->emit(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emit(double value) const {
    return valid() && client_ != nullptr
               ? client_->emit(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emitInt(int64_t value) const {
    return valid() && client_ != nullptr
               ? client_->emit(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emitUnsigned(uint64_t value) const {
    return valid() && client_ != nullptr
               ? client_->emit(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emitText(StringView value) const {
    return valid() && client_ != nullptr
               ? client_->emitText(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emitBytes(ByteView value) const {
    return valid() && client_ != nullptr
               ? client_->emitBytes(*this, value)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::emitNull() const {
    return valid() && client_ != nullptr
               ? client_->emitNull(*this)
               : Result::failure(valid() ? ErrorCode::NotConfigured : error_);
}

Result EndpointHandle::current(EndpointValueView& value) const {
    if (!valid() || client_ == nullptr) {
        return Result::failure(valid() ? ErrorCode::NotConfigured : error_);
    }
    ByteView encoded;
    cbor::Type type = cbor::Type::Invalid;
    Result result = client_->currentState(*this, encoded, type);
    if (result) value = EndpointValueView(encoded, type);
    return result;
}

EndpointCatalog::EndpointCatalog(
    Client& client,
    EndpointRegistry& registry,
    EndpointSlot* slots,
    size_t capacity)
    : client_(client),
      registry_(registry),
      slots_(slots),
      capacity_(capacity) {}

EndpointSlot* EndpointCatalog::slotFor(
    const EndpointDescriptor& descriptor) {
    uint16_t id = 0U;
    Result result = registry_.idOf(descriptor.key, id);
    if (!result || id == 0U || slots_ == nullptr || id > capacity_) {
        return nullptr;
    }
    return &slots_[id - 1U];
}

Result EndpointCatalog::validateStorage() const {
    return slots_ != nullptr && capacity_ >= registry_.size()
               ? Result::success()
               : Result::failure(ErrorCode::NotConfigured);
}

EndpointHandle EndpointCatalog::fromSlot(
    EndpointSlot& slot,
    uint16_t id,
    ErrorCode error) {
    return EndpointHandle(&client_, &registry_, &slot, id, error);
}

EndpointHandle EndpointCatalog::find(StringView key) {
    const EndpointDescriptor* descriptor = registry_.find(key);
    if (descriptor == nullptr) {
        return EndpointHandle(
            &client_, &registry_, nullptr, 0, ErrorCode::NotFound);
    }
    uint16_t id = 0U;
    Result result = registry_.idOf(key, id);
    if (!result) {
        return EndpointHandle(
            &client_, &registry_, nullptr, 0, result.code());
    }
    EndpointSlot* slot = slotFor(*descriptor);
    return slot != nullptr
               ? fromSlot(*slot, id, ErrorCode::Ok)
               : EndpointHandle(
                     &client_,
                     &registry_,
                     nullptr,
                     0,
                     ErrorCode::InternalError);
}

} // namespace blinker
