#ifndef BLINKER_RUNTIME_TASKTIMEENVIRONMENT_H
#define BLINKER_RUNTIME_TASKTIMEENVIRONMENT_H

#include "TaskEngine.h"
#include "../interface/ITimeSync.h"

namespace blinker {

// Internal composition adapter. Authority reader is a bounded, side-effect-free
// read of CURRENT verified authority, never a binding copied from task input.
// No configured tick / no verified scope means fail closed; absent UTC alone
// still supplies a monotonic tick so countdowns can run without network time.
class TaskTimeEnvironment final : public ITaskEnvironment {
public:
    typedef void (*AuthorityReader)(void* context, TaskAuthorityObservation& output);
    TaskTimeEnvironment(ITimeSync& time, AuthorityReader authority, void* context)
        : time_(&time), authority_(authority), context_(context) {}
    TaskTimeEnvironment(AuthorityReader authority, void* context)
        : time_(nullptr), authority_(authority), context_(context) {}
    Result attach(ITimeSync& time) {
        if (time_ != nullptr && time_ != &time) return Result::failure(ErrorCode::AlreadyExists);
        time_ = &time; return Result::success();
    }
    void observe(TaskObservation& output) override {
        output = TaskObservation();
        if (time_ != nullptr && time_->observeTime(output.time, output.nowMillis) && authority_ != nullptr)
            authority_(context_, output.authority);
    }
private:
    ITimeSync* time_;
    AuthorityReader authority_;
    void* context_;
};
} // namespace blinker
#endif
