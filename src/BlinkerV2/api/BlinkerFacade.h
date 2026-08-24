#ifndef BLINKER_API_BLINKERFACADE_H
#define BLINKER_API_BLINKERFACADE_H

#include <utility>

#include "Product.h"
#include "StateUpdate.h"
#include "WifiDeviceKeyProfile.h"
#include "../core/Diagnostics.h"

class Print;

#if (defined(BLINKER_PRODUCT_WIFI) && defined(BLINKER_PRODUCT_BLE)) || \
    (defined(BLINKER_PRODUCT_WIFI) && defined(BLINKER_PRODUCT_WIFI_BLE)) || \
    (defined(BLINKER_PRODUCT_BLE) && defined(BLINKER_PRODUCT_WIFI_BLE))
#error "Select exactly one Blinker product"
#endif

namespace blinker {

namespace facade_detail {

struct BleProfile {};
struct WifiBleProfile {};

Diagnostics& diagnostics();
void observeProductStatus(const ProductStatus& status);

} // namespace facade_detail

// A product package implements exactly one of these overloads. The selected
// public entry header and its package are a compile-time product choice.
namespace integration {
IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile);
IProductLifecycle& lifecycle(facade_detail::BleProfile);
IProductLifecycle& lifecycle(facade_detail::WifiBleProfile);
} // namespace integration

namespace facade_detail {

template <typename T>
struct TypeIdentity {
    typedef T Type;
};

template <typename... Interactions>
class Session {
public:
    Session(
        IProductLifecycle& lifecycle,
        Interactions&... interactions)
        : schema_(interactions...),
          device_(schema_, DeviceConfig(), &diagnostics()),
          product_(device_, lifecycle) {}

    Product& product() { return product_; }
    bool matches(Interactions&... interactions) const {
        return schema_.matches(interactions...);
    }

private:
    DeviceSchema<Interactions...> schema_;
    Device device_;
    Product product_;

    Session(const Session&);
    Session& operator=(const Session&);
};

} // namespace facade_detail

// The one ordinary Sketch facade. It owns no second model, state cache,
// transport or platform object. The selected begin() specialization creates
// exactly one static, no-heap Session and delegates to Device/Product.
class BlinkerClass {
public:
    BlinkerClass() : product_(nullptr), lastError_(ErrorCode::Ok) {}

#if defined(BLINKER_PRODUCT_WIFI)
    // BlinkerWiFi.h selects the WiFi product before compilation, so begin()
    // only needs the device credential, network and no-heap schema.
    template <typename... Interactions>
    bool begin(
        StringView deviceKey,
        StringView ssid,
        StringView password,
        Interactions&... interactions) {
        return beginProfile(
            WifiDeviceKeyProfile(deviceKey, ssid, password),
            interactions...);
    }
#elif defined(BLINKER_PRODUCT_BLE)
    // BLE needs no connectivity arguments. DeviceSchema enforces that at
    // least one semantic interaction is present.
    template <typename First, typename... Rest>
    bool begin(First& first, Rest&... rest) {
        return beginProfile(
            facade_detail::BleProfile(),
            first,
            rest...);
    }
#elif defined(BLINKER_PRODUCT_WIFI_BLE)
    // WiFi+BLE onboarding is App-driven. Sketch credentials are neither
    // required nor accepted by this product selector.
    template <typename First, typename... Rest>
    bool begin(First& first, Rest&... rest) {
        return beginProfile(
            facade_detail::WifiBleProfile(),
            first,
            rest...);
    }
#endif

    void run(uint32_t totalBudgetMicros = 1000U) {
        if (product_ != nullptr) {
            product_->run(totalBudgetMicros);
            facade_detail::observeProductStatus(product_->status());
        }
        facade_detail::diagnostics().flush();
    }

    void end() {
        if (product_ != nullptr) product_->end();
        lastError_ = ErrorCode::Ok;
        facade_detail::observeProductStatus(status());
        facade_detail::diagnostics().flush();
    }

    ProductStatus status() const {
        return product_ != nullptr ? product_->status() : ProductStatus();
    }

