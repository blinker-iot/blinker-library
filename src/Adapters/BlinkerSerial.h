#ifndef BlinkerSerial_H
#define BlinkerSerial_H

#include <SoftwareSerial.h>
#include <Blinker/BlinkerProtocol.h>

SoftwareSerial *SerialBLE;

class BlinkerTransportStream
{
    public :
        BlinkerTransportStream()
            : stream(NULL), isConnect(false)
        {}

        bool available()
        {
            if (!SerialBLE->isListening()) {
                SerialBLE->listen();
                ::delay(100);
            }
            
            if (stream->available()) {
                strcpy(streamData, (stream->readString()).c_str());
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleSerial: "), streamData);
#endif
                return true;
            }
            else {
                return false;
            }
        }

        void begin(Stream& s)
        {
            stream = &s;
            stream->setTimeout(BLINKER_STREAM_TIMEOUT);
        }

        String lastRead() { return STRING_format(streamData); }

        void print(String s)
        {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if(connected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                stream->print(s);
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif
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
        char    streamData[BLINKER_BUFFER_SIZE];
        bool    isConnect;
};

class BlinkerSerail
    : public BlinkerProtocol<BlinkerTransportStream>
{
    typedef BlinkerProtocol<BlinkerTransportStream> Base;

    public :
        BlinkerSerail(BlinkerTransportStream& transp)
            : Base(transp)
        {}

        void begin(uint8_t ss_rx_pin = 2,
                    uint8_t ss_tx_pin = 3,
                    uint32_t ss_baud = 9600)
        {
            Base::begin();
            SerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SerialBLE->begin(ss_baud);
            this->conn.begin(*SerialBLE);
            BLINKER_LOG1("SerialBLE Initialled...");
        }
};

#endif