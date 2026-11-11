#ifndef BLINKER_ARDUINO_TASK_CONFIG_H
#define BLINKER_ARDUINO_TASK_CONFIG_H

// Maintainer admission, NOT per-user task definitions or credentials. A tested
// candidate supplies capacity and all four storage policy values library-wide.
// Stock source is closed; enabling a Sketch cannot invent production defaults.
#ifndef BLINKER_TASK_CAPACITY
#define BLINKER_TASK_CAPACITY 0
#endif

#if defined(BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS) && \
    (BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS != 1 || \
     !defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) || BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS != 1)
#error "BLE provisioned tasks require both explicit internal profile selectors"
#endif

#if defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) && BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS && \
    (!defined(BLINKER_ENABLE_TASKS) || !BLINKER_ENABLE_TASKS)
#error "Provisioned task profile requires explicit task enablement"
#endif

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if defined(BLINKER_PRODUCT_WIFI_BLE) && defined(BLINKER_ESP32_PROVISIONING_BLE) && \
    (!defined(BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS) || BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS != 1)
#error "BLE WiFiBLE tasks require the explicit internal BLE task profile"
#endif
#if defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) && BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS
#if defined(BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS)
#if !defined(ARDUINO_ARCH_ESP32) || !defined(BLINKER_PRODUCT_WIFI_BLE) || \
    !defined(BLINKER_ESP32_PROVISIONING_BLE) || BLINKER_ESP32_PROVISIONING_BLE != 1 || \
    defined(BLINKER_ESP32_PROVISIONING_SOFTAP) || BLINKER_TASK_CAPACITY != 4
#error "BLE provisioned task profile requires ESP32, BLE WiFiBLE and exactly four slots"
#endif
#else
#if !defined(ARDUINO_ARCH_ESP32) || !defined(BLINKER_PRODUCT_WIFI_BLE) || \
    !defined(BLINKER_ESP32_PROVISIONING_SOFTAP) || defined(BLINKER_ESP32_PROVISIONING_BLE) || \
    BLINKER_TASK_CAPACITY != 4
#error "Provisioned task profile requires ESP32, SoftAP WiFiBLE and exactly four slots"
#endif
#endif
#if !defined(BLINKER_OFFICIAL_CLOUD_SESSION_TLS) || !BLINKER_OFFICIAL_CLOUD_SESSION_TLS || \
    !defined(BLINKER_OFFICIAL_MQTT_TLS) || !BLINKER_OFFICIAL_MQTT_TLS
#error "Provisioned task profile requires HTTPS and MQTTS"
#endif
#if (defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \
    (defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET)
#error "Provisioned tasks do not admit a combined LAN/control profile"
#endif
#endif
#if BLINKER_TASK_CAPACITY < 1 || BLINKER_TASK_CAPACITY > 10
#error "Tasks require a maintainer-admitted BLINKER_TASK_CAPACITY (1..10)"
#endif
#if !defined(BLINKER_TASK_WINDOW_MS) || !defined(BLINKER_TASK_WRITES) || \
    !defined(BLINKER_TASK_MIN_START_MS) || !defined(BLINKER_TASK_MAX_COMMIT_MS)
#error "Tasks require an explicit maintainer storage policy; no production defaults"
#endif
#if BLINKER_TASK_WRITES < 2 || BLINKER_TASK_WRITES > 65535 || \
    BLINKER_TASK_WINDOW_MS < 1 || BLINKER_TASK_WINDOW_MS > 0x7FFFFFFF || \
    BLINKER_TASK_MIN_START_MS < 1 || BLINKER_TASK_MIN_START_MS > 0x7FFFFFFF || \
    BLINKER_TASK_MAX_COMMIT_MS < 1 || BLINKER_TASK_MAX_COMMIT_MS > 0x7FFFFFFF
#error "Task storage policy values are out of range"
#endif
#endif

#endif
