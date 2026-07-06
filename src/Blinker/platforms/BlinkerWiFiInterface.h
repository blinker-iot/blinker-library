#ifndef BLINKER_WIFI_INTERFACE_H
#define BLINKER_WIFI_INTERFACE_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_NATIVE_WIFI)

#if defined(BLINKER_PLATFORM_ESP32)
    #include "BlinkerWiFiESP32.h"
#elif defined(BLINKER_PLATFORM_UNO_R4_WIFI)
    #include "BlinkerWiFiUnoR4.h"
#elif defined(BLINKER_PLATFORM_RP2040_WIFI)
    #include "BlinkerWiFiRP2040.h"
#else
    #error Unsupported native WiFi platform.
#endif

#include "BlinkerWebSocketsStub.h"

#ifndef HTTP_CODE_OK
    #define HTTP_CODE_OK 200
#endif

#ifndef HTTP_CODE_MOVED_PERMANENTLY
    #define HTTP_CODE_MOVED_PERMANENTLY 301
#endif

#include "BlinkerHTTPClientFallback.h"

#else

#if defined(ARDUINO)
    #include <Arduino.h>
#endif

static inline const char* blinkerWiFiPlatformName()
{
    return "No WiFi";
}

static inline bool blinkerWiFiConfigTime(long timezoneSeconds)
{
    (void)timezoneSeconds;
    return false;
}

static inline void blinkerWiFiRestart()
{
}

static inline void blinkerWiFiReconnect()
{
}

#endif

#endif
