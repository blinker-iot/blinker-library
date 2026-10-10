#ifndef BLINKER_API_TASKPRODUCTLIFECYCLE_H
#define BLINKER_API_TASKPRODUCTLIFECYCLE_H

#include "TaskPropertyTarget.h"
#include "../interface/IProductLifecycle.h"
#include "../provisioning/TaskCommandAdapter.h"

namespace blinker {

// Internal opt-in composition, NOT a task management/owner authorization API.
// Module supplies begin() and engine(); its storage/authority must outlive this
// object and be usable AFTER the transport starts and WHILE it is stopped.
// The product is the sole cooperative caller. Native callbacks only post work.
// No task allocations, threads, default capability or per-platform reset copy.
template <typename Module>
class TaskProductLifecycle final : public IProductLifecycle {
public:
    TaskProductLifecycle(IProductLifecycle& transport, Module& tasks, TaskPropertyTarget& target,
        TaskCommandAdapter* management = nullptr)
        : transport_(transport), tasks_(tasks), target_(target), management_(management),
          taskError_(ErrorCode::NotConfigured), resetError_(ErrorCode::Ok),
          reset_(Reset::None), begun_(false), running_(false), polling_(false), stopPending_(false) {}

    Result attach(Client& client) override {
        Result result = transport_.attach(client);
        if (result) result = target_.attach(client);
        if (!result) transport_.stop();
        return result;
    }
    Result attachTime(Client& client, ITimeSync& time) override {
        return transport_.attachTime(client, time);
    }
    Result start() override {
        if (polling_) return Result::failure(ErrorCode::WouldBlock);
        if (running_) return Result::failure(ErrorCode::AlreadyExists);
        if (reset_ == Reset::Pending || reset_ == Reset::Failed)
            return Result::failure(ErrorCode::StateConflict);
        // Starting may admit a new owner. Do not let a cached successful reset
        // skip erasing that NEW root on a later physical reset. The task engine
        // itself remains Cleared until a fresh, trusted module lifecycle.
        if (reset_ == Reset::Complete) reset_ = Reset::None;
        // Catch missing begin() members/callbacks before admitting task storage
        // or transport. Persistence faults after admission remain independent
        // of ordinary control, as before.
        TaskTargetView unused; size_t total = 0U;
        Result result = target_.describe(UINT32_MAX, unused, total);
        if (!result) { taskError_ = result.code(); return result; }
        result = transport_.start();
        if (!result) return result;
        running_ = true;
        if (!begun_) {
            begun_ = true; // Never reopen a possibly dirty task persistence handle.
            result = tasks_.begin();
            taskError_ = result.code() == ErrorCode::NotFound &&
                         tasks_.engine().state() == TaskEngineState::Empty ? ErrorCode::Ok : result.code();
        }
        if (management_ != nullptr) {
            const Result managed = management_->start();
            if (!managed) taskError_ = managed.code();
        }
        // Optional task failure must not brick ordinary control/recovery.
        // It is visible separately; no task feature is advertised by this seam.
        return Result::success();
    }
    void poll(uint32_t totalBudgetMicros) override {
        if (!running_ || polling_) return;
        polling_ = true;
        if (management_ != nullptr) management_->poll();
        transport_.poll(totalBudgetMicros); // Consume retirement before execution.
        if (management_ != nullptr) management_->poll();
        const TaskEngineState state = tasks_.engine().state();
        if (running_ && reset_ == Reset::None &&
            (state == TaskEngineState::Ready || state == TaskEngineState::Recovering ||
             state == TaskEngineState::AwaitingAuthority)) {
            TaskPollReport report;
            taskError_ = tasks_.engine().poll(report).code();
        }
        polling_ = false;
        if (stopPending_) {
            stopPending_ = false;
            transport_.stop();
        }
        // One execution at most. Synchronous Flash latency is NOT bounded by
        // totalBudgetMicros; products must admit the measured storage policy.
    }
    void stop() override {
        running_ = false;
        if (management_ != nullptr) management_->stop();
        if (polling_) stopPending_ = true; // Never tear down a live callback stack.
        else transport_.stop();
        // Ordinary stop/reconnect does not retire or clear installed tasks.
    }
    Result resetAccess() override {
        if (!capabilities().supports(ProductCapabilityAccessReset))
            return Result::failure(ErrorCode::UnsupportedFeature);
        if (reset_ == Reset::Complete) return Result::success();
        if (reset_ == Reset::Failed) return Result::failure(resetError_);
        reset_ = Reset::Pending;
        if (management_ != nullptr) management_->retire();
        tasks_.engine().retire(); // Reject management/execution immediately.
        stop();
        if (polling_) return Result::failure(ErrorCode::WouldBlock);
        Result result = tasks_.engine().clearRetired();
        // A reentrant task callback can request retirement even if its caller
        // did not enter through our poll(). Retry only after that stack exits.
        if (result.code() == ErrorCode::WouldBlock) return result;
        if (result) result = transport_.resetAccess();
        resetError_ = result.code();
        reset_ = result ? Reset::Complete : Reset::Failed;
        return result; // Uncertain root/store failures never retry this instance.
    }
    Result resetNetwork() override {
        if (polling_) return Result::failure(ErrorCode::WouldBlock);
        if (reset_ == Reset::Pending || reset_ == Reset::Failed)
            return Result::failure(ErrorCode::StateConflict);
        // Product stops/restarts the transport; no task initialization or erase.
        return transport_.resetNetwork();
    }
    ProductLifecycleStatus status() const override { return transport_.status(); }
    ProductCapabilities capabilities() const override { return transport_.capabilities(); }
    ErrorCode taskError() const { return taskError_; }

private:
    enum class Reset : uint8_t { None, Pending, Failed, Complete };
    IProductLifecycle& transport_;
    Module& tasks_;
    TaskPropertyTarget& target_;
    TaskCommandAdapter* management_;
    ErrorCode taskError_, resetError_;
    Reset reset_;
    bool begun_, running_, polling_, stopPending_;
    TaskProductLifecycle(const TaskProductLifecycle&) = delete;
    TaskProductLifecycle& operator=(const TaskProductLifecycle&) = delete;
};

} // namespace blinker
#endif
