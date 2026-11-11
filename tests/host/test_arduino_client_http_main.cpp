#include <ArduinoClientHttp.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>

namespace {

#define CHECK_TRUE(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::cerr << "Arduino HTTP check failed at line "              \
                      << __LINE__ << ": " << #condition << std::endl;      \
            std::exit(1);                                                    \
        }                                                                    \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class FakeClock final : public blinker::IClock {
public:
    FakeClock() : now(0U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t milliseconds) { now += milliseconds; }
    uint32_t now;
};

class FakeClient {
public:
    FakeClient()
        : input(nullptr),
          inputSize(0U),
          inputOffset(0U),
          connectCalls(0U),
          addressConnectCalls(0U),
          connectTimeout(-1),
          stopCalls(0U),
          writeCalls(0U),
          connectedState(false) {}

    bool connect(const char*, uint16_t) {
        ++connectCalls;
        connectedState = true;
        return true;
    }

    bool connect(const char* host, uint16_t port, int32_t timeout) {
        connectTimeout = timeout;
        return connect(host, port);
    }

    bool connect(const IPAddress&, uint16_t) {
        ++connectCalls;
        ++addressConnectCalls;
        connectedState = true;
        return true;
    }

    bool connect(const IPAddress& address, uint16_t port, int32_t timeout) {
        connectTimeout = timeout;
        return connect(address, port);
    }

    size_t write(const uint8_t*, size_t size) {
        ++writeCalls;
        return size;
    }

    int available() const {
        return static_cast<int>(inputSize - inputOffset);
    }

    int read() {
        return inputOffset < inputSize ? input[inputOffset++] : -1;
    }

    bool connected() const { return connectedState; }

    void stop() {
        ++stopCalls;
        connectedState = false;
    }

    void respond(const char* response) {
        input = reinterpret_cast<const uint8_t*>(response);
        inputSize = std::strlen(response);
        inputOffset = 0U;
    }

    const uint8_t* input;
    size_t inputSize;
    size_t inputOffset;
    uint32_t connectCalls;
    uint32_t addressConnectCalls;
    int32_t connectTimeout;
    uint32_t stopCalls;
    uint32_t writeCalls;
    bool connectedState;
};

class FakeUntimedClient final : public FakeClient {
public:
    bool connect(const char* host, uint16_t port) {
        return FakeClient::connect(host, port);
    }

    bool connect(const IPAddress& address, uint16_t port) {
        return FakeClient::connect(address, port);
    }
};

class FakeHostOnlyClient final : public FakeClient {
public:
    bool connect(const char* host, uint16_t port) {
        return FakeClient::connect(host, port);
    }
};

blinker::HttpRequest makeRequest(uint8_t* response, size_t capacity) {
    blinker::HttpRequest request;
    request.host = blinker::StringView("gateway.example.com");
    request.port = 80U;
    request.security = blinker::HttpSecurity::PlainTcp;
    request.target = blinker::StringView("/session");
    request.responseBuffer = blinker::MutableByteSpan(response, capacity);
    request.timeoutMillis = 1000U;
    return request;
}

void testStagedRequestAndResponse() {
    FakeClock clock;
    FakeClient network;
    blinker::ArduinoClientHttpAdapter<FakeClient> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    const blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));

    CHECK_TRUE(http.start(request));
    CHECK_EQ(static_cast<uint32_t>(0U), network.connectCalls);
    http.poll(100U);
    CHECK_EQ(static_cast<uint32_t>(1U), network.connectCalls);
    CHECK_EQ(static_cast<int32_t>(250), network.connectTimeout);
    CHECK_TRUE(network.writeCalls != 0U);

    network.respond(
        "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabc");
    http.poll(1000U);
    CHECK_EQ(blinker::HttpClientState::Complete, http.state());
    blinker::HttpResponse response;
    CHECK_TRUE(http.response(response));
    CHECK_EQ(static_cast<uint16_t>(200U), response.statusCode);
    CHECK_EQ(static_cast<size_t>(3U), response.body.size);
    CHECK_TRUE(std::memcmp(response.body.data, "abc", 3U) == 0);
}

void testUntimedClientFallback() {
    FakeClock clock;
    FakeUntimedClient network;
    blinker::ArduinoClientHttpAdapter<FakeUntimedClient> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};

    CHECK_TRUE(http.start(makeRequest(responseBuffer, sizeof(responseBuffer))));
    http.poll(100U);
    CHECK_EQ(static_cast<uint32_t>(1U), network.connectCalls);
    CHECK_EQ(static_cast<int32_t>(-1), network.connectTimeout);
}

void testNumericIpv4BypassesHostResolution() {
    FakeClock clock;
    FakeClient network;
    blinker::ArduinoClientHttpAdapter<FakeClient> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));
    request.host = blinker::StringView("192.168.2.76");

    CHECK_TRUE(http.start(request));
    http.poll(100U);
    CHECK_EQ(static_cast<uint32_t>(1U), network.connectCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), network.addressConnectCalls);
    CHECK_EQ(static_cast<int32_t>(250), network.connectTimeout);
}

