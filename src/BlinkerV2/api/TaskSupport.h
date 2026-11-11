#ifndef BLINKER_API_TASK_SUPPORT_H
#define BLINKER_API_TASK_SUPPORT_H

#include "Interactions.h"

namespace blinker {
namespace integration {

// Product specialization owns optional composition; facade owns no task table,
// SDK, storage or authorization. Unsupported/disabled products allocate nothing.
template <typename Profile>
struct TaskSupport {
    template <typename... T>
    static Result configure(Property<T>&...) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
};

} // namespace integration
} // namespace blinker
#endif
