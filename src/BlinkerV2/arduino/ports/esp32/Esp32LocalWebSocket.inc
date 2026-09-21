#if defined(ARDUINO_ARCH_ESP32) && defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET
#include "Esp32LocalWebSocket.h"
#include <BlinkerV2/core/Utf8.h>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/sockets.h>
#include <errno.h>

namespace blinker {

namespace {
bool validWebSocketClose(ByteView payload) {
    if (payload.empty()) return true;
    if (payload.size < 2U || payload.size > 125U) return false;
    const uint16_t code = static_cast<uint16_t>((static_cast<uint16_t>(payload.data[0]) << 8U) | payload.data[1]);
    const bool registered = code >= 1000U && code <= 1014U && code != 1004U && code != 1005U && code != 1006U;
    return (registered || (code >= 3000U && code <= 4999U)) &&
        isValidUtf8(StringView(reinterpret_cast<const char*>(payload.data + 2U), payload.size - 2U));
}
} // namespace

Esp32LocalWebSocket::Esp32LocalWebSocket(IRandom& random, Esp32LocalWebSocketConfig config)
    : discovery_(random), config_(config), mutex_(xSemaphoreCreateMutexStatic(&mutexStorage_)), guard_(kMaximum) {}

Esp32LocalWebSocket& Esp32LocalWebSocket::owner(httpd_handle_t server) {
    return *static_cast<Esp32LocalWebSocket*>(httpd_get_global_user_ctx(server));
}

Result Esp32LocalWebSocket::start() {
    if (server_ != nullptr) return Result::failure(ErrorCode::AlreadyExists);
    if (config_.port == 0U || config_.controlPort == 0U) return Result::failure(ErrorCode::InvalidArgument);
    // IP events also invalidate an unchanged IPv4 after a fast disconnect/rejoin.
    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, networkEvent, this, &wifiEvent_) != ESP_OK ||
        esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, networkEvent, this, &ipEvent_) != ESP_OK) {
        stop(); return Result::failure(ErrorCode::NotConfigured);
    }
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = config_.port; config.ctrl_port = config_.controlPort;
    config.max_open_sockets = 1U; config.max_uri_handlers = 1U; config.backlog_conn = 1U;
    config.lru_purge_enable = false; // A scanner cannot evict an authenticated caller.
    config.stack_size = 6144U;
    config.recv_wait_timeout = 1U; config.send_wait_timeout = 1U;
    config.global_user_ctx = this; config.global_user_ctx_free_fn = keepContext;
    config.open_fn = opened; config.close_fn = closed;
    if (httpd_start(&server_, &config) != ESP_OK) {
        server_ = nullptr; stop(); return Result::failure(ErrorCode::InternalError);
    }
    httpd_uri_t uri = {};
    uri.uri = "/bbp2"; uri.method = HTTP_GET; uri.handler = request;
    uri.is_websocket = true; uri.handle_ws_control_frames = true;
    if (httpd_register_uri_handler(server_, &uri) != ESP_OK) {
        stop(); return Result::failure(ErrorCode::InternalError);
    }
    // SDK mDNS registration is a bounded blocking initialization operation,
    // never repeated in the data-path poll. SDK manages address up/down events.
    const Result discovery = discovery_.start(config_.port, config_.encryptedMessages);
    if (!discovery) { stop(); return discovery; }
    return Result::success();
}

void Esp32LocalWebSocket::stop() {
    close();
    if (wifiEvent_ != nullptr) esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEvent_);
    if (ipEvent_ != nullptr) esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ipEvent_);
    wifiEvent_ = nullptr; ipEvent_ = nullptr;
    discovery_.stop();
    if (server_ != nullptr) { httpd_stop(server_); server_ = nullptr; }
}

