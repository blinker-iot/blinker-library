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
            , isFormat(false)
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
        
//         template <typename T>
// #if defined(BLINKER_MQTT)
//         void print(T n, bool state = false) {
//             String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
//             if (data.length() <= BLINKER_MAX_SEND_SIZE) {
//                 conn.print(data, state);
//             }
//             else {
//                 BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//         }
// #else
//         void print(T n) {
//             String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
//             if (data.length() <= BLINKER_MAX_SEND_SIZE) {
//                 conn.print(data);
//             }
//             else {
//                 BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//             _print(n);
//         }
// #endif
        void beginFormat() {
            isFormat = true;
            memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
        }

        void endFormat() {
            isFormat = false;
            _print("{" + STRING_format(_sendBuf) + "}");
        }

        template <typename T>
        void print(T n) {
            if (!isFormat)
                _print(n);
        }
        void print() {
            if (!isFormat)
                _print("");
        }
        
        template <typename T>
        void println(T n) {
            if (!isFormat)
                _print(n);
        }
        void println() {
            if (!isFormat)
                _print("");
        }

        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }
        
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        // template <typename T1, typename T2>
        // void print(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        
        // template <typename T1, typename T2>
        // void println(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        
        void print(const String &s1, const String &s2) {
            String _msg = "\"" + s1 + "\":\"" + s2 + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str1[], const char str2[]) {
            String _msg = "\"" + STRING_format(str1) + "\":\"" + STRING_format(str2) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], char c) {
            String _msg = "\"" + STRING_format(str) + "\":\"" + STRING_format(c) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], unsigned char b) {
            String _msg = "\"" + STRING_format(str) + "\":\"" + STRING_format(b) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], int n) {
            String _msg = "\"" + STRING_format(str) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], unsigned int n) {
            String _msg = "\"" + STRING_format(str) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], long n) {
            String _msg = "\"" + STRING_format(str) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], unsigned long n) {
            String _msg = "\"" + STRING_format(str) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void print(const char str[], double n) {
            String _msg = "\"" + STRING_format(str) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        void println(const String &s1, const String &s2)    { print(s1, s2); }
        void println(const char str1[], const char str2[])  { print(str1, str2); }
        void println(const char str[], char c)              { print(str, c); }
        void println(const char str[], unsigned char b)     { print(str, b); }
        void println(const char str[], int n)               { print(str, n); }
        void println(const char str[], unsigned int n)      { print(str, n); }        
        void println(const char str[], long n)              { print(str, n); }        
        void println(const char str[], unsigned long n)     { print(str, n); }        
        void println(const char str[], double n)            { print(str, n); }
        
        template <typename T>
        void notify(T n) {
            String _msg = "\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"";
            if (isFormat) {

            }
            else {
                _print("{" + _msg + "}");
            }
// #if defined(BLINKER_MQTT)
//             print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}", true);
// #else
//             _print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}");
// #endif
        }
        
        void flush() {
            isFresh = false;
            availState = false;
        }

    private :
        template <typename T>
        void _print(T n) {
            String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
            if (data.length() <= BLINKER_MAX_SEND_SIZE) {
                conn.print(data);
            }
            else {
                BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
            }
        }

        void formatData(String data) {
            if (strlen(_sendBuf) > 0) {
                data = "," + data;
                strcat(_sendBuf, data.c_str());
            }
            else {
                strcpy(_sendBuf, data.c_str());
            }
        }

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
        bool            isFormat;
        char            _sendBuf[BLINKER_MAX_SEND_SIZE];
        
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