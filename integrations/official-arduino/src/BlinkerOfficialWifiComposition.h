#ifndef BLINKER_OFFICIAL_WIFI_COMPOSITION_H
#define BLINKER_OFFICIAL_WIFI_COMPOSITION_H

#include "BlinkerOfficialWifiCloudStack.h"

#include <string.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline EnrollmentKeyHttpConfig enrollmentHttpConfig() {
    EnrollmentKeyHttpConfig config;
    config.host = StringView(official::controlHost);
    config.port = official::controlPort;
    config.security = official::cloudSessionTls
                          ? HttpSecurity::Tls
                          : HttpSecurity::PlainTcp;
    return config;
}

// WiFi-only onboarding above the shared identity/WiFi/cloud stack. Both
// pre-enrolled and headless EnrollmentKey devices use this one graph and the
// same public Blinker.begin().
template <typename Platform>
class WifiComposition final : public IProductLifecycle {
public:
    WifiComposition()
        : stack_(),
          enrollmentHttp_(
              stack_.controlHttp(), enrollmentHttpConfig(), StringView()),
          onboarding_(
              stack_.claim(),
              stack_.cloudEnrollment(),
              enrollmentHttp_,
              stack_.clock()),
          lifecycle_(
              stack_.wifiLifecycle(),
              onboarding_,
              stack_.arena(),
              stack_.cloudTransport(),
              stack_.clock()),
          setupSsid_(),
          setupCredential_(),
          enrollmentKey_(),
          setupSsidSize_(0U),
          setupCredentialSize_(0U),
          enrollmentKeySize_(0U),
          configurationError_(ErrorCode::Ok),
          setupConfigured_(false) {}

    ~WifiComposition() override {
        lifecycle_.stop();
        stack_.end();
        clearSetup();
    }

    void configure(const WifiOnboardingProfile& profile) {
        if (stack_.initialized() || setupConfigured_) {
            configurationError_ = ErrorCode::AlreadyExists;
            return;
        }

        WifiNetworkConfig network;
        network.ssid = asBytes(profile.ssid());
        network.authentication = profile.openNetwork()
                                     ? WifiAuthentication::Open
                                     : WifiAuthentication::Wpa2Personal;
        network.credential = asBytes(profile.password());
        Result result = validateWifiNetworkConfig(network);
        if (result) result = validateEnrollmentKey(profile.enrollmentKey());
        if (!result) {
            configurationError_ = result.code();
            return;
        }

        memcpy(setupSsid_, network.ssid.data, network.ssid.size);
        if (!network.credential.empty()) {
            memcpy(
                setupCredential_,
                network.credential.data,
                network.credential.size);
        }
        memcpy(
            enrollmentKey_,
            profile.enrollmentKey().data,
            profile.enrollmentKey().size);
        setupSsidSize_ = static_cast<uint8_t>(network.ssid.size);
        setupCredentialSize_ =
            static_cast<uint8_t>(network.credential.size);
        enrollmentKeySize_ =
            static_cast<uint8_t>(profile.enrollmentKey().size);
        setupConfigured_ = true;
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = lifecycle_.attach(client);
        if (!result) configurationError_ = result.code();
        return result;
    }

    Result start() override {
        const Result result = stack_.initialized()
                                  ? lifecycle_.start()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        if (!result) configurationError_ = result.code();
        return result;
    }

    void poll(uint32_t totalBudgetMicros) override {
        lifecycle_.poll(totalBudgetMicros);
        if (enrollmentKeySize_ != 0U && onboarding_.active()) {
            const Result result = enrollmentHttp_.setEnrollmentKey(
                StringView());
            if (result) clearEnrollmentKey();
        }
    }

    void stop() override { lifecycle_.stop(); }

    ProductLifecycleStatus status() const override {
        ProductLifecycleStatus current = lifecycle_.status();
        if (configurationError_ != ErrorCode::Ok &&
            current.state == ProductLifecycleState::Stopped) {
            current.state = ProductLifecycleState::Fault;
            current.lastError = configurationError_;
        }
        return current;
    }

    ProductCapabilities capabilities() const override {
        return lifecycle_.capabilities();
    }

private:
    static ByteView asBytes(StringView value) {
        return ByteView(
            reinterpret_cast<const uint8_t*>(value.data), value.size);
    }

    WifiNetworkConfig setupNetwork() const {
        WifiNetworkConfig network;
        network.ssid = ByteView(setupSsid_, setupSsidSize_);
        network.authentication = setupCredentialSize_ == 0U
                                     ? WifiAuthentication::Open
                                     : WifiAuthentication::Wpa2Personal;
        network.credential = ByteView(
            setupCredential_, setupCredentialSize_);
        return network;
    }

    Result initialize() {
        if (stack_.initialized()) return Result::success();
        if (configurationError_ != ErrorCode::Ok) {
            return Result::failure(configurationError_);
        }

        Result result = stack_.begin();
        if (result && setupConfigured_) {
            result = stack_.platform().wifiCredentials().commit(
                setupNetwork());
            clearNetworkSetup();
        }
        if (result) {
            const StringView key = setupConfigured_
                                       ? StringView(
                                             enrollmentKey_,
                                             enrollmentKeySize_)
                                       : StringView();
            result = enrollmentHttp_.setEnrollmentKey(key);
        }
        if (!result) {
            stack_.end();
            configurationError_ = result.code();
            clearSetup();
        }
        return result;
    }

    void clearNetworkSetup() {
        secureZero(MutableByteSpan(setupSsid_, sizeof(setupSsid_)));
        secureZero(MutableByteSpan(
            setupCredential_, sizeof(setupCredential_)));
        setupSsidSize_ = 0U;
        setupCredentialSize_ = 0U;
    }

    void clearEnrollmentKey() {
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(enrollmentKey_),
            sizeof(enrollmentKey_)));
        enrollmentKeySize_ = 0U;
    }

    void clearSetup() {
        clearNetworkSetup();
        clearEnrollmentKey();
    }

    WifiCloudStack<Platform> stack_;
    EnrollmentKeyHttpExchange enrollmentHttp_;
    WifiOnboardingFlow onboarding_;
    WifiOnboardingCloudLifecycle lifecycle_;

    uint8_t setupSsid_[kWifiSsidMaxSize];
    uint8_t setupCredential_[kWifiCredentialMaxSize];
    char enrollmentKey_[kEnrollmentKeyMaximumSize];
    uint8_t setupSsidSize_;
    uint8_t setupCredentialSize_;
    uint8_t enrollmentKeySize_;
    ErrorCode configurationError_;
    bool setupConfigured_;

    WifiComposition(const WifiComposition&);
    WifiComposition& operator=(const WifiComposition&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
