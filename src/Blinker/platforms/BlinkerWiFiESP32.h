#ifndef BLINKER_WIFI_ESP32_H
#define BLINKER_WIFI_ESP32_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_PLATFORM_ESP32)

#include <Arduino.h>
#include <Client.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>

extern "C" {
    #include <esp_err.h>
    #include <esp_wifi.h>
    #include <esp_event.h>
}

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
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(hostname);
    WiFi.mode(WIFI_STA);
}

static inline void blinkerWiFiSecureSetInsecure(BlinkerWiFiSecureClient& client)
{
    client.setInsecure();
}

static inline bool blinkerWiFiConfigTime(long timezoneSeconds)
{
    configTime(timezoneSeconds, 0, "ntp1.aliyun.com", "120.25.108.11", "time.pool.aliyun.com");
    return true;
}

static inline void blinkerWiFiRestart()
{
    ESP.restart();
}

static inline void blinkerWiFiReconnect()
{
    WiFi.reconnect();
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
