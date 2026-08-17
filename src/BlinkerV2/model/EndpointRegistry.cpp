#include "EndpointRegistry.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "../core/Utf8.h"

namespace blinker {

namespace {

bool equal(StringView first, StringView second) {
    return first.size == second.size &&
           (first.size == 0U || memcmp(first.data, second.data, first.size) == 0);
}

bool validEndpointKind(EndpointKind kind) {
    return static_cast<uint8_t>(kind) <=
           static_cast<uint8_t>(EndpointKind::Event);
}

bool validValueType(ValueType type) {
    return static_cast<uint8_t>(type) <=
           static_cast<uint8_t>(ValueType::Null);
}

bool numericType(ValueType type) {
    return type == ValueType::SignedInteger ||
           type == ValueType::UnsignedInteger ||
           type == ValueType::Float32 || type == ValueType::Float64;
}

bool finiteNumber(double value) {
    return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

bool exactlySafeInteger(double value) {
    static const double kMaximumSafeInteger = 9007199254740991.0;
    return finiteNumber(value) && value >= -kMaximumSafeInteger &&
           value <= kMaximumSafeInteger && floor(value) == value;
}

Result validateName(StringView name, size_t maximum) {
    if (name.empty() || name.size > maximum || name.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (!isValidUtf8(name)) {
        return Result::failure(ErrorCode::InvalidUtf8);
    }
    for (size_t index = 0; index < name.size; ++index) {
        if (name.data[index] == '\0') {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    return Result::success();
}

Result validateConstraints(
    ValueType type,
    const PropertyConstraints* propertyConstraints) {
    if (propertyConstraints == nullptr) return Result::success();
    const PropertyConstraints& constraints = *propertyConstraints;
    if (constraints.flags == ConstraintNone ||
        (constraints.flags & ~kKnownPropertyConstraintFlags) != 0U) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    const uint8_t numericFlags = static_cast<uint8_t>(
        constraints.flags &
        static_cast<uint8_t>(
            ConstraintMinimum | ConstraintMaximum | ConstraintStep));
    if (numericFlags != 0U && !numericType(type)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if ((constraints.flags & ConstraintMaxLength) != 0U) {
        if (constraints.maxLength == 0U ||
            (type != ValueType::Text &&
             type != ValueType::Bytes &&
             type != ValueType::Array &&
             type != ValueType::Object)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    if ((constraints.flags & ConstraintUnit) != 0U) {
        Result result = validateName(
            constraints.unit,
            kMaxPropertyUnitLength);
        if (!result) return result;
        if (!numericType(type)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    if (numericFlags != 0U) {
        if (((constraints.flags & ConstraintMinimum) != 0U &&
             !finiteNumber(constraints.minimum)) ||
            ((constraints.flags & ConstraintMaximum) != 0U &&
             !finiteNumber(constraints.maximum)) ||
            ((constraints.flags & ConstraintStep) != 0U &&
             (!finiteNumber(constraints.step) || constraints.step <= 0.0))) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        if ((constraints.flags & ConstraintMinimum) != 0U &&
            (constraints.flags & ConstraintMaximum) != 0U &&
            constraints.minimum > constraints.maximum) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (type == ValueType::SignedInteger ||
            type == ValueType::UnsignedInteger) {
            if (((constraints.flags & ConstraintMinimum) != 0U &&
                 !exactlySafeInteger(constraints.minimum)) ||
                ((constraints.flags & ConstraintMaximum) != 0U &&
                 !exactlySafeInteger(constraints.maximum)) ||
                ((constraints.flags & ConstraintStep) != 0U &&
                 !exactlySafeInteger(constraints.step))) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            if (type == ValueType::UnsignedInteger &&
                (((constraints.flags & ConstraintMinimum) != 0U &&
                  constraints.minimum < 0.0) ||
                 ((constraints.flags & ConstraintMaximum) != 0U &&
                  constraints.maximum < 0.0))) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
        }
    }
    if ((constraints.flags & ConstraintEnumText) != 0U) {
        if (type != ValueType::Text ||
            constraints.enumTextValues == nullptr ||
            constraints.enumTextCount == 0U ||
            constraints.enumTextCount > kMaxPropertyEnumValues) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        for (size_t index = 0; index < constraints.enumTextCount; ++index) {
            const StringView value = constraints.enumTextValues[index];
            Result result = validateName(value, kMaxEndpointKeyLength);
            if (!result) return result;
            if ((constraints.flags & ConstraintMaxLength) != 0U &&
                value.size > constraints.maxLength) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            for (size_t prior = 0; prior < index; ++prior) {
                if (equal(constraints.enumTextValues[prior], value)) {
                    return Result::failure(ErrorCode::DuplicateField);
                }
            }
        }
    }
    return Result::success();
}

} // namespace

Result validateEndpointDescriptor(const EndpointDescriptor& descriptor) {
    Result result = validateName(descriptor.key, kMaxEndpointKeyLength);
    if (!result) return result;
    if (!validEndpointKind(descriptor.kind)) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (!validValueType(descriptor.type) || descriptor.access == AccessNone ||
        (descriptor.access & ~kKnownPropertyAccess) != 0U) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    const uint8_t propertyAccess = static_cast<uint8_t>(
        AccessRead | AccessWrite | AccessNotify);
    if (descriptor.kind == EndpointKind::Property) {
        if ((descriptor.access & ~propertyAccess) != 0U ||
            (descriptor.access &
             static_cast<uint8_t>(AccessRead | AccessWrite)) == 0U ||
            ((descriptor.access & AccessNotify) != 0U &&
             (descriptor.access & AccessRead) == 0U) ||
            descriptor.type == ValueType::Null) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (descriptor.kind == EndpointKind::Action) {
        if (descriptor.access != AccessCommand) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (descriptor.kind == EndpointKind::Event) {
        if (descriptor.access != AccessEvent) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    if (descriptor.type == ValueType::Null &&
        descriptor.constraints != nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return validateConstraints(descriptor.type, descriptor.constraints);
}

EndpointRegistry::EndpointRegistry(
    const EndpointDescriptor* descriptors,
    size_t count)
    : descriptorContext_(descriptors),
      descriptorAccessor_(&EndpointRegistry::contiguousAt),
      storage_(nullptr),
      capacity_(count),
      size_(count),
      sealed_(false),
      staticTable_(true) {}

EndpointRegistry::EndpointRegistry(const EndpointDescriptorTable& table)
    : descriptorContext_(table.context),
      descriptorAccessor_(table.accessor),
      storage_(nullptr),
      capacity_(table.count),
      size_(table.count),
      sealed_(false),
      staticTable_(true) {}

EndpointRegistry::EndpointRegistry(
    const EndpointDescriptor** storage,
    size_t capacity)
    : descriptorContext_(nullptr),
      descriptorAccessor_(nullptr),
      storage_(storage),
      capacity_(capacity),
      size_(0),
      sealed_(false),
      staticTable_(false) {}

Result EndpointRegistry::add(const EndpointDescriptor& descriptor) {
    if (sealed_ || staticTable_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (storage_ == nullptr || capacity_ == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result result = validateEndpointDescriptor(descriptor);
    if (!result) return result;
    if (find(descriptor.key) != nullptr) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (size_ >= UINT16_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (size_ >= capacity_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    storage_[size_++] = &descriptor;
    return Result::success();
}

Result EndpointRegistry::seal() {
    if (sealed_) return Result::success();
    if (capacity_ == 0U || size_ > UINT16_MAX ||
        (staticTable_
             ? descriptorContext_ == nullptr || descriptorAccessor_ == nullptr
             : storage_ == nullptr)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    for (size_t index = 0; index < size_; ++index) {
        const EndpointDescriptor* descriptor = at(index);
        if (descriptor == nullptr) {
            return Result::failure(ErrorCode::InternalError);
        }
        Result result = validateEndpointDescriptor(*descriptor);
        if (!result) return result;
        for (size_t prior = 0; prior < index; ++prior) {
            const EndpointDescriptor* priorDescriptor = at(prior);
            if (priorDescriptor == nullptr) {
                return Result::failure(ErrorCode::InternalError);
            }
            if (equal(descriptor->key, priorDescriptor->key)) {
                return Result::failure(ErrorCode::DuplicateField);
            }
        }
    }
    sealed_ = true;
    return Result::success();
}

const EndpointDescriptor* EndpointRegistry::find(StringView key) const {
    if (key.data == nullptr || key.empty()) return nullptr;
    for (size_t index = 0; index < size_; ++index) {
        const EndpointDescriptor* descriptor = at(index);
        if (descriptor != nullptr && equal(descriptor->key, key)) {
            return descriptor;
        }
    }
    return nullptr;
}

const EndpointDescriptor* EndpointRegistry::findById(uint16_t id) const {
    return id == 0U ? nullptr : at(static_cast<size_t>(id - 1U));
}

Result EndpointRegistry::idOf(StringView key, uint16_t& id) const {
    id = 0;
    if (key.data == nullptr || key.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    for (size_t index = 0; index < size_; ++index) {
        const EndpointDescriptor* descriptor = at(index);
        if (descriptor == nullptr || !equal(descriptor->key, key)) continue;
        if (index >= UINT16_MAX) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        id = static_cast<uint16_t>(index + 1U);
        return Result::success();
    }
    return Result::failure(ErrorCode::NotFound);
}

const EndpointDescriptor* EndpointRegistry::at(size_t index) const {
    if (index >= size_) return nullptr;
    if (staticTable_) {
        return descriptorContext_ != nullptr &&
                       descriptorAccessor_ != nullptr
                   ? descriptorAccessor_(descriptorContext_, index)
                   : nullptr;
    }
    return storage_ != nullptr ? storage_[index] : nullptr;
}

const EndpointDescriptor* EndpointRegistry::contiguousAt(
    const void* context,
    size_t index) {
    const EndpointDescriptor* descriptors =
        static_cast<const EndpointDescriptor*>(context);
    return descriptors != nullptr ? &descriptors[index] : nullptr;
}

} // namespace blinker