bool Esp32LocalWebSocket::currentLocked(int fd) const { return active_ && fd_ == fd; }
bool Esp32LocalWebSocket::deadlineLocked(uint32_t now) const {
    return (!websocket_ && static_cast<uint32_t>(now - openedAt_) >= kUpgradeDeadline) ||
        (reading_ && static_cast<uint32_t>(now - readAt_) >= kIoDeadline);
}

void Esp32LocalWebSocket::invalidateLocked() {
    active_ = false; session_ = 0U; ready_ = false; rxSize_ = 0U;
    txSize_ = 0U; txOffset_ = 0U; controlSize_ = 0U; closeAfterWrite_ = false;
    secureZero(MutableByteSpan(rx_, sizeof(rx_)));
    // borrowed_ belongs only to the product loop. SDK/network callbacks must
    // not mutate it while the caller is decoding/decrypting its borrowed view.
    secureZero(MutableByteSpan(tx_, sizeof(tx_)));
    secureZero(MutableByteSpan(control_, sizeof(control_)));
    if (fd_ >= 0) ::shutdown(fd_, SHUT_RDWR); // Do not close/reuse SDK-owned fd.
}

int Esp32LocalWebSocket::requestUrl(http_parser* parser, const char* data, size_t size) {
    auto& self = *static_cast<Esp32LocalWebSocket*>(parser->data);
    const char path[] = "/bbp2";
    if (size > sizeof(path) - 1U - self.urlBytes_ || memcmp(data, path + self.urlBytes_, size) != 0) return -1;
    self.urlBytes_ = static_cast<uint8_t>(self.urlBytes_ + size);
    return 0;
}

int Esp32LocalWebSocket::requestHeaders(http_parser* parser) {
    auto& self = *static_cast<Esp32LocalWebSocket*>(parser->data);
    // Reuse the SDK parser without retaining a second header buffer. Newer
    // HTTPD no longer calls the GET handler after upgrading; validate before
    // handing bytes to HTTPD, independently of its optional callback settings.
    if (self.urlBytes_ != 5U || parser->method != HTTP_GET || !parser->upgrade ||
        (parser->flags & F_CHUNKED) != 0U ||
        ((parser->flags & F_CONTENTLENGTH) != 0U && parser->content_length != 0U)) return -1;
    self.headersReady_ = true;
    return 0;
}

void Esp32LocalWebSocket::acceptUpgradeLocked() {
    // Public SDK state, not a parsed 101 response, is the upgrade boundary.
    // Called from both loop and SDK receive paths, so immediate peer input
    // cannot overtake the bounded frame guard. Never increments twice.
    if (active_ && !websocket_ && headersReady_ &&
        httpd_ws_get_fd_info(server_, fd_) == HTTPD_WS_CLIENT_WEBSOCKET) {
        websocket_ = true; session_ = ++counter_;
    }
}

void Esp32LocalWebSocket::close() {
    Lock lock(mutex_); invalidateLocked();
    secureZero(MutableByteSpan(borrowed_, sizeof(borrowed_)));
}
uint32_t Esp32LocalWebSocket::sessionId() const { Lock lock(mutex_); return session_; }
TransportState Esp32LocalWebSocket::state() const {
    return server_ == nullptr ? TransportState::Stopped : TransportState::Online;
}

void Esp32LocalWebSocket::networkEvent(void* context, esp_event_base_t base, int32_t event, void*) {
    if ((base == WIFI_EVENT && (event == WIFI_EVENT_STA_DISCONNECTED || event == WIFI_EVENT_STA_STOP)) ||
        (base == IP_EVENT && (event == IP_EVENT_STA_GOT_IP || event == IP_EVENT_STA_LOST_IP)))
    {
        Esp32LocalWebSocket& self = *static_cast<Esp32LocalWebSocket*>(context);
        Lock lock(self.mutex_); self.invalidateLocked();
    }
}

