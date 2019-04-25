#ifndef BLINKER_LOW_POWER_GPRS_H
#define BLINKER_LOW_POWER_GPRS_H

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

// #include "Adapters/BlinkerSerialAIR202.h"
#include "Blinker/BlinkerApi.h"

#if defined(ESP32)
    #include <HardwareSerial.h>

    HardwareSerial *HSerial;
#else
    #include <SoftwareSerial.h>

    SoftwareSerial *SSerial;
#endif

typedef BlinkerApi BApi;

void listening()
{
    yield();

    #if defined(__AVR__) || defined(ESP8266)
    if (!SSerial->isListening())
    {
        SSerial->listen();
        ::delay(100);
    }
    #endif
}

class BlinkerLowPowerGPRS : public BlinkerApi
{
    public :
        void begin(const char* _key, const char* _type,
                uint8_t _rx_pin = 2, uint8_t _tx_pin = 3,
                uint32_t _baud = 9600)
        {}
};

#endif
