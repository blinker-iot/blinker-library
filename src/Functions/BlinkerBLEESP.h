#ifndef BLINKER_BLE_ESP_H
#define BLINKER_BLE_ESP_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerBLEESP : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
    public :
        BlinkerBLEESP()
            : deviceConnected(false), isAvail(false)
        {}
        ~BlinkerBLEESP();

        void begin();
        bool init()     { return true; }
        int available();
        int  read();
        int  timedRead();
        char * lastRead();// { return _isFresh ? BLEBuf : ""; }
        void flush();
        // bool print(String s, bool needCheck = true);
        int print(char * data, bool needCheck = true);
        int connect()      { return deviceConnected; }
        void disconnect()   { deviceConnected = false; }
        int connected()    { return deviceConnected; }

    private :
        bool                    deviceConnected;
        char*                   BLEBuf = nullptr;//[BLINKER_MAX_READ_SIZE];
        bool                    _isFresh = false;
        bool                    isAvail = false;
        bool                    isFresh = false;
        uint32_t                _bufLen = 0;
        uint32_t                freshTime = 0;
        BLEServer               *pServer = nullptr;
        BLEService              *pService = nullptr;
        BLECharacteristic       *pCharacteristic = nullptr;
        BLEAdvertising          *pAdvertising = nullptr;
        BLEAdvertisementData    pAdvertisementData;
        uint8_t                 respTimes = 0;
        uint32_t                respTime = 0;

        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
        void onWrite(BLECharacteristic *pCharacteristic);
        int checkTimeOut();
        int checkPrintSpan();
};

BlinkerBLEESP   BLEESP;

BlinkerBLEESP::~BlinkerBLEESP()
{
    if (pCharacteristic) {
        pCharacteristic->setCallbacks(nullptr);
        pCharacteristic = nullptr;
    }
    
    if (pService) {
        pService->stop();
        pService = nullptr;
    }

    if (pServer) {
        pServer->setCallbacks(nullptr);
        BLEDevice::deinit();
        pServer = nullptr;
    }

    if (pAdvertising) {
        BLEDevice::getAdvertising()->stop();
        pAdvertising = nullptr;
    }

    if (BLEBuf) {
        free(BLEBuf);
        BLEBuf = nullptr;
    }
}

void BlinkerBLEESP::begin()
{
    BLEDevice::init("Blinker");
    pServer = BLEDevice::createServer();

    pService = pServer->createService(BLEUUID((uint16_t)0xffe0));//SERVICE_UUID
    pServer->setCallbacks(this);

    pCharacteristic = pService->createCharacteristic(
                                    BLEUUID((uint16_t)0xffe1),//CHARACTERISTIC_UUID,
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_NOTIFY |
                                    BLECharacteristic::PROPERTY_WRITE_NR
                                );

    pCharacteristic->setCallbacks(this);

    pCharacteristic->addDescriptor(new BLE2902());
    
    pCharacteristic->setValue("Blinker");
    pService->start();

    pAdvertising = pServer->getAdvertising();

    BLEAddress otherAddress = BLEDevice::getAddress();

    esp_bd_addr_t ble_m_address;
    memcpy(ble_m_address, otherAddress.getNative(), ESP_BD_ADDR_LEN);
    char macStr[9] = { 0 };
    macStr[0] = 0x48;
    macStr[1] = 0X4d;
    for (uint8_t cpynum = 0; cpynum < 6; cpynum++) {
        macStr[cpynum+2] = (char)ble_m_address[cpynum];
    }

    pAdvertisementData.setManufacturerData(macStr);
    pAdvertising->setAdvertisementData(pAdvertisementData);
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0xffe0));
    pAdvertising->start();

    _bufLen = 0;
}

int BlinkerBLEESP::available()
{
    // checkTimeOut();

    if (_bufLen)
    {
        bool isNewLine = false;

        for (uint8_t num = 0; num < _bufLen; num++)
        {
            if (BLEBuf[num] == '\n')
            {
                BLEBuf[num] = '\0';
                isNewLine = true;
            }
        }

        if (isNewLine)
        {                   
            BLINKER_LOG_ALL(BLINKER_F("GET1: "), BLEBuf);
            isAvail = true;
            _isFresh = true;

            // for (uint8_t num = 0; num < _bufLen; num++)
            // {
            //     Serial.print(BLEBuf[num]);
            // }
            // Serial.println();
        }
        else
        {
            if (checkTimeOut())
            {
                BLEBuf[_bufLen] = '\0';
                BLINKER_LOG_ALL(BLINKER_F("GET: "), BLEBuf);
                isAvail = true;
                _isFresh = true;
            }
        }
    }

    if (isAvail)
    {
        BLINKER_LOG_ALL(BLINKER_F("handleBLE: "), BLEBuf);

        BLINKER_LOG_FreeHeap_ALL();
        
        isAvail = false;

        return true;
    }
    else {
        return false;
    }

    return false;
}

