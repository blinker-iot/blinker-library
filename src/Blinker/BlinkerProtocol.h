#ifndef BlinkerProtocol_H
#define BlinkerProtocol_H

#include <Blinker/BlinkerApi.h>

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

        bool connect(uint32_t timeout = BLINKER_STREAM_TIMEOUT)
        {
            state = CONNECTING;

            uint32_t startTime = millis();
            while ( (state != CONNECTED) && (millis() - startTime) < timeout ) {
                run();
            }

            return state == CONNECTED;
        }

        void disconnect() 
        {
            conn.disconnect();
            state = DISCONNECTED;
        }

        void run();

        bool available() {
            bool _avail = availState;
            availState = false;
            return _avail;
        }

        String readString()
        {
            if (isFresh) {
                isFresh = false;
                return conn.lastRead();
            }
            else {
                return "";
            }
        }
        
        template <typename T>
        void print(T n) {
            String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
            if (data.length() <= BLINKER_MAX_SEND_SIZE) {
                conn.print(data);
            }
            else {
                BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
            }
        }
        void print()                        { print(""); }
        
        template <typename T>
        void println(T n)                   { print(n); }
        void println()                      { print(); }

        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3)     { print("{\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"}"); }
        
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3)   { print("{\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"}"); }

        // template <typename T1, typename T2>
        // void print(T1 n1, T2 n2)            { print("{\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"}"); }
        
        // template <typename T1, typename T2>
        // void println(T1 n1, T2 n2)          { print("{\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"}"); }
        
        void print(const String &s1, const String &s2)      { print("{\"" + s1 + "\":\"" + s2 + "\"}"); }
        void print(const char str1[], const char str2[])    { print("{\"" + STRING_format(str1) + "\":\"" + STRING_format(str2) + "\"}"); }
        void print(const char str[], char c)                { print("{\"" + STRING_format(str) + "\":" + STRING_format(c) + "}"); }
        void print(const char str[], unsigned char b)       { print("{\"" + STRING_format(str) + "\":" + STRING_format(b) + "}"); }
        void print(const char str[], int n)                 { print("{\"" + STRING_format(str) + "\":" + STRING_format(n) + "}"); }
        void print(const char str[], unsigned int n)        { print("{\"" + STRING_format(str) + "\":" + STRING_format(n) + "}"); }
        void print(const char str[], long n)                { print("{\"" + STRING_format(str) + "\":" + STRING_format(n) + "}"); }
        void print(const char str[], unsigned long n)       { print("{\"" + STRING_format(str) + "\":" + STRING_format(n) + "}"); }
        void print(const char str[], double n)              { print("{\"" + STRING_format(str) + "\":" + STRING_format(n) + "}"); }
        void println(const String &s1, const String &s2)    { print(s1, s2); }
        void println(const char str1[], const char str2[])  { print(str1, str2); }
        void println(const char str[], char c)              { print(str, c); }
        void println(const char str[], unsigned char b)     { print(str, b); }
        void println(const char str[], int n)               { print(str, n); }
        void println(const char str[], unsigned int n)      { print(str, n); }
        void println(const char str[], long n)              { print(str, n); }
        void println(const char str[], unsigned long n)     { print(str, n); }
        void println(const char str[], double n)            { print(str, n); }
        
        void flush() {
            isFresh = false;
            availState = false;
        }

    private :
        bool checkAvail()
        {
            isAvail = conn.available();
            if (isAvail) {
                isFresh = true;
                canParse = true;
                availState = true;
            }
            return isAvail;
        }

        String dataParse()
        {
            if (canParse) {
                return conn.lastRead();
            }
            else {
                return "";
            }
        }

        void isParsed() { isFresh = false; canParse = false; availState = false; }// BLINKER_LOG1("isParsed");

        bool parseState() { return canParse; }

    protected :
        Transp&         conn;
        BlinkerState    state;
        bool            isFresh;
        bool            isAvail;
        bool            availState;
        bool            canParse;
        
        void begin()
        {
            BLINKER_LOG1(BLINKER_F(""));
        #if defined(BLINKER_NO_LOGO)
            BLINKER_LOG2(BLINKER_F("Blinker v"), BLINKER_VERSION);
        #elif defined(BLINKER_LOGO_3D)
            BLINKER_LOG1(BLINKER_F("\n"
                " ____    ___                __                       \n"
                "/\\  _`\\ /\\_ \\    __        /\\ \\               v"BLINKER_VERSION"\n"
                "\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"
                " \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"
                "  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"
                "   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"
                "    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"));
        #else
            BLINKER_LOG1(BLINKER_F("\n"
                "   ___  ___      __    v"BLINKER_VERSION"\n"
                "  / _ )/ (_)__  / /_____ ____\n"
                " / _  / / / _ \\/  '_/ -_) __/\n"
                "/____/_/_/_//_/_/\\_\\\\__/_/   \n"));
        #endif
        }
};

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
    bool conState = conn.connected();

    switch (state)
    {
        case CONNECTING :
            if (conn.connect()) {
                state = CONNECTED;
            }
            break;
        case CONNECTED :
            if (conState) {
                checkAvail();
                if (isAvail) {
                    BApi::parse();
                }
            }
            else {
                state = DISCONNECTED;
            }
            break;
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;
    }
}

#endif