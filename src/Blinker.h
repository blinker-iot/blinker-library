#ifndef BLINKER_H
#define BLINKER_H

#if defined(ESP32) && (defined(BLINKER_WIFI) || defined(BLINKER_BLE))
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#elif defined(ARDUINO_ARCH_RENESAS_UNO) && defined(BLINKER_WIFI)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#else
    #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#include "Assistant/BlinkerAssistant.h"

#if defined(BLINKER_BLE)
    #include "BlinkerBLE.h"
#endif

#if defined(BLINKER_WIFI)
    #include "BlinkerWiFi.h"
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
    #include "Assistant/BlinkerAliGenie.h""
#endif

#if defined(BLINKER_DUEROS)
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_MIOT)
    #include "Assistant/BlinkerMIOT.h"
#endif

#endif
