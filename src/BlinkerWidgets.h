#ifndef BLINKER_WIDGETS_H
#define BLINKER_WIDGETS_H

#if defined(BLINKER_MQTT)
    #include "utility/BlinkerBridge.h"
#endif

#include "utility/BlinkerButton.h"
#include "utility/BlinkerJoystick.h"
#include "utility/BlinkerNumber.h"
#include "utility/BlinkerRGB.h"
#include "utility/BlinkerSlider.h"
#include "utility/BlinkerSwitch.h"
#include "utility/BlinkerText.h"

#if defined(BLINKER_ALIGENIE)
    #include "utility/BlinkerAliGenie.h"

    BLINKERALIGENIE BlinkerAliGenie;    
#endif

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI) || \
    defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_NBIOT)

    BlinkerSwitch BUILTIN_SWITCH;

#endif

#endif
