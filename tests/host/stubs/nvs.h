#ifndef BLINKER_TEST_STUB_NVS_H
#define BLINKER_TEST_STUB_NVS_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t esp_err_t;
typedef uint32_t nvs_handle_t;

typedef enum {
    NVS_READONLY = 0,
    NVS_READWRITE = 1
} nvs_open_mode_t;

#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NVS_BASE 0x1100
#define ESP_ERR_NVS_NOT_INITIALIZED (ESP_ERR_NVS_BASE + 0x01)
#define ESP_ERR_NVS_NOT_FOUND (ESP_ERR_NVS_BASE + 0x02)
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE (ESP_ERR_NVS_BASE + 0x05)
#define ESP_ERR_NVS_INVALID_NAME (ESP_ERR_NVS_BASE + 0x06)
#define ESP_ERR_NVS_INVALID_HANDLE (ESP_ERR_NVS_BASE + 0x07)
#define ESP_ERR_NVS_KEY_TOO_LONG (ESP_ERR_NVS_BASE + 0x09)
#define ESP_ERR_NVS_INVALID_LENGTH (ESP_ERR_NVS_BASE + 0x0c)
#define ESP_ERR_NVS_NO_FREE_PAGES (ESP_ERR_NVS_BASE + 0x0d)
#define ESP_ERR_NVS_VALUE_TOO_LONG (ESP_ERR_NVS_BASE + 0x0e)
#define ESP_ERR_NVS_PART_NOT_FOUND (ESP_ERR_NVS_BASE + 0x0f)

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t nvs_open(
    const char* namespace_name,
    nvs_open_mode_t open_mode,
    nvs_handle_t* out_handle);
esp_err_t nvs_open_from_partition(
    const char* part_name,
    const char* namespace_name,
    nvs_open_mode_t open_mode,
    nvs_handle_t* out_handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_get_blob(
    nvs_handle_t handle,
    const char* key,
    void* out_value,
    size_t* length);
esp_err_t nvs_set_blob(
    nvs_handle_t handle,
    const char* key,
    const void* value,
    size_t length);
esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key);
esp_err_t nvs_commit(nvs_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
