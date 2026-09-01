#ifndef BLINKER_RUNTIME_EDGEHUBLIFECYCLE_H
#define BLINKER_RUNTIME_EDGEHUBLIFECYCLE_H

#include "../api/Client.h"
#include "../control/Session.h"
#include "../interface/IProductLifecycle.h"
#include "GatewayAccessDeliveryProcessor.h"
#include "WifiConnectionLifecycle.h"

namespace blinker {

enum class EdgeHubLifecycleState : uint8_t {
    Stopped = 0U,
    NetworkConnecting,
    TimeSynchronizing,
    CloudConnecting,
    ManagementSubscribing,
    Online,
    Fault
};

// Portable Edge Hub ordering. Template parameters are narrow test seams; the
// ESP32 product supplies the production types. Cloud admission remains closed
// until WiFi and trusted time are both available.
template <
    typename WifiConnection,
    typename CloudTransport,
    typename SessionProvider,
    typename TrustedClock,
    typename ManagementClient,
    typename RouteClient>
class BasicEdgeHubLifecycle final : public IProductLifecycle {
public:
    BasicEdgeHubLifecycle(
        WifiConnection& wifi,
        CloudTransport& cloud,
        SessionProvider& session,
        TrustedClock& clock,
        ManagementClient& management,
        RouteClient& route)
        : wifi_(wifi), cloud_(cloud), session_(session), clock_(clock),
          management_(management), route_(route), client_(nullptr),
          cloudCredentialGeneration_(0U),
          state_(EdgeHubLifecycleState::Stopped),
          lastError_(ErrorCode::Ok) {}

    ~BasicEdgeHubLifecycle() override { stop(); }

    Result attach(Client& client) override {
        if (state_ != EdgeHubLifecycleState::Stopped || client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        const Result result = client.addTransport(cloud_);
        if (result) client_ = &client;
        return result;
    }

    Result start() override {
        if (state_ != EdgeHubLifecycleState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        cloud_.setNetworkAvailable(false);
        management_.reset();
        route_.reset();
        clock_.stop();
        cloudCredentialGeneration_ = 0U;
        lastError_ = ErrorCode::Ok;
        Result result = wifi_.start();
        if (result) result = client_->begin();
        if (!result) return failStart(result.code());
        state_ = EdgeHubLifecycleState::NetworkConnecting;
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == EdgeHubLifecycleState::Stopped ||
            state_ == EdgeHubLifecycleState::Fault) {
            return;
        }

        wifi_.poll();
        if (wifi_.state() == WifiConnectionState::Fault) {
            enterFault(wifi_.lastError());
            return;
        }
        if (!wifi_.online()) {
            closeAdmission();
            client_->poll(totalBudgetMicros);
            state_ = EdgeHubLifecycleState::NetworkConnecting;
            lastError_ = wifi_.lastError();
            return;
        }

        if (!clock_.started()) {
            const Result result = clock_.start();
            if (!result) {
                enterFault(result.code());
                return;
            }
        }
        clock_.poll();
        if (!clock_.trusted()) {
            closeAdmission();
            client_->poll(totalBudgetMicros);
            state_ = EdgeHubLifecycleState::TimeSynchronizing;
            lastError_ = ErrorCode::Ok;
            return;
        }

        cloud_.setNetworkAvailable(true);
        client_->poll(totalBudgetMicros);
        if (cloud_.state() == TransportState::Error) {
            enterFault(cloud_.lastError());
            return;
        }
        if (cloud_.state() != TransportState::Online) {
            state_ = EdgeHubLifecycleState::CloudConnecting;
            lastError_ = ErrorCode::Ok;
            return;
        }

        const Result bound = bindCloudClients();
        if (!bound) {
            enterFault(bound.code());
            return;
        }
        management_.poll();
        route_.poll();
        state_ = management_.subscribed() && route_.subscribed()
                     ? EdgeHubLifecycleState::Online
                     : EdgeHubLifecycleState::ManagementSubscribing;
        lastError_ = management_.lastError();
    }

    void stop() override {
        if (state_ == EdgeHubLifecycleState::Stopped) return;
        closeAdmission();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        clock_.stop();
        lastError_ = ErrorCode::Ok;
        state_ = EdgeHubLifecycleState::Stopped;
    }

    ProductLifecycleStatus status() const override {
        ProductLifecycleState product = ProductLifecycleState::Starting;
        if (state_ == EdgeHubLifecycleState::Stopped) {
            product = ProductLifecycleState::Stopped;
        } else if (state_ == EdgeHubLifecycleState::Fault) {
            product = ProductLifecycleState::Fault;
        } else if (state_ == EdgeHubLifecycleState::Online) {
            product = ProductLifecycleState::Active;
        } else if (wifi_.state() ==
                   WifiConnectionState::WaitingForCredentials) {
            product = ProductLifecycleState::Provisioning;
        }
        return ProductLifecycleStatus(product, lastError_, wifi_.online());
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(ProductCapabilityCloudData);
    }

    EdgeHubLifecycleState state() const { return state_; }

private:
    Result bindCloudClients() {
        const uint32_t generation = session_.credentialGeneration();
        if (generation == 0U) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (generation == cloudCredentialGeneration_ &&
            management_.configured() && route_.configured()) {
            return Result::success();
        }
        SessionCredentials credentials;
        Result result = session_.credentials(credentials);
        GatewayAccessDeliveryContext context;
        context.deviceKeyVersion =
            session_.authenticatedCredentialVersion();
        if (result && context.deviceKeyVersion == 0U) {
            result = Result::failure(ErrorCode::ProtocolError);
        }
        if (result) result = management_.configure(credentials, context);
        if (result) result = route_.configure(credentials);
        if (result) cloudCredentialGeneration_ = generation;
        return result;
    }

    Result failStart(ErrorCode error) {
        closeAdmission();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        clock_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = EdgeHubLifecycleState::Stopped;
        return Result::failure(lastError_);
    }

    void closeAdmission() {
        cloud_.setNetworkAvailable(false);
        management_.reset();
        route_.reset();
        cloudCredentialGeneration_ = 0U;
    }

    void enterFault(ErrorCode error) {
        closeAdmission();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        clock_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = EdgeHubLifecycleState::Fault;
    }

    WifiConnection& wifi_;
    CloudTransport& cloud_;
    SessionProvider& session_;
    TrustedClock& clock_;
    ManagementClient& management_;
    RouteClient& route_;
    Client* client_;
    uint32_t cloudCredentialGeneration_;
    EdgeHubLifecycleState state_;
    ErrorCode lastError_;

    BasicEdgeHubLifecycle(const BasicEdgeHubLifecycle&);
    BasicEdgeHubLifecycle& operator=(const BasicEdgeHubLifecycle&);
};

} // namespace blinker

#endif
