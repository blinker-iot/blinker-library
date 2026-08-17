#ifndef BLINKER_API_PRODUCT_H
#define BLINKER_API_PRODUCT_H

#include "Device.h"
#include "../interface/IProductLifecycle.h"

namespace blinker {

struct ProductStatus {
    ProductLifecycleState state;
    ErrorCode lastError;
    bool started;
    bool networkReady;
    bool localReady;
    bool cloudReady;

    ProductStatus()
        : state(ProductLifecycleState::Stopped),
          lastError(ErrorCode::Ok),
          started(false),
          networkReady(false),
          localReady(false),
          cloudReady(false) {}
};

// The single public lifecycle facade shared by every platform and connectivity
// profile. Platform integrations implement IProductLifecycle; this class owns
// no native client, transport, buffer or platform service.
class Product {
public:
    Product(Device& device, IProductLifecycle& lifecycle)
        : device_(device),
          lifecycle_(lifecycle),
          attached_(false),
          started_(false),
          lastError_(ErrorCode::Ok) {}

    ~Product() { end(); }

    Result begin() {
        if (started_) {
            return Result::failure(ErrorCode::AlreadyExists);
        }

        Result result = device_.prepare();
        if (result && !attached_) {
            result = lifecycle_.attach(device_.client());
            if (result) attached_ = true;
        }
        const bool startAttempted = result.ok();
        if (startAttempted) result = lifecycle_.start();
        if (!result) {
            if (startAttempted) lifecycle_.stop();
            lastError_ = result.code();
            return result;
        }

        started_ = true;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    void run(uint32_t totalBudgetMicros = 1000U) {
        if (started_) lifecycle_.poll(totalBudgetMicros);
    }

    void end() {
        if (!started_) return;
        lifecycle_.stop();
        started_ = false;
        lastError_ = ErrorCode::Ok;
    }

    ProductStatus status() const {
        const ProductLifecycleStatus lifecycleStatus = lifecycle_.status();
        const ClientStatus clientStatus = device_.status();
        ProductStatus result;
        result.state = lifecycleStatus.state;
        result.lastError = lastError_ != ErrorCode::Ok
                               ? lastError_
                               : lifecycleStatus.lastError;
        result.started = started_;
        result.networkReady = lifecycleStatus.networkReady;
        result.localReady = clientStatus.localReady;
        result.cloudReady = clientStatus.cloudReady;
        return result;
    }

    ProductCapabilities capabilities() const {
        return lifecycle_.capabilities();
    }

    Result resetOwnership() {
        if (!attached_) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        return lifecycle_.resetOwnership();
    }

private:
    Device& device_;
    IProductLifecycle& lifecycle_;
    bool attached_;
    bool started_;
    ErrorCode lastError_;

    Product(const Product&);
    Product& operator=(const Product&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(Product) <= 16U,
    "Public Product facade exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
