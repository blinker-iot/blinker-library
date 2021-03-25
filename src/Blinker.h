#ifndef BLINKER_H
#define BLINKER_H

#if defined(BLINKER_WIFI) && (defined(ESP32) || defined(ESP8266))
    #include "BlinkerWiFi.h"
#endif

#if defined(BLINKER_WIDGET)
    #include "Widgets/BlinkerButton.h"
    #include "Widgets/BlinkerImage.h"
    #include "Widgets/BlinkerNumber.h"
    #include "Widgets/BlinkerRGB.h"
#endif

#endif
