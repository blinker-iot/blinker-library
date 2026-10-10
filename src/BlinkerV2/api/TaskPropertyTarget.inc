#include "TaskPropertyTarget.h"
#include "../model/EndpointValue.h"
#include <string.h>

namespace blinker {
Result TaskPropertyTarget::validateBinding(const TaskPropertyBinding* bindings, size_t i) {
    const FieldSpec* f = bindings[i].field();
    if (f == nullptr || !validateEndpointDescriptor(*f) || f->key.size > kMaxEndpointKeyLength ||
        f->kind != EndpointKind::Property || f->type > ValueType::Float64 ||
        f->telemetryMinimumIntervalMs != 0U || (f->access & AccessWrite) == 0U)
        return Result::failure(ErrorCode::UnsupportedFeature);
    for (size_t j = 0U; j < i; ++j) {
        const FieldSpec* previous = bindings[j].field();
        if (previous == nullptr || previous->key.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
        if (previous->key.size == f->key.size && memcmp(previous->key.data, f->key.data, f->key.size) == 0)
            return Result::failure(ErrorCode::AlreadyExists);
    }
    return Result::success();
}

Result TaskPropertyTarget::configure(const TaskPropertyBinding* bindings, size_t count,
                                     TaskPropertyBinding* destination) {
    if (client_ != nullptr || bindings_ != nullptr) return Result::failure(ErrorCode::AlreadyExists);
    if (bindings == nullptr || count == 0U) return Result::failure(ErrorCode::InvalidArgument);
    if (count > kTaskMaximumTargets) return Result::failure(ErrorCode::CapacityExceeded);
    for (size_t i = 0U; i < count; ++i) {
        const Result valid = validateBinding(bindings, i);
        if (!valid) return valid;
    }
    if (destination != nullptr) {
        for (size_t i = 0U; i < count; ++i) destination[i] = bindings[i];
        bindings = destination;
    }
    bindings_ = bindings; count_ = count;
    return Result::success();
}

Result TaskPropertyTarget::describe(size_t index, TaskTargetView& output, size_t& total) {
    output = TaskTargetView(); total = 0U;
    if (applying_) return Result::failure(ErrorCode::WouldBlock);
    if (client_ == nullptr || !client_->prepared_ || bindings_ == nullptr || count_ == 0U)
        return Result::failure(ErrorCode::NotConfigured);
    if (count_ > kTaskMaximumTargets) return Result::failure(ErrorCode::CapacityExceeded);
    if (index != UINT32_MAX && index >= count_) return Result::failure(ErrorCode::NotFound);
    // Invalid/duplicate configured entries are a configuration error, not an
    // invitation to advertise ordinary writable properties outside this list.
    // Engine checks metadata once before streaming pages: avoid revalidating
    // the entire binding array for every entry hashed in that same read.
    const size_t first = index == UINT32_MAX ? 0U : index;
    const size_t end = index == UINT32_MAX ? count_ : index + 1U;
    for (size_t i = first; i < end; ++i) {
        const Result valid = validateBinding(bindings_, i);
        if (!valid) return valid;
        const Result ready = bindings_[i].invoke_(bindings_[i].context_, *client_, nullptr, false);
        if (!ready) return ready;
    }
    if (index != UINT32_MAX) { output.key = bindings_[index].field()->key; output.valueType = bindings_[index].field()->type; }
    total = count_; return Result::success();
}
Result TaskPropertyTarget::dispatch(const TaskSnapshotRecord& task, bool apply) {
    if (applying_) return Result::failure(ErrorCode::WouldBlock);
    if (client_ == nullptr || !client_->prepared_ || bindings_ == nullptr || count_ == 0U)
        return Result::failure(ErrorCode::NotConfigured);
    if (task.keyLength == 0U || task.keyLength > sizeof(task.key) ||
        task.valueLength == 0U || task.valueLength > sizeof(task.value) ||
        task.valueType > ValueType::Float64)
        return Result::failure(ErrorCode::InvalidArgument);
    const StringView key(reinterpret_cast<const char*>(task.key), task.keyLength);
    const TaskPropertyBinding* selected = nullptr;
    for (size_t i = 0U; i < count_; ++i) {
        const FieldSpec* field = bindings_[i].field();
        if (field == nullptr || field->key.data == nullptr || field->key.empty())
            return Result::failure(ErrorCode::InvalidArgument);
        if (field->key.size == key.size && memcmp(field->key.data, key.data, key.size) == 0) {
            if (selected != nullptr) return Result::failure(ErrorCode::AlreadyExists);
            selected = &bindings_[i];
        }
    }
    if (selected == nullptr) return Result::failure(ErrorCode::NotFound);
    const FieldSpec& field = *selected->field();
    if (field.kind != EndpointKind::Property || field.type != task.valueType ||
        field.telemetryMinimumIntervalMs != 0U || (field.access & AccessWrite) == 0U)
        return Result::failure(ErrorCode::UnsupportedFeature);
    const ByteView encoded(task.value, task.valueLength);
    cbor::Type type = cbor::Type::Invalid;
    cbor::Reader reader(encoded);
    Result result = reader.peekType(type);
    if (result) result = cbor::validate(encoded);
    const PropertyConstraints scalarLimits; // Finite numbers even without an explicit range.
    EndpointDescriptor checked = field;
    if (checked.constraints == nullptr) checked.constraints = &scalarLimits;
    if (result) result = validateEndpointValue(checked, encoded, type, AccessWrite);
    if (!result) return result;
    applying_ = true;
    const EndpointValueView value(encoded, type);
    result = selected->invoke_(selected->context_, *client_, &value, apply);
    applying_ = false;
    return result;
}
} // namespace blinker
