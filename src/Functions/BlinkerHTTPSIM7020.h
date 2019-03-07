#ifndef BLINKER_HTTP_SIM7020_H
#define BLINKER_HTTP_SIM7020_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "Adapters/BlinkerSerialMQTT.h"
#include "Blinker/BlinkerATMaster.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

// #if defined(ESP32)
//     #include <HardwareSerial.h>

//     HardwareSerial *HSerial_HTTP;
// #else
//     #include <SoftwareSerial.h>

//     SoftwareSerial *SSerial_HTTP;
// #endif

#define BLINKER_HTTP_SIM7020_DEFAULT_TIMEOUT 5000UL

class BLINKER_HTTP_SIM7020_DEFAULT_TIMEOUT
{
    public :

    protected :
        char c_hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        int8_t dcode_data(char d_test)
        {
            for (uint8_t num = 0; num < 16; num++)
            {
                if (d_test == c_hex[num]) return num;
            }

            return -1;
        }
};

#endif
