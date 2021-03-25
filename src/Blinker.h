#ifndef BLINKER_H
#define BLINKER_H

#include "Assistant/BlinkerAssistant.h"

#if defined(BLINKER_WIFI) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerWiFi.h"
#endif

#if defined(BLINKER_WIDGET)
    #include "Widgets/BlinkerButton.h"
    #include "Widgets/BlinkerImage.h"
    #include "Widgets/BlinkerNumber.h"
    #include "Widgets/BlinkerRGB.h"
    #include "Widgets/BlinkerSlider.h"
    #include "Widgets/BlinkerTab.h"
    #include "Widgets/BlinkerText.h"
#endif

#if defined(BLINKER_MIOT) && (defined(ESP32) || defined(ESP8266))
    #include "Assistant/BlinkerMIOT.h"
#endif

#endif
