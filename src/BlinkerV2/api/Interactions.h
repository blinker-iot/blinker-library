#ifndef BLINKER_API_INTERACTIONS_H
#define BLINKER_API_INTERACTIONS_H

#include <limits.h>
#include <stdint.h>

#include "Client.h"

namespace blinker {

// The Public API models device semantics, never App widgets. Property is
// state, Action is an invocation, and Event is an asynchronous occurrence.
enum class PropertyMode : uint8_t {
    ReadOnly = 0,
    ReadWrite = 1
};

struct PropertyOptions {
    PropertyMode mode;
    PropertyConstraints constraints;
    uint32_t telemetryMinimumIntervalMs;

    constexpr PropertyOptions(
        PropertyMode propertyMode,
        const PropertyConstraints& propertyConstraints =
            PropertyConstraints(),
        uint32_t minimumTelemetryIntervalMs = 0U)
        : mode(propertyMode),
          constraints(propertyConstraints),
          telemetryMinimumIntervalMs(minimumTelemetryIntervalMs) {}
};

constexpr PropertyOptions property(PropertyMode mode) {
    return PropertyOptions(mode);
}

// Short, intention-revealing forms for normal sketches. property(mode)
// remains available to code that already has a PropertyMode value.
constexpr PropertyOptions readOnly() {
    return PropertyOptions(PropertyMode::ReadOnly);
}

constexpr PropertyOptions readWrite() {
    return PropertyOptions(PropertyMode::ReadWrite);
}

// Declares an on-demand sampled Property. The App/Broker still chooses the
// active interval through a short lease; this value is the device-side floor.
constexpr PropertyOptions realtime(
    uint32_t minimumIntervalMs = 200U) {
    return PropertyOptions(
        PropertyMode::ReadOnly,
        PropertyConstraints(),
        minimumIntervalMs);
}

constexpr PropertyOptions rangedProperty(
    PropertyMode mode,
    double minimum,
    double maximum,
    double step = 0.0,
    StringView unit = StringView()) {
    return PropertyOptions(
        mode,
        PropertyConstraints(
            static_cast<uint8_t>(
                ConstraintMinimum | ConstraintMaximum |
                (step > 0.0
                     ? static_cast<uint8_t>(ConstraintStep)
                     : 0U) |
                (unit.size != 0U
                     ? static_cast<uint8_t>(ConstraintUnit)
                     : 0U)),
            minimum,
            maximum,
            step,
            0U,
            unit,
            nullptr,
            0U));
}

constexpr PropertyOptions textProperty(
    PropertyMode mode,
    uint32_t maximumLength) {
    return PropertyOptions(
        mode,
        PropertyConstraints(
            ConstraintMaxLength,
            0.0,
            0.0,
            0.0,
            maximumLength,
            StringView(),
            nullptr,
            0U));
}

constexpr PropertyOptions enumTextProperty(
    PropertyMode mode,
    const StringView* values,
    size_t valueCount,
    uint32_t maximumLength = 0U) {
    return PropertyOptions(
        mode,
        PropertyConstraints(
            static_cast<uint8_t>(
                ConstraintEnumText |
                (maximumLength != 0U
                     ? static_cast<uint8_t>(ConstraintMaxLength)
                     : 0U)),
            0.0,
            0.0,
            0.0,
            maximumLength,
            StringView(),
            values,
            valueCount));
}

namespace interaction_detail {

template <typename T>
struct ValueCodec;

template <>
struct ValueCodec<bool> {
    static constexpr ValueType type() { return ValueType::Boolean; }
    static Result decode(const EndpointValueView& input, bool& output) {
        return input.asBool(output);
    }
    static Result report(const EndpointHandle& field, bool value) {
        return field.set(value);
    }
    static Result emit(const EndpointHandle& field, bool value) {
        return field.emit(value);
    }
    static Result sample(bbp2::IdBodyWriter& writer, uint16_t id, bool value) {
        return writer.writeBool(id, value);
    }
};

template <>
struct ValueCodec<int32_t> {
    static constexpr ValueType type() { return ValueType::SignedInteger; }
    static Result decode(const EndpointValueView& input, int32_t& output) {
        int64_t value = 0;
        Result result = input.asInt(value);
        if (result && (value < INT32_MIN || value > INT32_MAX)) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (result) output = static_cast<int32_t>(value);
        return result;
    }
    static Result report(const EndpointHandle& field, int32_t value) {
        return field.setInt(value);
    }
    static Result emit(const EndpointHandle& field, int32_t value) {
        return field.emitInt(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, int32_t value) {
        return writer.writeInt(id, value);
    }
};

template <>
struct ValueCodec<uint32_t> {
    static constexpr ValueType type() { return ValueType::UnsignedInteger; }
    static Result decode(const EndpointValueView& input, uint32_t& output) {
        uint64_t value = 0U;
        Result result = input.asUnsigned(value);
        if (result && value > UINT32_MAX) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (result) output = static_cast<uint32_t>(value);
        return result;
    }
    static Result report(const EndpointHandle& field, uint32_t value) {
        return field.setUnsigned(value);
    }
    static Result emit(const EndpointHandle& field, uint32_t value) {
        return field.emitUnsigned(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, uint32_t value) {
        return writer.writeUnsigned(id, value);
    }
};

template <>
struct ValueCodec<float> {
    static constexpr ValueType type() { return ValueType::Float32; }
    static Result decode(const EndpointValueView& input, float& output) {
        double value = 0.0;
        Result result = input.asFloat(value);
        if (result) output = static_cast<float>(value);
        return result;
    }
    static Result report(const EndpointHandle& field, float value) {
        return field.set(value);
    }
    static Result emit(const EndpointHandle& field, float value) {
        return field.emit(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, float value) {
        return writer.writeFloat32(id, value);
    }
};

template <>
struct ValueCodec<double> {
    static constexpr ValueType type() { return ValueType::Float64; }
    static Result decode(const EndpointValueView& input, double& output) {
        return input.asFloat(output);
    }
    static Result report(const EndpointHandle& field, double value) {
        return field.set(value);
    }
    static Result emit(const EndpointHandle& field, double value) {
        return field.emit(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, double value) {
        return writer.writeFloat64(id, value);
    }
};

template <>
struct ValueCodec<StringView> {
    static constexpr ValueType type() { return ValueType::Text; }
    static Result decode(
        const EndpointValueView& input,
        StringView& output) {
        return input.asText(output);
    }
    static Result report(
        const EndpointHandle& field,
        StringView value) {
        return field.setText(value);
    }
    static Result emit(const EndpointHandle& field, StringView value) {
        return field.emitText(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, StringView value) {
        return writer.writeText(id, value);
    }
};

template <>
struct ValueCodec<ByteView> {
    static constexpr ValueType type() { return ValueType::Bytes; }
    static Result decode(const EndpointValueView& input, ByteView& output) {
        return input.asBytes(output);
    }
    static Result report(const EndpointHandle& field, ByteView value) {
        return field.setBytes(value);
    }
    static Result emit(const EndpointHandle& field, ByteView value) {
        return field.emitBytes(value);
    }
    static Result sample(
        bbp2::IdBodyWriter& writer, uint16_t id, ByteView value) {
        return writer.writeBytes(id, value);
    }
};

template <>
struct ValueCodec<void> {
    static constexpr ValueType type() { return ValueType::Null; }
    static Result decode(const EndpointValueView& input) {
        return input.asNull();
    }
    static Result emit(const EndpointHandle& field) {
        return field.emitNull();
    }
};

constexpr uint8_t propertyAccess(PropertyMode mode) {
    return mode == PropertyMode::ReadWrite
               ? static_cast<uint8_t>(
                     AccessRead | AccessWrite | AccessNotify)
               : static_cast<uint8_t>(AccessRead | AccessNotify);
}

constexpr const PropertyConstraints* constraintsOrNull(
    const PropertyConstraints& constraints) {
    return constraints.flags != ConstraintNone ? &constraints : nullptr;
}

inline Result validateBinding(
    Client& client,
    const FieldSpec& expected,
    Client*& boundClient) {
    if (boundClient != nullptr) {
        return boundClient == &client
                   ? Result::success()
                   : Result::failure(ErrorCode::AlreadyExists);
    }
    const EndpointDescriptor* actual =
        client.endpointRegistry().find(expected.key);
    if (actual != &expected) {
        return Result::failure(
            actual == nullptr ? ErrorCode::NotFound
                              : ErrorCode::AlreadyExists);
    }
    EndpointHandle field = client.endpoints().find(expected.key);
    if (!field.valid()) return Result::failure(field.error());
    boundClient = &client;
    return Result::success();
}

} // namespace interaction_detail

template <typename T>
class PropertySpec {
public:
    enum : size_t { fieldCount = 1U };

