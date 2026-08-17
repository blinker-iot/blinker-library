#ifndef BLINKER_PROVISIONING_ENROLLMENTKEYHTTPEXCHANGE_H
#define BLINKER_PROVISIONING_ENROLLMENTKEYHTTPEXCHANGE_H

#include "EnrollmentKey.h"
#include "../interface/IHttpClient.h"

namespace blinker {

struct EnrollmentKeyHttpConfig {
    StringView host;
    uint16_t port;
    HttpSecurity security;
    uint32_t timeoutMillis;

    EnrollmentKeyHttpConfig()
        : host(),
          port(0U),
          security(HttpSecurity::PlainTcp),
          timeoutMillis(10000U) {}
};

// Adds the WiFi-only bootstrap authorization header around one exact CBOR
// body. It never copies the key into the body or owns transport buffers.
class EnrollmentKeyHttpExchange {
public:
    EnrollmentKeyHttpExchange(
        IHttpClient& http,
        const EnrollmentKeyHttpConfig& config,
        StringView enrollmentKey);

    // Rebinds the borrowed key before a transaction starts. This lets the
    // official composition copy Sketch input into its own fixed storage
    // without constructing a second HTTP adapter.
    Result setEnrollmentKey(StringView enrollmentKey);
    Result start(
        StringView target,
        ByteView exactBody,
        MutableByteSpan responseBuffer);
    void poll(uint32_t budgetMicros) { http_.poll(budgetMicros); }
    HttpClientState state() const { return http_.state(); }
    Result response(HttpResponse& output) const {
        return http_.response(output);
    }
    ErrorCode lastError() const { return http_.lastError(); }
    void stop() { http_.stop(); }

private:
    IHttpClient& http_;
    EnrollmentKeyHttpConfig config_;
    HttpHeader authorizationHeader_;

    EnrollmentKeyHttpExchange(const EnrollmentKeyHttpExchange&);
    EnrollmentKeyHttpExchange& operator=(const EnrollmentKeyHttpExchange&);
};

} // namespace blinker

#endif
