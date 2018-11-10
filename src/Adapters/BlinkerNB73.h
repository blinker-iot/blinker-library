#ifndef BlinkerNB73_H
#define BlinkerNB73_H

#include <SoftwareSerial.h>
#include "HardwareSerial.h"
#include <Blinker/BlinkerProtocol.h>

SoftwareSerial *SSerialBLE;
// HardwareSerial *HSerialBLE;

enum BlinkerNB73Status{
    NB_INIT,
    NB_POWER_ON,
    NB_CGATT,
    NB_CGATT_FAIL,
    NB_CGATT_SUCCESS,
    NB_CREAT,
    NB_CREAT_FAIL,
    NB_CREAT_SUCCESS,
    NB_ADDOBJ,
    NB_ADDOBJ_FAIL,
    NB_ADDOBJ_SUCCESS,
    NB_OPEN,
    NB_OPEN_FAIL,
    NB_OPEN_SUCCESS,
    NB_OBSERVE,
    NB_OBSERVE_FAIL,
    NB_OBSERVE_SUCCESS,
    NB_DISCOVER,
    NB_DISCOVER_FAIL,
    NB_DISCOVER_SUCCESS,
    NB_INIT_SUCCESS
};

class NBdata
{
    public :
        NBdata()
            : _isReq(false)
        {}

        bool update(String data) {
            // _data = data;
            // BLINKER_LOG2(BLINKER_F("update data: "), data);
            _isReq = serialize(data);
            return _isReq;
        }

        bool isReq() { return _isReq; }

        String reqName() { return _reqName; }

        uint8_t paramNum() { return _paramNum; }

    private :
        bool _isReq;
        uint8_t _paramNum;
        // String _data;
        String _reqName;
        String _param[11];

        bool serialize(String _data) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("serialize _data: "), _data);
#endif
            int addr_start = _data.indexOf("+");
            int addr_end = 0;

            // BLINKER_LOG2(BLINKER_F("serialize addr_start: "), addr_start);
            // BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);

            if ((addr_start != -1) && STRING_contains_string(_data, ":")) {
                addr_start = 0;
                addr_end = _data.indexOf(":");

                if (addr_end == -1) {
                    return false;
                }
                else {
                    _reqName = _data.substring(addr_start, addr_end);
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("serialize _reqName: "), _reqName);
#endif
                }

                // BLINKER_LOG2(BLINKER_F("serialize _data: "), _data);

                String serData;
                uint16_t dataLen = _data.length();

                for (_paramNum = 0; _paramNum < 11; _paramNum++) {
                    addr_start += addr_end;
                    addr_start += 1;
                    serData = _data.substring(addr_start, dataLen);

                    addr_end = serData.indexOf(",");

                    // BLINKER_LOG2(BLINKER_F("serialize serData: "), serData);
                    // BLINKER_LOG2(BLINKER_F("serialize addr_start: "), addr_start);
                    // BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);

                    if (addr_end == -1) {
                        if (addr_start >= dataLen) return false;
                        _param[_paramNum] = serData;
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_param[_paramNum]: "), _param[_paramNum]);
#endif
                        _paramNum++;
                        return true;
                    }
                    else {
                        _param[_paramNum] = serData.substring(0, addr_end);
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_param[_paramNum]: "), _param[_paramNum]);
#endif
                }
                return true;
            }
            else {
                return false;
            }
        }
};

static class NBdata * _nbDataManager;

class BlinkerTransportStream
{
    public :
        BlinkerTransportStream()
            : stream(NULL), isConnect(false)
        {}

        bool available()
        {
            if (!isHWS) {
                if (!SSerialBLE->isListening()) {
                    SSerialBLE->listen();
                    ::delay(100);
                }
            }
            
            if (stream->available()) {
                strcpy(streamData, (stream->readStringUntil('\n')).c_str());
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleSerial: "), streamData);
#endif

                if (!_nbDataManager) {
                    _nbDataManager = new NBdata();

                    _nbDataManager->update(STRING_format(streamData));

                    BLINKER_LOG2(BLINKER_F("isReq: "), _nbDataManager->isReq());
                    BLINKER_LOG2(BLINKER_F("reqName: "), _nbDataManager->reqName());
                    BLINKER_LOG2(BLINKER_F("paramNum: "), _nbDataManager->paramNum());
                }
                else {
                    _nbDataManager->update(STRING_format(streamData));
                    BLINKER_LOG2(BLINKER_F("isReq: "), _nbDataManager->isReq());
                    BLINKER_LOG2(BLINKER_F("reqName: "), _nbDataManager->reqName());
                    BLINKER_LOG2(BLINKER_F("paramNum: "), _nbDataManager->paramNum());
                }

                return true;
            }
            else {
                return false;
            }
        }

        bool _available()
        {
            if (!isHWS) {
                if (!SSerialBLE->isListening()) {
                    SSerialBLE->listen();
                    ::delay(100);
                }
            }
            
            if (stream->available()) {
                strcpy(streamData, (stream->readStringUntil('\n')).c_str());
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleSerial: "), streamData);
#endif
                return true;
            }
            else {
                return false;
            }
        }

