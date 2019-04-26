#ifndef BLINKER_LOW_POWER_GPRS_H
#define BLINKER_LOW_POWER_GPRS_H

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerAIR202LP.h"
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
        {
            serialBegin(_key, _type, _rx_pin, _tx_pin, _baud);
            ::delay(100);
            BLINKER_LOG(BLINKER_F("Blinker GPRS initialized..."));
        }

    private :
        void serialBegin(const char* _key,
                        const char* _type,
                        uint8_t ss_rx_pin,
                        uint8_t ss_tx_pin,
                        uint32_t ss_baud)
        {
    #if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                // BApi::begin();
    #if defined (__AVR_ATmega32U4__)
                Serial1.begin(ss_baud);
                Transp.initStream(Serial1, true, listening);
                transport(Transp);

                BApi::begin(_key, _type, Serial1, true, listening);
    #else
                Serial.begin(ss_baud);
                Transp.initStream(Serial, true, listening);
                transport(Transp);
                
                BApi::begin(_key, _type, Serial, true, listening);
    #endif
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // BApi::begin();
                Serial1.begin(ss_baud);
                Transp.initStream(Serial1, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_key, _type, Serial1, true, listening);
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // BApi::begin();
                Serial2.begin(ss_baud);
                Transp.initStream(Serial2, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_key, _type, Serial2, true, listening);
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // BApi::begin();
                Serial3.begin(ss_baud);
                Transp.initStream(Serial3, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_key, _type, Serial3, true, listening);
                return;
            }
    #endif  
            else {
                // BApi::begin();
                SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerial->begin(ss_baud);
                Transp.initStream(*SSerial, false, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));                
                
                BApi::begin(_key, _type, *SSerial, false, listening);
            }
#elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // BApi::begin();
                Serial.begin(ss_baud);
                Transp.initStream(Serial, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_key, _type, Serial, true, listening);
                return;
            }
            else {
                // BApi::begin();
                SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerial->begin(ss_baud);
                Transp.initStream(*SSerial, false, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_key, _type, *SSerial, false, listening);
            }
#elif defined(ESP32)
            // BApi::begin();
            HSerial = new HardwareSerial(1);
            HSerial->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            Transp.initStream(*HSerial, true, listening);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
            BApi::begin(_key, _type, *HSerial, true, listening);
#else
            // BApi::begin();
            SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerial->begin(ss_baud);
            Transp.initStream(*SSerial, false, listening);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
            BApi::begin(_key, _type, *SSerial, false, listening);
#endif
        }

        BlinkerAIR202LP Transp;
};

#endif
