#ifndef BLINKER_RUNTIME_TASKAUTHORITY_H
#define BLINKER_RUNTIME_TASKAUTHORITY_H

#include "../core/SecureMemory.h"

namespace blinker {

// Owned persistent metadata, NOT a credential or proof. Only the trusted
// authority adapter may establish/rebind it after server authorization.
// Scope is an independent 128-bit ownership tenure, never a widened epoch.
struct TaskAuthorityBinding {
    uint8_t deviceInstanceId[16];
    uint8_t taskScope[16];
    uint8_t rootDigest[32];

    TaskAuthorityBinding() : deviceInstanceId(), taskScope(), rootDigest() {}
    bool valid() const {
        uint8_t instance = 0U, scope = 0U, root = 0U;
        for (size_t i = 0U; i < 16U; ++i) {
            instance |= deviceInstanceId[i]; scope |= taskScope[i];
        }
        for (size_t i = 0U; i < 32U; ++i) root |= rootDigest[i];
        return instance != 0U && scope != 0U && root != 0U;
    }
    bool sameOwner(const TaskAuthorityBinding& other) const {
        return constantTimeEqual(ByteView(deviceInstanceId, 16U), ByteView(other.deviceInstanceId, 16U)) &&
               constantTimeEqual(ByteView(taskScope, 16U), ByteView(other.taskScope, 16U));
    }
    bool operator==(const TaskAuthorityBinding& other) const {
        return sameOwner(other) && constantTimeEqual(ByteView(rootDigest, 32U), ByteView(other.rootDigest, 32U));
    }
    bool operator!=(const TaskAuthorityBinding& other) const { return !(*this == other); }
};

// Pending is not retirement. Current must come from actual current roots and
// a verified scope mapping, never copied back out of the stored task header.
enum class TaskAuthorityState : uint8_t { Pending, Current, Retired };
struct TaskAuthorityObservation {
    TaskAuthorityBinding binding;
    TaskAuthorityState state;
    TaskAuthorityObservation() : binding(), state(TaskAuthorityState::Pending) {}
};

} // namespace blinker
#endif