void testNumericIpv4FallsBackForHostOnlyClient() {
    FakeClock clock;
    FakeHostOnlyClient network;
    blinker::ArduinoClientHttpAdapter<FakeHostOnlyClient> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));
    request.host = blinker::StringView("192.168.2.76");

    CHECK_TRUE(http.start(request));
    http.poll(100U);
    CHECK_EQ(static_cast<uint32_t>(1U), network.connectCalls);
    CHECK_EQ(static_cast<uint32_t>(0U), network.addressConnectCalls);
}

void testRequestTimeout() {
    FakeClock clock;
    FakeClient network;
    blinker::ArduinoClientHttpAdapter<FakeClient> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    const blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));

    CHECK_TRUE(http.start(request));
    http.poll(100U);
    clock.advance(1000U);
    http.poll(100U);
    CHECK_EQ(blinker::HttpClientState::Failed, http.state());
    CHECK_EQ(blinker::ErrorCode::NotConnected, http.lastError());
    CHECK_TRUE(!network.connected());
}

void testLongNonFramingHeaderIsDiscarded() {
    FakeClock clock;
    FakeClient network;
    blinker::ArduinoClientHttpAdapter<FakeClient, 128U, 512U, 32U> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    const blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));

    CHECK_TRUE(http.start(request));
    http.poll(100U);
    network.respond(
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Headers: Content-Type,Authorization,"
        "Idempotency-Key,X-Request-ID\r\n"
        "Content-Length: 3\r\n\r\nabc");
    http.poll(1000U);

    CHECK_EQ(blinker::HttpClientState::Complete, http.state());
    blinker::HttpResponse response;
    CHECK_TRUE(http.response(response));
    CHECK_EQ(static_cast<size_t>(3U), response.body.size);
    CHECK_TRUE(std::memcmp(response.body.data, "abc", 3U) == 0);
}

void testLongFramingHeaderIsRejected() {
    FakeClock clock;
    FakeClient network;
    blinker::ArduinoClientHttpAdapter<FakeClient, 128U, 512U, 24U> http(
        network, blinker::HttpSecurity::PlainTcp, clock);
    uint8_t responseBuffer[32U] = {};
    const blinker::HttpRequest request =
        makeRequest(responseBuffer, sizeof(responseBuffer));

    CHECK_TRUE(http.start(request));
    http.poll(100U);
    network.respond(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length:          3\r\n\r\nabc");
    http.poll(1000U);

    CHECK_EQ(blinker::HttpClientState::Failed, http.state());
    CHECK_EQ(blinker::ErrorCode::CapacityExceeded, http.lastError());
}

void testChunkedErrorRetryClassification() {
    const unsigned statuses[] = {200U, 302U, 400U, 401U, 403U, 404U, 408U,
        409U, 425U, 429U, 499U, 500U, 502U, 503U, 504U, 599U};
    for (unsigned status : statuses) {
        FakeClock clock;
        FakeClient network;
        blinker::ArduinoClientHttpAdapter<FakeClient> http(
            network, blinker::HttpSecurity::PlainTcp, clock);
        uint8_t bytes[32U] = {};
        CHECK_TRUE(http.start(makeRequest(bytes, sizeof(bytes))));
        http.poll(100U);
        char response[128U];
        std::snprintf(response, sizeof(response),
            "HTTP/1.1 %u Response\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n", status);
        network.respond(response);
        http.poll(1000U);
        const bool transient = status == 408U || status == 425U || status == 429U || status >= 500U;
        CHECK_EQ(blinker::HttpClientState::Failed, http.state());
        CHECK_EQ(transient ? blinker::ErrorCode::WouldBlock : blinker::ErrorCode::UnsupportedFeature,
                 http.lastError());
        CHECK_TRUE(!network.connected());
        blinker::HttpResponse rejected;
        CHECK_TRUE(!http.response(rejected)); // Never accept the chunk bytes as CBOR.
        network.respond(""); // A new socket does not inherit unread old response bytes.
        CHECK_TRUE(http.start(makeRequest(bytes, sizeof(bytes))));
        http.poll(100U);
        network.respond("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabc");
        http.poll(1000U);
        CHECK_EQ(blinker::HttpClientState::Complete, http.state());
    }
}

} // namespace

int main() {
    testStagedRequestAndResponse();
    testUntimedClientFallback();
    testNumericIpv4BypassesHostResolution();
    testNumericIpv4FallsBackForHostOnlyClient();
    testRequestTimeout();
    testLongNonFramingHeaderIsDiscarded();
    testLongFramingHeaderIsRejected();
    testChunkedErrorRetryClassification();
    std::cout << "Arduino Client HTTP adapter tests passed" << std::endl;
    return 0;
}
