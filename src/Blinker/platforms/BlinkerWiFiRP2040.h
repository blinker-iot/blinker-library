#ifndef BLINKER_WIFI_RP2040_H
#define BLINKER_WIFI_RP2040_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_PLATFORM_RP2040_WIFI)

#include <Arduino.h>
#include <Client.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

typedef WiFiClientSecure BlinkerWiFiSecureClient;

static inline const char* blinkerWiFiPlatformName()
{
    return BLINKER_WIFI_PLATFORM_NAME;
}

static inline void blinkerWiFiModeSTA()
{
    WiFi.mode(WIFI_STA);
}

static inline void blinkerWiFiModeAP()
{
    WiFi.mode(WIFI_AP);
}

static inline void blinkerWiFiSetHostname(const char* hostname)
{
    (void)hostname;
}

static inline void blinkerWiFiSecureSetInsecure(BlinkerWiFiSecureClient& client)
{
    client.setInsecure();
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
    EEPROM.begin(size);
}

static inline void blinkerEEPROMCommit()
{
    EEPROM.commit();
}

static inline void blinkerEEPROMEnd()
{
    EEPROM.end();
}

#endif

#endif
