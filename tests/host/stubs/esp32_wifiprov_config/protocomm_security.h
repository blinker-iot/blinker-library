#ifndef BLINKER_TEST_PROTOCOMM_SECURITY_H
#define BLINKER_TEST_PROTOCOMM_SECURITY_H

#include <network_provisioning/network_config.h>

using protocomm_security_handle_t = void*;
struct protocomm_security1_params_t { const uint8_t* data; uint16_t len; };

struct protocomm_security_t {
    int ver;
    uint8_t patch_ver;
    esp_err_t (*init)(protocomm_security_handle_t*);
    esp_err_t (*cleanup)(protocomm_security_handle_t);
    esp_err_t (*new_transport_session)(protocomm_security_handle_t, uint32_t);
    esp_err_t (*close_transport_session)(protocomm_security_handle_t, uint32_t);
    esp_err_t (*security_req_handler)(protocomm_security_handle_t, const void*, uint32_t,
        const uint8_t*, ssize_t, uint8_t**, ssize_t*, void*);
    esp_err_t (*encrypt)(protocomm_security_handle_t, uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*);
    esp_err_t (*decrypt)(protocomm_security_handle_t, uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*);
};

#endif