esp_err_t Esp32LocalWebSocket::opened(httpd_handle_t server, int fd) {
    Esp32LocalWebSocket& self = owner(server);
    // HTTPD listens on all interfaces. Refuse AP/other interfaces before HTTP.
    // With CONFIG_LWIP_IPV6 the SDK uses a dual-stack listener: an IPv4
    // connection can have an IPv4-mapped IPv6 local socket address. Normalize
    // that representation only; native IPv6 still has no admitted LAN route.
    sockaddr_storage local = {}; socklen_t size = sizeof(local);
    esp_netif_ip_info_t ip = {};
    esp_netif_t* station = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (station == nullptr || esp_netif_get_ip_info(station, &ip) != ESP_OK || ip.ip.addr == 0U ||
        getsockname(fd, reinterpret_cast<sockaddr*>(&local), &size) != 0) return ESP_FAIL;
    uint32_t address = 0U;
    if (local.ss_family == AF_INET && size >= sizeof(sockaddr_in)) {
        address = reinterpret_cast<const sockaddr_in*>(&local)->sin_addr.s_addr;
    }
#if CONFIG_LWIP_IPV6
    else if (local.ss_family == AF_INET6 && size >= sizeof(sockaddr_in6)) {
        const in6_addr& v6 = reinterpret_cast<const sockaddr_in6*>(&local)->sin6_addr;
        if (!IN6_IS_ADDR_V4MAPPED(&v6)) return ESP_FAIL;
        memcpy(&address, reinterpret_cast<const uint8_t*>(&v6) + 12U, sizeof(address));
    }
#endif
    else return ESP_FAIL;
    if (address != ip.ip.addr) return ESP_FAIL;
    {
        Lock lock(self.mutex_);
        if (self.fd_ >= 0 || self.counter_ == UINT32_MAX) return ESP_FAIL;
        self.fd_ = fd; self.active_ = true; self.websocket_ = false; self.reading_ = false;
        self.httpBytes_ = 0U; self.urlBytes_ = 0U; self.headersReady_ = false;
        http_parser_init(&self.http_, HTTP_REQUEST); self.http_.data = &self;
        self.guard_.reset(); self.openedAt_ = millis();
    }
    if (httpd_sess_set_recv_override(server, fd, readSocket) != ESP_OK ||
        httpd_sess_set_send_override(server, fd, writeUpgrade) != ESP_OK) return ESP_FAIL;
    return ESP_OK;
}

void Esp32LocalWebSocket::closed(httpd_handle_t server, int fd) {
    Esp32LocalWebSocket& self = owner(server);
    Lock lock(self.mutex_);
    if (self.fd_ == fd) { self.invalidateLocked(); self.fd_ = -1; }
    ::close(fd);
}

int Esp32LocalWebSocket::readSocket(httpd_handle_t server, int fd, char* buffer, size_t size, int) {
    Esp32LocalWebSocket& self = owner(server);
    for (;;) {
        {
            Lock lock(self.mutex_);
            const uint32_t now = millis();
            if (!self.currentLocked(fd) || self.deadlineLocked(now)) return HTTPD_SOCK_ERR_FAIL;
            self.acceptUpgradeLocked();
            const int count = ::recv(fd, buffer, size, MSG_DONTWAIT);
            if (count > 0) {
                if (self.websocket_) {
                    if (!self.reading_) { self.reading_ = true; self.readAt_ = now; }
                    if (!self.guard_.consume(ByteView(reinterpret_cast<uint8_t*>(buffer), static_cast<size_t>(count))))
                        return HTTPD_SOCK_ERR_FAIL;
                    // Deadline is cleared by the URI handler, not by partial input.
                } else {
                    // Bound the complete request, and reject early pipelined
                    // WS bytes which would otherwise bypass the decoder tap
                    // through HTTPD's private HTTP read-ahead buffer.
                    if (static_cast<size_t>(count) > 1024U - self.httpBytes_ || self.headersReady_)
                        return HTTPD_SOCK_ERR_FAIL;
                    self.httpBytes_ = static_cast<uint16_t>(self.httpBytes_ + count);
                    static const http_parser_settings settings = {
                        nullptr, requestUrl, nullptr, nullptr, nullptr, requestHeaders,
                        nullptr, nullptr, nullptr, nullptr };
                    if (http_parser_execute(&self.http_, &settings, buffer, static_cast<size_t>(count)) !=
                        static_cast<size_t>(count) || HTTP_PARSER_ERRNO(&self.http_) != HPE_OK)
                        return HTTPD_SOCK_ERR_FAIL;
                }
                return count;
            }
            if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) return HTTPD_SOCK_ERR_FAIL;
            if (self.websocket_ && !self.reading_) { self.reading_ = true; self.readAt_ = now; }
        }
        vTaskDelay(1U); // SDK task only; absolute deadline is never extended.
    }
}

