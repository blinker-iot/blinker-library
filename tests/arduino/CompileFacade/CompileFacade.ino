#ifndef BLINKER_FACADE_PROFILE
#define BLINKER_FACADE_PROFILE 1
#endif

#ifndef BLINKER_FACADE_DIAGNOSTICS
#define BLINKER_FACADE_DIAGNOSTICS 0
#endif

#if BLINKER_FACADE_PROFILE == 1
#define BLINKER_PRODUCT_WIFI 1
#elif BLINKER_FACADE_PROFILE == 2
#define BLINKER_PRODUCT_BLE 1
#else
#error "BLINKER_FACADE_PROFILE must be 1 or 2"
#endif

// This portable facade fixture provides its own lifecycle below. Public
// product/SDK linkage is covered separately by CompilePublicWiFi/BleQuickStart.
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker { namespace facade_detail {
template <typename First, typename... Rest>
struct EntryProfile<First, Rest...>
#if BLINKER_FACADE_PROFILE == 1
    : WifiEntryProfile {};
#else
    : LocalEntryProfile<BleProfile> {};
#endif
} }

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(temperature, float, blinker::readOnly());
BLINKER_ACTION(restartDevice);
BLINKER_EVENT(alarmEvent, uint32_t);

class CompileLifecycle : public blinker::IProductLifecycle
{
public:
    CompileLifecycle() : client_(nullptr), running_(false) {}

    blinker::Result attach(blinker::Client& client) override
    {
        if (client_ != nullptr && client_ != &client)
            return blinker::Result::failure(blinker::ErrorCode::AlreadyExists);
        client_ = &client;
        return blinker::Result::success();
    }

    blinker::Result start() override
    {
        running_ = client_ != nullptr;
        return running_
                   ? blinker::Result::success()
                   : blinker::Result::failure(blinker::ErrorCode::NotConfigured);
    }

    void poll(uint32_t budgetMicros) override
    {
        if (client_ != nullptr) client_->poll(budgetMicros);
    }

    void stop() override { running_ = false; }

    blinker::ProductLifecycleStatus status() const override
    {
        return blinker::ProductLifecycleStatus(
            running_ ? blinker::ProductLifecycleState::Active
                     : blinker::ProductLifecycleState::Stopped,
            blinker::ErrorCode::Ok,
            false);
    }

    blinker::ProductCapabilities capabilities() const override
    {
        return blinker::ProductCapabilities(
            blinker::ProductCapabilityCloudData);
    }

private:
    blinker::Client* client_;
    bool running_;
};

CompileLifecycle compileLifecycle;

namespace blinker {
namespace integration {

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile&)
{
    return compileLifecycle;
}

IProductLifecycle& lifecycle(facade_detail::BleProfile)
{
    return compileLifecycle;
}

} // namespace integration
} // namespace blinker

void setPower(bool requested)
{
    power.report(requested);
}

blinker::Result validateRestart()
{
    return blinker::Result::success();
}

void setup()
{
#if BLINKER_FACADE_DIAGNOSTICS != 0
    Serial.begin(115200);
#if BLINKER_FACADE_DIAGNOSTICS == 1
    Blinker.debug(Serial);
#elif BLINKER_FACADE_DIAGNOSTICS == 2
    Blinker.debug(Serial, blinker::LogLevel::Debug);
#else
#error "BLINKER_FACADE_DIAGNOSTICS must be 0, 1 or 2"
#endif
#endif
    power.onWrite(setPower);
    restartDevice.onInvokeChecked(validateRestart);
#if BLINKER_FACADE_PROFILE == 1
    Blinker.begin(
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8",
        "example-network",
        "example-password",
        power,
        temperature,
        restartDevice,
        alarmEvent);
#elif BLINKER_FACADE_PROFILE == 2
    Blinker.begin(
        power,
        temperature,
        restartDevice,
        alarmEvent);
#endif
    Blinker.report(power, false, temperature, 20.0f);
#if BLINKER_FACADE_DIAGNOSTICS != 0
    Blinker.printDiagnostics(Serial);
#endif
}

void loop()
{
    Blinker.run();
}
