#ifndef BLINKER_SERIAL_BLE_H
#define BLINKER_SERIAL_BLE_H

#if !defined (__AVR__)
    #ifndef BLINKER_ARDUINOJSON
        #define BLINKER_ARDUINOJSON
    #endif
    #ifndef ARDUINOJSON_VERSION_MAJOR
    #include "modules/ArduinoJson/ArduinoJson.h"
    #endif

    #if defined(ESP32)
        #include <HardwareSerial.h>    
    #elif defined (__AVR__) || defined(ESP8266)
        #include <SoftwareSerial.h>
    #endif
#else
    #if defined (BLINKER_ARDUINOJSON)
        #ifndef ARDUINOJSON_VERSION_MAJOR
        #include "modules/ArduinoJson/ArduinoJson.h"
        #endif
    #endif
#endif

#include "Adapters/BlinkerSerial.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerSerialBLE : public BlinkerApi
{
    public :
        void begin( uint8_t ss_rx_pin = 2,
                    uint8_t ss_tx_pin = 3,
                    uint32_t ss_baud = 9600)
        {
        #if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                BApi::begin();
            #if defined (__AVR_ATmega32U4__)
                    Serial1.begin(ss_baud);
                    Transp.begin(Serial1, true);
                    transport(Transp);
                    // this->conn.begin(Serial1, true);
            #else
                    Serial.begin(ss_baud);
                    Transp.begin(Serial, true);
                    transport(Transp);
                    // this->conn.begin(Serial, true);
            #endif
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
                else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                    BApi::begin();
                    Serial1.begin(ss_baud);
                    Transp.begin(Serial1, true);
                    transport(Transp);
                    // this->conn.begin(Serial1, true);
                    BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                    return;
                }
                else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                    BApi::begin();
                    Serial2.begin(ss_baud);
                    Transp.begin(Serial2, true);
                    transport(Transp);
                    // this->conn.begin(Serial2, true);
                    BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                    return;
                }
                else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                    BApi::begin();
                    Serial3.begin(ss_baud);
                    Transp.begin(Serial3, true);
                    transport(Transp);
                    // this->conn.begin(Serial3, true);
                    BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                    return;
                }
                else {
                    BApi::begin();
                    SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                    SSerialBLE->begin(ss_baud);
                    Transp.begin(*SSerialBLE, false);
                    transport(Transp);
                    // this->conn.begin(*SSerialBLE, false);
                    BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                }
            #endif
        #elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                BApi::begin();
                Serial.begin(ss_baud);
                Transp.begin(Serial, true);
                transport(Transp);
                // this->conn.begin(Serial, true);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else {
                BApi::begin();
                SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialBLE->begin(ss_baud);
                Transp.begin(*SSerialBLE, false);
                transport(Transp);
                // this->conn.begin(*SSerialBLE, false);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
        #elif defined(ESP32)
            BApi::begin();
            HSerialBLE = new HardwareSerial(1);
            HSerialBLE->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            Transp.begin(*HSerialBLE, true);
            transport(Transp);
            // this->conn.begin(*HSerialBLE, true);
            BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
        #elif defined(ARDUINO_SAM_DUE)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                BApi::begin();
                Serial.begin(ss_baud);
                Transp.begin(Serial, true);
                transport(Transp);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                BApi::begin();
                Serial1.begin(ss_baud);
                Transp.begin(Serial1, true);
                transport(Transp);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                BApi::begin();
                Serial2.begin(ss_baud);
                Transp.begin(Serial2, true);
                transport(Transp);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                BApi::begin();
                Serial3.begin(ss_baud);
                Transp.begin(Serial3, true);
                transport(Transp);
                BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
                return;
            }
            else
            {
                BLINKER_ERR_LOG(BLINKER_F("Arduino DUE, Please set right Hardware Serial!"));
                return;
            }
        #else
            BApi::begin();
            SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialBLE->begin(ss_baud);
            Transp.begin(*SSerialBLE, false);
            transport(Transp);
            // this->conn.begin(*SSerialBLE, false);
            BLINKER_LOG(BLINKER_F("SerialBLE initialized..."));
        #endif
        }

    private :
        BlinkerSerial Transp;
};

#endif
