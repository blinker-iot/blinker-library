#ifndef BlinkerSerialMQTT_H
#define BlinkerSerialMQTT_H

#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "Blinker/BlinkerProtocol.h"

#if defined(ESP32)
    HardwareSerial *HSerialMQTT;
#else
    SoftwareSerial *SSerialMQTT;
#endif

class BlinkerTransportStream
{
    public :
        BlinkerTransportStream()
            : stream(NULL), isConnect(false)
        {}

        bool available()
        {
            if (!isHWS) {
                if (!SSerialMQTT->isListening()) {
                    SSerialMQTT->listen();
                    ::delay(100);
                }
            }
            
            if (stream->available()) {
                // if (!isFresh) {
                //     streamData = (char*)malloc(1*sizeof(char));
                // }
                // int16_t dNum = 0;
                // int c_d = timedRead();
                // while (dNum < BLINKER_MAX_READ_SIZE && 
                //     c_d >=0 && c_d != '\n') {
                //     if (c_d != '\r') {
                //         streamData[dNum] = (char)c_d;
                //         dNum++;
                //         streamData = (char*)realloc(streamData, dNum*sizeof(char));
                //     }

                //     c_d = timedRead();
                // }
                // dNum++;
                // streamData = (char*)realloc(streamData, dNum*sizeof(char));
                // streamData[dNum-1] = '\0';

                // BLINKER_LOG_ALL(BLINKER_F("c_d: "), c_d);

                // BLINKER_LOG_ALL(BLINKER_F("dNum: "), dNum);

                // BLINKER_LOG_ALL(BLINKER_F("strlen: "), strlen(streamData));
                
                // for (uint8_t nnum=0; nnum < strlen(streamData); nnum++)
                // {
                //     BLINKER_LOG_ALL(BLINKER_F("streamData: "), (int)streamData[nnum]);
                // }
                // BLINKER_LOG_ALL(BLINKER_F("streamData: "), c_d);

                if (!isFresh) streamData = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));

                strcpy(streamData, (stream->readStringUntil('\n')).c_str());
                stream->flush();
                
                BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData);
                BLINKER_LOG_FreeHeap();

                // if (dNum < BLINKER_MAX_READ_SIZE) {
                if (strlen(streamData) < BLINKER_MAX_READ_SIZE) {
                    if (streamData[strlen(streamData) - 1] == '\r')
                        streamData[strlen(streamData) - 1] = '\0';
                    isFresh = true;
                    return true;
                }
                else {
                    free(streamData);
                    return false;
                }
            }
            else {
                return false;
            }
        }

        int timedRead()
        {
            int c;
            uint32_t _startMillis = millis();
            do {
                c = stream->read();
                if (c >= 0) return c;
            } while(millis() - _startMillis < 1000);
            return -1; 
        }

        void begin(Stream& s, bool state)
        {
            stream = &s;
            stream->setTimeout(BLINKER_STREAM_TIMEOUT);
            isHWS = state;
        }

        char * lastRead() { return isFresh ? streamData : NULL; }

        void flush() { 
            if (isFresh) {
                free(streamData); isFresh = false;
            }
        }

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
            
            BLINKER_LOG_ALL(BLINKER_F("Response: "), s);
            
            if(connected()) {
                BLINKER_LOG_ALL(BLINKER_F("Succese..."));
                
                stream->println(s);
                return true;
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
                
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
        char*   streamData;
        // char    streamData[BLINKER_MAX_READ_SIZE];
        bool    isFresh = false;
        bool    isConnect;
        bool    isHWS = false;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
                    BLINKER_ERR_LOG_ALL(BLINKER_F("DEVICE NOT CONNECT OR MSG LIMIT"));
                    
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

class BlinkerSerialMQTT
    : public BlinkerProtocol<BlinkerTransportStream>
{
    typedef BlinkerProtocol<BlinkerTransportStream> Base;

    public :
        BlinkerSerialMQTT(BlinkerTransportStream& transp)
            : Base(transp)
        {}

         
#if defined(BLINKER_ESP_SMARTCONFIG)
        void begin( const char* _auth,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }
#elif defined(BLINKER_APCONFIG)
        void begin( const char* _auth,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }
#endif

        void begin( const char* _auth,
                    const char* _ssid,
                    const char* _pswd,
                    uint8_t _rx_pin = 2,
                    uint8_t _tx_pin = 3,
                    uint32_t _baud = 9600)
        {
            Base::begin();
            ::delay(100);
            serialBegin(_rx_pin, _tx_pin, _baud);
            Base::atInit(_auth, _ssid, _pswd);
            BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
        }

    private :
        void serialBegin(uint8_t ss_rx_pin,
                        uint8_t ss_tx_pin,
                        uint32_t ss_baud)
        {
    #if defined (__AVR__)
            if (ss_rx_pin == 0 && ss_tx_pin == 1){
                // Base::begin();
    #if defined (__AVR_ATmega32U4__)
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
    #else
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
    #endif
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #if defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__SAM3X8E__)
            else if (ss_rx_pin == 19 && ss_tx_pin == 18){
                // Base::begin();
                Serial1.begin(ss_baud);
                this->conn.begin(Serial1, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 17 && ss_tx_pin == 16){
                // Base::begin();
                Serial2.begin(ss_baud);
                this->conn.begin(Serial2, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else if (ss_rx_pin == 15 && ss_tx_pin == 14){
                // Base::begin();
                Serial3.begin(ss_baud);
                this->conn.begin(Serial3, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
    #endif  
            else {
                // Base::begin();
                SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialMQTT->begin(ss_baud);
                this->conn.begin(*SSerialMQTT, false);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP8266)
            if (ss_rx_pin == RX && ss_tx_pin == TX) {
                // Base::begin();
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
                return;
            }
            else {
                // Base::begin();
                SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
                SSerialMQTT->begin(ss_baud);
                this->conn.begin(*SSerialMQTT, false);
                // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
            }
#elif defined(ESP32)
            // Base::begin();
            HSerialMQTT = new HardwareSerial(1);
            HSerialMQTT->begin(ss_baud, SERIAL_8N1, ss_rx_pin, ss_tx_pin);
            this->conn.begin(*HSerialMQTT, true);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#else
            // Base::begin();
            SSerialMQTT = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialMQTT->begin(ss_baud);
            this->conn.begin(*SSerialMQTT, false);
            // BLINKER_LOG(BLINKER_F("SerialMQTT initialized..."));
#endif
        }
};

#endif