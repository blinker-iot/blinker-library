#ifndef BLINKER_BLE_H
#define BLINKER_BLE_H

#if !defined(BLINKER_BLE)
    #error "BLINKER_BLE must be defined to use BLE functionality!"
#endif

#if defined(ESP32)
    #include "Functions/BlinkerBLEESP.h"
    #include "Blinker/BlinkerProtocol.h"
    #include "modules/ArduinoJson/ArduinoJson.h"
    
    class BlinkerBLE: public BlinkerProtocol<BlinkerBLEESP>
    {
        typedef BlinkerProtocol<BlinkerBLEESP> Base;

        public :
            BlinkerBLE(BlinkerBLEESP &transp) : Base(transp) {}

            void begin()
            {
                Base::begin();
                this->conn.begin();
            }

        private :

    };
    
#elif defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_UNOR4_WIFI)
    #include "Functions/BlinkerBLEUNO.h"
    #include "Blinker/BlinkerProtocol.h"
    #include "modules/ArduinoJson/ArduinoJson.h"
    
    class BlinkerBLE: public BlinkerProtocol<BlinkerBLEUNO>
    {
        typedef BlinkerProtocol<BlinkerBLEUNO> Base;

        public :
            BlinkerBLE(BlinkerBLEUNO &transp) : Base(transp) {}

            void begin()
            {
                Base::begin();
                this->conn.begin();
            }

        private :

    };    
#else
    #error "BLE is not supported on this platform! Supported platforms: ESP32, Arduino UNO R4 WiFi"
#endif

#endif
