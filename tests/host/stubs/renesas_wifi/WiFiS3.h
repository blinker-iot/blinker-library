#ifndef BLINKER_TEST_RENESAS_WIFI_S3_H
#define BLINKER_TEST_RENESAS_WIFI_S3_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    WL_NO_SHIELD = 255,
    WL_NO_MODULE = WL_NO_SHIELD,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL,
    WL_SCAN_COMPLETED,
    WL_CONNECTED,
    WL_CONNECT_FAILED,
    WL_CONNECTION_LOST,
    WL_DISCONNECTED
} wl_status_t;

class CWifi {
public:
    CWifi()
        : beginResult(WL_CONNECT_FAILED),
          statusResult(WL_IDLE_STATUS),
          disconnectResult(1),
          timeoutValue(10000U),
          beginCalls(0),
          disconnectCalls(0),
          openBegin(false),
          lastSsid(),
          lastPassword() {}

    int begin(const char* ssid) {
        openBegin = true;
        return beginImpl(ssid, nullptr);
    }

    int begin(const char* ssid, const char* password) {
        openBegin = false;
        return beginImpl(ssid, password);
    }

    uint8_t status() { return static_cast<uint8_t>(statusResult); }
    void setTimeout(unsigned long timeout) { timeoutValue = timeout; }

    int disconnect() {
        ++disconnectCalls;
        return disconnectResult;
    }

    void reset() { *this = CWifi(); }

    int beginResult;
    wl_status_t statusResult;
    int disconnectResult;
    unsigned long timeoutValue;
    unsigned int beginCalls;
    unsigned int disconnectCalls;
    bool openBegin;
    char lastSsid[33];
    char lastPassword[65];

private:
    int beginImpl(const char* ssid, const char* password) {
        ++beginCalls;
        copy(lastSsid, sizeof(lastSsid), ssid);
        copy(lastPassword, sizeof(lastPassword), password);
        return beginResult;
    }

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

extern CWifi WiFi;

#endif
