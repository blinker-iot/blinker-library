#ifndef BLINKER_SERIAL_BLE_H
#define BLINKER_SERIAL_BLE_H

#include "Adapters/BlinkerSerial.h"
#include "Blinker/BlinkerProtocol.h"

#if defined(ESP32)
    #include <HardwareSerial.h>
#else
    #include <SoftwareSerial.h>
#endif

class BlinkerSerialBLE : public BlinkerProtocol<BlinkerSerial>
{
    typedef BlinkerProtocol<BlinkerSerial> Base;

    public :
        BlinkerSerialBLE(BlinkerSerial& transp)
            : Base(transp)
        {}

        void begin( uint8_t ss_rx_pin = 2,
                    uint8_t ss_tx_pin = 3,
                    uint32_t ss_baud = 9600)
        {
            Base::begin();
            ::delay(100);
        #if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                // Base::begin();
        #if defined (__AVR_ATmega32U4__)
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
        #else
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
        #endif
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
        #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // Base::begin();
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // Base::begin();
                Serial2.begin(ss_baud);
                this->conn.begin(Serial2, true);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // Base::begin();
                Serial3.begin(ss_baud);
                this->conn.begin(Serial3, true);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
        #endif  
            else {
                // Base::begin();
                SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialBLE->begin(ss_baud);
                this->conn.begin(*SSerialBLE, false);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
            }
        #elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // Base::begin();
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else {
                // Base::begin();
                SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialBLE->begin(ss_baud);
                this->conn.begin(*SSerialBLE, false);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
            }
        #elif defined(ESP32)
                    // Base::begin();
                    HSerialBLE = new HardwareSerial(1);
                    HSerialBLE->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
                    this->conn.begin(*HSerialBLE, true);
                    BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
        #else
            // Base::begin();
            SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialBLE->begin(ss_baud);
            this->conn.begin(*SSerialBLE, false);
            BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
        #endif
        }
        
};

#endif
