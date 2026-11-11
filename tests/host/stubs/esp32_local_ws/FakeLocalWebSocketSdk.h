#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <cerrno>

// Deterministic adapter harness, NOT an HTTP/WS engine or radio emulator.
typedef int esp_err_t;
// Parser callback contract only, not an HTTP parser implementation. Scripted
// parsed fields below let the adapter tests exercise admission independently
// of the SDK version. Real wire syntax is covered by the hardware socket gate.
enum { HTTP_REQUEST, HPE_OK = 0, F_CHUNKED = 1, F_CONTENTLENGTH = 128 };
struct http_parser {
    void* data = nullptr;
    unsigned method = 1, flags = 0, upgrade = 1, http_errno = 0;
    uint64_t content_length = 0;
    std::string input;
};
typedef int (*http_cb)(http_parser*);
typedef int (*http_data_cb)(http_parser*, const char*, size_t);
struct http_parser_settings {
    http_cb on_message_begin; http_data_cb on_url, on_status, on_header_field, on_header_value;
    http_cb on_headers_complete, on_body, on_message_complete, on_chunk_header, on_chunk_complete;
};
#define HTTP_PARSER_ERRNO(p) ((p)->http_errno)
inline void http_parser_init(http_parser* parser, int) { *parser = http_parser(); }
enum { ESP_OK = 0, ESP_FAIL = -1, HTTPD_SOCK_ERR_FAIL = -2, ESP_ERR_INVALID_STATE = -3, ESP_EVENT_ANY_ID = -1,
    HTTP_GET = 1, HTTPD_WS_TYPE_CLOSE = 8, HTTPD_WS_TYPE_PONG = 10,
    HTTPD_WS_CLIENT_WEBSOCKET = 2, AF_INET = 2, AF_INET6 = 10, SHUT_RDWR = 2, MSG_DONTWAIT = 64,
    WL_CONNECTED = 3, WIFI_EVENT_STA_DISCONNECTED = 1, WIFI_EVENT_STA_STOP = 2,
    IP_EVENT_STA_GOT_IP = 3, IP_EVENT_STA_LOST_IP = 4 };
