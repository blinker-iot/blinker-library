#ifndef BLINKER_CONTROL_HTTPDEVICEKEYSESSIONPROVIDER_H
#define BLINKER_CONTROL_HTTPDEVICEKEYSESSIONPROVIDER_H

#include "../identity/DeviceInstanceId.h"
#include "../interface/IClock.h"
#include "../interface/IDeviceKeySource.h"
#include "../interface/IHttpClient.h"
#include "../interface/IRandom.h"
#include "../interface/ISessionProvider.h"
#include "DeviceKeySessionContract.h"

namespace blinker {

enum : size_t {
    kHttpDeviceKeySessionMinimumRequestBufferSize =
        kDeviceKeySessionRequestMaxEncodedSize,
    kHttpDeviceKeySessionMinimumResponseBufferSize =
        kDeviceKeySessionResponseMaxEncodedSize
};

struct HttpDeviceKeySessionConfig {
    StringView host;
    uint16_t port;
    StringView firmwareVersion;
    HttpSecurity httpSecurity;
    MqttSecurity mqttSecurity;
    uint32_t requestTimeoutMs;
    uint32_t initialRetryMs;
    uint32_t maximumRetryMs;
    uint32_t maximumServerRetryMs;

    HttpDeviceKeySessionConfig()
        : port(80U),
          httpSecurity(HttpSecurity::PlainTcp),
          mqttSecurity(MqttSecurity::PlainTcp),
          requestTimeoutMs(15000U),
          initialRetryMs(1000U),
          maximumRetryMs(30000U),
          maximumServerRetryMs(3600000U) {}
};

// Non-blocking two-step DeviceKey challenge/session exchange. The raw key is
// loaded only while deriving locator/proof and is wiped before HTTP starts.
// Request/response storage and the short-term MQTT credential arena are
// supplied by the composition, so the provider owns no heap allocation.
class HttpDeviceKeySessionProvider final : public ISessionProvider {
public:
    HttpDeviceKeySessionProvider(
        IDeviceKeySource& keySource,
        const DeviceInstanceId& deviceInstanceId,
        IRandom& random,
        IHttpClient& http,
        IClock& clock,
        const HttpDeviceKeySessionConfig& config,
        MutableByteSpan requestBuffer,
        MutableByteSpan responseBuffer,
        MutableCharSpan credentialArena);
    ~HttpDeviceKeySessionProvider();

    Result start() override;
    Result requestRefresh() override;
    void setNetworkAvailable(bool available) override;
    void poll(uint32_t budgetMicros) override;
    SessionProviderState state() const override { return state_; }
    ErrorCode lastError() const override { return lastError_; }
    uint32_t credentialGeneration() const override {
        return credentialGeneration_;
    }
    Result credentials(SessionCredentials& output) const override;
    void stop() override;

    bool hasWireError() const { return hasWireError_; }
    DeviceKeySessionWireError lastWireError() const {
        return lastWireError_;
    }
    uint32_t authenticatedCredentialVersion() const {
        return authenticatedCredentialVersion_;
    }

private:
    enum class Phase : uint8_t {
        Challenge = 0U,
        Session
    };

    Result validateConfiguration() const;
    Result validateKeySource();
    Result beginChallenge(bool createTransaction);
    Result beginSession();
    Result startHttp(StringView target, ByteView body);
    void processResponse();
    void processChallengeSuccess(ByteView body);
    void processSessionSuccess(ByteView body);
    void processError(uint16_t statusCode, ByteView body);
    void scheduleRetry(ErrorCode error, uint32_t delayMs = 0U);
    void restartChallenge();
    void fail(ErrorCode error);
    void finishHttp();
    void clearWireBuffers();
    void clearTransaction();
    void clearCredentials();
    uint32_t retryDelay() const;
    uint32_t serverRetryDelay(uint32_t seconds) const;
    bool retryDue(uint32_t now) const;
    bool challengeExpired(uint32_t now) const;
    bool credentialsExpired(uint32_t now) const;
    static bool retryableClientError(ErrorCode error);
    static ErrorCode mapWireError(DeviceKeySessionWireError error);
    static bool buffersOverlap(ByteView first, ByteView second);

    IDeviceKeySource& keySource_;
    const DeviceInstanceId& deviceInstanceId_;
    IRandom& random_;
    IHttpClient& http_;
    IClock& clock_;
    HttpDeviceKeySessionConfig config_;
    MutableByteSpan requestBuffer_;
    MutableByteSpan responseBuffer_;
    MutableCharSpan credentialArena_;
    SessionCredentials credentials_;
    uint8_t requestId_[kDeviceKeyRequestIdSize];
    uint8_t locator_[kDeviceKeyLocatorSize];
    uint8_t clientNonce_[kDeviceKeyNonceSize];
    uint8_t challengeId_[kDeviceKeyChallengeIdSize];
    uint8_t serverNonce_[kDeviceKeyNonceSize];
    uint32_t challengeCredentialVersion_;
    uint32_t authenticatedCredentialVersion_;
    uint32_t credentialGeneration_;
    uint32_t challengeExpiresAt_;
    uint32_t retryAt_;
    uint32_t expiresAt_;
    uint32_t refreshAt_;
    uint8_t retryAttempt_;
    Phase phase_;
    SessionProviderState state_;
    ErrorCode lastError_;
    DeviceKeySessionWireError lastWireError_;
    bool started_;
    bool networkAvailable_;
    bool hasTransaction_;
    bool hasChallenge_;
    bool hasCredentials_;
    bool hasWireError_;

    HttpDeviceKeySessionProvider(const HttpDeviceKeySessionProvider&);
    HttpDeviceKeySessionProvider& operator=(
        const HttpDeviceKeySessionProvider&);
};

} // namespace blinker

#endif
