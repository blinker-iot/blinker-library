#include "Blinker/BlinkerProtocol.h"

void BlinkerProtocol::flush()
{
    isFresh = false; availState = false; 
    canParse = false; isAvail = false;
    if (isInit) conn->flush();
}

void BlinkerProtocol::print(const String & data)
{
    checkFormat();
    strcpy(_sendBuf, data.c_str());
    _print(_sendBuf);
    free(_sendBuf);
    autoFormat = false;
}

void BlinkerProtocol::print(const String & key, const String & data)
{
    checkFormat();
    autoFormatData(key, data);
    autoFormatFreshTime = millis();
}

int BlinkerProtocol::checkAvail()
{
    if (!isInit) return false;

    flush();

    isAvail = conn->available();
    if (isAvail)
    {
        isFresh = true;
        canParse = true;
        availState = true;
    }

    return isAvail;
}

void BlinkerProtocol::checkFormat()
{
    if (!autoFormat)
    {
        autoFormat = true;
        _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
    }
}

void BlinkerProtocol::checkAutoFormat()
{
    if (autoFormat)
    {
        if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
        {
            if (strlen(_sendBuf))
            {
                #if defined(BLINKER_ARDUINOJSON)
                    _print(_sendBuf);
                #else
                    strcat(_sendBuf, "}");
                    _print(_sendBuf);
                #endif
            }
            free(_sendBuf);
            autoFormat = false;
        }
    }
}

void BlinkerProtocol::printNow()
{
    if (strlen(_sendBuf) && autoFormat)
    {
        #if defined(BLINKER_ARDUINOJSON)
            _print(_sendBuf);
        #else
            strcat(_sendBuf, "}");
            _print(_sendBuf);
        #endif

        free(_sendBuf);
        autoFormat = false;
    }
}

void BlinkerProtocol::_timerPrint(const String & n)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (n.length() <= BLINKER_MAX_SEND_SIZE)
    {
        checkFormat();
        checkState(false);
        strcpy(_sendBuf, n.c_str());
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

void BlinkerProtocol::_print(char * n, bool needCheckLength)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        BLINKER_LOG_FreeHeap_ALL();
        conn->print(n, isCheck);
        if (!isCheck) isCheck = true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}
