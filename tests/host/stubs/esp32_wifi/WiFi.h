#ifndef BLINKER_TEST_ESP32_WIFI_H
#define BLINKER_TEST_ESP32_WIFI_H

#include <stdint.h>
#include <string.h>

typedef enum {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL,
    WL_SCAN_COMPLETED,
    WL_CONNECTED,
    WL_CONNECT_FAILED,
    WL_CONNECTION_LOST,
    WL_DISCONNECTED
} wl_status_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA3_PSK
} wifi_auth_mode_t;

class WiFiClass {
public:
    WiFiClass()
        : beginResult(WL_IDLE_STATUS),
          statusResult(WL_IDLE_STATUS),
          disconnectResult(true),
          beginCalls(0),
          persistentCalls(0),
          autoReconnectCalls(0),
          minSecurityCalls(0),
          disconnectCalls(0),
          lastMinSecurity(WIFI_AUTH_OPEN),
          lastPersistent(true),
          lastAutoReconnect(true),
          passwordWasNull(false),
          lastSsid(),
          lastPassword() {}

    wl_status_t begin(const char* ssid, const char* password = nullptr) {
        ++beginCalls;
        passwordWasNull = password == nullptr;
        copy(lastSsid, sizeof(lastSsid), ssid);
        copy(lastPassword, sizeof(lastPassword), password);
        return beginResult;
    }

    wl_status_t status() { return statusResult; }

    void persistent(bool value) {
        ++persistentCalls;
        lastPersistent = value;
    }

    void setAutoReconnect(bool value) {
        ++autoReconnectCalls;
        lastAutoReconnect = value;
    }

    void setMinSecurity(wifi_auth_mode_t mode) {
        ++minSecurityCalls;
        lastMinSecurity = mode;
    }

    bool disconnectAsync(bool, bool) {
        ++disconnectCalls;
        return disconnectResult;
    }

    void reset() { *this = WiFiClass(); }

    wl_status_t beginResult;
    wl_status_t statusResult;
    bool disconnectResult;
    unsigned int beginCalls;
    unsigned int persistentCalls;
    unsigned int autoReconnectCalls;
    unsigned int minSecurityCalls;
    unsigned int disconnectCalls;
    wifi_auth_mode_t lastMinSecurity;
    bool lastPersistent;
    bool lastAutoReconnect;
    bool passwordWasNull;
    char lastSsid[33];
    char lastPassword[65];

private:
    static void copy(char* output, size_t capacity, const char* input) {
        output[0] = '\0';
        if (input == nullptr) return;
        const size_t length = strlen(input);
        const size_t copied = length < capacity - 1U
                                  ? length
                                  : capacity - 1U;
        memcpy(output, input, copied);
        output[copied] = '\0';
    }
};

extern WiFiClass WiFi;

#endif