typedef int esp_event_base_t;
const esp_event_base_t WIFI_EVENT = 1, IP_EVENT = 2;
typedef void* esp_event_handler_instance_t;
typedef void* httpd_handle_t;
typedef unsigned socklen_t;
typedef int esp_netif_t;
struct sockaddr {};
struct sockaddr_in { int sin_family; struct { uint32_t s_addr; } sin_addr; };
struct in6_addr { uint8_t bytes[16]; };
struct sockaddr_in6 { int sin6_family; in6_addr sin6_addr; };
struct sockaddr_storage { int ss_family; uint8_t bytes[124]; };
inline bool IN6_IS_ADDR_V4MAPPED(const in6_addr* value) {
    for (unsigned i = 0; i < 10; ++i) if (value->bytes[i] != 0U) return false;
    return value->bytes[10] == 255U && value->bytes[11] == 255U;
}
struct esp_netif_ip_info_t { struct { uint32_t addr; } ip; };
typedef std::mutex StaticSemaphore_t;
typedef std::mutex* SemaphoreHandle_t;
const uint32_t portMAX_DELAY = UINT32_MAX;
inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* value) { return value; }
inline void xSemaphoreTake(SemaphoreHandle_t value, uint32_t) { value->lock(); }
inline void xSemaphoreGive(SemaphoreHandle_t value) { value->unlock(); }
inline void vSemaphoreDelete(SemaphoreHandle_t) {}
struct httpd_req_t { httpd_handle_t handle; int method; size_t content_len; const char* uri; };
struct httpd_ws_frame_t { uint8_t type; bool final; uint8_t* payload; size_t len; };
struct httpd_config_t {
    uint16_t server_port = 80, ctrl_port = 32768, max_open_sockets = 7, max_uri_handlers = 8, backlog_conn = 5;
    bool lru_purge_enable = true;
    size_t stack_size = 4096;
    uint16_t recv_wait_timeout = 5, send_wait_timeout = 5;
    void* global_user_ctx = nullptr;
    void (*global_user_ctx_free_fn)(void*) = nullptr;
    esp_err_t (*open_fn)(httpd_handle_t, int) = nullptr;
    void (*close_fn)(httpd_handle_t, int) = nullptr;
};
#define HTTPD_DEFAULT_CONFIG() httpd_config_t()
struct httpd_uri_t {
    const char* uri; int method; esp_err_t (*handler)(httpd_req_t*);
    bool is_websocket, handle_ws_control_frames;
};
typedef int (*httpd_recv_func_t)(httpd_handle_t, int, char*, size_t, int);
typedef int (*httpd_send_func_t)(httpd_handle_t, int, const char*, size_t, int);
struct mdns_txt_item_t { const char* key; const char* value; };
enum { MDNS_NAME_BUF_LEN = 64 };
namespace fake_local_ws {
struct Event { esp_event_base_t base; void (*callback)(void*, esp_event_base_t, int32_t, void*); void* context; };
struct State {
    uint32_t now = 1U, ip = 123U, localIp = 123U;
    int localFamily = AF_INET6; // Real SDK dual-stack listener, not IPv4-only.
    bool mappedIpv4 = true, shortAddress = false;
    httpd_config_t config;
    httpd_uri_t uri = {};
    httpd_recv_func_t receive = nullptr;
    httpd_send_func_t transmit = nullptr;
    Event events[2] = {};
    int eventCount = 0, unregisters = 0, stops = 0, shutdowns = 0, closes = 0;
    bool failRegisterUri = false, failAdvertise = false, connected = true, upgraded = false;
    unsigned parsedMethod = HTTP_GET, parsedFlags = 0, parsedUpgrade = 1;
    uint64_t parsedContentLength = 0;
    std::string parsedUrl = "/bbp2";
    bool blocked = false;
    size_t writeLimit = 424U, readLimit = 1024U;
    std::string hostname = "user-owned-host", service, securityHint;
    int hostnameChanges = 0, serviceAdds = 0, serviceRemoves = 0;
    std::vector<uint8_t> wireInput, wireOutput, payload;
    uint8_t type = 2U; bool final = true;
    std::function<void()> delayed;
};
extern State state;
inline httpd_handle_t handle() { return &state; }
}
inline uint32_t millis() { return fake_local_ws::state.now; }
inline size_t http_parser_execute(http_parser* parser, const http_parser_settings* settings,
                                  const char* bytes, size_t size) {
    const auto& sdk = fake_local_ws::state;
    for (size_t index = 0; index < size; ++index) {
        parser->input.push_back(bytes[index]);
        if (parser->input.size() < 4U || parser->input.compare(parser->input.size() - 4U, 4U, "\r\n\r\n") != 0) continue;
        parser->method = sdk.parsedMethod; parser->flags = sdk.parsedFlags;
        parser->content_length = sdk.parsedContentLength; parser->upgrade = sdk.parsedUpgrade;
        for (const char value : sdk.parsedUrl) {
            if (settings->on_url(parser, &value, 1U) != 0) { parser->http_errno = 1; return index; }
        }
        if (settings->on_headers_complete(parser) != 0) { parser->http_errno = 1; return index; }
        return index + 1U;
    }
    return size;
}
inline void vTaskDelay(uint32_t value) {
    fake_local_ws::state.now += value;
    const auto callback = fake_local_ws::state.delayed;
    if (callback) callback();
}
struct FakeWiFi {
    int status() const { return fake_local_ws::state.connected ? WL_CONNECTED : 0; }
    uint32_t localIP() const { return fake_local_ws::state.ip; }
};
extern FakeWiFi WiFi;
inline esp_netif_t* esp_netif_get_handle_from_ifkey(const char*) { static int netif; return &netif; }
inline esp_err_t esp_netif_get_ip_info(esp_netif_t*, esp_netif_ip_info_t* ip) { ip->ip.addr = fake_local_ws::state.ip; return ESP_OK; }
inline int getsockname(int, sockaddr* address, socklen_t* size) {
    const auto& state = fake_local_ws::state;
    const socklen_t required = state.localFamily == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    if (*size < required) return -1;
    *size = state.shortAddress ? required - 1U : required;
    if (state.localFamily == AF_INET) {
        auto* local = reinterpret_cast<sockaddr_in*>(address);
        local->sin_family = AF_INET; local->sin_addr.s_addr = state.localIp;
    } else {
        auto* local = reinterpret_cast<sockaddr_in6*>(address);
        local->sin6_family = state.localFamily;
        if (state.mappedIpv4) local->sin6_addr.bytes[10] = local->sin6_addr.bytes[11] = 255U;
        std::memcpy(local->sin6_addr.bytes + 12U, &state.localIp, sizeof(state.localIp));
    }
    return 0;
}
inline int shutdown(int, int) { ++fake_local_ws::state.shutdowns; return 0; }
inline int close(int) { ++fake_local_ws::state.closes; return 0; }
inline int send(int, const void* buffer, size_t size, int) {
    auto& s = fake_local_ws::state;
    if (s.blocked) { errno = EAGAIN; return -1; }
    size = (std::min)(size, s.writeLimit);
    const uint8_t* bytes = static_cast<const uint8_t*>(buffer);
    s.wireOutput.insert(s.wireOutput.end(), bytes, bytes + size);
    return static_cast<int>(size);
}
inline int recv(int, void* buffer, size_t size, int) {
    auto& s = fake_local_ws::state;
    if (s.wireInput.empty()) { errno = EAGAIN; return -1; }
    size = (std::min)((std::min)(size, s.wireInput.size()), s.readLimit);
    std::memcpy(buffer, s.wireInput.data(), size);
    s.wireInput.erase(s.wireInput.begin(), s.wireInput.begin() + static_cast<std::ptrdiff_t>(size));
    return static_cast<int>(size);
}
inline esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int,
    void (*callback)(void*, esp_event_base_t, int32_t, void*), void* context, esp_event_handler_instance_t* instance) {
    auto& s = fake_local_ws::state;
    ++s.eventCount; *instance = &s.events[base - 1];
    *static_cast<fake_local_ws::Event*>(*instance) = {base, callback, context}; return ESP_OK;
}
inline esp_err_t esp_event_handler_instance_unregister(esp_event_base_t, int, esp_event_handler_instance_t) {
    ++fake_local_ws::state.unregisters; return ESP_OK;
}
inline esp_err_t httpd_start(httpd_handle_t* handle, const httpd_config_t* config) {
    fake_local_ws::state.config = *config; *handle = fake_local_ws::handle(); return ESP_OK;
}
inline esp_err_t httpd_stop(httpd_handle_t) { ++fake_local_ws::state.stops; return ESP_OK; }
inline void* httpd_get_global_user_ctx(httpd_handle_t) { return fake_local_ws::state.config.global_user_ctx; }
inline esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t* uri) {
    fake_local_ws::state.uri = *uri; return fake_local_ws::state.failRegisterUri ? ESP_FAIL : ESP_OK;
}
inline esp_err_t httpd_sess_set_recv_override(httpd_handle_t, int, httpd_recv_func_t function) {
    fake_local_ws::state.receive = function; return ESP_OK;
}
inline esp_err_t httpd_sess_set_send_override(httpd_handle_t, int, httpd_send_func_t function) {
    fake_local_ws::state.transmit = function; return ESP_OK;
}
inline int httpd_req_to_sockfd(httpd_req_t*) { return 7; }
inline int httpd_ws_get_fd_info(httpd_handle_t, int) { return fake_local_ws::state.upgraded ? HTTPD_WS_CLIENT_WEBSOCKET : 0; }
inline esp_err_t httpd_ws_recv_frame(httpd_req_t*, httpd_ws_frame_t* frame, size_t maximum) {
    auto& s = fake_local_ws::state;
    if (s.payload.size() > maximum) return ESP_FAIL;
    frame->type = s.type; frame->final = s.final; frame->len = s.payload.size();
    if (!s.payload.empty()) std::memcpy(frame->payload, s.payload.data(), frame->len);
    return ESP_OK;
}
inline esp_err_t mdns_init() { return ESP_OK; }
inline esp_err_t mdns_hostname_get(char* hostname) {
    const auto& value = fake_local_ws::state.hostname;
    if (value.empty()) return ESP_ERR_INVALID_STATE;
    if (value.size() >= MDNS_NAME_BUF_LEN) return ESP_FAIL;
    std::memcpy(hostname, value.c_str(), value.size() + 1U); return ESP_OK;
}
inline esp_err_t mdns_hostname_set(const char* hostname) {
    ++fake_local_ws::state.hostnameChanges; fake_local_ws::state.hostname = hostname; return ESP_OK;
}
inline esp_err_t mdns_service_add_for_host(const char* instance, const char*, const char*, const char*,
    uint16_t, mdns_txt_item_t* txt, size_t count) {
    auto& s = fake_local_ws::state;
    if (s.failAdvertise) return ESP_FAIL;
    for (size_t i = 0; i < count; ++i) if (std::string(txt[i].key) == "security") s.securityHint = txt[i].value;
    ++s.serviceAdds; s.service = instance; return ESP_OK;
}
inline esp_err_t mdns_service_remove_for_host(const char* instance, const char*, const char*, const char*) {
    auto& s = fake_local_ws::state;
    if (s.service != instance) return ESP_FAIL;
    ++s.serviceRemoves; s.service.clear(); return ESP_OK;
}
