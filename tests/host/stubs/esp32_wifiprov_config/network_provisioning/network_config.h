#pragma once

#include <stddef.h>
#include <stdint.h>

#define CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI 1
#define CONFIG_NETWORK_PROV_NETWORK_TYPE_THREAD 0

using esp_err_t = int;
#ifdef _WIN32
using ssize_t = ptrdiff_t;
#endif
constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_ERR_INVALID_ARG = 0x102;
constexpr esp_err_t ESP_ERR_INVALID_STATE = 0x103;

struct network_prov_ctx { unsigned sets, applies, statuses; };
using network_prov_ctx_t = network_prov_ctx;
struct network_prov_config_set_wifi_data_t {};
struct network_prov_config_get_wifi_data_t {};
struct network_prov_config_handlers_t {
    esp_err_t (*wifi_get_status_handler)(network_prov_config_get_wifi_data_t*, network_prov_ctx_t**);
    esp_err_t (*wifi_set_config_handler)(const network_prov_config_set_wifi_data_t*, network_prov_ctx_t**);
    esp_err_t (*wifi_apply_config_handler)(network_prov_ctx_t**);
    network_prov_ctx_t* ctx;
};

esp_err_t network_prov_config_data_handler(uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*, void*);