        void begin(Stream& s, bool state)
        {
            stream = &s;
            stream->setTimeout(BLINKER_STREAM_TIMEOUT);
            isHWS = state;

            connect();

            // BlinkerNB73Status init_status = NB_INIT;

            // while (init_status != NB_INIT_SUCCESS) {
            //     // available();

            //     switch (init_status) 
            //     {
            //         case NB_INIT :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_NB_NB73) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_NB73)) {
            //                     init_status = NB_POWER_ON;
            //                     print(BLINKER_CMD_NB_CGATT);

            //                     BLINKER_LOG1("NB_POWER_ON");
            //                 }
            //             }
            //             break;
            //         case NB_POWER_ON :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_NB_CGATT_SUCCESSED) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_CGATT_SUCCESSED)) {
            //                     init_status = NB_CGATT_SUCCESS;
            //                     print(BLINKER_CMD_NB_CREATE);

            //                     BLINKER_LOG1("NB_CGATT_SUCCESS");
            //                 }
            //                 // else if (lastRead() == BLINKER_CMD_NB_CGATT_FAILED) {
            //                 else if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_CGATT_FAILED)) {
            //                     init_status = NB_CGATT_FAIL;

            //                     BLINKER_LOG1("NB_CGATT_FAIL");
            //                 }
            //             }
            //             break;
            //         case NB_CGATT_SUCCESS :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_NB_CREATE_SUCCESSED) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_CREATE_SUCCESSED)) {
            //                     init_status = NB_CREAT_SUCCESS;
            //                     print(BLINKER_CMD_NB_ADDOBJ);
                                
            //                     BLINKER_LOG1("NB_CREAT_SUCCESS");
            //                 }
            //             }
            //             break;
            //         case NB_CREAT_SUCCESS :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_OK) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_OK)) {
            //                     init_status = NB_ADDOBJ_SUCCESS;
            //                     print(BLINKER_CMD_NB_OPEN);
                                
            //                     BLINKER_LOG1("NB_ADDOBJ_SUCCESS");
            //                 }
            //             }
            //             break;
            //         case NB_ADDOBJ_SUCCESS :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_NB_EVENT_6) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_EVENT_6)) {
            //                     init_status = NB_OPEN_SUCCESS;
            //                     // print(BLINKER_CMD_NB_ADDOBJ);

            //                     BLINKER_LOG1("NB_OPEN_SUCCESS");
            //                 }
            //             }
            //             break;
            //         case NB_OPEN_SUCCESS :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_NB_OBSERVE) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_NB_OBSERVE)) {
            //                     init_status = NB_OBSERVE;
            //                     print(BLINKER_CMD_NB_OBSERVERSP);

            //                     BLINKER_LOG1("NB_OBSERVERSP");
            //                 }
            //             }
            //             break;
            //         case NB_OBSERVE :
            //             if (_available()) {
            //                 // if (lastRead() == BLINKER_CMD_OK) {
            //                 if (STRING_contains_string(lastRead(), BLINKER_CMD_OK)) {
            //                     init_status = NB_INIT_SUCCESS;
            //                     // print(BLINKER_CMD_NB_OBSERVERSP);

            //                     BLINKER_LOG1("NB_INIT_SUCCESS");
            //                 }
            //             }
            //             break;
            //         default :
            //             break;
            //     }
            // }
        }

        String lastRead() { return STRING_format(streamData); }

        bool print(String s)
        {
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
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if(connected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                stream->println(s);
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif
                return false;
            }
        }

        bool connect()
        {
            isConnect = true;
            return connected();
        }

        bool connected() { return isConnect; }

        void disconnect() { isConnect = false; }

    protected :
        Stream* stream;
        char    streamData[BLINKER_MAX_READ_SIZE];
        bool    isConnect;
        bool    isHWS = false;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1("DEVICE NOT CONNECT OR MSG LIMIT");
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

class BlinkerNB73
    : public BlinkerProtocol<BlinkerTransportStream>
{
    typedef BlinkerProtocol<BlinkerTransportStream> Base;

    public :
        BlinkerNB73(BlinkerTransportStream& transp)
            : Base(transp)
        {}

        void begin(uint8_t ss_rx_pin = 2,
                    uint8_t ss_tx_pin = 3,
                    uint32_t ss_baud = 9600)
        {
#if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                Base::begin();
    #if defined (__AVR_ATmega32U4__)
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
    #else
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
    #endif
                BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                Base::begin();
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
                BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                Base::begin();
                Serial2.begin(ss_baud);
                this->conn.begin(Serial2, true);
                BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                Base::begin();
                Serial3.begin(ss_baud);
                this->conn.begin(Serial3, true);
                BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
                return;
            }
    #endif  
            else {
                Base::begin();
                SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialBLE->begin(ss_baud);
                this->conn.begin(*SSerialBLE, false);
                BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
            }
#else
            Base::begin();
            SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialBLE->begin(ss_baud);
            this->conn.begin(*SSerialBLE, false);
            BLINKER_LOG1(BLINKER_F("NBIoT initialized..."));
#endif
        }
};

#endif