#ifndef BLINKER_QRCODE_SERIAL_SIM_NBIOT_H
#define BLINKER_QRCODE_SERIAL_SIM_NBIOT_H

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerQRCodeSIM7020.h"
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

class BlinkerQRCodeSerialSIMNBIoT : public BlinkerApi
{
    public :
        // void begin( const char* _type = BLINKER_AIR_DETECTOR,
        //             uint8_t _rx_pin = 2,
        //             uint8_t _tx_pin = 3,
        //             uint32_t _baud = 9600)
        void begin( const char* _type = BLINKER_AIR_DETECTOR,
                    const char* _auth = BLINKER_AIR_DETECTOR,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            #if defined(BLINKER_ALIGENIE_LIGHT)
                String _aliType = BLINKER_F("&aliType=light");
            #elif defined(BLINKER_ALIGENIE_OUTLET)
                String _aliType = BLINKER_F("&aliType=outlet");
            #elif defined(BLINKER_ALIGENIE_SENSOR)
                String _aliType = BLINKER_F("&aliType=sensor");
            #else
                String _aliType = BLINKER_F("");
            #endif

            #if defined(BLINKER_DUEROS_LIGHT)
                String _duerType = BLINKER_F("&duerType=LIGHT");
            #elif defined(BLINKER_DUEROS_OUTLET)
                String _duerType = BLINKER_F("&duerType=SOCKET");
            #elif defined(BLINKER_DUEROS_SENSOR)
                String _duerType = BLINKER_F("&duerType=AIR_MONITOR");
            #else
                String _duerType = BLINKER_F("");
            #endif
            
            Transp.aliType(_aliType);
            Transp.duerType(_duerType);

            serialBegin(_type, _auth, _rx_pin, _tx_pin, _baud);
            // BApi::begin(_type);
            ::delay(100);
            // BApi::atInit();
            BLINKER_LOG(BLINKER_F("Blinker NBIoT initialized..."));
        }

    private :
        void serialBegin(const char* _type,
                        const char* _auth,
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

                BApi::begin(Serial1, true, listening);
    #else
                Serial.begin(ss_baud);
                Transp.initStream(Serial, true, listening);
                transport(Transp);
                
                BApi::begin(_type, _auth, Serial, true, listening);
    #endif
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // BApi::begin();
                Serial1.begin(ss_baud);
                Serial1.println("test!!!");
                Transp.initStream(Serial1, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_type, _auth, Serial1, true, listening);
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // BApi::begin();
                Serial2.begin(ss_baud);
                Transp.initStream(Serial2, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_type, _auth, Serial2, true, listening);
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // BApi::begin();
                Serial3.begin(ss_baud);
                Transp.initStream(Serial3, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_type, _auth, Serial3, true, listening);
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
                
                BApi::begin(_type, _auth, *SSerial, false, listening);
            }
#elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // BApi::begin();
                Serial.begin(ss_baud);
                Transp.initStream(Serial, true, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_type, _auth, Serial, true, listening);
                return;
            }
            else {
                // BApi::begin();
                SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerial->begin(ss_baud);
                Transp.initStream(*SSerial, false, listening);
                transport(Transp);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
                BApi::begin(_type, _auth, *SSerial, false, listening);
            }
#elif defined(ESP32)
            // BApi::begin();
            HSerial = new HardwareSerial(1);
            HSerial->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            Transp.initStream(*HSerial, true, listening);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
            BApi::begin(_type, _auth, *HSerial, true, listening);
#else
            // BApi::begin();
            SSerial = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerial->begin(ss_baud);
            Transp.initStream(*SSerial, false, listening);
            transport(Transp);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                
            BApi::begin(_type, _auth, *SSerial, false, listening);
#endif
        }

    private :
        BlinkerQRCodeSIM7020 Transp;
};

#endif
