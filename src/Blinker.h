#ifndef BLINKER_H
#define BLINKER_H

#include "Assistant/BlinkerAssistant.h"

#if defined(BLINKER_WIFI) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerWiFi.h"
#endif

#if defined(BLINKER_PRO_ESP) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerPro.h"
#endif

#if defined(BLINKER_WIDGET)
    #include "Widgets/BlinkerButton.h"
    #include "Widgets/BlinkerImage.h"
    #include "Widgets/BlinkerNumber.h"
    #include "Widgets/BlinkerRGB.h"
    #include "Widgets/BlinkerSlider.h"
    #include "Widgets/BlinkerSwitch.h"
    #include "Widgets/BlinkerTab.h"
    #include "Widgets/BlinkerText.h"
#endif

#if defined(BLINKER_ALIGENIE) && (defined(BLINKER_DUEROS) || defined(ESP8266))
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_DUEROS) && (defined(BLINKER_DUEROS) || defined(ESP8266))
    #include "Assistant/BlinkerDuerOS.h"
#endif

#if defined(BLINKER_MIOT) && (defined(ESP32) || defined(ESP8266))
    #include "Assistant/BlinkerMIOT.h"
#endif

#endif