    ErrorCode lastError() const { return lastError_; }
    const char* lastErrorText() const;
    const DiagnosticCounters& diagnosticCounters() const {
        return facade_detail::diagnostics().counters();
    }
    OutboundSchedulerCounters outboundCounters() const {
        return product_ != nullptr
                   ? product_->outboundCounters()
                   : OutboundSchedulerCounters();
    }
    TelemetryCounters telemetryCounters() const {
        return product_ != nullptr
                   ? product_->telemetryCounters()
                   : TelemetryCounters();
    }
    void debug(Print& output, LogLevel level = LogLevel::Info);
    void noDebug();
    void printDiagnostics(Print& output);

    template <typename T, typename... Rest>
    bool report(
        Property<T>& property,
        typename facade_detail::TypeIdentity<T>::Type value,
        Rest&&... rest) {
        static_assert(
            sizeof...(Rest) % 2U == 0U,
            "Blinker.report requires Property/value pairs");
        Client* client = property.client_;
        if (client == nullptr) {
            return remember(Result::failure(ErrorCode::NotConfigured));
        }

        StateUpdate update(*client);
        Result result = update.begin(1U + sizeof...(Rest) / 2U);
        bool commitAttempted = false;
        if (result) {
            result = stageAll(
                update,
                *client,
                property,
                value,
                std::forward<Rest>(rest)...);
        }
        if (result) {
            commitAttempted = true;
            result = update.commit();
        }
        if (!result && !commitAttempted) {
            facade_detail::diagnostics().recordOperation(
                DiagnosticOperation::StateReport, result);
        }
        return remember(result);
    }

private:
    template <typename Profile, typename... Interactions>
    bool beginProfile(Profile profile, Interactions&... interactions) {
        static facade_detail::Session<Interactions...> session(
            integration::lifecycle(profile),
            interactions...);
        Product* selected = &session.product();
        if (product_ != nullptr && product_ != selected) {
            return remember(Result::failure(ErrorCode::AlreadyExists));
        }
        if (!session.matches(interactions...)) {
            return remember(Result::failure(ErrorCode::AlreadyExists));
        }
        product_ = selected;
        const Result result = product_->begin();
        facade_detail::diagnostics().recordOperation(
            DiagnosticOperation::Begin, result);
        facade_detail::observeProductStatus(product_->status());
        const bool succeeded = remember(result);
        facade_detail::diagnostics().flush();
        return succeeded;
    }
    bool remember(Result result) {
        lastError_ = result.code();
        return result.ok();
    }

    static Result stageAll(StateUpdate&, Client&) {
        return Result::success();
    }

    template <typename T, typename... Rest>
    static Result stageAll(
        StateUpdate& update,
        Client& client,
        Property<T>& property,
        typename facade_detail::TypeIdentity<T>::Type value,
        Rest&&... rest) {
        if (property.client_ != &client || property.spec_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        EndpointHandle endpoint = client.endpoints().find(
            property.spec_->field().key);
        Result result = endpoint.valid()
                            ? stageValue(update, endpoint, value)
                            : Result::failure(endpoint.error());
        return result
                   ? stageAll(
                         update,
                         client,
                         std::forward<Rest>(rest)...)
                   : result;
    }

    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        bool value) {
        return update.set(endpoint, value);
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        int32_t value) {
        return update.set(endpoint, static_cast<int64_t>(value));
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        uint32_t value) {
        return update.set(endpoint, static_cast<uint64_t>(value));
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        float value) {
        return update.set(endpoint, value);
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        double value) {
        return update.set(endpoint, value);
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        StringView value) {
        return update.setText(endpoint, value);
    }
    static Result stageValue(
        StateUpdate& update,
        const EndpointHandle& endpoint,
        ByteView value) {
        return update.setBytes(endpoint, value);
    }

    Product* product_;
    ErrorCode lastError_;

    BlinkerClass(const BlinkerClass&);
    BlinkerClass& operator=(const BlinkerClass&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(BlinkerClass) <= 8U,
    "Global Blinker facade exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

extern blinker::BlinkerClass Blinker;

#endif
