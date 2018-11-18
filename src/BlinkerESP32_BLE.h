#ifndef BlinkerESP32_BLE_H
#define BlinkerESP32_BLE_H

#include <Blinker/BlinkerProtocol.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// #define SERVICE_UUID        "0000ffe0-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "0000ffe1-36e1-4688-b7f5-ea07361b26a8"

class BlinkerBLE
    :
    public BLEServerCallbacks,
    public BLECharacteristicCallbacks
{
    public :
        BlinkerBLE()
            : deviceConnected(false), isAvail(false)
        {}

        void begin()
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
        }

        bool available()
        {
            checkTimeOut();
            if (isAvail) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG(BLINKER_F("handleBLE: "), BLEBuf);//
#endif
                isAvail = false;
                isFresh = false;

                return true;
            }
            else {
                return false;
            }
        }

        char * lastRead() { return _isFresh ? BLEBuf : ""; }//

        void flush() {
            if (isFresh) {
                free(BLEBuf); isFresh = false; 
                isAvail = false; _isFresh = false;
            }
        }
        
        bool print(String s) {
            bool state = STRING_contains_string(s, BLINKER_CMD_NOTICE);

            if (!state) {
                state = (STRING_contains_string(s, BLINKER_CMD_STATE) 
                    && STRING_contains_string(s, BLINKER_CMD_CONNECTED));
            }

            if (!state) {
                if (!checkPrintSpan()) {
                    respTime = millis();
                    return false;
                }
            }

            respTime = millis();

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG(BLINKER_F("Response: "), s);
#endif
            if (connected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG(BLINKER_F("Succese..."));
#endif

                s += BLINKER_CMD_NEWLINE;
                
                String s_send;
                uint8_t parts = s.length()/20 + 1;
                for (uint8_t num = 0; num < parts; num++) {
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
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG(BLINKER_F("Faile... Disconnected"));
#endif                
                return false;
            }
        }

        bool connect() { return deviceConnected; }

        void disconnect() { deviceConnected = false; }

        bool connected() { return deviceConnected; }

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

        void onConnect(BLEServer* pServer) {
            deviceConnected = true;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG("BLE connect");
#endif
        };

        void onDisconnect(BLEServer* pServer) {
            deviceConnected = false;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG("BLE disconnect");
#endif
        }

        void onWrite(BLECharacteristic *pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            int vlen = value.length();

            if (vlen > 0) {
                freshTime = millis();

                if (_bufLen == 0) {
                    // memset(BLEBuf, '\0', BLINKER_MAX_READ_SIZE);
                    if (!_isFresh) BLEBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                }

                for (uint8_t _num = 0; _num < vlen; _num++) {
                    BLEBuf[_bufLen] = value[_num];
                    _bufLen++;
                }

                isFresh = true;
                _isFresh = true;

                if (value[vlen-1] == '\n') {
                    isAvail = true;
                    _bufLen = 0;
                }
            }
        }

        void checkTimeOut() {
            if (isFresh && !isAvail && (millis() - freshTime) > BLINKER_STREAM_TIMEOUT) {
                isAvail = true;
                _bufLen = 0;
            }
        }

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG("DEVICE NOT CONNECT OR MSG LIMIT");
#endif
                    return false;
                }
                else {
                    respTimes++;
                    return true;
                }
            }
            else {
                respTimes = 0;
                return true;
            }
        }
};

class BlinkerESP32_BLE
    : public BlinkerProtocol<BlinkerBLE>
{
    typedef BlinkerProtocol<BlinkerBLE> Base;

    public:
        BlinkerESP32_BLE(BlinkerBLE & transp)
            : Base(transp)
        {}

        void begin()
        {
            Base::begin();
            this->conn.begin();
            BLINKER_LOG("ESP32_BLE initialized...");
        }
};

static BlinkerBLE _blinkerTransportBLE;
BlinkerESP32_BLE Blinker(_blinkerTransportBLE);

#include <BlinkerWidgets.h>

#endif