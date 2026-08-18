#ifndef BLINKER_API_BLINKERFACADE_H
#define BLINKER_API_BLINKERFACADE_H

#include <utility>

#include "Product.h"
#include "StateUpdate.h"
#include "WifiDeviceKeyProfile.h"

namespace blinker {

struct WiFiTag {};
struct BleTag {};
struct WiFiBleTag {};

constexpr WiFiTag WiFi = WiFiTag();
constexpr BleTag BLE = BleTag();
constexpr WiFiBleTag WiFiBLE = WiFiBleTag();

// Platform packages implement only the profiles they support. These static
// overloads keep radio SDKs out of the facade and let the linker discard every
// unselected product graph.
namespace integration {
IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile);
IProductLifecycle& lifecycle(BleTag);
IProductLifecycle& lifecycle(WiFiBleTag);
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
          device_(schema_, DeviceConfig()),
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

    // The ordinary WiFi path mirrors the familiar Arduino/Blynk ordering:
    // mode, device credential, network, then the explicit no-heap schema.
    template <typename... Interactions>
    bool begin(
        WiFiTag,
        StringView deviceKey,
        StringView ssid,
        StringView password,
        Interactions&... interactions) {
        return beginProfile(
            WifiDeviceKeyProfile(deviceKey, ssid, password),
            interactions...);
    }

    template <typename... Interactions>
    bool begin(BleTag profile, Interactions&... interactions) {
        return beginProfile(profile, interactions...);
    }

    template <typename... Interactions>
    bool begin(WiFiBleTag profile, Interactions&... interactions) {
        return beginProfile(profile, interactions...);
    }

    void run(uint32_t totalBudgetMicros = 1000U) {
        if (product_ != nullptr) product_->run(totalBudgetMicros);
    }

    void end() {
        if (product_ != nullptr) product_->end();
        lastError_ = ErrorCode::Ok;
    }

    ProductStatus status() const {
        return product_ != nullptr ? product_->status() : ProductStatus();
    }

    bool resetOwnership() {
        return remember(
            product_ != nullptr
                ? product_->resetOwnership()
                : Result::failure(ErrorCode::NotConfigured));
    }

    ErrorCode lastError() const { return lastError_; }
    const char* lastErrorText() const;

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
        if (result) {
            result = stageAll(
                update,
                *client,
                property,
                value,
                std::forward<Rest>(rest)...);
        }
        if (result) result = update.commit();
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
        return remember(product_->begin());
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
