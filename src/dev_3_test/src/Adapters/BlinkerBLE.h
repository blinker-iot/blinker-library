#ifndef BLINKER_BLE_H
#define BLINKER_BLE_H

#if defined(ESP32)

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class BlinkerBLE : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
    public :
        BlinkerBLE()
            : deviceConnected(false), isAvail(false)
        {}

        void begin();
        bool available();
        char * lastRead();// { return _isFresh ? BLEBuf : ""; }
        void flush();        
        bool print(String s, bool needCheck = true);
        bool connect()      { return deviceConnected; }
        void disconnect()   { deviceConnected = false; }
        bool connected()    { return deviceConnected; }

    private :
        bool                    deviceConnected;
        char*                   BLEBuf;//[BLINKER_MAX_READ_SIZE];
        bool                    _isFresh = false;
        bool                    isAvail;
        bool                    isFresh;
        uint32_t                _bufLen;
        uint32_t                freshTime;
        BLEServer               *pServer;
        BLEService              *pService;
        BLECharacteristic       *pCharacteristic;
        BLEAdvertising          *pAdvertising;
        BLEAdvertisementData    pAdvertisementData;
        uint8_t                 respTimes = 0;
        uint32_t                respTime = 0;

        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
        void onWrite(BLECharacteristic *pCharacteristic);
        void checkTimeOut();
        bool checkPrintSpan();
};

#endif

#endif
