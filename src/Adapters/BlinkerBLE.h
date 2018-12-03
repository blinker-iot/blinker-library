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

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "utility/BlinkerUtility.h"

class BlinkerBLE : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
    public :
        BlinkerBLE()
            : deviceConnected(false), isAvail(false)
        {}

        void begin();
        bool available();
        int  read();
        int  timedRead();
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

        
        bool                    isAvailBLE = false;
        uint8_t*                bleReadBuf;//[20];
        uint32_t                bleReadBufLen = 0;
        uint32_t                getNum = 0;

        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
        void onWrite(BLECharacteristic *pCharacteristic);
        void checkTimeOut();
        bool checkPrintSpan();
};

void BlinkerBLE::begin()
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

    // Base::loadTimer();

    // bleReadBuf = (char*)malloc(1*sizeof(char));
    // memset(bleReadBuf, '\0', 1);

    BLEBuf = (char*)malloc(1*sizeof(char));

    _bufLen = 0;
}

bool BlinkerBLE::available()
{
    // checkTimeOut();

    // if (isAvail)
    // {
    //     BLINKER_LOG_ALL(BLINKER_F("handleBLE: "), BLEBuf);

    //     BLINKER_LOG_FreeHeap_ALL();
        
    //     isAvail = false;
    //     isFresh = false;

    //     return true;
    // }
    // else {
    //     return false;
    // }

    if (isAvailBLE)
    {
        if (_isFresh) 
        {
            free(BLEBuf);
            BLEBuf = (char*)malloc(1*sizeof(char));

            _bufLen = 0;
        }
        else if(!_isFresh && _bufLen == 0)
        {
            BLEBuf = (char*)malloc(1*sizeof(char));
        }

        int ble_d = timedRead();
        while (_bufLen < BLINKER_MAX_READ_SIZE &&
            ble_d >= 0 && ble_d != '\n')
        {
            BLEBuf[_bufLen] = (char)ble_d;
            // Serial.print(BLEBuf[_bufLen]);
            _bufLen++;
            BLEBuf = (char*)realloc(BLEBuf, (_bufLen+1)*sizeof(char));

            ble_d = timedRead();
        }

        if (ble_d == '\n') 
        {
            BLINKER_LOG_ALL(BLINKER_F("GET \\n"));

            // _bufLen++;
            // BLEBuf = (char*)realloc(BLEBuf, _bufLen*sizeof(char));

            BLEBuf[_bufLen+1] = '\0';
        }

        BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), BLEBuf);
        BLINKER_LOG_ALL(BLINKER_F("_bufLen: "), _bufLen);
        BLINKER_LOG_FreeHeap_ALL();

        for (uint8_t _num = 0; _num < bleReadBufLen; _num++)
        {
            Serial.print((char)bleReadBuf[_num]);
        }

        isAvailBLE = false;
        free(bleReadBuf);

        if (strlen(BLEBuf) < BLINKER_MAX_READ_SIZE && ble_d == '\n')
        {
            _isFresh = true;
            isFresh = true;
            return true;
        }
        else
        {
            free(BLEBuf);
            return false;
        }
    }

    return false;
}

int BlinkerBLE::read()
{
    uint32_t num = getNum;
    if (num < bleReadBufLen)
    {
        getNum++;
        return bleReadBuf[num];
    }
    else
    {
        return -1;
    }
}

int BlinkerBLE::timedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

char * BlinkerBLE::lastRead()
{
    if (_isFresh)  return BLEBuf;
    else return "";
}

void BlinkerBLE::flush()
{
    if (isFresh)
    {
        free(BLEBuf); isFresh = false; 
        isAvail = false; _isFresh = false;
        _bufLen = 0;
    }
}

bool BlinkerBLE::print(String s, bool needCheck)
{
    if (needCheck)
    {
        if (!checkPrintSpan())
        {
            respTime = millis();
            return false;
        }
    }

    respTime = millis();

    BLINKER_LOG_ALL(BLINKER_F("Response: "), s);
        
    if (connected())
    {
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));

        s += BLINKER_CMD_NEWLINE;
        
        String s_send;
        uint8_t parts = s.length()/20 + 1;
        for (uint8_t num = 0; num < parts; num++)
        {
            if ((num + 1) == parts)
                s_send = s.substring(num*(20), s.length()); 
            else
                s_send = s.substring(num*(20), (num+1)*20);    

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

void BlinkerBLE::onConnect(BLEServer* pServer)
{
    deviceConnected = true;
    BLINKER_LOG_ALL("BLE connect");
}

void BlinkerBLE::onDisconnect(BLEServer* pServer)
{
    deviceConnected = false;
    BLINKER_LOG_ALL("BLE disconnect");
}

void BlinkerBLE::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string value = pCharacteristic->getValue();
    int vlen = value.length();

    if (vlen > 0)
    {
        isAvailBLE = true;
        getNum = 0;

        bleReadBuf = (uint8_t*)malloc((vlen+1)*sizeof(uint8_t));

        for (uint8_t _num = 0; _num < vlen; _num++)
        {
            bleReadBuf[_num] = value[_num];
        }

        bleReadBufLen = vlen;

        BLINKER_LOG_ALL("onWrite vlen: ", vlen);
        // BLINKER_LOG_ALL("onWrite value: ", value);
        // BLINKER_LOG_ALL("bleReadBuf: ", bleReadBuf);

        // freshTime = millis();

        // BLINKER_LOG_ALL("isAvail: ", isAvail);
        // BLINKER_LOG_ALL("isFresh: ", isFresh);
        // BLINKER_LOG_ALL("_bufLen: ", _bufLen);

        // if (_bufLen == 0) {
        //     // memset(BLEBuf, '\0', BLINKER_MAX_READ_SIZE);
        //     // if (_isFresh) free(BLEBuf);
        //     // BLEBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
        //     // memset(BLEBuf, '\0', BLINKER_MAX_READ_SIZE);

        //     BLINKER_LOG_FreeHeap_ALL();
        // }

        // for (uint8_t _num = 0; _num < vlen; _num++) {
        //     BLEBuf[_bufLen] = value[_num];
        //     _bufLen++;
        //     // BLEBuf = (char*)realloc(BLEBuf, (_bufLen+1)*sizeof(char));
        // }

        // isFresh = true;
        // _isFresh = true;

        // BLINKER_LOG_ALL("_bufLen: ", _bufLen);

        // if (value[vlen] == '\n') {
        //     isAvail = true;
        //     _bufLen = 0;
        // }

        // BLINKER_LOG_ALL("BLEBuf: ", BLEBuf);
    }
}

void BlinkerBLE::checkTimeOut()
{
    // if (isFresh && !isAvail && (millis() - freshTime) > BLINKER_STREAM_TIMEOUT) {
    //     isAvail = true;
    //     _bufLen = 0;
    // }
}

bool BlinkerBLE::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
    {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT)
        {
            BLINKER_ERR_LOG_ALL("DEVICE NOT CONNECT OR MSG LIMIT");
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
