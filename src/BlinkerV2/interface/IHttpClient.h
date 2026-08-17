#ifndef BLINKER_INTERFACE_IHTTPCLIENT_H
#define BLINKER_INTERFACE_IHTTPCLIENT_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class HttpSecurity : uint8_t {
    PlainTcp = 0,
    Tls
};

enum class HttpMethod : uint8_t {
    Get = 0,
    Post
};

enum class HttpClientState : uint8_t {
    Idle = 0,
    InProgress,
    Complete,
    Failed
};

struct HttpHeader {
    StringView name;
    StringView value;

    HttpHeader() : name(), value() {}
    HttpHeader(StringView headerName, StringView headerValue)
        : name(headerName), value(headerValue) {}
};

struct HttpRequest {
    HttpMethod method;
    StringView host;
    uint16_t port;
    HttpSecurity security;
    StringView target;
    StringView contentType;
    const HttpHeader* headers;
    uint8_t headerCount;
    ByteView body;
    MutableByteSpan responseBuffer;
    uint32_t timeoutMillis;

    HttpRequest()
        : method(HttpMethod::Get),
          port(0),
          security(HttpSecurity::Tls),
          headers(nullptr),
          headerCount(0U),
          timeoutMillis(10000) {}
};

struct HttpResponse {
    uint16_t statusCode;
    ByteView body;

    HttpResponse() : statusCode(0), body() {}
};

// start() only validates and stages the request. It must not initiate DNS,
// TCP, or TLS connection work; implementations do that work from poll().
// Request/header views and the response buffer must remain valid until
// Complete, Failed, or stop(). Implementations must never follow redirects to
// a weaker security mode without returning UnsupportedFeature.
class IHttpClient {
public:
    virtual ~IHttpClient() {}

    virtual Result start(const HttpRequest& request) = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual HttpClientState state() const = 0;
    virtual Result response(HttpResponse& response) const = 0;
    virtual ErrorCode lastError() const = 0;
    virtual void stop() = 0;
};

} // namespace blinker

#endif