int Esp32LocalWebSocket::writeUpgrade(httpd_handle_t server, int fd, const char* buffer, size_t size, int) {
    Esp32LocalWebSocket& self = owner(server);
    size_t offset = 0U;
    while (offset < size) {
        {
            Lock lock(self.mutex_);
            // SDK may write only HTTP upgrade/errors, never WS business data.
            if (!self.currentLocked(fd) || self.websocket_ || self.deadlineLocked(millis())) return HTTPD_SOCK_ERR_FAIL;
            const int count = ::send(fd, buffer + offset, size - offset, MSG_DONTWAIT);
            if (count > 0) offset += static_cast<size_t>(count);
            else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) return HTTPD_SOCK_ERR_FAIL;
        }
        if (offset < size) vTaskDelay(1U);
    }
    return static_cast<int>(offset);
}

esp_err_t Esp32LocalWebSocket::request(httpd_req_t* request) {
    Esp32LocalWebSocket& self = owner(request->handle);
    const int fd = httpd_req_to_sockfd(request);
    if (request->method == HTTP_GET) {
        Lock lock(self.mutex_);
        if (!self.currentLocked(fd) || self.deadlineLocked(millis()) || request->content_len != 0U ||
            strcmp(request->uri, "/bbp2") != 0 ||
            httpd_ws_get_fd_info(request->handle, fd) != HTTPD_WS_CLIENT_WEBSOCKET) return ESP_FAIL;
        self.acceptUpgradeLocked();
        return ESP_OK;
    }
    uint8_t payload[kMaximum]; // SDK-task scratch; never handed to Client.
    httpd_ws_frame_t incoming = {};
    incoming.payload = payload;
    if (httpd_ws_recv_frame(request, &incoming, sizeof(payload)) != ESP_OK) return ESP_FAIL;
    if (incoming.type == HTTPD_WS_TYPE_CLOSE && !validWebSocketClose(ByteView(payload, incoming.len))) return ESP_FAIL;
    const uint32_t waitingAt = millis();
    for (;;) {
        {
            Lock lock(self.mutex_);
            if (!self.currentLocked(fd) || self.deadlineLocked(millis())) return ESP_FAIL;
            if (incoming.type >= HTTPD_WS_TYPE_CLOSE) {
                if (self.guard_.messageBoundary()) self.reading_ = false;
                if (incoming.type == HTTPD_WS_TYPE_PONG) return ESP_OK;
                if (self.controlSize_ != 0U) return ESP_FAIL; // Bounded control queue.
                const bool closing = incoming.type == HTTPD_WS_TYPE_CLOSE;
                self.controlSize_ = static_cast<uint16_t>(frame(self.control_, closing ? 8U : 10U,
                    closing ? ByteView() : ByteView(payload, incoming.len)));
                self.closeAfterWrite_ = closing;
                if (self.txSize_ == 0U) self.txAt_ = millis();
                return ESP_OK;
            }
            if (!self.ready_) {
                if (incoming.len > kMaximum - self.rxSize_) return ESP_FAIL;
                memcpy(self.rx_ + self.rxSize_, payload, incoming.len);
                self.rxSize_ = static_cast<uint16_t>(self.rxSize_ + incoming.len);
                self.ready_ = incoming.final;
                // All fragments of a message share one absolute 3 s deadline.
                if (incoming.final) self.reading_ = false;
                return ESP_OK;
            }
            if (static_cast<uint32_t>(millis() - waitingAt) >= kIoDeadline) return ESP_FAIL;
        }
        vTaskDelay(1U); // Fixed one-message backpressure; no unbounded queue.
    }
}

