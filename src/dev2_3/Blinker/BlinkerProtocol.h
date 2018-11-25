#ifndef BlinkerProtocol_H
#define BlinkerProtocol_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#define BLINKER_MQTT

#include "Blinker/BlinkerApi.h"
#include "Blinker/BlinkerConfig.h"

template <class Transp>
class BlinkerProtocol
    : public BlinkerApi< BlinkerProtocol<Transp> >
{
    friend class BlinkerApi< BlinkerProtocol<Transp> >;

    typedef BlinkerApi< BlinkerProtocol<Transp> > BApi;

    public :
        enum BlinkerState{
            CONNECTING,
            CONNECTED,
            DISCONNECTED
        };

        BlinkerProtocol(Transp& transp)
            : BApi()
            , conn(transp)
            , state(CONNECTING)
            , isFresh(false)
            , isAvail(false)
            , availState(false)
            , canParse(false)
        {}

        bool connected() { return state == CONNECTED; }
        bool connect(uint32_t timeout = BLINKER_STREAM_TIMEOUT);
        void disconnect();
        bool available();
        String readString();
        void flush();

        template <typename T>
        void print(T n) { _print("\""  + STRING_format(n)+ "\""); }
        void print()    { _print("\"\""); }
        template <typename T>
        void println(T n)   { _print("\""  + STRING_format(n)+ "\""); }
        void println()      { _print("\"\""); }
        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3);
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3);

        template <typename T1>
        void printArray(T1 n1, const String &s2);

        template <typename T1>
        void printObject(T1 n1, const String &s2);

        template <typename T1>
        void print(T1 n1, const String &s2);
        template <typename T1>
        void print(T1 n1, const char str2[]);
        template <typename T1>
        void print(T1 n1, char c);
        template <typename T1>
        void print(T1 n1, unsigned char b);
        template <typename T1>
        void print(T1 n1, int n);
        template <typename T1>
        void print(T1 n1, unsigned int n);
        template <typename T1>
        void print(T1 n1, long n);
        template <typename T1>
        void print(T1 n1, unsigned long n);
        template <typename T1>
        void print(T1 n1, double n);

        template <typename T1>
        void println(T1 n1, const String &s2)    { print(n1, s2); }
        template <typename T1>
        void println(T1 n1, const char str2[])   { print(n1, str2); }
        template <typename T1>
        void println(T1 n1, char c)              { print(n1, c); }
        template <typename T1>
        void println(T1 n1, unsigned char b)     { print(n1, b); }
        template <typename T1>
        void println(T1 n1, int n)               { print(n1, n); }
        template <typename T1>
        void println(T1 n1, unsigned int n)      { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, long n)              { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, unsigned long n)     { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, double n)            { print(n1, n); }

        template <typename T>
        void notify(T n);

        void run();

        void begin();

    // #if defined(BLINKER_MQTT)
        void begin(const char* _auth);
    // #endif

        void _print(const String & n, \
                bool needParse = true, \
                bool needCheckLength = true);

    private :
        void autoFormatData(String & data);
        bool checkAvail();
        char * dataParse();
        // char * lastRead();
        void isParsed();
        bool parseState();
        void printNow();
        void checkAutoFormat();

    protected :
        Transp&         conn;
        BlinkerState    state;
        bool            isFresh;
        bool            isAvail;
        bool            availState;
        bool            canParse;
        bool            autoFormat = false;
        uint32_t        autoFormatFreshTime;
        char*           _sendBuf;

    // #if defined(BLINKER_MQTT)
        char            _authKey[BLINKER_AUTHKEY_SIZE];
        char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
        bool            _isInit = false;
        uint8_t         _disconnectCount = 0;
        uint32_t        _disFreshTime = 0;
        uint32_t        _disconnectTime = 0;
        uint32_t        _refreshTime = 0;

        uint32_t        _reconTime = 0;
    // #endif

    
};

#endif
