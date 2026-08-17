#include "DirectBleLifecycle.h"

#include "../api/Client.h"

namespace blinker {

DirectBleLifecycle::DirectBleLifecycle(IFrameTransport& transport)
    : transport_(transport),
      client_(nullptr),
      lastError_(ErrorCode::Ok),
      started_(false) {}

DirectBleLifecycle::~DirectBleLifecycle() {
    stop();
}

Result DirectBleLifecycle::attach(Client& client) {
    if (started_) return Result::failure(ErrorCode::AlreadyExists);
    if (client_ != nullptr) {
        return client_ == &client
                   ? Result::success()
                   : Result::failure(ErrorCode::AlreadyExists);
    }
    Result result = client.addTransport(transport_);
    if (result) client_ = &client;
    lastError_ = result.code();
    return result;
}

Result DirectBleLifecycle::start() {
    if (started_) return Result::failure(ErrorCode::AlreadyExists);
    if (client_ == nullptr) {
        lastError_ = ErrorCode::NotConfigured;
        return Result::failure(lastError_);
    }
    Result result = client_->begin();
    if (result) started_ = true;
    lastError_ = result.code();
    return result;
}

void DirectBleLifecycle::poll(uint32_t totalBudgetMicros) {
    if (started_) client_->poll(totalBudgetMicros);
}

void DirectBleLifecycle::stop() {
    if (started_ && client_ != nullptr) client_->end();
    started_ = false;
    lastError_ = ErrorCode::Ok;
}

ProductLifecycleStatus DirectBleLifecycle::status() const {
    return ProductLifecycleStatus(
        started_ ? ProductLifecycleState::Active
                 : lastError_ == ErrorCode::Ok
                       ? ProductLifecycleState::Stopped
                       : ProductLifecycleState::Fault,
        lastError_,
        false);
}

ProductCapabilities DirectBleLifecycle::capabilities() const {
    return ProductCapabilities(ProductCapabilityDirectBleData);
}

} // namespace blinker
