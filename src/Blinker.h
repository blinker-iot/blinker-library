#ifndef BLINKER_H
#define BLINKER_H


#if defined(ESP32)
    #define BLINKER_PLATFORM_SUPPORTED
#elif defined(ARDUINO_UNOR4_WIFI)
    #define BLINKER_PLATFORM_SUPPORTED
    #define ARDUINO_ARCH_RENESAS_UNO
#else
    #error "Unsupported platform! Blinker only supports ESP32 and Arduino UNO R4 WiFi platforms."
#endif

#if !defined(BLINKER_WIFI) && !defined(BLINKER_BLE)
    #error "No communication method defined! Please define BLINKER_WIFI or BLINKER_BLE before including Blinker.h"
#endif

#if defined(ESP32)
    #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
    #else
        #error "ESP32 platform requires BLINKER_WIFI or BLINKER_BLE to be defined!"
    #endif
#elif defined(ARDUINO_UNOR4_WIFI)
    #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
    #else
        #error "Arduino UNO R4 WiFi platform requires BLINKER_WIFI or BLINKER_BLE to be defined!"
    #endif
#endif

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Assistant/BlinkerAssistant.h"

#if defined(BLINKER_BLE)
    #include "BlinkerBLE.h"
    
    #if defined(ESP32)
        extern BlinkerBLEESP BLEESP;
        #define Blinker BlinkerBLE::getInstance(BLEESP)
    #elif defined(ARDUINO_UNOR4_WIFI)
        extern BlinkerBLEUNO BLEUNO;
        #define Blinker BlinkerBLE::getInstance(BLEUNO)
    #else
        #error "BLE is not supported on this platform!"
    #endif
#endif

#if defined(BLINKER_WIFI)
    #include "BlinkerWiFi.h"
    
    #if defined(ESP32)
        extern BlinkerWiFiESP WiFiESP;
        #define Blinker BlinkerWiFi::getInstance(WiFiESP)
    #elif defined(ARDUINO_UNOR4_WIFI)
        extern BlinkerWiFiUNO WiFiUNO;
        #define Blinker BlinkerWiFi::getInstance(WiFiUNO)
    #else
        #error "WiFi is not supported on this platform!"
    #endif
#endif

#if defined(BLINKER_WIFI) && defined(BLINKER_BLE)
    #error "Cannot define both BLINKER_WIFI and BLINKER_BLE at the same time! Please choose one communication method."
#endif

#if defined(BLINKER_WIDGET)
    #include "Widgets/BlinkerButton.h"
    #include "Widgets/BlinkerImage.h"
    #include "Widgets/BlinkerJoystick.h"
    #include "Widgets/BlinkerNumber.h"
    #include "Widgets/BlinkerRGB.h"
    #include "Widgets/BlinkerSlider.h"
    #include "Widgets/BlinkerSwitch.h"
    #include "Widgets/BlinkerTab.h"
    #include "Widgets/BlinkerText.h"
#endif

#if defined(BLINKER_ALIGENIE)
    #include "Assistant/BlinkerAliGenie.h"
#endif

#if defined(BLINKER_DUEROS)
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_MIOT)
    #include "Assistant/BlinkerMIOT.h"
#endif

#if defined(BLINKER_DEBUG)
    #pragma message("Blinker Debug Mode Enabled")
#endif

#if defined(ESP32)
    #pragma message("Compiling for ESP32 platform")
#elif defined(ARDUINO_UNOR4_WIFI)
    #pragma message("Compiling for Arduino UNO R4 WiFi platform")
#endif

#if defined(BLINKER_WIFI)
    #pragma message("WiFi communication enabled")
#elif defined(BLINKER_BLE)
    #pragma message("BLE communication enabled")
#endif

#endif