size_t Esp32LocalWebSocket::frame(uint8_t* output, uint8_t opcode, ByteView body) {
    output[0] = static_cast<uint8_t>(128U | opcode);
    size_t header = 2U;
    if (body.size <= 125U) output[1] = static_cast<uint8_t>(body.size);
    else {
        output[1] = 126U; output[2] = static_cast<uint8_t>(body.size >> 8U);
        output[3] = static_cast<uint8_t>(body.size); header = 4U;
    }
    if (!body.empty()) memcpy(output + header, body.data, body.size);
    return header + body.size;
}

Result Esp32LocalWebSocket::send(ByteView message) {
    Lock lock(mutex_);
    if (session_ == 0U || closeAfterWrite_) return Result::failure(ErrorCode::NotConnected);
    if (message.data == nullptr || message.empty() || message.size > kMaximum) return Result::failure(ErrorCode::InvalidArgument);
    if (txSize_ != 0U || controlSize_ != 0U) return Result::failure(ErrorCode::WouldBlock);
    txSize_ = static_cast<uint16_t>(frame(tx_, 2U, message)); txOffset_ = 0U; txAt_ = millis();
    return Result::success();
}

Result Esp32LocalWebSocket::receive(ByteView& message) {
    Lock lock(mutex_);
    message = ByteView(); secureZero(MutableByteSpan(borrowed_, sizeof(borrowed_)));
    if (session_ == 0U) return Result::failure(ErrorCode::NotConnected);
    if (!ready_) return Result::failure(ErrorCode::WouldBlock);
    memcpy(borrowed_, rx_, rxSize_); message = ByteView(borrowed_, rxSize_);
    secureZero(MutableByteSpan(rx_, sizeof(rx_))); rxSize_ = 0U; ready_ = false;
    return Result::success();
}

void Esp32LocalWebSocket::poll(uint32_t) {
    if (server_ == nullptr) return;
    const bool online = WiFi.status() == WL_CONNECTED && static_cast<uint32_t>(WiFi.localIP()) != 0U;
    if (!online) { close(); return; }
    Lock lock(mutex_);
    secureZero(MutableByteSpan(borrowed_, sizeof(borrowed_)));
    if (!active_) return;
    const uint32_t now = millis();
    if (deadlineLocked(now) || ((txSize_ != 0U || controlSize_ != 0U) &&
        static_cast<uint32_t>(now - txAt_) >= kIoDeadline)) { invalidateLocked(); return; }
    acceptUpgradeLocked();
    if (txSize_ == 0U && controlSize_ != 0U) {
        memcpy(tx_, control_, controlSize_); txSize_ = controlSize_; txOffset_ = 0U;
        secureZero(MutableByteSpan(control_, sizeof(control_))); controlSize_ = 0U;
    }
    if (txSize_ == 0U) return;
    // One nonblocking syscall per poll, preserving exact bytes after a short write.
    const int count = ::send(fd_, tx_ + txOffset_, txSize_ - txOffset_, MSG_DONTWAIT);
    if (count > 0) {
        txOffset_ = static_cast<uint16_t>(txOffset_ + count);
        if (txOffset_ == txSize_) {
            secureZero(MutableByteSpan(tx_, sizeof(tx_))); txSize_ = 0U; txOffset_ = 0U;
            if (closeAfterWrite_ && controlSize_ == 0U) invalidateLocked();
        }
    } else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) invalidateLocked();
}

} // namespace blinker
#endif
