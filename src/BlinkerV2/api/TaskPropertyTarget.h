#ifndef BLINKER_API_TASKPROPERTYTARGET_H
#define BLINKER_API_TASKPROPERTYTARGET_H

#include "Interactions.h"
#include "../runtime/TaskEngine.h"

namespace blinker {

// Internal opt-in list, not a published task facade or authorization proof.
// Entries borrow stable Property objects/specs; the list must outlive target.
// No flag or extra callback pointer is added to every ordinary Property/slot.
class TaskPropertyBinding {
public:
    TaskPropertyBinding() : field_(nullptr), context_(nullptr), invoke_(nullptr) {}
    template <typename T>
    explicit TaskPropertyBinding(Property<T>& property)
        : field_(property.fieldAt(0U)), context_(&property), invoke_(&invoke<T>) {}
    const FieldSpec* field() const { return field_; }

private:
    template <typename T>
    static Result invoke(void* context, Client& client, const EndpointValueView* value, bool apply) {
        Property<T>& property = *static_cast<Property<T>*>(context);
        if (property.client_ != &client || property.spec_ == nullptr)
            return Result::failure(ErrorCode::NotConfigured);
        const FieldSpec& field = property.spec_->field();
        if (client.endpointRegistry().find(field.key) != &field)
            return Result::failure(ErrorCode::StateConflict);
        const EndpointHandle handle = client.endpoints().find(field.key);
        // A low-level network handler replacement is not a local task target.
        if (!handle.valid() || handle.slot_->context != &property ||
            handle.slot_->callback.command != &Property<T>::commandThunk)
            return Result::failure(ErrorCode::NotConfigured);
        if (value != nullptr) return property.writeValue(*value, apply);
        return ((property.callbackMode_ == Property<T>::Normal && property.callback_.normal != nullptr) ||
                (property.callbackMode_ == Property<T>::Checked && property.callback_.checked != nullptr))
            ? Result::success() : Result::failure(ErrorCode::NotConfigured);
    }
    const FieldSpec* field_;
    void* context_;
    Result (*invoke_)(void*, Client&, const EndpointValueView*, bool);
    friend class TaskPropertyTarget;
};

class TaskPropertyTarget final : public ITaskTarget {
public:
    TaskPropertyTarget(Client& client, const TaskPropertyBinding* bindings, size_t count)
        : client_(&client), bindings_(bindings), count_(count), applying_(false) {}
    // Product composition binds the real Client during attach(), before the
    // Device prepares its properties. No private Client accessor is needed.
    TaskPropertyTarget(const TaskPropertyBinding* bindings, size_t count)
        : client_(nullptr), bindings_(bindings), count_(count), applying_(false) {}
    // Optional destination belongs to the product's static typed call site.
    // Validate before copying so failed/repeated calls cannot alter a live list.
    Result configure(const TaskPropertyBinding* bindings, size_t count,
                     TaskPropertyBinding* destination = nullptr);
    Result attach(Client& client) {
        if (client_ != nullptr && client_ != &client)
            return Result::failure(ErrorCode::StateConflict);
        client_ = &client;
        return Result::success();
    }
    Result validate(const TaskSnapshotRecord& task) override { return dispatch(task, false); }
    Result apply(const TaskSnapshotRecord& task) override { return dispatch(task, true); }
    Result describe(size_t index, TaskTargetView& output, size_t& total) override;

private:
    static Result validateBinding(const TaskPropertyBinding* bindings, size_t index);
    Result dispatch(const TaskSnapshotRecord& task, bool apply);
    Client* client_;
    const TaskPropertyBinding* bindings_;
    size_t count_;
    bool applying_;
    TaskPropertyTarget(const TaskPropertyTarget&) = delete;
    TaskPropertyTarget& operator=(const TaskPropertyTarget&) = delete;
};

// No heap or fixed maximum reservation. One binding per explicitly selected
// Property. Runtime validation precedes publishing the list; a failed first
// attempt does not pin arguments in a function-local static initializer.
template <typename... T>
Result configureTaskProperties(TaskPropertyTarget& target, Property<T>&... properties) {
    static_assert(sizeof...(T) > 0U && sizeof...(T) <= kTaskMaximumTargets,
                  "Select 1..32 task Properties");
    const TaskPropertyBinding selected[] = {TaskPropertyBinding(properties)...};
    static TaskPropertyBinding storage[sizeof...(T)];
    static TaskPropertyTarget* owner = nullptr;
    if (owner != nullptr && owner != &target) return Result::failure(ErrorCode::AlreadyExists);
    const Result result = target.configure(selected, sizeof...(T), storage);
    if (result) owner = &target;
    return result;
}

} // namespace blinker
#endif
