#ifndef BLINKER_ESP_MQTT_AT_H
#define BLINKER_ESP_MQTT_AT_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTTAT.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPMQTTAT : public BlinkerApi
{
    public :
        void begin()
        {
            BApi::begin();

            ::delay(100);

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_SERIALCFG, serialSet);

            uint32_t ss_baud = serialSet >> 8 & 0x00FFFFFF;
            ss_cfg = serConfig();

            if (ss_baud != 300    || ss_baud != 1200    || ss_baud != 2400   ||
                ss_baud != 4800   || ss_baud != 9600    || ss_baud != 19200  ||
                ss_baud != 38400  || ss_baud != 57600   || ss_baud != 74880  ||
                ss_baud != 115200 || ss_baud != 230400  || ss_baud != 250000 ||
                ss_baud != 500000 || ss_baud != 1000000 || ss_baud != 2000000)
            {
                serialSet = BLINKER_SERIAL_DEFAULT;
                ss_baud = 9600;
                ss_cfg = SERIAL_8N1;
                
                EEPROM.put(BLINKER_EEP_ADDR_SERIALCFG, serialSet);
            }

            EEPROM.commit();
            EEPROM.end();

            Serial.begin(ss_baud, ss_cfg);
            Transp.serialBegin(Serial, true);
            transport(Transp);
            BApi::atBegin();
            // strcpy(BApi::_authKey, this->conn.authKey().c_str());
            // strcpy(BApi::_deviceName, this->conn.deviceName().c_str());
            BApi::loadTimer();
            BLINKER_LOG(BLINKER_F("BLINKER_MQTT_AT initialized..."));
        }

    private :
        BlinkerMQTTAT Transp;
};

#endif

#endif
