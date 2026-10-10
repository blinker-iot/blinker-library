#pragma once

// Internal probe only. Link with --wrap for these five public ESP-IDF NVS
// functions. Observe the exact access namespace/key; never dump its content
// or alter a return value. Counts are API calls, not flash-page operations.
#if defined(BLINKER_TEST_NVS_WITNESS)
#include <nvs.h>
#include <string.h>

struct AccessWriteWitness {
    nvs_handle_t handle = 0;
    uint32_t opens = 0, attempts = 0, writes = 0, commits = 0, erases = 0;
    bool ambiguous = false;
};
AccessWriteWitness accessWrites;

extern "C" {
esp_err_t __real_nvs_open(const char*, nvs_open_mode_t, nvs_handle_t*);
void __real_nvs_close(nvs_handle_t);
esp_err_t __real_nvs_set_blob(nvs_handle_t, const char*, const void*, size_t);
esp_err_t __real_nvs_commit(nvs_handle_t);
esp_err_t __real_nvs_erase_key(nvs_handle_t, const char*);

esp_err_t __wrap_nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* handle) {
    const esp_err_t result = __real_nvs_open(name, mode, handle);
    if (result == ESP_OK && mode == NVS_READWRITE && strcmp(name, "bl_v2_access") == 0) {
        if (accessWrites.handle != 0) accessWrites.ambiguous = true;
        accessWrites.handle = *handle;
        ++accessWrites.opens;
    }
    return result;
}
void __wrap_nvs_close(nvs_handle_t handle) {
    if (handle == accessWrites.handle) accessWrites.handle = 0;
    __real_nvs_close(handle);
}
esp_err_t __wrap_nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t size) {
    const bool observed = handle != 0 && handle == accessWrites.handle && strcmp(key, "root") == 0;
    if (observed) ++accessWrites.attempts;
    const esp_err_t result = __real_nvs_set_blob(handle, key, value, size);
    if (observed && result == ESP_OK) ++accessWrites.writes;
    return result;
}
esp_err_t __wrap_nvs_commit(nvs_handle_t handle) {
    const esp_err_t result = __real_nvs_commit(handle);
    if (handle != 0 && handle == accessWrites.handle && result == ESP_OK) ++accessWrites.commits;
    return result;
}
esp_err_t __wrap_nvs_erase_key(nvs_handle_t handle, const char* key) {
    const esp_err_t result = __real_nvs_erase_key(handle, key);
    if (handle != 0 && handle == accessWrites.handle && strcmp(key, "root") == 0 && result == ESP_OK) {
        ++accessWrites.erases;
    }
    return result;
}
}

void printAccessWriteWitness() {
    Serial.printf("accessWrites observed=%u ambiguous=%u opens=%lu attempts=%lu writes=%lu commits=%lu erases=%lu\n",
                  accessWrites.handle != 0 ? 1U : 0U, accessWrites.ambiguous ? 1U : 0U,
                  static_cast<unsigned long>(accessWrites.opens), static_cast<unsigned long>(accessWrites.attempts),
                  static_cast<unsigned long>(accessWrites.writes), static_cast<unsigned long>(accessWrites.commits),
                  static_cast<unsigned long>(accessWrites.erases));
}
#endif
