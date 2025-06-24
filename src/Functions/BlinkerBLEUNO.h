#ifndef BLINKER_BLE_UNO_H
#define BLINKER_BLE_UNO_H

#if defined(ARDUINO_ARCH_RENESAS)

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "../modules/ArduinoBLE/ArduinoBLE.h"

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerBLEUNO
{
    public :
        BlinkerBLEUNO()
            : deviceConnected(false), isAvail(false)
        {}
        ~BlinkerBLEUNO();

        void begin();
        bool init()     { return true; }
        int available();
        int  read();
        int  timedRead();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        int connect()      { return connected(); }
        void disconnect()   { 
            BLEDevice central = BLE.central();
            if (central && central.connected()) {
                central.disconnect();
            }
        }
        int connected()    { 
            updateCentralConnection(); 
            return deviceConnected; 
        }

    private :
        bool                    deviceConnected;
        char*                   BLEBuf = nullptr;
        bool                    _isFresh = false;
        bool                    isAvail = false;
        bool                    isFresh = false;
        uint32_t                _bufLen = 0;
        uint32_t                freshTime = 0;
        uint8_t                 respTimes = 0;
        uint32_t                respTime = 0;

        BLEService*             pService = nullptr;
        BLECharacteristic*      pCharacteristic = nullptr;
        
        int checkTimeOut();
        int checkPrintSpan();
        void processReceivedData();
        void updateCentralConnection();
};

BlinkerBLEUNO   BLEUNO;

BlinkerBLEUNO::~BlinkerBLEUNO()
{
    BLEDevice central = BLE.central();
    if (central && central.connected()) {
        central.disconnect();
    }
    
    BLE.end();

    if (BLEBuf) {
        free(BLEBuf);
        BLEBuf = nullptr;
    }

    if (pService) {
        delete pService;
        pService = nullptr;
    }

    if (pCharacteristic) {
        delete pCharacteristic;
        pCharacteristic = nullptr;
    }
}

void BlinkerBLEUNO::begin()
{
    if (!BLE.begin()) {
        BLINKER_ERR_LOG("Starting BLE failed!");
        return;
    }

    BLE.setLocalName("Blinker");
    BLE.setDeviceName("Blinker");

    pService = new BLEService("FFE0");

    pCharacteristic = new BLECharacteristic("FFE1", 
                                           BLERead | BLEWrite | BLENotify, 
                                           512, false);

    pService->addCharacteristic(*pCharacteristic);

    BLE.addService(*pService);

    pCharacteristic->writeValue("Blinker");

    BLE.setAdvertisedService(*pService);
    
    String address = BLE.address();
    uint8_t macData[8];
    macData[0] = 0x48;  // 'H'
    macData[1] = 0x4D;  // 'M'
    
    address.replace(":", "");
    for (int i = 0; i < 6 && i * 2 < address.length(); i++) {
        String hexByte = address.substring(i * 2, i * 2 + 2);
        macData[i + 2] = (uint8_t)strtol(hexByte.c_str(), NULL, 16);
    }

    BLE.setManufacturerData(macData, 8);

    BLE.advertise();

    _bufLen = 0;
    deviceConnected = false;

    BLINKER_LOG_ALL("BLE advertising started");
}

int BlinkerBLEUNO::available()
{
    updateCentralConnection();
    processReceivedData();
    
    if (!deviceConnected) {
        return false;
    }

    if (_bufLen > 0)
    {
        bool isComplete = false;
        String bufferStr = String(BLEBuf);
        
        if (bufferStr.indexOf('\n') >= 0) {
            isComplete = true;
            BLINKER_LOG_ALL(BLINKER_F("Complete packet (newline): "), BLEBuf);
        }
        else if (bufferStr.indexOf('{') >= 0 && bufferStr.indexOf('}') >= 0) {
            isComplete = true;
            BLINKER_LOG_ALL(BLINKER_F("Complete packet (JSON): "), BLEBuf);
        }
        else if (checkTimeOut()) {
            isComplete = true;
            BLINKER_LOG_ALL(BLINKER_F("Complete packet (timeout): "), BLEBuf);
        }
        
        if (isComplete) {
            for (uint8_t num = 0; num < _bufLen; num++) {
                if (BLEBuf[num] == '\n' || BLEBuf[num] == '\r') {
                    BLEBuf[num] = '\0';
                    break;
                }
            }
            
            isAvail = true;
            _isFresh = true;
        }
    }

    if (isAvail)
    {
        BLINKER_LOG_ALL(BLINKER_F("handleBLE: "), BLEBuf);
        BLINKER_LOG_FreeHeap_ALL();
        isAvail = false;
        return true;
    }
    
    return false;
}

