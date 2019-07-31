#ifndef BLINKER_SUBSTREAM_H
#define BLINKER_SUBSTREAM_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerSubStream : public BlinkerStream
{
    public :
        BlinkerSubStream()
            : isInit(false)
        {}

        int available()
        { 
            if (_subAvail && _subRead)
            {
                if (_subAvail())
                {
                    String data = _subRead();

                    if (isFresh) free(streamData);
                    streamData = (char*)malloc((data.length()+1)*sizeof(char));
                    strcpy(streamData, data.c_str());
                    isFresh = true;

                    return true;
                }
            }
            return false;
        }

        void begin(const char* auth)
        {
            _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
            strcpy(_authKey, auth);
            BLINKER_LOG_ALL(BLINKER_F("_authKey: "), auth);
            if (_subBegin) _subBegin();
        }

        char * lastRead()
        {
            if (isFresh) return streamData;
            else return "";
        }

        void flush()
        {
            if (isFresh)
            {
                free(streamData);
                isFresh = false;
            }
        }

        int print(char * data, bool needCheck = true)
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
            
            BLINKER_LOG_ALL(BLINKER_F("Response: "), data);

            if(connected() && _subPrint)
            {
                BLINKER_LOG_ALL(BLINKER_F("Success..."));
                
                _subPrint(data);
                return true;
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
                
                return false;
            }
        }

        int connect()
        {
            if (_subConnect) return _subConnect();
            else return true;
        }

        int connected()
        {
            if (_subConnected) return _subConnected();
            else return true;
        }

        void disconnect()
        {
            if (_subDisconnect) _subDisconnect();
        }

        int checkPrintSpan()
        {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
            {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT)
                {
                    BLINKER_ERR_LOG(BLINKER_F("DEVICE NOT CONNECT OR MSG LIMIT"));
                    
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

        void attachAvailable(blinker_callback_return_int_t func)
        {
            _subAvail = func;
            initStatus |= 0x01 << 0;
            if (initStatus == 0x07) isInit = true;
        }

        void attachRead(blinker_callback_return_string_t func)
        {
            _subRead = func;
            initStatus |= 0x01 << 1;
            if (initStatus == 0x07) isInit = true;
        }

        void attachPrint(blinker_callback_with_string_arg_t func)
        {
            _subPrint = func;
            initStatus |= 0x01 << 2;
            if (initStatus == 0x07) isInit = true;
        }        

        void attachBegin(blinker_callback_t func) { _subBegin = func; }
        void attachConnect(blinker_callback_return_int_t func) { _subConnect = func; }
        void attachConnected(blinker_callback_return_int_t func) { _subConnected = func; }
        void attachDisconnect(blinker_callback_t func) { _subDisconnect = func; }

    protected :
        char*   _authKey;
        bool    isInit;
        int8_t  initStatus = 0;
        char*   streamData;
        bool    isFresh;
        bool    isConnect;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;
        blinker_callback_return_int_t       _subAvail = NULL;
        blinker_callback_return_string_t    _subRead = NULL;
        blinker_callback_with_string_arg_t  _subPrint = NULL;
        blinker_callback_t                  _subBegin = NULL;
        blinker_callback_return_int_t       _subConnect = NULL;
        blinker_callback_return_int_t       _subConnected = NULL;
        blinker_callback_t                  _subDisconnect = NULL;
};

#endif
