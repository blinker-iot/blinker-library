#ifndef BLINKER_SERIAL_ESP_MQTT_H
#define BLINKER_SERIAL_ESP_MQTT_H

#include "Adapters/BlinkerSerialMQTT.h"
#include "Blinker/BlinkerProtocol.h"

#if defined(ESP32)
    #include <HardwareSerial.h>
#else
    #include <SoftwareSerial.h>
#endif

class BlinkerSerialESPMQTT : public BlinkerProtocol<BlinkerSerialMQTT>
{
    typedef BlinkerProtocol<BlinkerSerialMQTT> Base;

    public :
        BlinkerSerialESPMQTT(BlinkerSerialMQTT& transp)
            : Base(transp)
        {}

         
#if defined(BLINKER_ESP_SMARTCONFIG)
        void begin( const char* _auth,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }
#elif defined(BLINKER_APCONFIG)
        void begin( const char* _auth,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }
#endif

        void begin( const char* _auth,
                    const char* _ssid,
                    const char* _pswd,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth, _ssid, _pswd);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }

    private :
        void serialBegin(uint8_t ss_rx_pin,
                        uint8_t ss_tx_pin,
                        uint32_t ss_baud)
        {
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
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // Base::begin();
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // Base::begin();
                Serial2.begin(ss_baud);
                this->conn.begin(Serial2, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // Base::begin();
                Serial3.begin(ss_baud);
                this->conn.begin(Serial3, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #endif  
            else {
                // Base::begin();
                SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialMQTT->begin(ss_baud);
                this->conn.begin(*SSerialMQTT, false);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // Base::begin();
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else {
                // Base::begin();
                SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialMQTT->begin(ss_baud);
                this->conn.begin(*SSerialMQTT, false);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP32)
            // Base::begin();
            HSerialMQTT = new HardwareSerial(1);
            HSerialMQTT->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            this->conn.begin(*HSerialMQTT, true);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#else
            // Base::begin();
            SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialMQTT->begin(ss_baud);
            this->conn.begin(*SSerialMQTT, false);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#endif
        }
};

#endif