void BlinkerBLEUNO::updateCentralConnection()
{
    BLEDevice central = BLE.central();
    
    if (central && central.connected()) {
        if (!deviceConnected) {
            deviceConnected = true;
            BLINKER_LOG_ALL("BLE connect");
        }
    } else {
        if (deviceConnected) {
            deviceConnected = false;
            BLINKER_LOG_ALL("BLE disconnect");
        }
    }
}

void BlinkerBLEUNO::processReceivedData()
{
    if (!deviceConnected) return;
    
    BLEDevice central = BLE.central();
    if (!central || !central.connected()) return;
    
    if (pCharacteristic->written()) {
        int dataLength = pCharacteristic->valueLength();
        const uint8_t* data = pCharacteristic->value();
        
        BLINKER_LOG_ALL(BLINKER_F("Raw data length: "), dataLength);
        
        if (dataLength > 0) {
            if (!BLEBuf) {
                BLEBuf = (char*)malloc(BLINKER_MAX_READ_SIZE * sizeof(char));
                if (!BLEBuf) {
                    BLINKER_ERR_LOG("Memory allocation failed");
                    return;
                }
                _bufLen = 0;
            }
            
            String debugData = "";
            for (int i = 0; i < dataLength; i++) {
                debugData += String(data[i], HEX) + " ";
            }
            BLINKER_LOG_ALL(BLINKER_F("Raw hex data: "), debugData);

            if (_bufLen + dataLength >= BLINKER_MAX_READ_SIZE - 1) {
                BLINKER_ERR_LOG("Buffer overflow, resetting buffer");
                _bufLen = 0;
            }

            for (int num = 0; num < dataLength; num++)
            {
                if (_bufLen >= BLINKER_MAX_READ_SIZE - 1) {
                    break;
                }
                
                BLEBuf[_bufLen] = (char)data[num];
                _bufLen++;
                
                if (data[num] == '\n') {
                    BLINKER_LOG_ALL(BLINKER_F("Found newline character"));
                    break;
                }
            }

            BLEBuf[_bufLen] = '\0';
            
            freshTime = millis();

            BLINKER_LOG_ALL(BLINKER_F("Total buffer length: "), _bufLen);
            BLINKER_LOG_ALL(BLINKER_F("Buffer content: "), BLEBuf);
            
            String bufferStr = String(BLEBuf);
            if (bufferStr.indexOf('{') >= 0 && bufferStr.indexOf('}') >= 0) {
                BLINKER_LOG_ALL(BLINKER_F("Complete JSON packet received"));
            }
        }
    }
    else if (pCharacteristic->subscribed()) {
        // BLINKER_LOG_ALL(BLINKER_F("Characteristic subscribed, waiting for data..."));
    // } else {
    //     BLINKER_LOG_ALL(BLINKER_F("Characteristic not subscribed, no data received"));
    }
}

int BlinkerBLEUNO::read()
{
    return -1;
}

int BlinkerBLEUNO::timedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

char * BlinkerBLEUNO::lastRead()
{
    if (_isFresh) return BLEBuf;
    else return "";
}

void BlinkerBLEUNO::flush()
{
    if (_isFresh || _bufLen)
    {
        BLINKER_LOG_ALL(BLINKER_F("flush"));
        if (BLEBuf) {
            free(BLEBuf);
            BLEBuf = nullptr;
        }
        isAvail = false;
        _isFresh = false;
        _bufLen = 0;
    }
}

int BlinkerBLEUNO::print(char * data, bool needCheck)
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
        
        pCharacteristic->writeValue(s.c_str());
        
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Failed... Disconnected"));
        return false;
    }
}

int BlinkerBLEUNO::checkTimeOut()
{
    ::delay(10);
    uint32_t timeout_ms = millis() - freshTime;
    return timeout_ms > 500;
}

int BlinkerBLEUNO::checkPrintSpan()
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

#endif