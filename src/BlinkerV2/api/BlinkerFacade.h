#ifndef BLINKER_API_BLINKERFACADE_H
#define BLINKER_API_BLINKERFACADE_H

#include <utility>

#include "Product.h"
#include "StateUpdate.h"
#include "WifiDeviceKeyProfile.h"
#include "TaskSupport.h"
#include "../core/Diagnostics.h"
#include "../interface/IDiagnosticOutput.h"
#include "../runtime/TimeSyncService.h"
#include "../location/Location.h"

#if defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_ARCH_RENESAS)
// Core API exposes arduino::Print via a using declaration. Match that entity
// even when this portable facade is included before Arduino.h.
namespace arduino { class Print; }
using arduino::Print;
#else
class Print;
#endif

#if (defined(BLINKER_PRODUCT_WIFI) && defined(BLINKER_PRODUCT_BLE)) || \
    (defined(BLINKER_PRODUCT_WIFI) && defined(BLINKER_PRODUCT_WIFI_BLE)) || \
    (defined(BLINKER_PRODUCT_BLE) && defined(BLINKER_PRODUCT_WIFI_BLE))
#error "Select exactly one Blinker product"
#endif

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE) || \
    (defined(BLINKER_PRODUCT_WIFI) && !defined(ARDUINO_ARCH_ESP32) && \
     !((defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_ARCH_RENESAS)) && \
       defined(BLINKER_INTERNAL_UNO_R4_TASKS) && BLINKER_INTERNAL_UNO_R4_TASKS) && \
     !((defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)) && \
       defined(BLINKER_INTERNAL_WIO_TASKS) && BLINKER_INTERNAL_WIO_TASKS))
#error "Tasks require an admitted manual WiFi platform (ESP32 or internal UNO R4/Wio)"
#endif
#endif

namespace blinker {

namespace facade_detail {

struct BleProfile {};
struct WifiBleProfile {};
struct NoProductProfile {};

// Entry headers specialize only nonempty interaction packs at the call site.
// Library translation units see the same facade and primary template, without
// needing the Sketch's product macro or allocating another global facade.
template <typename... Interactions>
struct EntryProfile {
    typedef NoProductProfile Profile;
};

struct WifiEntryProfile {
    typedef WifiDeviceKeyProfile Profile;
    static Profile make(StringView key, StringView ssid, StringView password) {
        return Profile(key, ssid, password);
    }
};

template <typename Selected>
struct LocalEntryProfile {
    typedef Selected Profile;
    static Profile make() { return Profile(); }
};

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
    BlinkerClass() : product_(nullptr), time_(nullptr), lastError_(ErrorCode::Ok) {}

    // Optional business UTC. Call before begin; no heap/global clock is created
    // unless this method is used. Does not set system time or credential time.
    bool enableTime() {
        if (product_ != nullptr) return remember(Result::failure(ErrorCode::AlreadyExists));
        static TimeSyncService service;
        time_ = &service;
        return remember(Result::success());
    }

    bool time(uint64_t& utcSeconds) {
        utcSeconds = 0U;
        return time_ != nullptr && time_->time(utcSeconds);
    }

    // Observe the same optional business clock, including unavailable/stale
    // state. Source is provenance, not trust; false never means UTC zero.
    bool timeStatus(WallClockSnapshot& snapshot) {
        snapshot = WallClockSnapshot();
        return time_ != nullptr && time_->readTime(snapshot);
    }

    // Accept one user-supplied WGS84 observation locally. This does not send,
    // authorize a stream, acknowledge delivery or set business/system UTC.
    // Without sampledAtMillis the caller asserts this is a new observation.
    bool location(Location& target, const LocationFix& observation) {
        return remember(target.update(observation));
    }
    bool location(Location& target, const LocationFix& observation, uint32_t sampledAtMillis) {
        return remember(target.update(observation, sampledAtMillis));
    }
    bool location(Location& target, double latitude, double longitude) {
        return remember(target.update(latitude, longitude));
    }
    bool location(Location& target, double latitude, double longitude, uint32_t sampledAtMillis) {
        return remember(target.update(latitude, longitude, sampledAtMillis));
    }
    // Loss suppresses further sampling but keeps the last local fix. Clear
    // also forgets that observation/sequence; neither writes ordinary State.
    bool locationLost(Location& target) { return remember(target.loseFix()); }
    bool clearLocation(Location& target) { return remember(target.clear()); }

