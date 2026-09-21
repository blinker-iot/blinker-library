#ifndef BLINKER_ESP32_LOCALWEBSOCKET_H
#define BLINKER_ESP32_LOCALWEBSOCKET_H

#include <sdkconfig.h>
#if !defined(ARDUINO_ARCH_ESP32) || !defined(CONFIG_HTTPD_WS_SUPPORT)
#error "Optional ESP32 LAN port requires the SDK HTTP server WebSocket feature"
#endif

#include <BlinkerV2/interface/ILocalMessageLink.h>
#include <BlinkerV2/interface/IRandom.h>
#include "Esp32LocalDiscovery.h"
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/transport/detail/WebSocketIngressGuard.h>
#include <esp_http_server.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace blinker {

struct Esp32LocalWebSocketConfig {
    uint16_t port = 8920U;
    uint16_t controlPort = 32769U; // Own HTTPD instance, not a device/MQTT identity.
    bool encryptedMessages = false; // Discovery hint only; never an authorization decision.
};

// Internal candidate: one STA IPv4 caller, SDK HTTP/WS decoder and mDNS.
// SDK task only fills a fixed mailbox. Product loop alone flushes WS output,
// AFTER LocalAccessFrameTransport has checked its shared authority. No crypto,
// Client callbacks, MQTT or authorization is performed in SDK tasks.
class Esp32LocalWebSocket final : public ILocalMessageLink {
public:
    explicit Esp32LocalWebSocket(IRandom& random,
        Esp32LocalWebSocketConfig config = Esp32LocalWebSocketConfig());
    ~Esp32LocalWebSocket() override { stop(); vSemaphoreDelete(mutex_); }
    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override;
    uint32_t sessionId() const override;
    size_t maxMessageSize() const override { return kMaximum; }
    Result receive(ByteView& message) override;
    Result send(ByteView message) override;
    void close() override;
    bool advertised() const { return discovery_.advertised(); }
    uint16_t port() const { return config_.port; }

private:
    enum : size_t { kMaximum = 420U };
    enum : uint32_t { kIoDeadline = 3000U, kUpgradeDeadline = 3000U };
    class Lock {
    public:
        explicit Lock(SemaphoreHandle_t mutex) : mutex_(mutex) { xSemaphoreTake(mutex_, portMAX_DELAY); }
        ~Lock() { xSemaphoreGive(mutex_); }
    private:
        SemaphoreHandle_t mutex_;
    };
    static Esp32LocalWebSocket& owner(httpd_handle_t server);
    static esp_err_t opened(httpd_handle_t server, int fd);
    static void closed(httpd_handle_t server, int fd);
    static int readSocket(httpd_handle_t server, int fd, char* buffer, size_t size, int flags);
    static int writeUpgrade(httpd_handle_t server, int fd, const char* buffer, size_t size, int flags);
    static esp_err_t request(httpd_req_t* request);
    static void networkEvent(void* context, esp_event_base_t base, int32_t event, void* data);
    static void keepContext(void*) {}
    void invalidateLocked();
    bool currentLocked(int fd) const;
    bool deadlineLocked(uint32_t now) const;
    void acceptUpgradeLocked();
    static int requestUrl(http_parser* parser, const char* data, size_t size);
    static int requestHeaders(http_parser* parser);
    // Fixed server frame encoding (no mask, no extensions, at most 420 B).
    static size_t frame(uint8_t* output, uint8_t opcode, ByteView body);

    Esp32LocalDiscovery discovery_;
    Esp32LocalWebSocketConfig config_;
    StaticSemaphore_t mutexStorage_;
    SemaphoreHandle_t mutex_;
    httpd_handle_t server_ = nullptr;
    esp_event_handler_instance_t wifiEvent_ = nullptr, ipEvent_ = nullptr;
    int fd_ = -1;
    uint32_t counter_ = 0U, session_ = 0U, openedAt_ = 0U, readAt_ = 0U, txAt_ = 0U;
    http_parser http_ = {};
    uint16_t httpBytes_ = 0U;
    uint8_t urlBytes_ = 0U;
    bool headersReady_ = false;
    bool active_ = false, websocket_ = false, reading_ = false, ready_ = false;
    bool closeAfterWrite_ = false;
    WebSocketIngressGuard guard_;
    uint16_t rxSize_ = 0U, txSize_ = 0U, txOffset_ = 0U, controlSize_ = 0U;
    uint8_t rx_[kMaximum] = {}, borrowed_[kMaximum] = {};
    uint8_t tx_[kMaximum + 4U] = {}, control_[127U] = {};
    Esp32LocalWebSocket(const Esp32LocalWebSocket&) = delete;
    Esp32LocalWebSocket& operator=(const Esp32LocalWebSocket&) = delete;
};

} // namespace blinker
#endif
