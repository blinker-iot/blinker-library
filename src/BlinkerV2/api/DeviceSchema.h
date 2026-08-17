#ifndef BLINKER_API_DEVICESCHEMA_H
#define BLINKER_API_DEVICESCHEMA_H

#include "../model/EndpointRegistry.h"

namespace blinker {

// Explicit, zero-copy schema composition. A Public interaction delegates to
// its macro-generated static const Spec owner. DeviceSchema keeps exactly one
// pointer per listed source and provides ordered access without a per-field
// SRAM pointer table or hidden global registration.
template <typename... Interactions>
class DeviceSchema;

template <typename T>
struct SchemaInteractionType {
    typedef T Type;
};

template <typename T>
struct SchemaInteractionType<const T> {
    typedef T Type;
};

template <>
class DeviceSchema<> {
public:
    enum : size_t { fieldCount = 0U };

    constexpr DeviceSchema() {}

    const EndpointDescriptor* fieldAt(size_t) const { return nullptr; }
    template <typename Binder>
    Result bindAll(Binder&) const {
        return Result::success();
    }
    bool matches() const { return true; }
};

template <typename Head, typename... Tail>
class DeviceSchema<Head, Tail...> : private DeviceSchema<Tail...> {
public:
    typedef typename SchemaInteractionType<Head>::Type HeadType;
    typedef DeviceSchema<Tail...> TailSchema;

    enum : size_t {
        fieldCount =
            static_cast<size_t>(HeadType::fieldCount) +
            static_cast<size_t>(TailSchema::fieldCount)
    };

    constexpr DeviceSchema(Head& head, Tail&... tail)
        : TailSchema(tail...), head_(&head) {}

    const EndpointDescriptor* fieldAt(size_t index) const {
        return index < HeadType::fieldCount
                   ? head_->fieldAt(index)
                   : TailSchema::fieldAt(index - HeadType::fieldCount);
    }

    template <typename Binder>
    Result bindAll(Binder& binder) const {
        Result result = binder.bind(*head_);
        return result ? TailSchema::bindAll(binder) : result;
    }

    bool matches(Head& head, Tail&... tail) const {
        return head_ == &head && TailSchema::matches(tail...);
    }

    EndpointDescriptorTable endpointTable() const {
        return EndpointDescriptorTable(
            this,
            &DeviceSchema::fieldAtThunk,
            fieldCount);
    }

    static const EndpointDescriptor* fieldAtThunk(
        const void* context,
        size_t index) {
        const DeviceSchema* schema =
            static_cast<const DeviceSchema*>(context);
        return schema != nullptr ? schema->fieldAt(index) : nullptr;
    }

private:
    // Head may itself be const (Advanced descriptor-only schemas). Public
    // runtime interactions remain mutable so Device::begin() can bind their
    // compact handles without a second SRAM pointer table.
    Head* head_;
};

// Lvalue references deliberately reject temporary interaction Specs whose
// descriptor/member pointers would dangle after this expression.
template <typename... Interactions>
DeviceSchema<Interactions...> makeDeviceSchema(Interactions&... interactions) {
    return DeviceSchema<Interactions...>(interactions...);
}

} // namespace blinker

#endif