    constexpr PropertySpec(StringView key, PropertyOptions options)
        : options_(options),
          field_(
              key,
              EndpointKind::Property,
              interaction_detail::ValueCodec<T>::type(),
              interaction_detail::propertyAccess(options.mode),
              interaction_detail::constraintsOrNull(options_.constraints),
              options.telemetryMinimumIntervalMs) {}

    const FieldSpec* fieldAt(size_t index) const {
        return index == 0U ? &field_ : nullptr;
    }
    const FieldSpec& field() const { return field_; }
    bool writable() const {
        return options_.mode == PropertyMode::ReadWrite;
    }

private:
    PropertyOptions options_;
    FieldSpec field_;

    PropertySpec(const PropertySpec&);
    PropertySpec& operator=(const PropertySpec&);
};

template <typename T>
class ActionSpec {
public:
    enum : size_t { fieldCount = 1U };

    constexpr explicit ActionSpec(
        StringView key,
        const PropertyConstraints& constraints = PropertyConstraints())
        : constraints_(constraints),
          field_(
              key,
              EndpointKind::Action,
              interaction_detail::ValueCodec<T>::type(),
              AccessCommand,
              interaction_detail::constraintsOrNull(constraints_)) {}

    const FieldSpec* fieldAt(size_t index) const {
        return index == 0U ? &field_ : nullptr;
    }
    const FieldSpec& field() const { return field_; }

private:
    PropertyConstraints constraints_;
    FieldSpec field_;

