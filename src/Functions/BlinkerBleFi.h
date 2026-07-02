#ifndef BLINKER_BLEFI_H
#define BLINKER_BLEFI_H

#if defined(ESP32)

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"

class BlinkerBleFi : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
    public:
        BlinkerBleFi()
            : _connected(false), _available(false), _started(false),
              _bufLen(0), _server(NULL), _service(NULL),
              _characteristic(NULL), _advertising(NULL)
        {}

        void begin(const char* name)
        {
            if (_started) return;

            BLEDevice::init(name);
            _server = BLEDevice::createServer();
            _server->setCallbacks(this);

            _service = _server->createService(BLEUUID((uint16_t)0xffe0));
            _characteristic = _service->createCharacteristic(
                BLEUUID((uint16_t)0xffe1),
                BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_NOTIFY |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_WRITE_NR
            );

            _characteristic->setCallbacks(this);
            _characteristic->addDescriptor(new BLE2902());
            _characteristic->setValue("bleFi");
            _service->start();

            _advertising = _server->getAdvertising();
            _advertising->addServiceUUID(BLEUUID((uint16_t)0xffe0));
            _advertising->start();

            _bufLen = 0;
            _available = false;
            _started = true;

            BLINKER_LOG(BLINKER_F("bleFi started: "), name);
        }

        bool available()
        {
            return _available;
        }

        String readString()
        {
            _available = false;
            String data = _payload;
            _payload = "";
            _bufLen = 0;
            return data;
        }

        void response(const String& data)
        {
            if (!_connected || !_characteristic) return;

            String sendData = data;
            sendData += BLINKER_CMD_NEWLINE;
            uint8_t parts = sendData.length() / 20 + 1;

            for (uint8_t num = 0; num < parts; num++)
            {
                uint32_t start = num * 20;
                uint32_t end = start + 20;
                if (end > sendData.length()) end = sendData.length();
                String part = sendData.substring(start, end);
                if (!part.length()) continue;
                _characteristic->setValue(part.c_str());
                _characteristic->notify();
                delay(5);
            }
        }

        void close()
        {
            if (!_started) return;

            if (_advertising) _advertising->stop();
            _started = false;
        }

    private:
        bool _connected;
        bool _available;
        bool _started;
        char _buffer[BLINKER_MAX_READ_SIZE];
        uint16_t _bufLen;
        String _payload;
        BLEServer* _server;
        BLEService* _service;
        BLECharacteristic* _characteristic;
        BLEAdvertising* _advertising;

        void onConnect(BLEServer* server)
        {
            (void)server;
            _connected = true;
            BLINKER_LOG_ALL(BLINKER_F("bleFi connect"));
        }

        void onDisconnect(BLEServer* server)
        {
            _connected = false;
            BLINKER_LOG_ALL(BLINKER_F("bleFi disconnect"));
            if (_started && server) server->startAdvertising();
        }

        void onWrite(BLECharacteristic* characteristic)
        {
            String value = characteristic->getValue().c_str();
            for (uint16_t num = 0; num < value.length(); num++)
            {
                char ch = value[num];

                if (_bufLen < BLINKER_MAX_READ_SIZE - 1)
                {
                    _buffer[_bufLen++] = ch;
                    _buffer[_bufLen] = '\0';
                }

                if (ch == '\n' || ch == '\r')
                {
                    _payload = String(_buffer);
                    _payload.trim();
                    _available = _payload.length();
                    return;
                }
            }

            String current = String(_buffer);
            current.trim();
            if (current.startsWith("{") && current.endsWith("}"))
            {
                _payload = current;
                _available = true;
            }
        }
};

#endif

#endif
