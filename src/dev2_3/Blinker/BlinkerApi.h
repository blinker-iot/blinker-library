#ifndef BlinkerApi_H
#define BlinkerApi_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerConfig.h"

enum b_ahrsattitude_t {
    Yaw,
    Pitch,
    Roll
};

enum b_gps_t {
    LONG,
    LAT
};

template <class Proto>
class BlinkerApi
{
    public :
        BlinkerApi() {
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
            gpsValue[LONG] = 0.0;//"0.000000";
            gpsValue[LAT] = 0.0;//"0.000000";
        }

    private :
        int16_t     ahrsValue[3];
        float       gpsValue[2];
};

#endif
