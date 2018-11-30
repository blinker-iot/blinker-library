// #if defined(ESP32)

// #define BLINKER_BLE

// #include "Adapters/BlinkerBLE.h"
// #include "Blinker/BlinkerConfig.h"
// #include "Blinker/BlinkerDebug.h"
// #include "utility/BlinkerUtility.h"

// void BlinkerBLE::begin()
// {
//     BLEDevice::init("Blinker");
//     pServer = BLEDevice::createServer();

//     pService = pServer->createService(BLEUUID((uint16_t)0xffe0));//SERVICE_UUID
//     pServer->setCallbacks(this);

//     pCharacteristic = pService->createCharacteristic(
//                                     BLEUUID((uint16_t)0xffe1),//CHARACTERISTIC_UUID,
//                                     BLECharacteristic::PROPERTY_READ |
//                                     BLECharacteristic::PROPERTY_NOTIFY |
//                                     BLECharacteristic::PROPERTY_WRITE_NR
//                                 );

//     pCharacteristic->setCallbacks(this);

//     pCharacteristic->addDescriptor(new BLE2902());
    
//     pCharacteristic->setValue("Blinker");
//     pService->start();

//     pAdvertising = pServer->getAdvertising();

//     BLEAddress otherAddress = BLEDevice::getAddress();

//     esp_bd_addr_t ble_m_address;
//     memcpy(ble_m_address, otherAddress.getNative(), ESP_BD_ADDR_LEN);
//     char macStr[9] = { 0 };
//     macStr[0] = 0x48;
//     macStr[1] = 0X4d;
//     for (uint8_t cpynum = 0; cpynum < 6; cpynum++) {
//         macStr[cpynum+2] = (char)ble_m_address[cpynum];
//     }

//     pAdvertisementData.setManufacturerData(macStr);
//     pAdvertising->setAdvertisementData(pAdvertisementData);
//     pAdvertising->addServiceUUID(BLEUUID((uint16_t)0xffe0));
//     pAdvertising->start();

//     // Base::loadTimer();
// }

// bool BlinkerBLE::available()
// {
//     checkTimeOut();
//     if (isAvail)
//     {
//         BLINKER_LOG_ALL(BLINKER_F("handleBLE: "), BLEBuf);
        
//         isAvail = false;
//         isFresh = false;

//         return true;
//     }
//     else {
//         return false;
//     }
// }

// char * BlinkerBLE::lastRead()
// {
//     if (_isFresh) return BLEBuf;
//     else return "";
// }

// void BlinkerBLE::flush()
// {
//     if (isFresh)
//     {
//         free(BLEBuf); isFresh = false; 
//         isAvail = false; _isFresh = false;
//     }
// }

// bool BlinkerBLE::print(String s, bool needCheck)
// {
//     if (needCheck)
//     {
//         if (!checkPrintSpan())
//         {
//             respTime = millis();
//             return false;
//         }
//     }

//     respTime = millis();

//     BLINKER_LOG_ALL(BLINKER_F("Response: "), s);
        
//     if (connected())
//     {
//         BLINKER_LOG_ALL(BLINKER_F("Succese..."));

//         s += BLINKER_CMD_NEWLINE;
        
//         String s_send;
//         uint8_t parts = s.length()/20 + 1;
//         for (uint8_t num = 0; num < parts; num++)
//         {
//             if ((num + 1) == parts)
//                 s_send = s.substring(num*(20), s.length()); 
//             else
//                 s_send = s.substring(num*(20), (num+1)*20);    

//             pCharacteristic->setValue(s_send.c_str());
//             pCharacteristic->notify();
//             delay(5);
//         }
//         return true;
//     }
//     else
//     {
//         BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
//         return false;
//     }
// }

// void BlinkerBLE::onConnect(BLEServer* pServer)
// {
//     deviceConnected = true;
//     BLINKER_LOG_ALL("BLE connect");
// }

// void BlinkerBLE::onDisconnect(BLEServer* pServer)
// {
//     deviceConnected = false;
//     BLINKER_LOG_ALL("BLE disconnect");
// }

// void BlinkerBLE::onWrite(BLECharacteristic *pCharacteristic)
// {
//     std::string value = pCharacteristic->getValue();
//     int vlen = value.length();

//     if (vlen > 0) {
//         freshTime = millis();

//         if (_bufLen == 0) {
//             // memset(BLEBuf, '\0', BLINKER_MAX_READ_SIZE);
//             if (_isFresh) free(BLEBuf);
//             BLEBuf = (char*)malloc(1*sizeof(char));
//         }

//         for (uint8_t _num = 0; _num < vlen; _num++) {
//             BLEBuf[_bufLen] = value[_num];
//             _bufLen++;
//             BLEBuf = (char*)realloc(BLEBuf, (_bufLen+1)*sizeof(char));
//         }

//         isFresh = true;
//         _isFresh = true;

//         if (value[vlen-1] == '\n') {
//             isAvail = true;
//             _bufLen = 0;
//         }
//     }
// }

// void BlinkerBLE::checkTimeOut()
// {
//     if (isFresh && !isAvail && (millis() - freshTime) > BLINKER_STREAM_TIMEOUT) {
//         isAvail = true;
//         _bufLen = 0;
//     }
// }

// bool BlinkerBLE::checkPrintSpan()
// {
//     if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
//     {
//         if (respTimes > BLINKER_PRINT_MSG_LIMIT)
//         {
//             BLINKER_ERR_LOG_ALL("DEVICE NOT CONNECT OR MSG LIMIT");
//             return false;
//         }
//         else
//         {
//             respTimes++;
//             return true;
//         }
//     }
//     else
//     {
//         respTimes = 0;
//         return true;
//     }
// }

// #endif
