#ifndef Blinker_BLE_H
#define Blinker_BLE_H

#include "Blinker/BlinkerProtocol.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// #define SERVICE_UUID        "0000ffe0-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "0000ffe1-36e1-4688-b7f5-ea07361b26a8"

class BlinkerBLE:public BLEServerCallbacks,public BLECharacteristicCallbacks
{
    public:
        BlinkerBLE()
            : deviceConnected(false), isAvail(false)
        {}

        void begin();
        bool available();
        void flush();
        bool print(String s);
        bool connect();
        void disconnect();
        bool connected();
        String lastRead();
    private:
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
