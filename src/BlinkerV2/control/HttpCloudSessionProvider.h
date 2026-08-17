#ifndef BLINKER_CONTROL_HTTPCLOUDSESSIONPROVIDER_H
#define BLINKER_CONTROL_HTTPCLOUDSESSIONPROVIDER_H

#include "../interface/IClock.h"
#include "../interface/IHttpClient.h"
#include "../interface/ICloudCredentialSource.h"
#include "../interface/IRandom.h"
#include "../interface/ISessionProvider.h"
#include "CloudSessionContract.h"

namespace blinker {

enum : size_t {
    kHttpCloudSessionMinimumRequestBufferSize = 256U,
    kHttpCloudSessionMinimumResponseBufferSize = 640U
};

struct HttpCloudSessionConfig {
    StringView host;
    uint16_t port;
    StringView path;
    StringView firmwareVersion;
    HttpSecurity httpSecurity;
    MqttSecurity mqttSecurity;
    uint32_t requestTimeoutMs;
    uint32_t initialRetryMs;
    uint32_t maximumRetryMs;
    uint32_t maximumServerRetryMs;

    HttpCloudSessionConfig()
        : port(80U),
          httpSecurity(HttpSecurity::PlainTcp),
          mqttSecurity(MqttSecurity::PlainTcp),
          requestTimeoutMs(15000U),
          initialRetryMs(1000U),
          maximumRetryMs(30000U),
          maximumServerRetryMs(3600000U) {}
};

// Canonical-CBOR Device V2 session provider over an explicit HTTP security
// profile. Cloud authentication secrets are
// loaded only while calculating one HMAC and are wiped before the HTTP
// transaction begins. The long-lived arena contains only short-term MQTT
// credentials returned by the server.
class HttpCloudSessionProvider : public ISessionProvider {
public:
    HttpCloudSessionProvider(
        ICloudCredentialSource& credentialSource,
        IRandom& random,
        IHttpClient& http,
        IClock& clock,
        const HttpCloudSessionConfig& config,
        MutableByteSpan requestBuffer,
        MutableByteSpan responseBuffer,
        MutableCharSpan credentialArena);
    ~HttpCloudSessionProvider();

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
    CloudSessionWireError lastWireError() const { return lastWireError_; }

private:
    Result validateConfiguration() const;
    Result validateActiveCredential();
    Result beginRequest(bool createRequestId);
    Result startHttp(ByteView body);
    void processResponse();
    void processSuccess(ByteView body);
    void processError(uint16_t statusCode, ByteView body);
    void scheduleRetry(ErrorCode error, uint32_t delayMs = 0U);
    void fail(ErrorCode error);
    void finishRequest();
    void clearRequestData();
    void clearCredentials();
    uint32_t retryDelay() const;
    uint32_t serverRetryDelay(uint32_t seconds) const;
    bool retryDue(uint32_t now) const;
    bool credentialsExpired(uint32_t now) const;
    static bool retryableClientError(ErrorCode error);
    static ErrorCode mapWireError(CloudSessionWireError error);
    static bool validPath(StringView value);
    static bool buffersOverlap(ByteView left, ByteView right);

    ICloudCredentialSource& credentialSource_;
    IRandom& random_;
    IHttpClient& http_;
    IClock& clock_;
    HttpCloudSessionConfig config_;
    MutableByteSpan requestBuffer_;
    MutableByteSpan responseBuffer_;
    MutableCharSpan credentialArena_;
    HttpHeader requestHeaders_[2];
    SessionCredentials credentials_;
    uint8_t requestId_[kCloudSessionRequestIdSize];
    uint32_t requestCredentialVersion_;
    uint32_t requestGeneration_;
    uint32_t credentialGeneration_;
    uint32_t retryAt_;
    uint32_t expiresAt_;
    uint32_t refreshAt_;
    uint8_t retryAttempt_;
    SessionProviderState state_;
    ErrorCode lastError_;
    CloudSessionWireError lastWireError_;
    bool started_;
    bool networkAvailable_;
    bool hasRequestId_;
    bool newRequestPending_;
    bool hasCredentials_;
    bool hasWireError_;

    HttpCloudSessionProvider(const HttpCloudSessionProvider&);
    HttpCloudSessionProvider& operator=(const HttpCloudSessionProvider&);
};

} // namespace blinker

#endif