// int BlinkerBLEESP::read()
// {
//     uint32_t num = getNum;
//     if (num < bleReadBufLen)
//     {
//         getNum++;
//         return bleReadBuf[num];
//     }
//     else
//     {
//         return -1;
//     }
// }

// int BlinkerBLEESP::timedRead()
// {
//     int c;
//     uint32_t _startMillis = millis();
//     do {
//         c = read();
//         if (c >= 0) return c;
//     } while(millis() - _startMillis < 1000);
//     return -1; 
// }

char * BlinkerBLEESP::lastRead()
{
    if (_isFresh)  return BLEBuf;
    else return "";
}

void BlinkerBLEESP::flush()
{
    if (_isFresh || _bufLen)
    {
        BLINKER_LOG_ALL(BLINKER_F("flush"));
        free(BLEBuf); //isFresh = false; 
        isAvail = false; _isFresh = false;
        _bufLen = 0; //isNewLine = false;
    }
}

// bool BlinkerBLEESP::print(String s, bool needCheck)
int BlinkerBLEESP::print(char * data, bool needCheck)
{
    if (needCheck)
    {
        if (!checkPrintSpan())
        {
            respTime = millis();
            return false;
        }
    }

    String s = data;

    respTime = millis();

    BLINKER_LOG_ALL(BLINKER_F("Response: "), s);
        
    if (connected())
    {
        BLINKER_LOG_ALL(BLINKER_F("Success..."));

        s += BLINKER_CMD_NEWLINE;
        
        String s_send;
        uint8_t parts = s.length()/20 + 1;
        for (uint8_t num = 0; num < parts; num++)
        {
            if ((num + 1) == parts)
                s_send = s.substring(num*(20), s.length()); 
            else
                s_send = s.substring(num*(20), (num+1)*20);    
            // BLINKER_LOG_ALL("s_send: ", s_send);
            pCharacteristic->setValue(s_send.c_str());
            pCharacteristic->notify();
            delay(5);
        }
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        return false;
    }
}

void BlinkerBLEESP::onConnect(BLEServer* pServer)
{
    deviceConnected = true;
    BLINKER_LOG_ALL("BLE connect");
}

void BlinkerBLEESP::onDisconnect(BLEServer* pServer)
{
    deviceConnected = false;
    BLINKER_LOG_ALL("BLE disconnect");
    pServer->startAdvertising();
}

void BlinkerBLEESP::onWrite(BLECharacteristic *pCharacteristic)
{
    String value = pCharacteristic->getValue().c_str();
    int vlen = value.length();

    if (vlen > 0)
    {
        int data;
        uint8_t num;

        if (!_bufLen) BLEBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));

        for (num = 0; num < vlen; num++)
        {
            data = value[num];
            if (data == '\n')
            {
                BLEBuf[_bufLen+num] = data;
                num++;
                BLINKER_LOG_ALL(BLINKER_F("GET \\n"));                
                break;
            }
            BLEBuf[_bufLen+num] = (char)data;
        }

        if (_bufLen) _bufLen += num;
        else _bufLen = num;

        freshTime = millis();

        BLINKER_LOG_ALL(BLINKER_F("vlen: "), vlen);
        BLINKER_LOG_ALL(BLINKER_F("_bufLen: "), _bufLen);
    }
}

int BlinkerBLEESP::checkTimeOut()
{
    ::delay(10);

    uint32_t timeout_ms = millis() - freshTime;

    // BLINKER_LOG_ALL(BLINKER_F("timeout_ms: "), timeout_ms);    

    return timeout_ms > 1000;
}

int BlinkerBLEESP::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
    {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT)
        {
            BLINKER_ERR_LOG("DEVICE NOT CONNECT OR MSG LIMIT");
            return false;
        }
        else
        {
            respTimes++;
            return true;
        }
    }
    else
    {
        respTimes = 0;
        return true;
    }
}

#endif