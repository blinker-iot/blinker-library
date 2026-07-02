#ifndef BLINKER_WIDGETS_H
#define BLINKER_WIDGETS_H

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

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)

    BlinkerSwitch BUILTIN_SWITCH;
#endif

#endif
