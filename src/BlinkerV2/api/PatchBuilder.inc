#include "PatchBuilder.h"

#include "../model/EndpointValue.h"

namespace blinker {

PatchBuilder::PatchBuilder(
    const EndpointRegistry& registry,
    MutableByteSpan output,
    const cbor::Limits& limits,
    uint8_t requiredAccess)
    : registry_(registry),
      writer_(output, limits),
      limits_(limits),
      requiredAccess_(requiredAccess) {}

Result PatchBuilder::begin(size_t fieldCount) {
    return writer_.begin(fieldCount);
}

Result PatchBuilder::validateHandle(const EndpointHandle& endpoint) const {
    if (!endpoint.valid() || endpoint.registry_ != &registry_ ||
        endpoint.slot_ == nullptr) {
        return Result::failure(
            endpoint.valid() ? ErrorCode::InvalidArgument : endpoint.error());
    }
    return Result::success();
}

Result PatchBuilder::validateHandle(
    const EndpointHandle& endpoint,
    ValueType expected) const {
    Result result = validateHandle(endpoint);
    if (!result) return result;
    const EndpointDescriptor* descriptor =
        registry_.findById(endpoint.id_);
    if (descriptor == nullptr || descriptor->type != expected ||
        (descriptor->access & requiredAccess_) == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result PatchBuilder::write(const EndpointHandle& endpoint, bool value) {
    Result result = validateHandle(endpoint, ValueType::Boolean);
    return result ? writer_.writeBool(endpoint.key(), value) : result;
}

Result PatchBuilder::write(const EndpointHandle& endpoint, int64_t value) {
    Result result = validateHandle(endpoint, ValueType::SignedInteger);
    return result ? writer_.writeInt(endpoint.key(), value) : result;
}

Result PatchBuilder::write(
    const EndpointHandle& endpoint,
    uint64_t value) {
    Result result = validateHandle(endpoint, ValueType::UnsignedInteger);
    return result ? writer_.writeUnsigned(endpoint.key(), value) : result;
}

Result PatchBuilder::write(const EndpointHandle& endpoint, float value) {
    Result result = validateHandle(endpoint, ValueType::Float32);
    return result ? writer_.writeFloat32(endpoint.key(), value) : result;
}

Result PatchBuilder::write(const EndpointHandle& endpoint, double value) {
    Result result = validateHandle(endpoint, ValueType::Float64);
    return result ? writer_.writeFloat64(endpoint.key(), value) : result;
}

Result PatchBuilder::writeText(
    const EndpointHandle& endpoint,
    StringView value) {
    Result result = validateHandle(endpoint, ValueType::Text);
    return result ? writer_.writeText(endpoint.key(), value) : result;
}

Result PatchBuilder::writeBytes(
    const EndpointHandle& endpoint,
    ByteView value) {
    Result result = validateHandle(endpoint, ValueType::Bytes);
    return result ? writer_.writeBytes(endpoint.key(), value) : result;
}

Result PatchBuilder::writeNull(const EndpointHandle& endpoint) {
    Result result = validateHandle(endpoint, ValueType::Null);
    return result ? writer_.writeNull(endpoint.key()) : result;
}

Result PatchBuilder::writeEncoded(
    const EndpointHandle& endpoint,
    ByteView value) {
    Result result = validateHandle(endpoint);
    return result
               ? writer_.writeEncoded(endpoint.key(), value, limits_)
               : result;
}

Result PatchBuilder::validateValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    const PatchBuilder* builder = static_cast<const PatchBuilder*>(context);
    if (builder == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint =
        builder->registry_.find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        builder->requiredAccess_,
        builder->limits_);
}

Result PatchBuilder::finish(ByteView& encoded) {
    Result result = writer_.finish(encoded);
    if (!result) return result;
    return bbp2::visitKeyedBody(
        encoded,
        &PatchBuilder::validateValue,
        this,
        limits_);
}

} // namespace blinker
