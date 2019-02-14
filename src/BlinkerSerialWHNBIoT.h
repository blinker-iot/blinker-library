#ifndef BLINKER_SERIAL_WH_NBIOT_H
#define BLINKER_SERIAL_WH_NBIOT_H

#include "Adapters/BlinkerSerialNBIoT.h"
#include "Blinker/BlinkerApi.h"

#if defined(ESP32)
    #include <HardwareSerial.h>
#else
    #include <SoftwareSerial.h>
#endif

typedef BlinkerApi BApi;

class BlinkerSerialWHNBIoT : public BlinkerApi
{
    public :
        void begin( uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            BApi::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            BApi::atInit();
            BLINKER_LOG(BLINKER_F("Blinker NBIoT initialized..."));
        }

    private :
        void serialBegin(uint8_t ss_rx_pin,
                        uint8_t ss_tx_pin,
                        uint32_t ss_baud)
        {
    #if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                // BApi::begin();
    #if defined (__AVR_ATmega32U4__)
                Serial1.begin(ss_baud);
                Transp.begin(Serial1, true);
                transport(Transp);
    #else
                Serial.begin(ss_baud);
                Transp.begin(Serial, true);
                transport(Transp);
    #endif
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // BApi::begin();
                Serial1.begin(ss_baud);
                Transp.begin(Serial1, true);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // BApi::begin();
                Serial2.begin(ss_baud);
                Transp.begin(Serial2, true);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // BApi::begin();
                Serial3.begin(ss_baud);
                Transp.begin(Serial3, true);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #endif  
            else {
                // BApi::begin();
                SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerial->begin(ss_baud);
                Transp.begin(*SSerial, false);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // BApi::begin();
                Serial.begin(ss_baud);
                Transp.begin(Serial, true);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else {
                // BApi::begin();
                SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerial->begin(ss_baud);
                Transp.begin(*SSerial, false);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP32)
            // BApi::begin();
            HSerial = new HardwareSerial(1);
            HSerial->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            Transp.begin(*HSerial, true);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#else
            // BApi::begin();
            SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerial->begin(ss_baud);
            Transp.begin(*SSerial, false);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#endif
        }

    private :
        BlinkerSerialNBIoT Transp;
};

#endif