    ActionSpec(const ActionSpec&);
    ActionSpec& operator=(const ActionSpec&);
};

template <typename T>
class EventSpec {
public:
    enum : size_t { fieldCount = 1U };

    constexpr explicit EventSpec(
        StringView key,
        const PropertyConstraints& constraints = PropertyConstraints())
        : constraints_(constraints),
          field_(
              key,
              EndpointKind::Event,
              interaction_detail::ValueCodec<T>::type(),
              AccessEvent,
              interaction_detail::constraintsOrNull(constraints_)) {}

    const FieldSpec* fieldAt(size_t index) const {
        return index == 0U ? &field_ : nullptr;
    }
    const FieldSpec& field() const { return field_; }

private:
    PropertyConstraints constraints_;
    FieldSpec field_;

    EventSpec(const EventSpec&);
    EventSpec& operator=(const EventSpec&);
};

class Device;
class BlinkerClass;

template <typename T>
class Property {
public:
    enum : size_t { fieldCount = 1U };
    typedef void (*WriteHandler)(T requestedValue);
    typedef Result (*CheckedWriteHandler)(T requestedValue);
    typedef T (*SampleHandler)();

    explicit Property(const PropertySpec<T>& spec)
        : spec_(&spec),
          client_(nullptr),
          callback_(),
          callbackMode_(None) {}

