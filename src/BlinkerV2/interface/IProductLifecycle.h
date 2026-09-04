#ifndef BLINKER_INTERFACE_IPRODUCTLIFECYCLE_H
#define BLINKER_INTERFACE_IPRODUCTLIFECYCLE_H

#include "../core/Result.h"

namespace blinker {

class Client;

enum class ProductLifecycleState : uint8_t {
    Stopped = 0U,
    Starting,
    Provisioning,
    Enrolling,
    Active,
    Fault
};

enum ProductCapability : uint16_t {
    ProductCapabilityNone = 0U,
    ProductCapabilityCloudData = 1U << 0,
    ProductCapabilityDirectBleData = 1U << 1,
    ProductCapabilityBleSetup = 1U << 2,
    ProductCapabilityAccessReset = 1U << 3,
    ProductCapabilityNetworkReset = 1U << 4
};

struct ProductCapabilities {
    uint16_t flags;

    explicit ProductCapabilities(uint16_t value = ProductCapabilityNone)
        : flags(value) {}

    bool supports(ProductCapability capability) const {
        return (flags & static_cast<uint16_t>(capability)) != 0U;
    }
};

struct ProductLifecycleStatus {
    ProductLifecycleState state;
    ErrorCode lastError;
    bool networkReady;

    ProductLifecycleStatus(
        ProductLifecycleState current = ProductLifecycleState::Stopped,
        ErrorCode error = ErrorCode::Ok,
        bool network = false)
        : state(current), lastError(error), networkReady(network) {}
};

// A platform integration attaches exactly once to one Device Client. start()
// may be called again after stop(), while poll() and stop() must stay bounded.
// Implementations own transport lifecycle ordering but never own Client.
class IProductLifecycle {
public:
    virtual ~IProductLifecycle() {}

    virtual Result attach(Client& client) = 0;
    virtual Result start() = 0;
    virtual void poll(uint32_t totalBudgetMicros) = 0;
    virtual void stop() = 0;
    // Clears the durable cloud/direct access root while preserving physical
    // transport configuration such as WiFi credentials and BLE bonds.
    // Product stops active sessions before calling this method.
    virtual Result resetAccess() {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    // Clears only durable WiFi credentials. Device identity, cloud key,
    // Direct access and BLE bond state remain intact.
    virtual Result resetNetwork() {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    virtual ProductLifecycleStatus status() const = 0;
    virtual ProductCapabilities capabilities() const = 0;
};

} // namespace blinker

#endif
