#ifndef BLINKER_TEST_STUB_ESP_PARTITION_H
#define BLINKER_TEST_STUB_ESP_PARTITION_H

#include <stdbool.h>

typedef enum {
    ESP_PARTITION_TYPE_DATA = 1
} esp_partition_type_t;

typedef enum {
    ESP_PARTITION_SUBTYPE_DATA_NVS = 2
} esp_partition_subtype_t;

typedef struct esp_partition_t {
    bool encrypted;
} esp_partition_t;

#ifdef __cplusplus
extern "C" {
#endif

const esp_partition_t* esp_partition_find_first(
    esp_partition_type_t type,
    esp_partition_subtype_t subtype,
    const char* label);

#ifdef __cplusplus
}
#endif

#endif
