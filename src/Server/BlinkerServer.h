#ifndef BLINKER_SERVER_H
#define BLINKER_SERVER_H

#ifndef BLINKER_SERVER_HTTPS
    #ifndef BLINKER_WITHOUT_SSL
        #define BLINKER_SERVER_HTTPS    "https://iot.diandeng.tech"
    #else
        #define BLINKER_SERVER_HTTPS    "http://iot.diandeng.tech"
    #endif
#endif

#ifndef BLINKER_SERVER_HOST
    #define BLINKER_SERVER_HOST         "iot.diandeng.tech"
#endif

#ifndef BLINKER_STORAGE_HTTP
    #ifndef BLINKER_WITHOUT_SSL
        #define BLINKER_STORAGE_HTTP    "https://storage.diandeng.tech"
    #else
        #define BLINKER_STORAGE_HTTP    "http://storage.diandeng.tech"
    #endif
#endif

#endif