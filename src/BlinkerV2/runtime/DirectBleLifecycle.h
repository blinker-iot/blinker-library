#ifndef BLINKER_RUNTIME_DIRECTBLELIFECYCLE_H
#define BLINKER_RUNTIME_DIRECTBLELIFECYCLE_H

#include "../interface/IProductLifecycle.h"
#include "../transport/IFrameTransport.h"

namespace blinker {

// Minimal BLE-only product lifecycle. The BLE transport stays managed by the
// one Client/Runtime graph, so BLE-only does not import WiFi, MQTT,
// provisioning or a product lifecycle supplied by the application.
class DirectBleLifecycle : public IProductLifecycle {
public:
    explicit DirectBleLifecycle(IFrameTransport& transport);
    ~DirectBleLifecycle();

    Result attach(Client& client) override;
    Result start() override;
    void poll(uint32_t totalBudgetMicros) override;
    void stop() override;
    ProductLifecycleStatus status() const override;
    ProductCapabilities capabilities() const override;

private:
    IFrameTransport& transport_;
    Client* client_;
    ErrorCode lastError_;
    bool started_;

    DirectBleLifecycle(const DirectBleLifecycle&);
    DirectBleLifecycle& operator=(const DirectBleLifecycle&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(DirectBleLifecycle) <= 16U,
    "Direct BLE lifecycle exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
