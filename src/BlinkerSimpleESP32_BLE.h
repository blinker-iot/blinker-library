#ifndef BlinkerSimplerESP32_BLE_H
#define BlinkerSimplerESP32_BLE_H

#include <Blinker/BlinkerProtocol.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// #define SERVICE_UUID        "0000ffe0-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "0000ffe1-36e1-4688-b7f5-ea07361b26a8"

class BlinkerTransportESP32_BLE
    :
    public BLEServerCallbacks,
    public BLECharacteristicCallbacks
{
    public :
        BlinkerTransportESP32_BLE()
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
        }

        bool available()
        {
            checkTimeOut();
            if (isAvail) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleBLE: "), BLEBuf);//
#endif
                isAvail = false;
                isFresh = false;

                return true;
            }
            else {
                return false;
            }
        }

        String lastRead() { return STRING_format(BLEBuf); }//

        void print(String s) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if (connected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                pCharacteristic->setValue(s.c_str());
                pCharacteristic->notify();
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif                
            }
        }

        bool connect() { return deviceConnected; }

        void disconnect() { deviceConnected = false; }

        bool connected() { return deviceConnected; }

    private :
        bool                    deviceConnected;
        char                    BLEBuf[BLINKER_BUFFER_SIZE];
        bool                    isAvail;
        bool                    isFresh;
        uint32_t                _bufLen;
        uint32_t                freshTime;
        BLEServer               *pServer;
        BLEService              *pService;
        BLECharacteristic       *pCharacteristic;
        BLEAdvertising          *pAdvertising;
        BLEAdvertisementData    pAdvertisementData;

        void onConnect(BLEServer* pServer) {
            deviceConnected = true;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("BLE connect");
#endif
        };

        void onDisconnect(BLEServer* pServer) {
            deviceConnected = false;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("BLE disconnect");
#endif
        }

        void onWrite(BLECharacteristic *pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            int vlen = value.length();

            if (vlen > 0) {
                freshTime = millis();

                if (_bufLen == 0) {
                    memset(BLEBuf, '\0', BLINKER_BUFFER_SIZE);
                }

                for (uint8_t _num = 0; _num < vlen; _num++) {
                    BLEBuf[_bufLen] = value[_num];
                    _bufLen++;
                }

                isFresh = true;

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
};

class BlinkerESP32_BLE
    : public BlinkerProtocol<BlinkerTransportESP32_BLE>
{
    typedef BlinkerProtocol<BlinkerTransportESP32_BLE> Base;

    public:
        BlinkerESP32_BLE(BlinkerTransportESP32_BLE & transp)
            : Base(transp)
        {}

        void begin()
        {
            Base::begin();
            this->conn.begin();
            BLINKER_LOG1("ESP32_BLE Initialled...");
        }
};

static BlinkerTransportESP32_BLE _blinkerTransportBLE;
BlinkerESP32_BLE Blinker(_blinkerTransportBLE);

#endif