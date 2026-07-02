#ifndef BLINKER_WIFI_UNO_R4_H
#define BLINKER_WIFI_UNO_R4_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_PLATFORM_UNO_R4_WIFI)

#include <Arduino.h>
#include <Client.h>
#include <EEPROM.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>

typedef WiFiSSLClient BlinkerWiFiSecureClient;

static inline const char* blinkerWiFiPlatformName()
{
    return BLINKER_WIFI_PLATFORM_NAME;
}

static inline void blinkerWiFiModeSTA()
{
}

static inline void blinkerWiFiModeAP()
{
}

static inline void blinkerWiFiSetHostname(const char* hostname)
{
    (void)hostname;
}

static inline void blinkerWiFiSecureSetInsecure(BlinkerWiFiSecureClient& client)
{
    (void)client;
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

static inline void blinkerEEPROMBegin(size_t size)
{
    (void)size;
    EEPROM.begin();
}

static inline void blinkerEEPROMCommit()
{
}

static inline void blinkerEEPROMEnd()
{
}

#endif

#endif
