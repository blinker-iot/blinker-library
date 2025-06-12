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

        private:
            static BlinkerBLE* instance;
            BlinkerBLEESP* transport;

            BlinkerBLE(BlinkerBLEESP &transp) : Base(transp), transport(&transp) {}

            BlinkerBLE(const BlinkerBLE&) = delete;
            BlinkerBLE& operator=(const BlinkerBLE&) = delete;

        public:
            static BlinkerBLE& getInstance(BlinkerBLEESP &transp) 
            {
                if (instance == nullptr) {
                    instance = new BlinkerBLE(transp);
                }
                return *instance;
            }
            static BlinkerBLE& getInstance() 
            {
                if (instance == nullptr) {
                }
                return *instance;
            }

            void begin()
            {
                Base::begin();
                this->conn.begin();
            }

            ~BlinkerBLE() 
            {
                instance = nullptr;
            }
    };

    BlinkerBLE* BlinkerBLE::instance = nullptr;
    
#elif defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_UNOR4_WIFI)
    #include "Functions/BlinkerBLEUNO.h"
    #include "Blinker/BlinkerProtocol.h"
    #include "modules/ArduinoJson/ArduinoJson.h"
    
    class BlinkerBLE: public BlinkerProtocol<BlinkerBLEUNO>
    {
        typedef BlinkerProtocol<BlinkerBLEUNO> Base;

        private:
            static BlinkerBLE* instance;
            BlinkerBLEUNO* transport;

            BlinkerBLE(BlinkerBLEUNO &transp) : Base(transp), transport(&transp) {}

            BlinkerBLE(const BlinkerBLE&) = delete;
            BlinkerBLE& operator=(const BlinkerBLE&) = delete;

        public:
            static BlinkerBLE& getInstance(BlinkerBLEUNO &transp) 
            {
                if (instance == nullptr) {
                    instance = new BlinkerBLE(transp);
                }
                return *instance;
            }

            static BlinkerBLE& getInstance() 
            {
                if (instance == nullptr) {
                }
                return *instance;
            }

            void begin()
            {
                Base::begin();
                this->conn.begin();
            }

            ~BlinkerBLE() 
            {
                instance = nullptr;
            }
    };

    BlinkerBLE* BlinkerBLE::instance = nullptr;
    
#else
    #error "BLE is not supported on this platform! Supported platforms: ESP32, Arduino UNO R4 WiFi"
#endif

#endif
