#ifndef BLINKER_AT_MASTER_H
#define BLINKER_AT_MASTER_H

#include "BlinkerDebug.h"
#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

enum blinker_at_m_state_t {
    AT_M_NONE,
    AT_M_RESP,
    AT_M_OK,
    AT_M_ERR
};

#define BLINKER_MAX_AT_MASTER_PARAM_NUM 6

#define BLINKER_MAX_AT_MASTER_DATA_SIZE 48

class BlinkerMasterAT
{
    public :
        BlinkerMasterAT() {}
        //     : _isReq(false)
        // {}

        void update(const String & data) {
            // _data = data;
            // BLINKER_LOG(BLINKER_F("update data: "), data);
            serialize(data);
            // return _isReq;
        }

        blinker_at_m_state_t getState() { return _isReq; }

        String reqName() { return _reqName; }

        uint8_t paramNum() { return _paramNum; }

        String getParam(uint8_t num) {
            if (num >= _paramNum) return "";
            else return _param[num];
        }

    private :
        blinker_at_m_state_t _isReq;
        uint8_t _paramNum;
        // String _data;
        char _reqName[32];
        char _param[BLINKER_MAX_AT_MASTER_PARAM_NUM][BLINKER_MAX_AT_MASTER_DATA_SIZE];

        void serialize(const String & _data) {
            BLINKER_LOG_ALL(BLINKER_F("serialize _data: "), _data);
            
            // _reqName = "";
            memset(_reqName, '\0', 32);
            _isReq = AT_M_NONE;
            int addr_start = _data.indexOf("+");
            int addr_end = 0;

            // BLINKER_LOG(BLINKER_F("serialize addr_start: "), addr_start);
            // BLINKER_LOG(BLINKER_F("serialize addr_end: "), addr_end);

            if ((addr_start != -1) && STRING_contains_string(_data, ":")) {
                addr_start = 0;
                addr_end = _data.indexOf(":");

                if (addr_end == -1) {
                    _isReq = AT_M_NONE;
                    return;
                }
                else {
                    strcpy(_reqName, _data.substring(addr_start + 1, addr_end).c_str());
                    
                    BLINKER_LOG_ALL(BLINKER_F("serialize _reqName: "), _reqName);
                }

                // _isReq = true;

                // BLINKER_LOG(BLINKER_F("serialize _data: "), _data);

                String serData;
                uint16_t dataLen = _data.length();

                addr_start = 0;

                for (_paramNum = 0; _paramNum < BLINKER_MAX_AT_MASTER_PARAM_NUM; _paramNum++) {
                    addr_start += addr_end;
                    addr_start += 1;
                    serData = _data.substring(addr_start, dataLen);

                    addr_end = serData.indexOf(",");

                    // BLINKER_LOG(BLINKER_F("serialize serData: "), serData);
                    // BLINKER_LOG(BLINKER_F("serialize addr_start: "), addr_start);
                    // BLINKER_LOG(BLINKER_F("serialize addr_end: "), addr_end);

                    if (addr_end == -1) {
                        if (addr_start >= dataLen) {
                            _isReq = AT_M_NONE;
                            return;
                        }

                        addr_end = serData.indexOf(" ");

                        if (addr_end != -1) {
                            // _param[_paramNum] = serData.substring(0, addr_end);
                            if (addr_end != 0)
                            {
                                if (serData.substring(0, addr_end).length() < BLINKER_MAX_AT_MASTER_DATA_SIZE)
                                {
                                    strcpy(_param[_paramNum], serData.substring(0, addr_end).c_str());
                                }
                                _isReq = AT_M_RESP;
                                BLINKER_LOG_ALL(BLINKER_F("_param0["), _paramNum, \
                                            BLINKER_F("]: "), _param[_paramNum],
                                            BLINKER_F(", addr_end: "), addr_end);
                                _paramNum++;
                                return;
                            }
                            else if (addr_end == 0 && serData.length() > 0)
                            {
                                serData = serData.substring(1, serData.length());

                                if (serData.substring(0, serData.length()).length() < BLINKER_MAX_AT_MASTER_DATA_SIZE)
                                {
                                    strcpy(_param[_paramNum], serData.substring(0, serData.length()).c_str());
                                }
                                _isReq = AT_M_RESP;
                                BLINKER_LOG_ALL(BLINKER_F("_param0["), _paramNum, \
                                            BLINKER_F("]: "), _param[_paramNum],
                                            BLINKER_F(", addr_end: "), addr_end);
                                _paramNum++;
                                return;
                            }
                        }

                        // _param[_paramNum] = serData;
                        if (serData.length() < BLINKER_MAX_AT_MASTER_DATA_SIZE)
                        {
                            strcpy(_param[_paramNum], serData.c_str());
                        }
                        
                        // BLINKER_LOG_ALL(BLINKER_F("serialize serData: "), serData);
                        
                        BLINKER_LOG_ALL(BLINKER_F("_param1["), _paramNum, \
                                        BLINKER_F("]: "), _param[_paramNum], \
                                        " ", serData);

                        // BLINKER_LOG_ALL(BLINKER_F("serialize serData: "), serData);
                        
                        _paramNum++;
                        _isReq = AT_M_RESP;

                        return;
                    }
                    else {
                        // _param[_paramNum] = serData.substring(0, addr_end);
                        if (serData.substring(0, addr_end).length() < BLINKER_MAX_AT_MASTER_DATA_SIZE)
                        {
                            strcpy(_param[_paramNum], serData.substring(0, addr_end).c_str());
                        }
                    }
                    BLINKER_LOG_ALL(BLINKER_F("_param["), _paramNum, \
                                    BLINKER_F("]: "), _param[_paramNum]);
                }
                _isReq = AT_M_RESP;
                return;
            }
            else if (_data == BLINKER_CMD_OK) {
                _isReq = AT_M_OK;
                return;
            }
            else if (_data == BLINKER_CMD_ERROR) {
                _isReq = AT_M_ERR;
                return;
            }
            else {
                _isReq = AT_M_NONE;
                return;
            }
        }
};

#endif
