#ifndef BlinkerDataTicker_H
#define BlinkerDataTicker_H

#if defined(ESP8266) || defined(ESP32)

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerTimer.h"

#include <Ticker.h>

Ticker dataTicker;

#endif

#endif
