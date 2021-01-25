#ifndef BLINKER_WIDGETS_H
#define BLINKER_WIDGETS_H

#if defined(BLINKER_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_MQTT_AUTO)
    #include "Functions/BlinkerBridge.h"
#endif

#include "Functions/BlinkerButton.h"
#include "Functions/BlinkerNumber.h"
#include "Functions/BlinkerImage.h"
#include "Functions/BlinkerRGB.h"
// #if defined(BLINKER_BLE)
    #include "Functions/BlinkerJoystick.h"
// #endif
#include "Functions/BlinkerSlider.h"
#include "Functions/BlinkerSwitch.h"
#include "Functions/BlinkerTab.h"
#include "Functions/BlinkerText.h"

#if defined(BLINKER_ALIGENIE)
    #include "Functions/BlinkerAliGenie.h"

    BLINKERALIGENIE BlinkerAliGenie;    
#endif

#if defined(BLINKER_DUEROS)
    #include "Functions/BlinkerDuerOS.h"

    BLINKERDUEROS BlinkerDuerOS;    
#endif

#if defined(BLINKER_MIOT)
    #include "Functions/BlinkerMIOT.h"

    BLINKERMIOT BlinkerMIOT;    
#endif

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI) || \
    defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_NBIOT_WH) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKE_HTTP)

    BlinkerSwitch BUILTIN_SWITCH;
#endif

#if defined(BLINKER_PRO_ESP)
    #include "Functions/BlinkerEvent.h"

    BLINKEREVENT BlinkerEvent;
#endif

#endif
