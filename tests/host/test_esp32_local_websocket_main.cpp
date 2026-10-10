#include <BlinkerV2/arduino/ports/esp32/Esp32LocalWebSocket.inc>
#include <BlinkerV2/arduino/ports/esp32/Esp32LocalDiscovery.inc>
#include <cstdlib>
#include <iostream>

fake_local_ws::State fake_local_ws::state;
FakeWiFi WiFi;
namespace {
using namespace blinker;
using Bytes = std::vector<uint8_t>;
auto& sdk = fake_local_ws::state;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (false)
class Random final : public IRandom {
public:
    uint8_t next = 0U;
    Result fill(MutableByteSpan output) override {
        for (size_t i = 0; i < output.size; ++i) output.data[i] = ++next;
        return Result::success();
    }
};
httpd_req_t request(int method = 0) { return {fake_local_ws::handle(), method, 0U, "/bbp2"}; }
void open(Esp32LocalWebSocket& port, bool legacyGetCallback = false) {
    sdk.upgraded = false;
    CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
    CHECK(port.sessionId() == 0U);
    const std::string http = "GET /bbp2 HTTP/1.1\r\nHost: local\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n";
    sdk.wireInput.assign(http.begin(), http.end());
    char buffer[1024];
    while (!sdk.wireInput.empty()) CHECK(sdk.receive(fake_local_ws::handle(), 7, buffer, sizeof(buffer), 0) > 0);
    port.poll(100U); CHECK(port.sessionId() == 0U); // Parsed headers are not a completed upgrade.
    sdk.upgraded = true;
    if (legacyGetCallback) { auto req = request(HTTP_GET); CHECK(sdk.uri.handler(&req) == ESP_OK); }
    port.poll(100U); CHECK(port.sessionId() != 0U);
    const auto session = port.sessionId();
    if (legacyGetCallback) { auto req = request(HTTP_GET); CHECK(sdk.uri.handler(&req) == ESP_OK); }
    port.poll(100U); CHECK(port.sessionId() == session); // Never allocate twice.
}
void closed() { sdk.config.close_fn(fake_local_ws::handle(), 7); }
void event(esp_event_base_t base, int id) {
    auto& value = sdk.events[base - 1]; value.callback(value.context, base, id, nullptr);
}
void incoming(const Bytes& payload, uint8_t type = 2U, bool final = true) {
    sdk.type = type; sdk.final = final; sdk.payload = payload;
    sdk.wireInput = {static_cast<uint8_t>(type | (final ? 128U : 0U))};
    if (payload.size() <= 125U) sdk.wireInput.push_back(static_cast<uint8_t>(128U | payload.size()));
    else {
        sdk.wireInput.push_back(254U); sdk.wireInput.push_back(static_cast<uint8_t>(payload.size() >> 8U));
        sdk.wireInput.push_back(static_cast<uint8_t>(payload.size()));
    }
    sdk.wireInput.insert(sdk.wireInput.end(), 4U, 0U); // Zero mask makes payload assertion clear.
    sdk.wireInput.insert(sdk.wireInput.end(), payload.begin(), payload.end());
    char buffer[1024];
    while (!sdk.wireInput.empty()) CHECK(sdk.receive(fake_local_ws::handle(), 7, buffer, sizeof(buffer), 0) > 0);
}
}