    bool valid() const { return client_ != nullptr; }
    Result onWrite(WriteHandler handler) {
        if (handler == nullptr || spec_ == nullptr || !spec_->writable()) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            EndpointHandle field = client_->endpoints().find(
                spec_->field().key);
            Result result = field.onCommand(&Property::voidCommandThunk, this);
            if (!result) return result;
        }
        callback_.normal = handler;
        callbackMode_ = Normal;
        return Result::success();
    }
    Result onWriteChecked(CheckedWriteHandler handler) {
        if (handler == nullptr || spec_ == nullptr || !spec_->writable()) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            EndpointHandle field = client_->endpoints().find(
                spec_->field().key);
            Result result = field.onCommand(
                &Property::checkedCommandThunk, this);
            if (!result) return result;
        }
        callback_.checked = handler;
        callbackMode_ = Checked;
        return Result::success();
    }
    Result onWrite(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result onWriteChecked(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result onSample(SampleHandler handler) {
        if (handler == nullptr || spec_ == nullptr ||
            spec_->field().telemetryMinimumIntervalMs == 0U) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            Result result = client_->endpoints()
                                .find(spec_->field().key)
                                .onSample(&Property::sampleThunk, this);
            if (!result) return result;
        }
        callback_.sample = handler;
        callbackMode_ = Sample;
        return Result::success();
    }
    Result report(T value) const {
        return client_ != nullptr && spec_ != nullptr
                   ? interaction_detail::ValueCodec<T>::report(
                         client_->endpoints().find(spec_->field().key),
                         value)
                   : Result::failure(ErrorCode::NotConfigured);
    }
    Result current(T& value) const {
        if (client_ == nullptr || spec_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        EndpointValueView currentValue;
        Result result = client_->endpoints()
                            .find(spec_->field().key)
                            .current(currentValue);
        return result
                   ? interaction_detail::ValueCodec<T>::decode(
                         currentValue, value)
                   : result;
    }
    const FieldSpec* fieldAt(size_t index) const {
        return spec_ != nullptr ? spec_->fieldAt(index) : nullptr;
    }

private:
    enum CallbackMode : uint8_t {
        None = 0U,
        Normal,
        Checked,
        Sample
    };

    union Callback {
        WriteHandler normal;
        CheckedWriteHandler checked;
        SampleHandler sample;

        Callback() : normal(nullptr) {}
    };

    static Result voidCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Property* property = static_cast<Property*>(context);
        if (property == nullptr || property->callbackMode_ != Normal ||
            property->callback_.normal == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        T decoded;
        Result result = interaction_detail::ValueCodec<T>::decode(
            value, decoded);
        if (result) property->callback_.normal(decoded);
        return result;
    }
    static Result checkedCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Property* property = static_cast<Property*>(context);
        if (property == nullptr || property->callbackMode_ != Checked ||
            property->callback_.checked == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        T decoded;
        Result result = interaction_detail::ValueCodec<T>::decode(
            value, decoded);
        return result ? property->callback_.checked(decoded) : result;
    }
    static Result sampleThunk(
        void* context,
        uint16_t endpointId,
        bbp2::IdBodyWriter& writer) {
        Property* property = static_cast<Property*>(context);
        if (property == nullptr || property->callbackMode_ != Sample ||
            property->callback_.sample == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        return interaction_detail::ValueCodec<T>::sample(
            writer, endpointId, property->callback_.sample());
    }
    Result bind(Client& client) {
        if (spec_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = interaction_detail::validateBinding(
            client, spec_->field(), client_);
        if (result &&
            (callbackMode_ == Normal || callbackMode_ == Checked)) {
            result = client.endpoints()
                         .find(spec_->field().key)
                         .onCommand(
                             callbackMode_ == Normal
                                 ? &Property::voidCommandThunk
                                 : &Property::checkedCommandThunk,
                             this);
            if (!result) client_ = nullptr;
        }
        if (result && callbackMode_ == Sample) {
            result = client.endpoints()
                         .find(spec_->field().key)
                         .onSample(&Property::sampleThunk, this);
            if (!result) client_ = nullptr;
        }
        return result;
    }

    const PropertySpec<T>* spec_;
    Client* client_;
    Callback callback_;
    CallbackMode callbackMode_;

    friend class Device;
    friend class BlinkerClass;
};

template <typename T>
class Action {
public:
    enum : size_t { fieldCount = 1U };
    typedef void (*InvokeHandler)(T input);
    typedef Result (*CheckedInvokeHandler)(T input);

    explicit Action(const ActionSpec<T>& spec)
        : spec_(&spec), client_(nullptr), callback_(), callbackMode_(None) {}

    bool valid() const { return client_ != nullptr; }
    Result onInvoke(InvokeHandler handler) {
        if (handler == nullptr || spec_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            Result result = client_->endpoints()
                                .find(spec_->field().key)
                                .onCommand(&Action::voidCommandThunk, this);
            if (!result) return result;
        }
        callback_.normal = handler;
        callbackMode_ = Normal;
        return Result::success();
    }
    Result onInvokeChecked(CheckedInvokeHandler handler) {
        if (handler == nullptr || spec_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            Result result = client_->endpoints()
                                .find(spec_->field().key)
                                .onCommand(&Action::checkedCommandThunk, this);
            if (!result) return result;
        }
        callback_.checked = handler;
        callbackMode_ = Checked;
        return Result::success();
    }
    Result onInvoke(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result onInvokeChecked(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const FieldSpec* fieldAt(size_t index) const {
        return spec_ != nullptr ? spec_->fieldAt(index) : nullptr;
    }

private:
    enum CallbackMode : uint8_t {
        None = 0U,
        Normal,
        Checked
    };

    union Callback {
        InvokeHandler normal;
        CheckedInvokeHandler checked;

        Callback() : normal(nullptr) {}
    };

    static Result voidCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Action* action = static_cast<Action*>(context);
        if (action == nullptr || action->callbackMode_ != Normal ||
            action->callback_.normal == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        T decoded;
        Result result = interaction_detail::ValueCodec<T>::decode(
            value, decoded);
        if (result) action->callback_.normal(decoded);
        return result;
    }
    static Result checkedCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Action* action = static_cast<Action*>(context);
        if (action == nullptr || action->callbackMode_ != Checked ||
            action->callback_.checked == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        T decoded;
        Result result = interaction_detail::ValueCodec<T>::decode(
            value, decoded);
        return result ? action->callback_.checked(decoded) : result;
    }
    Result bind(Client& client) {
        if (spec_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = interaction_detail::validateBinding(
            client, spec_->field(), client_);
        if (result && callbackMode_ != None) {
            result = client.endpoints()
                         .find(spec_->field().key)
                         .onCommand(
                             callbackMode_ == Normal
                                 ? &Action::voidCommandThunk
                                 : &Action::checkedCommandThunk,
                             this);
            if (!result) client_ = nullptr;
        }
        return result;
    }

    const ActionSpec<T>* spec_;
    Client* client_;
    Callback callback_;
    CallbackMode callbackMode_;

    friend class Device;
};

template <>
class Action<void> {
public:
    enum : size_t { fieldCount = 1U };
    typedef void (*InvokeHandler)();
    typedef Result (*CheckedInvokeHandler)();

    explicit Action(const ActionSpec<void>& spec)
        : spec_(&spec), client_(nullptr), callback_(), callbackMode_(None) {}

    bool valid() const { return client_ != nullptr; }
    Result onInvoke(InvokeHandler handler) {
        if (handler == nullptr || spec_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            Result result = client_->endpoints()
                                .find(spec_->field().key)
                                .onCommand(&Action::voidCommandThunk, this);
            if (!result) return result;
        }
        callback_.normal = handler;
        callbackMode_ = Normal;
        return Result::success();
    }
    Result onInvokeChecked(CheckedInvokeHandler handler) {
        if (handler == nullptr || spec_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (client_ != nullptr) {
            Result result = client_->endpoints()
                                .find(spec_->field().key)
                                .onCommand(&Action::checkedCommandThunk, this);
            if (!result) return result;
        }
        callback_.checked = handler;
        callbackMode_ = Checked;
        return Result::success();
    }
    Result onInvoke(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result onInvokeChecked(decltype(nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const FieldSpec* fieldAt(size_t index) const {
        return spec_ != nullptr ? spec_->fieldAt(index) : nullptr;
    }

private:
    enum CallbackMode : uint8_t {
        None = 0U,
        Normal,
        Checked
    };

    union Callback {
        InvokeHandler normal;
        CheckedInvokeHandler checked;

        Callback() : normal(nullptr) {}
    };

    static Result voidCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Action* action = static_cast<Action*>(context);
        if (action == nullptr || action->callbackMode_ != Normal ||
            action->callback_.normal == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = interaction_detail::ValueCodec<void>::decode(value);
        if (result) action->callback_.normal();
        return result;
    }
    static Result checkedCommandThunk(
        void* context,
        const EndpointHandle&,
        const EndpointValueView& value,
        const RxContext&) {
        Action* action = static_cast<Action*>(context);
        if (action == nullptr || action->callbackMode_ != Checked ||
            action->callback_.checked == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = interaction_detail::ValueCodec<void>::decode(value);
        return result ? action->callback_.checked() : result;
    }
    Result bind(Client& client) {
        if (spec_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = interaction_detail::validateBinding(
            client, spec_->field(), client_);
        if (result && callbackMode_ != None) {
            result = client.endpoints()
                         .find(spec_->field().key)
                         .onCommand(
                             callbackMode_ == Normal
                                 ? &Action::voidCommandThunk
                                 : &Action::checkedCommandThunk,
                             this);
            if (!result) client_ = nullptr;
        }
        return result;
    }

    const ActionSpec<void>* spec_;
    Client* client_;
    Callback callback_;
    CallbackMode callbackMode_;

    friend class Device;
};

template <typename T>
class Event {
public:
    enum : size_t { fieldCount = 1U };

    explicit Event(const EventSpec<T>& spec)
        : spec_(&spec), client_(nullptr) {}

    bool valid() const { return client_ != nullptr; }
    Result emit(T value) const {
        return client_ != nullptr && spec_ != nullptr
                   ? interaction_detail::ValueCodec<T>::emit(
                         client_->endpoints().find(spec_->field().key),
                         value)
                   : Result::failure(ErrorCode::NotConfigured);
    }
    const FieldSpec* fieldAt(size_t index) const {
        return spec_ != nullptr ? spec_->fieldAt(index) : nullptr;
    }

private:
    Result bind(Client& client) {
        return spec_ != nullptr
                   ? interaction_detail::validateBinding(
                         client, spec_->field(), client_)
                   : Result::failure(ErrorCode::NotConfigured);
    }

    const EventSpec<T>* spec_;
    Client* client_;

    friend class Device;
};

template <>
class Event<void> {
public:
    enum : size_t { fieldCount = 1U };

    explicit Event(const EventSpec<void>& spec)
        : spec_(&spec), client_(nullptr) {}

    bool valid() const { return client_ != nullptr; }
    Result emit() const {
        return client_ != nullptr && spec_ != nullptr
                   ? interaction_detail::ValueCodec<void>::emit(
                         client_->endpoints().find(spec_->field().key))
                   : Result::failure(ErrorCode::NotConfigured);
    }
    const FieldSpec* fieldAt(size_t index) const {
        return spec_ != nullptr ? spec_->fieldAt(index) : nullptr;
    }

private:
    Result bind(Client& client) {
        return spec_ != nullptr
                   ? interaction_detail::validateBinding(
                         client, spec_->field(), client_)
                   : Result::failure(ErrorCode::NotConfigured);
    }

    const EventSpec<void>* spec_;
    Client* client_;

    friend class Device;
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(Property<bool>) <= 16U && sizeof(Action<void>) <= 16U &&
        sizeof(Event<uint32_t>) <= 8U,
    "Public interaction runtime exceeds its 32-bit compact-view gate");
#endif

} // namespace blinker

// The normal declaration uses the C++ object name as the stable Manifest key.
// *_KEY is the explicit-key form for product schemas that cannot use it.
#define BLINKER_PROPERTY(name, valueType, options)                            \
    static const ::blinker::PropertySpec<valueType> name##Spec(               \
        #name, options);                                                       \
    ::blinker::Property<valueType> name(name##Spec)

#define BLINKER_PROPERTY_KEY(name, keyLiteral, valueType, options)            \
    static const ::blinker::PropertySpec<valueType> name##Spec(               \
        keyLiteral, options);                                                  \
    ::blinker::Property<valueType> name(name##Spec)

#define BLINKER_ACTION(name)                                                   \
    static const ::blinker::ActionSpec<void> name##Spec(#name);               \
    ::blinker::Action<void> name(name##Spec)

#define BLINKER_ACTION_KEY(name, keyLiteral)                                  \
    static const ::blinker::ActionSpec<void> name##Spec(keyLiteral);          \
    ::blinker::Action<void> name(name##Spec)

#define BLINKER_TYPED_ACTION(name, valueType)                                 \
    static const ::blinker::ActionSpec<valueType> name##Spec(#name);          \
    ::blinker::Action<valueType> name(name##Spec)

#define BLINKER_TYPED_ACTION_KEY(name, keyLiteral, valueType)                 \
    static const ::blinker::ActionSpec<valueType> name##Spec(keyLiteral);     \
    ::blinker::Action<valueType> name(name##Spec)

#define BLINKER_EVENT(name, valueType)                                        \
    static const ::blinker::EventSpec<valueType> name##Spec(#name);           \
    ::blinker::Event<valueType> name(name##Spec)

#define BLINKER_EVENT_KEY(name, keyLiteral, valueType)                        \
    static const ::blinker::EventSpec<valueType> name##Spec(keyLiteral);      \
    ::blinker::Event<valueType> name(name##Spec)

#define BLINKER_VOID_EVENT(name)                                              \
    static const ::blinker::EventSpec<void> name##Spec(#name);                \
    ::blinker::Event<void> name(name##Spec)

#define BLINKER_VOID_EVENT_KEY(name, keyLiteral)                              \
    static const ::blinker::EventSpec<void> name##Spec(keyLiteral);           \
    ::blinker::Event<void> name(name##Spec)

#endif
