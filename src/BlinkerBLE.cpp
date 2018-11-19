#include "BlinkerBLE.h"
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
    for (uint8_t cpynum = 0; cpynum < 6; cpynum++)
    {
        macStr[cpynum+2] = (char)ble_m_address[cpynum];
    }

    pAdvertisementData.setManufacturerData(macStr);
    pAdvertising->setAdvertisementData(pAdvertisementData);
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0xffe0));
    pAdvertising->start();

    // Base::loadTimer();
}
bool BlinkerBLE::available()
{
    checkTimeOut();
    if (isAvail)
    {
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2(BLINKER_F("handleBLE: "), BLEBuf);//
#endif
        isAvail = false;
        isFresh = false;

        return true;
    }
    else
    {
        return false;
    }
}
String BlinkerBLE::lastRead()
{
    return _isFresh ? STRING_format(BLEBuf) : STRING_format("");
}

void BlinkerBLE::flush()
{
    free(BLEBuf); isFresh = false;
    isAvail = false; _isFresh = false;
}
bool BlinkerBLE::print(String s)
{
    bool state = STRING_contains_string(s, BLINKER_CMD_NOTICE);

    if (!state)
    {
        state = (STRING_contains_string(s, BLINKER_CMD_STATE) 
            && STRING_contains_string(s, BLINKER_CMD_CONNECTED));
        if (!checkPrintSpan())
        {
            respTime = millis();
            return false;
        }
    }

    respTime = millis();

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
    if (connected())
    {
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
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
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif                
        return false;
    }
}
bool BlinkerBLE::connect()
{
    return deviceConnected;
}
void BlinkerBLE::disconnect()
{
    deviceConnected = false;
}
bool BlinkerBLE::connected()
{
    return deviceConnected;
}

void BlinkerBLE::onConnect(BLEServer* pServer)
{
    deviceConnected = true;
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("BLE connect");
#endif
};

void BlinkerBLE::onDisconnect(BLEServer* pServer)
{
    deviceConnected = false;
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("BLE disconnect");
#endif
    }

void BlinkerBLE::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string value = pCharacteristic->getValue();
    int vlen = value.length();

    if (vlen > 0)
    {
        freshTime = millis();

        if (_bufLen == 0)
        {
            // memset(BLEBuf, '\0', BLINKER_MAX_READ_SIZE);
            if (!_isFresh) BLEBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
        }

        for (uint8_t _num = 0; _num < vlen; _num++)
        {
            BLEBuf[_bufLen] = value[_num];
            _bufLen++;
        }

        isFresh = true;
        _isFresh = true;

        if (value[vlen-1] == '\n')
        {
            isAvail = true;
            _bufLen = 0;
        }
    }
}

void BlinkerBLE::checkTimeOut()
{
    if (isFresh && !isAvail && (millis() - freshTime) > BLINKER_STREAM_TIMEOUT)
    {
        isAvail = true;
        _bufLen = 0;
    }
}

bool BlinkerBLE::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
    {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT)
        {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_ERR_LOG1("DEVICE NOT CONNECT OR MSG LIMIT");
#endif
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