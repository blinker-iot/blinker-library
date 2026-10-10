#ifndef BLINKER_TEST_ESP_WIFI_H
#define BLINKER_TEST_ESP_WIFI_H

#include <WiFi.h>

inline int esp_wifi_disconnect() {
    ++WiFi.disconnectCalls;
    return 0;
}

#endif