int main() {
    Random random;
    for (const bool encrypted : {false, true}) {
        sdk = fake_local_ws::State(); Esp32LocalWebSocketConfig config; config.encryptedMessages = encrypted;
        Esp32LocalWebSocket port(random, config); CHECK(port.start());
        CHECK(sdk.securityHint == (encrypted ? "noise" : "plain"));
        port.stop();
    }
    {
        sdk = fake_local_ws::State(); Esp32LocalWebSocket port(random); CHECK(port.start());
        open(port, true); closed(); // SDK <= 5.5.4 callback remains compatible.
        open(port); closed(); // SDK >= 5.5.5 has no GET callback.
        const auto rejected = [&]() {
            sdk.upgraded = false; CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
            sdk.wireInput = Bytes{'\r','\n','\r','\n'}; char buffer[16];
            CHECK(sdk.receive(fake_local_ws::handle(), 7, buffer, sizeof(buffer), 0) == HTTPD_SOCK_ERR_FAIL);
            CHECK(port.sessionId() == 0U); closed();
        };
        sdk.parsedUrl = "/bbp2?x"; rejected(); sdk.parsedUrl = "/bbp2";
        sdk.parsedMethod = 0; rejected(); sdk.parsedMethod = HTTP_GET;
        sdk.parsedUpgrade = 0; rejected(); sdk.parsedUpgrade = 1;
        sdk.parsedFlags = F_CHUNKED; rejected(); sdk.parsedFlags = F_CONTENTLENGTH;
        sdk.parsedContentLength = 1; rejected(); sdk.parsedContentLength = 0;
        open(port); closed(); port.stop();
    }
    {
        sdk = fake_local_ws::State(); Esp32LocalWebSocket port(random); CHECK(port.start());
        for (const int family : {AF_INET, AF_INET6}) {
            sdk.localFamily = family; open(port); closed();
            sdk.localIp = 999U; CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL);
            sdk.localIp = sdk.ip; sdk.shortAddress = true;
            CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL); sdk.shortAddress = false;
        }
        sdk.mappedIpv4 = false; CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL);
        sdk.localFamily = 99; CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL);
        sdk.localFamily = AF_INET6; sdk.mappedIpv4 = true; sdk.ip = 0U;
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL); port.stop();
    }
    {
        sdk = fake_local_ws::State(); Esp32LocalWebSocket port(random);
        CHECK(port.start()); CHECK(port.advertised());
        CHECK(sdk.config.max_open_sockets == 1U && !sdk.config.lru_purge_enable);
        CHECK(sdk.hostnameChanges == 0 && sdk.serviceAdds == 1);
        CHECK(port.start().code() == ErrorCode::AlreadyExists);
        open(port); const uint32_t first = port.sessionId();
        Bytes payload(400U, 37U);
        CHECK(port.send(ByteView(payload.data(), payload.size())));
        CHECK(sdk.wireOutput.empty()); // Never a background or send()-time write.
        CHECK(port.send(ByteView(payload.data(), payload.size())).code() == ErrorCode::WouldBlock);
        sdk.writeLimit = 7U;
        for (size_t i = 0U; i < 60U; ++i) port.poll(100U);
        CHECK(sdk.wireOutput.size() == 404U);
        CHECK(sdk.wireOutput[0] == 130U && sdk.wireOutput[1] == 126U && sdk.wireOutput[2] == 1U && sdk.wireOutput[3] == 144U);
        CHECK(Bytes(sdk.wireOutput.begin() + 4, sdk.wireOutput.end()) == payload);

        auto req = request(); sdk.readLimit = 3U;
        incoming(Bytes{1U, 2U, 3U}); CHECK(sdk.uri.handler(&req) == ESP_OK);
        ByteView borrowed; CHECK(port.receive(borrowed)); CHECK(borrowed.size == 3U && borrowed.data[2] == 3U);
        event(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED);
        CHECK(port.sessionId() == 0U);
        CHECK(borrowed.data[2] == 3U); // Network callback cannot race the borrowed decoder buffer.
        port.poll(100U); CHECK(borrowed.data[2] == 0U);
        closed(); open(port); CHECK(port.sessionId() > first);

        // Exact queued record is discarded by local close/revoke before poll.
        sdk.wireOutput.clear(); CHECK(port.send(ByteView(payload.data(), payload.size())));
        port.close(); port.poll(100U); CHECK(sdk.wireOutput.empty());
        closed(); open(port); CHECK(port.send(ByteView(payload.data(), payload.size())));
        sdk.blocked = true; port.poll(100U); CHECK(port.sessionId() != 0U);
        sdk.now += 3000U; port.poll(100U); CHECK(port.sessionId() == 0U);
        CHECK(sdk.wireOutput.empty()); sdk.blocked = false; closed();
        port.stop(); CHECK(!port.advertised());
        CHECK(sdk.serviceRemoves == 1 && sdk.hostname == "user-owned-host" && sdk.unregisters == 2);
        CHECK(port.start()); open(port); CHECK(port.sessionId() > first); closed(); port.stop();
    }
    {
        sdk = fake_local_ws::State(); Esp32LocalWebSocket port(random); CHECK(port.start()); open(port);
        auto req = request(); ByteView output;
        incoming(Bytes{1U, 2U}, 2U, false); CHECK(sdk.uri.handler(&req) == ESP_OK);
        CHECK(port.receive(output).code() == ErrorCode::WouldBlock);
        sdk.now += 2000U;
        incoming(Bytes{9U}, 9U); CHECK(sdk.uri.handler(&req) == ESP_OK);
        port.poll(100U); CHECK(sdk.wireOutput == Bytes({138U, 1U, 9U}));
        sdk.now += 1000U; port.poll(100U); CHECK(port.sessionId() == 0U); // Ping cannot extend fragmented input.
        closed(); open(port);
        incoming(Bytes{1U, 2U}, 2U, false); CHECK(sdk.uri.handler(&req) == ESP_OK);
        incoming(Bytes{3U}, 0U, true); CHECK(sdk.uri.handler(&req) == ESP_OK);
        CHECK(port.receive(output)); CHECK(output.size == 3U && output.data[2] == 3U);
        // A slow consumer keeps a fixed mailbox, resumes on the next poll/receive.
        incoming(Bytes{4U}); CHECK(sdk.uri.handler(&req) == ESP_OK);
        incoming(Bytes{5U});
        sdk.delayed = [&]() { ByteView prior; CHECK(port.receive(prior)); sdk.delayed = nullptr; };
        CHECK(sdk.uri.handler(&req) == ESP_OK); CHECK(port.receive(output)); CHECK(output.data[0] == 5U);
        incoming(Bytes{3U, 237U}, 8U); CHECK(sdk.uri.handler(&req) == ESP_FAIL); // Reserved 1005.
        closed(); open(port);
        incoming(Bytes{3U, 232U, 255U}, 8U); CHECK(sdk.uri.handler(&req) == ESP_FAIL); // Invalid UTF-8 reason.
        closed(); open(port); sdk.wireOutput.clear();
        incoming(Bytes{3U, 232U}, 8U); CHECK(sdk.uri.handler(&req) == ESP_OK);
        port.poll(100U); CHECK(sdk.wireOutput == Bytes({136U, 0U})); CHECK(port.sessionId() == 0U);
        closed(); port.stop();
    }
    {
        sdk = fake_local_ws::State(); Esp32LocalWebSocket port(random); CHECK(port.start());
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
        sdk.now += 3000U; port.poll(100U); CHECK(sdk.shutdowns > 0);
        auto req = request(HTTP_GET); CHECK(sdk.uri.handler(&req) == ESP_FAIL); closed();
        // No AP ingress, ordinary HTTP admission, oversized header or early WS read-ahead.
        sdk.localIp = 999U; CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_FAIL); sdk.localIp = sdk.ip;
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
        sdk.upgraded = false; CHECK(sdk.uri.handler(&req) == ESP_FAIL); closed(); sdk.upgraded = true;
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
        sdk.wireInput = Bytes{'\r', '\n', '\r', '\n', 130U}; char bytes[2048];
        CHECK(sdk.receive(fake_local_ws::handle(), 7, bytes, sizeof(bytes), 0) == HTTPD_SOCK_ERR_FAIL); closed();
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
        sdk.wireInput.assign(1025U, 'A'); sdk.readLimit = 2048U;
        CHECK(sdk.receive(fake_local_ws::handle(), 7, bytes, sizeof(bytes), 0) == HTTPD_SOCK_ERR_FAIL); closed();
        // Upgrade writes use a short-write loop, but cannot exceed accept-time deadline.
        CHECK(sdk.config.open_fn(fake_local_ws::handle(), 7) == ESP_OK);
        sdk.writeLimit = 1U; CHECK(sdk.transmit(fake_local_ws::handle(), 7, "101", 3U, 0) == 3);
        sdk.blocked = true; CHECK(sdk.transmit(fake_local_ws::handle(), 7, "101", 3U, 0) == HTTPD_SOCK_ERR_FAIL);
        closed(); port.stop();
    }
    {
        sdk = fake_local_ws::State(); sdk.failAdvertise = true;
        Esp32LocalWebSocket port(random); CHECK(!port.start());
        CHECK(port.state() == TransportState::Stopped && sdk.stops == 1 && sdk.unregisters == 2);
        CHECK(sdk.serviceRemoves == 0); // Never remove another owner's discovery.
    }
    {
        sdk = fake_local_ws::State(); sdk.hostname.clear();
        Esp32LocalWebSocket port(random); CHECK(port.start());
        CHECK(sdk.hostnameChanges == 1 && sdk.hostname.size() == 32U);
        const std::string hostname = sdk.hostname;
        port.stop(); CHECK(sdk.hostname == hostname && sdk.serviceRemoves == 1);
    }
    std::cout << "ESP32 local WS adapter lifecycle/backpressure tests passed (mock SDK)\n";
}
