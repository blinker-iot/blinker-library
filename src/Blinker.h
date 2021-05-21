#ifndef BLINKER_H
#define BLINKER_H

#include "Assistant/BlinkerAssistant.h"

#if defined(BLINKER_BLE)
    #include "BlinkerBLE.h"
#endif

#if defined(BLINKER_WIFI) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerWiFi.h"
#endif

#if defined(BLINKER_PRO_ESP) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerPro.h"
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

#if defined(BLINKER_ALIGENIE) && (defined(ESP32) || defined(ESP8266))
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_DUEROS) && (defined(ESP32) || defined(ESP8266))
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_MIOT) && (defined(ESP32) || defined(ESP8266))
    #include "Assistant/BlinkerMIOT.h"
#endif

#endif