    // Before begin(), explicitly allow only these scalar Properties. The
    // product copies bindings into fixed static storage; Property objects must
    // still outlive the session. Also enables the existing business UTC source.
    // This is firmware opt-in, NOT owner authorization or task initialization.
    template <typename First, typename... Rest>
    bool enableTasks(Property<First>& first, Property<Rest>&... rest) {
        typedef typename facade_detail::EntryProfile<
            Property<First>, Property<Rest>...>::Profile Profile;
        return enableTaskProfile<Profile>(first, rest...);
    }

    // Product selection belongs to the entry's dependent helper, not this
    // class definition. Wrong-product calls are rejected during substitution.
    template <typename... Interactions>
    auto begin(
        StringView deviceKey,
        StringView ssid,
        StringView password,
        Interactions&... interactions)
        -> decltype(facade_detail::EntryProfile<Interactions...>::make(
                        deviceKey, ssid, password), bool()) {
        return beginProfile(
            facade_detail::EntryProfile<Interactions...>::make(
                deviceKey, ssid, password),
            interactions...);
    }

    // BLE and WiFi+BLE use the same no-credential call shape. Checking fieldAt
    // excludes string arguments before they can steal the WiFi overload.
    template <typename First, typename... Rest>
    auto begin(First& first, Rest&... rest)
        -> decltype(first.fieldAt(0U),
                    facade_detail::EntryProfile<First, Rest...>::make(),
                    bool()) {
        return beginProfile(
            facade_detail::EntryProfile<First, Rest...>::make(),
            first,
            rest...);
    }

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

    // Invalidates cloud/direct controller access without erasing network
    // configuration, DeviceInstanceId or BLE bond state. Supported products
    // restart in provisioning mode before this call returns successfully.
    bool resetAccess() {
        const Result result = product_ != nullptr
                                  ? product_->resetAccess()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        const bool succeeded = remember(result);
        facade_detail::observeProductStatus(status());
        facade_detail::diagnostics().flush();
        return succeeded;
    }

    // Clears only saved WiFi credentials and restarts a supported product in
    // provisioning mode. Device identity, DeviceKey and Direct access remain.
    bool resetNetwork() {
        const Result result = product_ != nullptr
                                  ? product_->resetNetwork()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        const bool succeeded = remember(result);
        facade_detail::observeProductStatus(status());
        facade_detail::diagnostics().flush();
        return succeeded;
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
    // Print compatibility output is synchronous: it has no latency guarantee.
    void debug(Print& output, LogLevel level = LogLevel::Info);
    // Bounded single-attempt output; the adapter must honor tryWrite's contract.
    void debug(IDiagnosticOutput& output, LogLevel level = LogLevel::Info);
    void noDebug();
    // Explicit synchronous snapshot; do not call continuously from loop().
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
    template <typename Profile, typename... T>
    bool enableTaskProfile(Property<T>&... properties) {
        if (product_ != nullptr) return remember(Result::failure(ErrorCode::AlreadyExists));
        const Result result = integration::TaskSupport<Profile>::configure(properties...);
        return result ? enableTime() : remember(result);
    }

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
        const Result result = product_->begin(time_);
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
    ITimeSync* time_;
    ErrorCode lastError_;

    BlinkerClass(const BlinkerClass&);
    BlinkerClass& operator=(const BlinkerClass&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(BlinkerClass) <= 12U,
    "Global Blinker facade exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

extern blinker::BlinkerClass Blinker;

#endif
