#ifndef BlinkerConfig_H
#define BlinkerConfig_H

// #include "Blinker/BlinkerDebug.h"
// #include "Blinker/BlinkerUtility.h"

#define BLINKER_VERSION                 "0.3.2"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_NTP_TIMEOUT             1000UL

#define BLINKER_GPS_MSG_LIMIT           30000UL

#define BLINKER_PRINT_MSG_LIMIT         20

#define BLINKER_MQTT_MSG_LIMIT          1000UL

#define BLINKER_PRO_MSG_LIMIT           200UL

#define BLINKER_MQTT_CONNECT_TIMESLOT   5000UL

#define BLINKER_BRIDGE_MSG_LIMIT        10000UL

#define BLINKER_LINK_MSG_LIMIT          10000UL

#define BLINKER_MQTT_KEEPALIVE          30000UL

#define BLINKER_SMS_MSG_LIMIT           60000UL

#define BLINKER_PUSH_MSG_LIMIT          60000UL

#define BLINKER_WECHAT_MSG_LIMIT        60000UL

#define BLINKER_WEATHER_MSG_LIMIT       60000UL

#define BLINKER_AQI_MSG_LIMIT           60000UL

#define BLINKER_CONFIG_UPDATE_LIMIT     10000UL

#define BLINKER_CONFIG_GET_LIMIT        10000UL

#define BLINKER_WIFI_AUTO_INIT_TIMEOUT  20000UL

#define BLINKER_AT_MSG_TIMEOUT          1000UL

#define BLINKER_SERVER_CONNECT_LIMIT    12

#if defined(BLINKER_DATA_HOUR_UPDATE)
    #define BLINKER_DATA_FREQ_TIME          3600UL
#else
    #define BLINKER_DATA_FREQ_TIME          60
#endif

#define BLINKER_DEVICE_HEARTBEAT_TIME   600

#define BLINKER_MDNS_SERVICE_BLINKER    "blinker"

#define BLINKER_ERROR                   0x00

#define BLINKER_SUCCESS                 0x01

#define BLINKER_MSG_FROM_MQTT           0

#define BLINKER_MSG_FROM_WS             1

// #if defined(BLINKER_MQTT)
//     #define BLINKER_MSG_AUTOFORMAT_TIMEOUT  1000
// #else
    #define BLINKER_MSG_AUTOFORMAT_TIMEOUT  100
// #endif

#define BLINKER_SMS_MAX_SEND_SIZE       128

#if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
    #define BLINKER_PRESSTIME_POWERDOWN     2000UL
    
    #define BLINKER_PRESSTIME_RESET         10000UL
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_WIFI_SUBDEVICE)
    #define BLINKER_MAX_WIDGET_SIZE         16
#else
    #define BLINKER_MAX_WIDGET_SIZE         6
#endif

#define BLINKER_OBJECT_NOT_AVAIL        -1

#ifndef BLINKER_MAX_READ_SIZE
    #if defined(ESP8266) || defined(ESP32)
        #define BLINKER_MAX_READ_SIZE       1024
    #else
        // #if defined(BLINKER_MQTT_AT)
        //     #define BLINKER_MAX_READ_SIZE       256
        // #else
            #define BLINKER_MAX_READ_SIZE       128
        // #endif
    #endif
#endif

#ifndef BLINKER_MAX_SEND_SIZE
    #if defined(ESP8266) || defined(ESP32)
        #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT) || \
            defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_PRO_SIM7020) || \
            defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_WIFI_SUBDEVICE) || defined(BLINKER_PRO_ESP)
            #define BLINKER_MAX_SEND_SIZE       1024
        #else
            #define BLINKER_MAX_SEND_SIZE       512
        #endif
    #else
        #if defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_GPRS_AIR202) || \
            defined(BLINKER_LOWPOWER_AIR202)
            #define BLINKER_MAX_SEND_SIZE       512
        #else
            #define BLINKER_MAX_SEND_SIZE       128
        #endif
    #endif
#endif

#ifndef BLINKER_MAX_SEND_BUFFER_SIZE
    #if defined(ESP8266) || defined(ESP32)
        #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT) || \
            defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_PRO_SIM7020) || \
            defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_WIFI_SUBDEVICE)
            #define BLINKER_MAX_SEND_BUFFER_SIZE       BLINKER_MAX_SEND_SIZE - 128
        #else
            #define BLINKER_MAX_SEND_BUFFER_SIZE       BLINKER_MAX_SEND_SIZE
        #endif
    #else
        #if defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_GPRS_AIR202) || \
            defined(BLINKER_LOWPOWER_AIR202)
            #define BLINKER_MAX_SEND_BUFFER_SIZE       512
        #else
            #define BLINKER_MAX_SEND_BUFFER_SIZE       128
        #endif
    #endif
#endif

#define BLINKER_AUTHKEY_SIZE            14

#if defined(ESP8266) || defined(ESP32)
    #define BLINKER_LOGO_3D    
#else
    #define BLINKER_NO_LOGO
#endif

#define BLINKER_MAX_SUMMARY_DATA_SIZE   20

// #define	BLINKER_DEBUG

#define BLINKER_CMD_ON                  "on"

#define BLINKER_CMD_OFF                 "off"

#define BLINKER_CMD_TRUE                "true"

#define BLINKER_CMD_FALSE               "false"

#define BLINKER_CMD_JOYSTICK            "joy"

#define BLINKER_CMD_GYRO                "gyro"

#define BLINKER_CMD_AHRS                "ahrs"

#define BLINKER_CMD_GPS                 "gps"

#define BLINKER_CMD_RGB                 "rgb"

#define BLINKER_CMD_VIBRATE             "vibrate"

#define BLINKER_CMD_BUTTON_TAP          "tap"

#define BLINKER_CMD_BUTTON_PRESSED      "press"

#define BLINKER_CMD_BUTTON_RELEASED     "pressup"

#define BLINKER_CMD_BUTTON_PRESS        "press"

#define BLINKER_CMD_BUTTON_PRESSUP      "pressup"

#define BLINKER_CMD_NEWLINE             "\n"

#define BLINKER_CMD_INTERSPACE          " "

#define BLINKER_CMD_DATA                "data"

#define BLINKER_CMD_FREQ                "freq"

#define BLINKER_CMD_GET                 "get"

#define BLINKER_CMD_SET                 "set"

#define BLINKER_CMD_STATE               "state"

#define BLINKER_CMD_ONLINE              "online"

#define BLINKER_CMD_CONNECTED           "connected"

#define BLINKER_CMD_VERSION             "version"

#define BLINKER_CMD_NOTICE              "notice"

#define BLINKER_CMD_BUILTIN_SWITCH      "switch"

#define BLINKER_CMD_FROMDEVICE          "fromDevice"

#define BLINKER_CMD_NOTFOUND            "device not found"

#define BLINKER_CMD_COMMAND             "cmd"

#define BLINKER_CMD_EVENT               "event"

#define BLINKER_CMD_AUTO                "auto"

#define BLINKER_CMD_AUTOID              "autoId"

#define BLINKER_CMD_ID                  "id"

#define BLINKER_CMD_AUTODATA            "autoData"

#define BLINKER_CMD_DELETID             "deletId"

#define BLINKER_CMD_LOGIC               "logic"

#define BLINKER_CMD_LOGICDATA           "logicData"

#define BLINKER_CMD_LOGICTYPE           "logicType"

#define BLINKER_CMD_LESS                "<"//"less"

#define BLINKER_CMD_EQUAL               "="//"equal"

#define BLINKER_CMD_GREATER             ">"//"greater"

#define BLINKER_CMD_NUMBERIC            "numberic"

#define BLINKER_CMD_OR                  "or"

#define BLINKER_CMD_AND                 "and"

#define BLINKER_CMD_COMPARETYPE         "compareType"

#define BLINKER_CMD_TRIGGER             "trigger"

#define BLINKER_CMD_DURATION            "duration"

#define BLINKER_CMD_TARGETKEY           "targetKey"

#define BLINKER_CMD_TARGETSTATE         "targetState"

#define BLINKER_CMD_TARGETDATA          "targetData"

#define BLINKER_CMD_TIMESLOT            "timeSlot"

#define BLINKER_CMD_RANGE               "range"

#define BLINKER_CMD_LINKDEVICE          "linkDevice"

#define BLINKER_CMD_LINKTYPE            "linkType"

#define BLINKER_CMD_LINKDATA            "linkData"

#define BLINKER_CMD_TRIGGEDDATA         "triggedData"

#define BLINKER_CMD_TYPE                "type"

#define BLINKER_CMD_TIMER               "timer"

#define BLINKER_CMD_RUN                 "run"

#define BLINKER_CMD_ENABLE              "ena"

#define BLINKER_CMD_COUNTDOWN           "countdown"

#define BLINKER_CMD_COUNTDOWNDATA       "countdownData"

#define BLINKER_CMD_TOTALTIME           "ttim"

#define BLINKER_CMD_RUNTIME             "rtim"

#define BLINKER_CMD_ACTION              "act"

#define BLINKER_CMD_ACTION1             "act1"

#define BLINKER_CMD_ACTION2             "act2"

#define BLINKER_CMD_LOOP                "loop"

#define BLINKER_CMD_LOOPDATA            "loopData"

#define BLINKER_CMD_TIME                "tim"

#define BLINKER_CMD_TIME_ALL            "time"

#define BLINKER_CMD_TIMES               "tis"

#define BLINKER_CMD_TRIGGED             "tri"

#define BLINKER_CMD_TIME1               "dur1"

#define BLINKER_CMD_TIME2               "dur2"

#define BLINKER_CMD_TIMING              "timing"

#define BLINKER_CMD_TIMINGDATA          "timingData"

#define BLINKER_CMD_DAY                 "day"

#define BLINKER_CMD_TASK                "task"

#define BLINKER_CMD_DELETETASK          "dlt"

#define BLINKER_CMD_DELETE              "dlt"

#define BLINKER_CMD_DETAIL              "detail"

#define BLINKER_CMD_OK                  "OK"

#define BLINKER_CMD_ERROR               "ERROR"

#define BLINKER_CMD_MESSAGE             "message"

#define BLINKER_CMD_DEVICENAME          "deviceName"

#define BLINKER_CMD_AUTHKEY             "authKey"

#define BLINKER_CMD_IOTID               "iotId"

#define BLINKER_CMD_IOTTOKEN            "iotToken"

#define BLINKER_CMD_PRODUCTKEY          "productKey"

#define BLINKER_CMD_BROKER              "broker"

#define BLINKER_CMD_UUID                "uuid"

#define BLINKER_CMD_KEY                 "key"

#define BLINKER_CMD_SMS                 "sms"

#define BLINKER_CMD_PUSH                "push"

#define BLINKER_CMD_WECHAT              "wechat"

#define BLINKER_CMD_WEATHER             "weather"

#define BLINKER_CMD_AQI                 "aqi"

#define BLINKER_CMD_CONFIG              "config"

#define BLINKER_CMD_DEFAULT             "default"

#define BLINKER_CMD_SWITCH              "swi"

#define BLINKER_CMD_VALUE               "val"

#define BLINKER_CMD_ICON                "ico"

#define BLINKER_CMD_COLOR               "clr"

#define BLINKER_CMD_COLOR_              "col"

#define BLINKER_CMD_TITLE               "tit"

#define BLINKER_CMD_CONTENT             "con"

#define BLINKER_CMD_TEXT                "tex"

#define BLINKER_CMD_TEXT1               "tex1"

#define BLINKER_CMD_TEXTCOLOR           "tco"

#define BLINKER_CMD_UNIT                "uni"

#define BLINKER_CMD_SUMMARY             "sum"

#define BLINKER_CMD_POWERSTATE          "pState"

#define BLINKER_CMD_NUM                 "num"

#define BLINKER_CMD_BRIGHTNESS          "bright"

#define BLINKER_CMD_UPBRIGHTNESS        "upBright"

#define BLINKER_CMD_DOWNBRIGHTNESS      "downBright"

#define BLINKER_CMD_COLORTEMP           "colTemp"

#define BLINKER_CMD_UPCOLORTEMP         "upColTemp"

#define BLINKER_CMD_DOWNCOLORTEMP       "downColTemp"

#define BLINKER_CMD_TEMP                "temp"

#define BLINKER_CMD_HUMI                "humi"

#define BLINKER_CMD_PM25                "pm25"

#define BLINKER_CMD_PM10                "pm10"

#define BLINKER_CMD_CO2                 "co2"

#define BLINKER_CMD_MAX                 "max"

#define BLINKER_CMD_MIN                 "min"

#define BLINKER_CMD_MODE                "mode"

#define BLINKER_CMD_CANCELMODE          "cMode"

#define BLINKER_CMD_READING             "reading"

#define BLINKER_CMD_MOVIE               "movie"

#define BLINKER_CMD_SLEEP               "sleep"

#define BLINKER_CMD_HOLIDAY             "holiday"

#define BLINKER_CMD_MUSIC               "music"

#define BLINKER_CMD_COMMON              "common"

#define BLINKER_CMD_ALIGENIE_READING    "reading"

#define BLINKER_CMD_ALIGENIE_MOVIE      "movie"

#define BLINKER_CMD_ALIGENIE_SLEEP      "sleep"

#define BLINKER_CMD_ALIGENIE_HOLIDAY    "holiday"

#define BLINKER_CMD_ALIGENIE_MUSIC      "music"

#define BLINKER_CMD_ALIGENIE_COMMON     "common"

#define BLINKER_CMD_DUEROS_READING      "READING"

#define BLINKER_CMD_DUEROS_SLEEP        "SLEEP"

#define BLINKER_CMD_DUEROS_ALARM        "ALARM"

#define BLINKER_CMD_DUEROS_NIGHT_LIGHT  "NIGHT_LIGHT"

#define BLINKER_CMD_DUEROS_ROMANTIC     "ROMANTIC"

#define BLINKER_CMD_DUEROS_SUNDOWN      "SUNDOWN"

#define BLINKER_CMD_DUEROS_SUNRISE      "SUNRISE"

#define BLINKER_CMD_DUEROS_RELAX        "RELAX"

#define BLINKER_CMD_DUEROS_LIGHTING     "LIGHTING"

#define BLINKER_CMD_DUEROS_SUN          "SUN"

#define BLINKER_CMD_DUEROS_STAR         "STAR"

#define BLINKER_CMD_DUEROS_ENERGY_SAVING "ENERGY_SAVING"

#define BLINKER_CMD_DUEROS_MOON         "MOON"

#define BLINKER_CMD_DUEROS_JUDI         "JUDI"

#define BLINKER_CMD_MIOT_DAY            0

#define BLINKER_CMD_MIOT_NIGHT          1

#define BLINKER_CMD_MIOT_COLOR          2

#define BLINKER_CMD_MIOT_WARMTH         3

#define BLINKER_CMD_MIOT_TV             4

#define BLINKER_CMD_MIOT_READING        5

#define BLINKER_CMD_MIOT_COMPUTER       6

#define BLINKER_CMD_UPGRADE             "upgrade"

#define BLINKER_CMD_SHARE               "share"

#define BLINKER_CMD_AUTO_UPDATE_KEY     "upKey"

#define BLINKER_CMD_CANCEL_UPDATE_KEY   "cKey"

#define BLINKER_CMD_ALIGENIE            "AliGenie"

#define BLINKER_CMD_DUEROS              "DuerOS"

#define BLINKER_CMD_MIOT                "MIOT"

#define BLINKER_CMD_SERVERCLIENT        "serverClient"

#define BLINKER_CMD_HELLO               "hello"

// #define BLINKER_CMD_WHOIS               "whois"

#define BLINKER_CMD_AT                  "AT"

#define BLINKER_CMD_GATE                "gate"

#define BLINKER_CMD_CONTROL             "ctrl"

#define BLINKER_CMD_DEVICEINFO          "dInf"

#define BLINKER_CMD_NEW                 "{\"hello\":\"new\"}"

#define BLINKER_CMD_WHOIS               "{\"hello\":\"whois\"}"

#define BLINKER_MESH_CHECK_FREQ         60000UL

#define BLINKER_CMD_MODE_READING_NUMBER         0

#define BLINKER_CMD_MODE_MOVIE_NUMBER           1

#define BLINKER_CMD_SLEEP_NUMBER                2

#define BLINKER_CMD_HOLIDAY_NUMBER              3

#define BLINKER_CMD_MUSIC_NUMBER                4

#define BLINKER_CMD_COMMON_NUMBER               5

#define BLINKER_CMD_QUERY_ALL_NUMBER            0

#define BLINKER_CMD_QUERY_POWERSTATE_NUMBER     1

#define BLINKER_CMD_QUERY_COLOR_NUMBER          2

#define BLINKER_CMD_QUERY_MODE_NUMBER           3

#define BLINKER_CMD_QUERY_COLORTEMP_NUMBER      4

#define BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER     5

#define BLINKER_CMD_QUERY_TEMP_NUMBER           6

#define BLINKER_CMD_QUERY_HUMI_NUMBER           7

#define BLINKER_CMD_QUERY_PM25_NUMBER           8

#define BLINKER_CMD_QUERY_PM10_NUMBER           9

#define BLINKER_CMD_QUERY_CO2_NUMBER            10

#define BLINKER_CMD_QUERY_AQI_NUMBER            11

#define BLINKER_CMD_QUERY_TIME_NUMBER           12

#define BLINKER_JOYSTICK_VALUE_DEFAULT          128

#define BLINKER_ONE_HOUR_TIME                   3600UL

#define BLINKER_ONE_DAY_TIME                    86400UL

#define BLINKER_CMD_TAB_0                       16 // 0x10000

#define BLINKER_CMD_TAB_1                       8  // 0x01000

#define BLINKER_CMD_TAB_2                       4  // 0x00100

#define BLINKER_CMD_TAB_3                       2  // 0x00010

#define BLINKER_CMD_TAB_4                       1  // 0x00001

#define BLINKER_MAX_SUB_DEVICE_NUM              36

// #define BLINKER_NTP_SERVER_1                    "ntp1.aliyun.com"

// #define BLINKER_NTP_SERVER_2                    "210.72.145.44"

// #define BLINKER_NTP_SERVER_3                    "time.pool.aliyun.com"

#ifndef BLINKER_MAX_BRIDGE_SIZE
    #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT) || \
        defined(BLINKER_MQTT_AUTO)
        #define BLINKER_MAX_BRIDGE_SIZE             4
    #elif defined(BLINKER_PRO) || defined(BLINKER_MQTT_AUTO) || \
        defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY) || \
        defined(BLINKER_WIFI_SUBDEVICE)
        #define BLINKER_MAX_BRIDGE_SIZE             16
    #endif
#endif

#define BLINKER_MAX_BLINKER_DATA_SIZE   8

#define BLINKER_MAX_DATA_COUNT          4

#define BLINKER_DATA_UPDATE_COUNT       2

#if defined(BLINKER_ESP_AT)

    #define BLINKER_ESP_AT_VERSION              "0.1.0"

    #define BLINKER_UART_PARAM_NUM              4

    #define BLINKER_COMWLAN_PARAM_NUM           4

    #define BLINKER_SMCFG_PARAM_NUM             2

    #define BLINKER_APCFG_PARAM_NUM             2

    #define BLINKER_IOSETCFG_PARAM_NUM          3

    #define BLINKER_IOGETCFG_PARAM_NUM          1

    #define BLINKER_GPIOWRITE_PARAM_NUM         2

    #define BLINKER_GPIOREAD_PARAM_NUM          1

    #define BLINKER_IO_INPUT_NUM                0

    #define BLINKER_IO_OUTPUT_NUM               1

    #define BLINKER_IO_DEFAULT_NUM              2

    #define BLINKER_ALIGENIE_CFG_NUM            0

    #define BLINKER_ALIGENIE_PARAM_NUM          1    

    #define BLINKER_DUEROS_CFG_NUM              0

    #define BLINKER_DUEROS_PARAM_NUM            1

    #define BLINKER_TIMEZONE_CFG_NUM            0

    #define BLINKER_TIMEZONE_PARAM_NUM          1

    #if defined(ESP8266)
        
        #define BLINKER_MAX_PIN_NUM                 17

    #elif defined(ESP32)

        #define BLINKER_MAX_PIN_NUM                 40

    #endif

    #define BLINKER_CMD_RST                     "RST"

    #define BLINKER_CMD_GMR                     "GMR"

    #define BLINKER_CMD_UART_CUR                "UART_CUR"

    #define BLINKER_CMD_UART_DEF                "UART_DEF"

    #define BLINKER_CMD_RAM                     "SYSRAM"

    #define BLINKER_CMD_ADC                     "SYSADC"

    #define BLINKER_CMD_IOSETCFG                "SYSIOSETCFG"

    #define BLINKER_CMD_IOGETCFG                "SYSIOGETCFG"

    #define BLINKER_CMD_GPIOWRITE               "SYSGPIOWRITE"

    #define BLINKER_CMD_GPIOWREAD               "SYSGPIOREAD"

    #define BLINKER_CMD_BLINKER_MQTT            "BLINKER_WIFI"

    #define BLINKER_CMD_BLINKER_ALIGENIE        "BLINKER_ALIGENIE"

    #define BLINKER_CMD_BLINKER_DUEROS          "BLINKER_DUEROS"

    #define BLINKER_CMD_BLINKER_MIOT            "BLINKER_MIOT"
    
    #define BLINKER_CMD_TIMEZONE                "TIMEZONE"

    #define BLINKER_CMD_TIME_AT                 "TIME"

    #define BLINKER_CMD_SECOND                  "SECOND"

    #define BLINKER_CMD_MINUTE                  "MINUTE"

    #define BLINKER_CMD_HOUR                    "HOUR"

    #define BLINKER_CMD_WDAY                    "WDAY"

    #define BLINKER_CMD_MDAY                    "MDAY"

    #define BLINKER_CMD_YDAY                    "YDAY"

    #define BLINKER_CMD_MONTH                   "MONTH"

    #define BLINKER_CMD_YEAR                    "YEAR"

    #define BLINKER_CMD_WEATHER_AT              "WEATHER"

    #define BLINKER_CMD_AQI_AT                  "AQI"

    #define BLINKER_CMD_NOTICE_AT               "NOTIFY"

    #define BLINKER_CMD_SMS_AT                  "SMS"

    #define BLINKER_CMD_SMARTCONFIG             "ESPTOUCH"

    #define BLINKER_CMD_APCONFIG                "APCONFIG"

    #define BLINKER_CMD_COMCONFIG               "COMCONFIG"

    #define BLINKER_SERIAL_DEFAULT              9600 << 8 | 8 << 4 | 1 << 2 | 0

    #define BLINKER_SERIAL_5N1                  5 << 4 | 1 << 2 | 0

    #define BLINKER_SERIAL_6N1                  6 << 4 | 1 << 2 | 0

    #define BLINKER_SERIAL_7N1                  7 << 4 | 1 << 2 | 0

    #define BLINKER_SERIAL_8N1                  8 << 4 | 1 << 2 | 0

    #define BLINKER_SERIAL_5N2                  5 << 4 | 2 << 2 | 0

    #define BLINKER_SERIAL_6N2                  6 << 4 | 2 << 2 | 0

    #define BLINKER_SERIAL_7N2                  7 << 4 | 2 << 2 | 0

    #define BLINKER_SERIAL_8N2                  8 << 4 | 2 << 2 | 0

    #define BLINKER_SERIAL_5E1                  5 << 4 | 1 << 2 | 2

    #define BLINKER_SERIAL_6E1                  6 << 4 | 1 << 2 | 2

    #define BLINKER_SERIAL_7E1                  7 << 4 | 1 << 2 | 2

    #define BLINKER_SERIAL_8E1                  8 << 4 | 1 << 2 | 2

    #define BLINKER_SERIAL_5E2                  5 << 4 | 2 << 2 | 2

    #define BLINKER_SERIAL_6E2                  6 << 4 | 2 << 2 | 2

    #define BLINKER_SERIAL_7E2                  7 << 4 | 2 << 2 | 2

    #define BLINKER_SERIAL_8E2                  8 << 4 | 2 << 2 | 2

    #define BLINKER_SERIAL_5O1                  5 << 4 | 1 << 2 | 1

    #define BLINKER_SERIAL_6O1                  6 << 4 | 1 << 2 | 1

    #define BLINKER_SERIAL_7O1                  7 << 4 | 1 << 2 | 1

    #define BLINKER_SERIAL_8O1                  8 << 4 | 1 << 2 | 1

    #define BLINKER_SERIAL_5O2                  5 << 4 | 2 << 2 | 1

    #define BLINKER_SERIAL_6O2                  6 << 4 | 2 << 2 | 1

    #define BLINKER_SERIAL_7O2                  7 << 4 | 2 << 2 | 1

    #define BLINKER_SERIAL_8O2                  8 << 4 | 2 << 2 | 1

    // #define BLINKER_EEP_ADDR_SERIALCFG          2432

    // #define BLINKER_SERIALCFG_SIZE              4
    
#endif

#if defined(BLINKER_MQTT_AT)
    
    #define BLINKER_MQTT_HOST_SIZE          50

    #define BLINKER_MQTT_ID_SIZE            110

    #define BLINKER_MQTT_NAME_SIZE          80

    #define BLINKER_MQTT_KEY_SIZE           50

    #define BLINKER_MQTT_PINFO_SIZE         12

    #define BLINKER_MQTT_UUID_SIZE          40

    #define BLINKER_MQTT_DEVICEID_SIZE      26

    #define BLINKER_MQTT_DEVICENAME_SIZE    14

#endif

#define BLINKER_CMD_COMCONFIG_NUM           0

#define BLINKER_CMD_SMARTCONFIG_NUM         1

#define BLINKER_CMD_APCONFIG_NUM            2

#if defined(BLINKER_NBIOT_WH)
/*

>[Success][17:20:49:705]
COM14 (9600,None,_8,_1,148) Opend 
>[Tx ->][17:21:06:364][Asc]
AT+CGATT?

>[Rx <-][17:21:06:548][Asc]

+CGATT:1

OK

>[Success][17:21:06:565]
Success--[AT+CGATT?]
>[Tx ->][17:21:06:565][Asc]
AT+MIPLCREATE

>[Rx <-][17:21:06:776][Asc]

+CIS ERROR:602

>[Warn][17:21:06:781]
Fail--[AT+MIPLCREATE]
>[Tx ->][17:21:10:595][Asc]
AT+MIPLDELETE=0

>[Rx <-][17:21:11:015][Asc]

+MIPLEVENT:0,15

>[Rx <-][17:21:11:291][Asc]

OK

>[Success][17:21:11:315]
Success--[AT+MIPLDELETE=0]
>[Tx ->][17:21:16:320][Asc]
AT+CGATT?

>[Rx <-][17:21:16:513][Asc]

+CGATT:1

OK

>[Success][17:21:16:533]
Success--[AT+CGATT?]
>[Tx ->][17:21:16:533][Asc]
AT+MIPLCREATE

>[Rx <-][17:21:16:760][Asc]

+MIPLCREATE:0

OK

>[Success][17:21:16:781]
Success--[AT+MIPLCREATE]
>[Tx ->][17:21:16:797][Asc]
AT+MIPLADDOBJ=0,3306,1,1,5,0

>[Rx <-][17:21:17:015][Asc]

OK

>[Success][17:21:17:048]
Success--[AT+MIPLADDOBJ=0,3306,1,1,5,0]
>[Tx ->][17:21:17:055][Asc]
AT+MIPLADDOBJ=0,3303,1,1,6,1

>[Rx <-][17:21:17:260][Asc]

OK

>[Success][17:21:17:294]
Success--[AT+MIPLADDOBJ=0,3303,1,1,6,1]
>[Tx ->][17:21:17:301][Asc]
AT+MIPLOPEN=0,3000

>[Rx <-][17:21:17:490][Asc]

OK

>[Success][17:21:17:526]
Success--[AT+MIPLOPEN=0,3000]
>[Rx <-][17:21:18:016][Asc]

+MIPLEVENT:0,1

>[Rx <-][17:21:21:031][Asc]

+MIPLEVENT:0,2

+MIPLEVENT:0,4

>[Rx <-][17:21:24:595][Asc]

+MIPLEVENT:0,6

+MIPLOBSERVE:0,88436,1,3303,0,-1

>[Tx ->][17:21:25:548][Asc]
AT+MIPLOBSERVERSP=0,88436,1

>[Rx <-][17:21:25:752][Asc]

OK

>[Success][17:21:25:796]
Success--[AT+MIPLOBSERVERSP=0,88436,1]
>[Tx ->][17:21:26:922][Asc]
AT+MIPLNOTIFY=0,88436,3303,0,5700,4,2,13,1,0

>[Rx <-][17:21:27:158][Asc]

OK

>[Success][17:21:27:189]
Success--[AT+MIPLNOTIFY=0,88436,3303,0,5700,4,2,13,1,0]
>[Tx ->][17:21:28:339][Asc]
AT+MIPLNOTIFY=0,88436,3303,0,5701,1,3,"cel",0,0

>[Rx <-][17:21:28:564][Asc]

OK

>[Success][17:21:28:595]
Success--[AT+MIPLNOTIFY=0,88436,3303,0,5701,1,3,"cel",0,0]
>[Rx <-][17:21:29:476][Asc]

+MIPLOBSERVE:0,153973,1,3306,0,-1

>[Tx ->][17:21:29:611][Asc]
AT+MIPLOBSERVERSP=0,153973,1

>[Rx <-][17:21:29:828][Asc]

OK

>[Success][17:21:29:875]
Success--[AT+MIPLOBSERVERSP=0,153973,1]
>[Rx <-][17:21:30:796][Asc]

+MIPLDISCOVER:0,22902,3303

>[Success][17:21:30:843]
Success--[AT+MIPLNOTIFY=0,153973,3306,0,5850,5,1,0,1,0]
>[Tx ->][17:21:32:048][Asc]
AT+MIPLNOTIFY=0,153973,3306,0,5851,3,2,76,0,0

>[Rx <-][17:21:32:356][Asc]

OK

>[Success][17:21:32:411]
Success--[AT+MIPLNOTIFY=0,153973,3306,0,5851,3,2,76,0,0]
>[Tx ->][17:21:32:418][Asc]
AT+MIPLDISCOVERRSP=0,22902,1,34,"5700;5701;5601;5602;5603;5604;5605"

>[Tx ->][17:21:32:564][Asc]
AT+MIPLNOTIFY=0,153973,3306,0,5850,5,1,0,1,0

>[Success][17:21:32:564]
Success--[AT+MIPLNOTIFY=0,153973,3306,0,5850,5,1,0,1,0]
>[Rx <-][17:21:32:797][Asc]

OK

OK

>[Success][17:21:32:906]
Success--[AT+MIPLDISCOVERRSP=0,22902,1,34,"5700;5701;5601;5602;5603;5604;5605"]
>[Rx <-][17:21:33:548][Asc]

+MIPLDISCOVER:0,22903,3306

>[Tx ->][17:21:33:922][Asc]
AT+MIPLDISCOVERRSP=0,22903,1,24,"5850;5851;5852;5853;5750"

>[Rx <-][17:21:34:174][Asc]

OK

>[Success][17:21:34:238]
Success--[AT+MIPLDISCOVERRSP=0,22903,1,24,"5850;5851;5852;5853;5750"]

*/

    // http://www.mokuai.cn/Down/WH-NB73_al_onenet_V1.0.0.pdf
    // http://www.mokuai.cn/Down/WH-NB73_at_V2.2.1.pdf
    // http://www.openmobilealliance.org/wp/OMNA/LwM2M/LwM2MRegistry.html
    #define BLINKER_NB_STREAM_TIMEOUT           60000UL

    #define BLINKER_NB_LIFTIME                  1800UL

    #define BLINKER_CMD_NB_NB73                 "[WH-NB73]"
    
    #define BLINKER_CMD_NB_RESET                "AT+NRB"

    #define BLINKER_CMD_NB_CGATT                "AT+CGATT?"

    #define BLINKER_CMD_CGATT                   "CGATT"
    // Waiting module attachment network
    #define BLINKER_CMD_NB_CGATT_SUCCESSED      "+CGATT:1"

    #define BLINKER_CMD_NB_CGATT_FAILED         "+CGATT:0"

    #define BLINKER_CMD_NB_MIPLCREATE           "AT+MIPLCREATE"
    // Create communication kite
    #define BLINKER_CMD_MIPLCREATE              "MIPLCREATE"

    #define BLINKER_CMD_NB_CREATE_SUCCESSED     "+MIPLCREATE:0"

    #define BLINKER_CMD_NB_MIPLDELETE           "AT+MIPLDELETE"
    
    #define BLINKER_CMD_NB_MIPLADDOBJ           "AT+MIPLADDOBJ"
    // Add object
    // AT+MIPLADDOBJ = <ref>,               0       通信套件编号
    //                 <objectid>,          3303    IPSO 对象ID
    //                 <instancecount>,     1       实例个数
    //                 <instancebitmap>,    1       实例位图
    //                 <attributecount>,    6       属性个数
    //                 <actioncount>        1       操作个数
    // AT+MIPLADDOBJ=0,3306,1,1,5,0
    // AT+MIPLADDOBJ=0,3303,1,1,6,1
    // 建议使用 Generic Sensor	3300
    #define BLINKER_CMD_NB_MIPLDELOBJ           "AT+MIPLDELOBJ"
    // AT+MIPLDELOBJ=<ref>,<objectid>
    // AT+MIPLDELOBJ=0,3303
    #define BLINKER_CMD_NB_MIPLOPEN             "AT+MIPLOPEN"
    // AT+MIPLOPEN=<ref>,<lifetime>[,<timeout>]
    // AT+MIPLOPEN=0,1200
    // lifetime 是设备连接服务器的一个参数，单位为秒，默认为 86400 秒，lifetime 到期
    // 后，平台就会把设备踢下线，此时设备无法上报数据，只能重新注册。设备必须
    // lifetime 未过期时通过发送 AT+MILPUPDATE 命令更新 lifetime 以保证设备在线。
    // 取值范围（15~‭268435455）
    #define BLINKER_CMD_NB_MIPLUPDATE           "AT+MIPLUPDATE"
    // AT+MIPLUPDATE=<ref>,<lifetime>,<withObjectFlag>
    // AT+MIPLUPDATE=0,86400,0
    #define BLINKER_CMD_NB_MIPLCLOSE            "AT+MIPLCLOSE"
    // AT+MIPLCLOSE=<ref>
    // AT+MIPLCLOSE=0
    #define BLINKER_CMD_MIPLEVENT               "MIPLEVENT"
    // +MIPLEVENT:<ref>,<eid>
    // +MIPLEVENT:0,11

    // 1 BOOTSTRAP_START
    // 2 BOOTSTRAP_SUCCESS
    // 3 BOOTSTRAP_FAILED
    // 4 CONNECT_SUCCESS
    // 5 CONNECT_FAILED
    // 6 REG_SUCCESS
    // 7 REG_FAILED
    // 8 REG_TIMEOUT
    // 9 LIFETIME_TIMEOUT
    // 10 STATUS_HALT
    // 11 UPDATE_SUCCESS
    // 12 UPDATE_FAILED
    // 13 UPDATE_TIMEOUT
    // 14 UPDATE_NEED
    // 20 RESPONSE_FAILED
    // 21 RESPONSE_SUCCESS
    // 25 NOTIFY_FAILED
    // 26 NOTIFY_SUCCESS
    #define BLINKER_CMD_NB_EVENT_1              "+MIPLEVENT:0,1"
    
    #define BLINKER_CMD_NB_EVENT_2              "+MIPLEVENT:0,2"

    #define BLINKER_CMD_NB_EVENT_4              "+MIPLEVENT:0,4"

    #define BLINKER_CMD_NB_EVENT_6              "+MIPLEVENT:0,6"

    #define BLINKER_CMD_NB_DISCOVEREQ           "+MIPLDISCOVER"
    // Resource discovery
    // +MIPLDISCOVER:<ref>,<msgid>,<objectid>
    // 3
    // +MIPLREAD:0,888888,3303

    // 3341	Addressable Text Display
    // This IPSO object is used to send text to a text-only or text mode graphics display.
    #define BLINKER_CMD_NB_DISCOVERESP          "AT+MIPLDISCOVERRSP"
    // AT+MIPLDISCOVERRSP= <ref>,          0
    //                     <msgid>,        %d
    //                     <result>,       1
    //     1 2.05 Content
    //     11 4.00 Bad Request
    //     12 4.01 Unauthorized
    //     13 4.04 Not Found
    //     14 4.05 Method Not Allowed
    //     15 4.06 Not Acceptable
    //                     <length>,       19
    //                     <valuestring>   "5850;5851;5852;5853"
    // AT+MIPLDISCOVERRSP=0,%d,1,19,"5850;5851;5852;5853"

    // 5527 Text RW String
    #define BLINKER_CMD_MIPLOBSERVE             "MIPLOBSERVE"

    #define BLINKER_CMD_NB_OBSERVEREQ           "+MIPLOBSERVE"
    // Resource observation
    // +MIPLOBSERVE:<ref>,<msgid>,<flag>,<objectid>,<instanceid>[,<resourceid>]
    // 6
    // +MIPLREAD:0,888888,1,3303,1,5700
    #define BLINKER_CMD_NB_OBSERVERSP           "AT+MIPLOBSERVERSP"
    // AT+MIPLOBSERVERSP=<ref>,<msgid>,<result>
    // 1 2.05 Content
    // 11 4.00 Bad Request
    // 12 4.01 Unauthorized
    // 13 4.04 Not Found
    // 14 4.05 Method Not Allowed
    // 15 4.06 Not Acceptable
    // AT+MIPLOBSERVERSP=0,88888,1
    #define BLINKER_CMD_NB_MIPLNOTIFY           "AT+MIPLNOTIFY"
    // AT+MIPLNOTIFY = <ref>,           0
    //                 <msgid>,         %d
    //                 <objectid>,      3303
    //                 <instanceid>,    0
    //                 <resourceid>,    5700
    //                 <valuetype>,     4  
    //     1-char 2-hex 3-int 4-float 5-bool
    //                 <len>,           3
    //                 <value>,         2.1
    //                 <index>,         1
    //                 <flag>[,         0
    //                 <ackid>]
    // AT+MIPLNOTIFY=0,%d,3303,0,5700,4,3,2.1,1,0
    #define BLINKER_CMD_NB_UPDATE               "AT+MIPLUPDATE"
    // AT+MIPLUPDATE=<ref>,<lifetime>,<withObjectFlag>
    // AT+MIPLUPDATE=0,0,0
    #define BLINKER_CMD_NB_LEVEL                "AT+MIPLVER"

    #define BLINKER_CMD_NB_WRITEREQ             "+MIPLWRITE"
    // +MIPLWRITE: <ref>,
    //             <msgid>,
    //             <objectid>,
    //             <instanceid>,
    //             <resourceid>,
    //             <valuetype>,
    //     1-char 2-hex 3-int 4-float 5-bool
    //             <len>,
    //             <value>,
    //             <flag>,
    //             <index>
    // 10
    // +MIPLWRITE:0,51130,3306,0,0,2,2,01,0,0
    #define BLINKER_CMD_NB_WRITERSP             "AT+MIPLWRITERSP"
    // AT+MIPLWRITERSP=<ref>,<msgid>,<result>
    // 2 2.04 Changed
    // 11 4.00 Bad Request
    // 12 4.01 Unauthorized
    // 13 4.04 Not Found
    // 14 4.05 Method Not Allowed
    // 16 2.31 Continue
    // 17 4.08 Request Entity Incomplete
    // 18 4.13 Request entity too large
    // 19 4.15 Unsupported content format
    // 3
    // AT+MIPLWRITERSP=0,88888,2
    #define BLINKER_CMD_NB_READREQ              "+MIPLREAD"
    // +MIPLREAD:<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>
    // 5
    // +MIPLREAD:0,888888,3303,1,5700
    #define BLINKER_CMD_NB_READRSP              "AT+MIPLREADRSP"
    // AT+MIPLREADRSP= <ref>,              0
    //                 <msgid>,            86635
    //                 <result>[,          1
    //         1 2.05 Content
    //         11 4.00 BadRequest
    //         12 4.01 Unauthorized
    //         13 4.04 NotFound
    //         14 4.05 MethodNotAllowed
    //         15 4.06 NotAcceptable
    //                 <objectid>,         3303
    //                 <instanceid>,       1
    //                 <resourceid>,       5700
    //                 <valuetype>,        4
    //                 <len>,              2
    //                 <value>,            20
    //                 <index>,            1
    //                 <flag>]             0
    // 11
    // AT+MIPLREADRSP=0,86635,1,3303,1,5700,4,2,20,1,0
    #define BLINKER_CMD_NB_EXECUTEREQ           "+MIPLEXECUTE"
    // +MIPLEXECUTE:<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,[,<len>,<arguments>]
    // 7
    // +MIPLEXECUTE:0,51131,3303,0, 5605,5,reset
    #define BLINKER_CMD_NB_EXECUTERSP           "AT+MIPLEXECUTERSP"
    // AT+MIPLEXECUTERSP=<ref>,<msgid>,<result>
    // 2 2.04 Changed
    // 11 4.00 Bad Request
    // 12 4.01 Unauthorized
    // 13 4.04 Not Found
    // 14 4.05 Method Not Allowed
    // AT+MIPLEXECUTERSP=0,88888,2
    #define BLINKER_CMD_NB_PARAMETERREQ         "+MIPLPARAMETER"
    // +MIPLPARAMETER:<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<len>,<parameter>
    // 7
    // +MIPLPARAMETER:0,88688,3203,1,5603,39,“pmin=1.8;pmax=5.0;gt=3.6;lt=3.0;stp=0.1”
    #define BLINKER_CMD_NB_PARAMETERRSP         "AT+MIPLPARAMETERSP"
    // AT+MIPLPARAMETERRSP=<ref>,<msgid>,<result>
    // 1 2.04 Changed
    // 11 4.00 Bad Request
    // 12 4.01 Unauthorized
    // 13 4.04 Not Found
    // 14 4.05 Method Not Allowed
    // AT+MIPLPARAMETERRSP=0,888888,1


    // Digital Input	3200
    // Digital Output	3201
    // Analogue Input	3202
    // Analogue Output	3203
    // Generic Sensor	3300
    // Illuminance Sensor	3301
    // Presence sensor	3302
    // Temperature Sensor	3303
    // Humidity Sensor	3304
    // Power Measurement	3305
    // Actuation	3306
    // Set Point	3308
    // Load Control	3310
    // Light Control	3311
    // Power Control	3312
    // Accelerometer	3313
    // Magnetometer	3314
    // Barometer	3315
    // Voltage	3316
    // Current	3317
    // Frequency	3318
    // Depth	3319
    // Percentage	3320
    // Altitude	3321
    // Load	3322
    // Pressure	3323
    // Loudness	3324
    // Concentration	3325
    // Acidity	3326
    // Conductivity	3327
    // Power	3328
    // Power Factor	3329
    // Distance	3330
    // Energy	3331
    // Direction	3332
    // Time	3333
    // Gyrometer	3334
    // Color	3335
    // GPS Location	3336
    // Positioner	3337
    // Buzzer	3338
    // Audio Clip	3339
    // Timer	3340
    // Addressable Text Display	3341
    // On/Off Switch	3342
    // Dimmer	3343
    // Up/Down Control	3344
    // Multiple Axis Joystick	3345
    // Rate	3346
    // Push Button	3347
    // Multi-state Selector	3348
    // Bitmap	3349
    // Stopwatch	3350

    // Digital Input State 5500 R Boolean
    // Digital Input Counter 5501 R Integer			
    // Digital Input Polarity 5502 R,W Boolean			
    // Digital Input Debounce 5503 R,W Integer			
    // Digital Input Edge Selection 5504 R,W Integer			
    // Digital Input Counter Reset 5505 E			
    // Current Time 5506 R,W Time			
    // Fractional Time 5507 R,W Float			
    // Min X Value 5508 R Float			
    // Max X Value 5509 R Float			
    // Min Y Value 5510 R Float			
    // Max Y Value 5511 R Float			
    // Min Z Value 5512 R Float			
    // Max Z Value 5513 R Float			
    // Latitude 5514 R String			
    // Longitude 5515 R String			
    // Uncertainty 5516 R String 
    // Velocity 5517 R Opaque 
    // Timestamp 5518 R Time 
    // Min Limit 5519 R Float 
    // Max Limit 5520 R Float 
    // Delay Duration 5521 R,W Float 
    // Clip 5522 R,W Opaque 
    // Trigger 5523 E 
    // Duration 5524 R,W Float 
    // Minimum Off-time 5525 R,W Float 
    // Mode 5526 R,W Integer 
    // Text 5527 R,W String 
    // X Coordinate 5528 R,W Integer 
    // Y Coordinate 5529 R,W Integer 
    // Clear Display 5530 E 
    // Contrast 5531 R,W Float 
    // Increase Input State 5532 R Boolean 
    // Decrease Input State 5533 R Boolean 
    // Counter 5534 R,W Integer 
    // Current Position 5536 R,W Float 
    // Transition Time 5537 R,W Float 
    // Remaining Time 5538 R Float 
    // Up Counter 5541 R,W Integer 
    // Down Counter 5542 R,W Integer 
    // Digital State 5543 R Boolean 
    // Cumulative Time 5544 R,W Float 
    // Max X Coordinate 5545 R Integer 
    // Max Y Coordinate 5546 R Integer 
    // Multi-state Input 5547 R Integer 
    // Level 5548 R,W Float 
    // Digital Output State 5550 R,W Boolean 
    // Digital Output Polarity 5551 R,W Boolean 
    // Analog Input State 5600 R Float 
    // Min Measured Value 5601 R Float 
    // Max Measured Value 5602 R Float 
    // Min Range Value 5603 R Float 
    // Max Range Value 5604 R Float 
    // Reset Min and Max Measured Values 5605 E 
    // Analog Output Current Value 5650 R,W Float 
    // Sensor Value 5700 R Float 
    // Sensor Units 5701 R String 
    // X Value 5702 R Float 
    // Y Value 5703 R Float 
    // Z Value 5704 R Float 
    // Compass Direction 5705 R Float 
    // Colour 5706 R,W String 
    // Application Type 5750 R,W String 
    // Sensor Type 5751 R String 
    // Instantaneous active power 5800 R Float 
    // Min Measured active power 5801 R Float 
    // Max Measured active power 5802 R Float 
    // Min Range active power 5803 R Float 
    // Max Range active power 5804 R Float 
    // Cumulative active power 5805 R Float 
    // Active Power Calibration 5806 W Float 
    // Instantaneous reactive power 5810 R Float 
    // Min Measured reactive power 5811 R Float 
    // Max Measured reactive power 5812 R Float 
    // Min Range reactive power 5813 R Float 
    // Max Range reactive power 5814 R Float 
    // Cumulative reactive power 5815 R Float 
    // Reactive Power Calibration 5816 W Float 
    // Power Factor 5820 R Float 
    // Current Calibration 5821 R,W Float 
    // Reset Cumulative energy 5822 E 
    // Event Identifier 5823 R,W String 
    // Start Time 5824 R,W Float 
    // Duration In Min 5825 R,W Float 
    // Criticality Level 5826 R,W Integer 
    // Avg Load Adj Pct 5827 R,W String 
    // Duty Cycle 5828 R,W Integer 
    // On/Off 5850 R,W Boolean 
    // Dimmer 5851 R,W Integer 
    // On Time 5852 R,W Integer 
    // Muti-state Output 5853 R,W String 
    // Off Time 5854 R,W Integer 
    // Set Point Value 5900 R,W Float 
    // Busy to Clear delay 5903 R,W Integer 
    // Clear to Busy delay 5904 R,W Integer 
    // Bitmap Input 5910 R Integer 
    // Bitmap Input Reset 5911 E 
    // Element Description 5912 R,W String 
    // UUID 5913 R,W String
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
    defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT) || \
    defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_NBIOT_SIM7020) || \
    defined(BLINKER_GPRS_AIR202) || defined(BLINKER_PRO_SIM7020) || \
    defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
    defined(BLINKER_PRO_ESP) || defined(BLINKER_LOWPOWER_AIR202) || \
    defined(BLINKER_WIFI_SUBDEVICE)
    #define BLINKER_CMD_SMS_NUMBER              1

    #define BLINKER_CMD_PUSH_NUMBER             2

    #define BLINKER_CMD_WECHAT_NUMBER           3

    #define BLINKER_CMD_WEATHER_NUMBER          4

    #define BLINKER_CMD_AQI_NUMBER              5

    #define BLINKER_CMD_BRIDGE_NUMBER           6

    #define BLINKER_CMD_CONFIG_UPDATE_NUMBER    7

    #define BLINKER_CMD_CONFIG_GET_NUMBER       8

    #define BLINKER_CMD_CONFIG_DELETE_NUMBER    9

    #define BLINKER_CMD_DATA_STORAGE_NUMBER     10

    #define BLINKER_CMD_DATA_GET_NUMBER         11

    #define BLINKER_CMD_DATA_DELETE_NUMBER      12

    #define BLINKER_CMD_AUTO_PULL_NUMBER        13

    #define BLINKER_CMD_OTA_NUMBER              14

    #define BLINKER_CMD_OTA_STATUS_NUMBER       15

    #define BLINKER_CMD_FRESH_SHARERS_NUMBER    16

    #define BLINKER_CMD_LOWPOWER_FREQ_GET_NUM   17

    #define BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER 18

    #define BLINKER_CMD_LOWPOWER_DATA_GET_NUM   19

    #define BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER 20

    #define BLINKER_CMD_EVENT_DATA_NUMBER       21

    #define BLINKER_CMD_GPS_DATA_NUMBER         22

    #define BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER 23

    #define BLINKER_CMD_EVENT_WARNING_NUMBER    24

    #define BLINKER_CMD_EVENT_ERROR_NUMBER      25

    #define BLINKER_CMD_EVENT_MSG_NUMBER        26

    #define BLINKER_CMD_DEFAULT_NUMBER          0

#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_GPRS_AIR202) || defined(BLINKER_NBIOT_SIM7020) || \
    defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_LOWPOWER_AIR202) || defined(BLINKER_WIFI_SUBDEVICE)
    #define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"

    #define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

    #define BLINKER_MQTT_ALIYUN_PORT        1883

    #define BLINKER_MQTT_BORKER_QCLOUD      "qcloud"

    #define BLINKER_MQTT_QCLOUD_HOST        "iotcloud-mqtt.gz.tencentdevices.com"

    #define BLINKER_MQTT_QCLOUD_PORT        8883

    #define BLINKER_MQTT_BORKER_ONENET      "onenet"

    #define BLINKER_MQTT_ONENET_HOST        "mqtt.heclouds.com"

    #define BLINKER_MQTT_ONENET_PORT        6002

    #define BLINKER_MQTT_PING_TIMEOUT       30000UL

    #define BLINKER_MQTT_HOST_SIZE          50

    #define BLINKER_MQTT_ID_SIZE            110

    #define BLINKER_MQTT_NAME_SIZE          80

    #define BLINKER_MQTT_KEY_SIZE           50

    #define BLINKER_MQTT_PINFO_SIZE         12

    #define BLINKER_MQTT_UUID_SIZE          40

    #define BLINKER_MQTT_DEVICEID_SIZE      26

    #define BLINKER_MQTT_DEVICENAME_SIZE    14

    #define BLINKER_MQTT_USER_UUID_SIZE     32

    #define BLINKER_MQTT_MAX_SHARERS_NUM    9

    #define BLINKER_MQTT_FROM_AUTHER        BLINKER_MQTT_MAX_SHARERS_NUM

    #define BLINKER_MQTT_FORM_SERVER        BLINKER_MQTT_MAX_SHARERS_NUM + 1

#endif

#if defined(ESP8266) || defined(ESP32)

    #define BLINKER_TIMING_TIMER_SIZE       10

    #define BLINKER_TYPE_STATE              0

    #define BLINKER_TYPE_NUMERIC            1

    #define BLINKER_TYPE_OR                 2

    #define BLINKER_TYPE_AND                3

    #define BLINKER_COMPARE_LESS            0

    #define BLINKER_COMPARE_EQUAL           1

    #define BLINKER_COMPARE_GREATER         2

    #define BLINKER_CHECK_DATA              170

    #define BLINKER_EEP_SIZE                4096

    #define BLINKER_EEP_ADDR_CHECK          0

    #define BLINKER_CHECK_SIZE              1

    #define BLINKER_EEP_ADDR_AUTONUM        (BLINKER_EEP_ADDR_CHECK + BLINKER_CHECK_SIZE)

    #define BLINKER_AUTONUM_SIZE            1

    #define BLINKER_EEP_ADDR_AUTO_START     (BLINKER_EEP_ADDR_AUTONUM + BLINKER_AUTONUM_SIZE)

    ////////////////////////////////////////////////////////////////////////////////////////

    #define BLINKER_EEP_ADDR_AUTOID         0

    #define BLINKER_AUTOID_SIZE             4

    #define BLINKER_EEP_ADDR_TYPESTATE      (BLINKER_EEP_ADDR_AUTOID + BLINKER_AUTOID_SIZE)

    #define BLINKER_TYPESTATE_SIZE          1

    #define BLINKER_EEP_ADDR_AUTO1          (BLINKER_EEP_ADDR_TYPESTATE + BLINKER_TYPESTATE_SIZE)

    #define BLINKER_AUTODATA_SIZE           4

    #define BLINKER_EEP_ADDR_TARGETKEY1     (BLINKER_EEP_ADDR_AUTO1 + BLINKER_AUTODATA_SIZE)

    #define BLINKER_TARGETKEY_SIZE          12

    #define BLINKER_EEP_ADDR_TARGETDATA1    (BLINKER_EEP_ADDR_TARGETKEY1 + BLINKER_TARGETKEY_SIZE)

    #define BLINKER_TARGETDATA_SIZE         4

    #define BLINKER_EEP_ADDR_AUTO2          (BLINKER_EEP_ADDR_TARGETDATA1 + BLINKER_TARGETDATA_SIZE)

    #define BLINKER_AUTODATA_SIZE           4

    #define BLINKER_EEP_ADDR_TARGETKEY2     (BLINKER_EEP_ADDR_AUTO2 + BLINKER_AUTODATA_SIZE)

    #define BLINKER_TARGETKEY_SIZE          12

    #define BLINKER_EEP_ADDR_TARGETDATA2    (BLINKER_EEP_ADDR_TARGETKEY2 + BLINKER_TARGETKEY_SIZE)

    #define BLINKER_TARGETDATA_SIZE         4

    // // #define BLINKER_TARGET_DATA_SIZE        (BLINKER_AUTOID_SIZE + BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + BLINKER_TARGETDATA_SIZE)

    // #define BLINKER_EEP_ADDR_LINKDEVICE1    (BLINKER_EEP_ADDR_TARGETDATA2 + BLINKER_TARGETDATA_SIZE)

    // #define BLINKER_LINKDEVICE_SIZE         26

    // #define BLINKER_EEP_ADDR_LINKTYPE1      (BLINKER_EEP_ADDR_LINKDEVICE1 + BLINKER_LINKDEVICE_SIZE)

    // #define BLINKER_LINKTYPE_SIZE           18

    // #define BLINKER_EEP_ADDR_LINKDATA1      (BLINKER_EEP_ADDR_LINKTYPE1 + BLINKER_LINKTYPE_SIZE)

    // #define BLINKER_LINKDATA_SIZE           212

    // #define BLINKER_EEP_ADDR_LINKDEVICE2    (BLINKER_EEP_ADDR_LINKDATA1 + BLINKER_LINKDATA_SIZE)

    // #define BLINKER_LINKDEVICE_SIZE         26

    // #define BLINKER_EEP_ADDR_LINKTYPE2      (BLINKER_EEP_ADDR_LINKDEVICE2 + BLINKER_LINKDEVICE_SIZE)

    // #define BLINKER_LINKTYPE_SIZE           18

    // #define BLINKER_EEP_ADDR_LINKDATA2      (BLINKER_EEP_ADDR_LINKTYPE2 + BLINKER_LINKTYPE_SIZE)

    // #define BLINKER_LINKDATA_SIZE           212

    #define BLINKER_ONE_AUTO_DATA_SIZE      (BLINKER_AUTOID_SIZE + BLINKER_TYPESTATE_SIZE + \
                                            (BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + \
                                            BLINKER_TARGETDATA_SIZE) * 2)// + BLINKER_LINKDEVICE_SIZE + BLINKER_LINKTYPE_SIZE + BLINKER_LINKDATA_SIZE) * 2)

    #define BLINKER_EEP_ADDR_AUTONUM_SUB    (BLINKER_EEP_ADDR_OTA_CHECK + BLINKER_OTA_CHECK_SIZE)

    #define BLINKER_AUTONUM_SUB_SIZE        1

    #define BLINKER_EEP_ADDR_AUTO_START_SUB (BLINKER_EEP_ADDR_AUTONUM_SUB + BLINKER_AUTONUM_SUB_SIZE)

    ////////////////////////////////////////////////////////////////////////////////////////

    #define BLINKER_EEP_ADDR_AUTOID_SUB     0

    #define BLINKER_AUTOID_SUB_SIZE         4
    
#endif

#if defined(BLINKER_PRO) || defined(BLINKER_GPRS_AIR202) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_PRO_SIM7020) || \
    defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
    defined(BLINKER_PRO_ESP) || defined(BLINKER_LOWPOWER_AIR202) || \
    defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_WIFI_SUBDEVICE)

    #ifndef BLINKER_BUTTON_PIN
        #define BLINKER_BUTTON_PIN              2
    #endif

    #define BLINKER_CMD_REGISTER            "register"

    #define BLINKER_DIY_DEVICE              "DiyArduino"

    #define BLINKER_AIR_STATION             "AirStation"

    #define BLINKER_AIR_DETECTOR            "OwnAirdetector"

    #define BLINKER_SMART_PLUGIN            "OwnPlug"

    #define BLINKER_SMART_LAMP              "OwnLight"

    #ifndef BLINKER_PRO_VERSION
        #define BLINKER_PRO_VERSION             "1.0.0"
    #endif

    // #define BLINKER_OTA_VERSION_CODE        B00000001

    // #define BLINKER_OTA_RUN                 B00000111

    // #define BLINKER_OTA_START               B01010011

    // #define BLINKER_OTA_CLEAR               B00000001

    #define BLINKER_CHECK_AUTH_TIME         120000UL

    #define BLINKER_AUTH_CHECK_DATA         0x55

    #define BLINKER_EEP_ADDR_SSID           1280

    #define BLINKER_SSID_SIZE               32

    #define BLINKER_EEP_ADDR_PSWD           (BLINKER_EEP_ADDR_SSID + BLINKER_SSID_SIZE)

    #define BLINKER_PSWD_SIZE               32

    #define BLINKER_EEP_ADDR_WLAN_CHECK     (BLINKER_EEP_ADDR_PSWD + BLINKER_PSWD_SIZE)

    #define BLINKER_WLAN_CHECK_SIZE         3

    #define BLINKER_EEP_ADDR_AUUID          (BLINKER_EEP_ADDR_WLAN_CHECK + BLINKER_WLAN_CHECK_SIZE)

    #define BLINKER_AUUID_SIZE              34

    #define BLINKER_EEP_ADDR_AUTH_CHECK     (BLINKER_EEP_ADDR_AUUID + BLINKER_AUUID_SIZE)

    #define BLINKER_AUTH_CHECK_SIZE         1

    #define BLINKER_EEP_ADDR_POWER_ON_COUNT (BLINKER_EEP_ADDR_AUTH_CHECK + BLINKER_AUTH_CHECK_SIZE)

    #define BLINKER_POWER_ON_COUNT_SIZE     1

    #define BLINKER_PRO_DATA_SIZE           (BLINKER_SSID_SIZE + BLINKER_PSWD_SIZE + BLINKER_WLAN_CHECK_SIZE + BLINKER_AUUID_SIZE + BLINKER_AUTH_CHECK_SIZE + BLINKER_POWER_ON_COUNT_SIZE)

    #define BLINKER_PRO_HELLO               "{\"message\":\"Registration successful\"}"
    
    // #define BLINKER_EEP_ADDR_OTA_INFO       (BLINKER_EEP_ADDR_AUTH_CHECK + BLINKER_AUTH_CHECK_SIZE)

    // #define BLINKER_OTA_INFO_SIZE           4

    // #define BLINKER_EEP_ADDR_OTA_CHECK      (BLINKER_EEP_ADDR_OTA_INFO + BLINKER_OTA_INFO_SIZE)

    // #define BLINKER_OTA_CHECK_SIZE          1
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_MQTT_AUTO) || \
    defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_SUBDEVICE)

    #ifndef BLINKER_OTA_VERSION_CODE

        #define BLINKER_OTA_VERSION_CODE        "0.1.0"

    #endif

    #define BLINKER_OTA_RUN                 1

    #define BLINKER_OTA_START               2

    #define BLINKER_OTA_CLEAR               0

    #define BLINKER_EEP_ADDR_OTA_INFO       2436

    #define BLINKER_OTA_INFO_SIZE           11

    #define BLINKER_EEP_ADDR_OTA_CHECK      (BLINKER_EEP_ADDR_OTA_INFO + BLINKER_OTA_INFO_SIZE)

    #define BLINKER_OTA_CHECK_SIZE          1

#endif

#if defined(ESP8266) || defined(ESP32)

    #define BLINKER_ACTION_SIZE                     30

    #define BLINKER_ACTION_NUM                      2

    #define BLINKER_EEP_ADDR_TIMER                  1536

    #define BLINKER_EEP_ADDR_TIMER_COUNTDOWN        BLINKER_EEP_ADDR_TIMER

    #define BLINKER_TIMER_COUNTDOWN_SIZE            4

    #define BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION (BLINKER_EEP_ADDR_TIMER_COUNTDOWN + BLINKER_TIMER_COUNTDOWN_SIZE)

    #define BLINKER_TIMER_COUNTDOWN_ACTION_SIZE     (BLINKER_ACTION_SIZE * BLINKER_ACTION_NUM)

    #define BLINKER_EEP_ADDR_TIMER_LOOP             (BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION + BLINKER_TIMER_COUNTDOWN_ACTION_SIZE)

    #define BLINKER_TIMER_LOOP_SIZE                 4

    #define BLINKER_EEP_ADDR_TIMER_LOOP_TRI         (BLINKER_EEP_ADDR_TIMER_LOOP + BLINKER_TIMER_LOOP_SIZE)

    #define BLINKER_TIMER_LOOP_TRI_SIZE             1

    #define BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1     (BLINKER_EEP_ADDR_TIMER_LOOP_TRI + BLINKER_TIMER_LOOP_TRI_SIZE)

    #define BLINKER_TIMER_LOOP_ACTION1_SIZE         (BLINKER_ACTION_SIZE * BLINKER_ACTION_NUM)

    #define BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2     (BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1 + BLINKER_TIMER_LOOP_ACTION1_SIZE)

    #define BLINKER_TIMER_LOOP_ACTION2_SIZE         (BLINKER_ACTION_SIZE * BLINKER_ACTION_NUM)

    #define BLINKER_EEP_ADDR_TIMER_TIMING_COUNT     (BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2 + BLINKER_TIMER_LOOP_ACTION2_SIZE)

    #define BLINKER_TIMER_TIMING_COUNT_SIZE         1

    #define BLINKER_EEP_ADDR_TIMER_TIMING           (BLINKER_EEP_ADDR_TIMER_TIMING_COUNT + BLINKER_TIMER_TIMING_COUNT_SIZE)

    #define BLINKER_TIMER_TIMING_SIZE               4

    #define BLINKER_TIMER_TIMING_ACTION_SIZE        (BLINKER_ACTION_SIZE * BLINKER_ACTION_NUM)

    #define BLINKER_ONE_TIMER_TIMING_SIZE           (BLINKER_TIMER_TIMING_SIZE + BLINKER_TIMER_TIMING_ACTION_SIZE)

    #define BLINKER_EEP_ADDR_TIMER_ERASE            2430

    #define BLINKER_TIMER_ERASE_SIZE                1

    #define BLINKER_EEP_ADDR_TIMER_END              (BLINKER_EEP_ADDR_TIMER_ERASE + BLINKER_TIMER_ERASE_SIZE)

    // 2 60 | 4 120 | 1 4 60 x 10 + 2 + 1
    // 793 896



    #define BLINKER_EVENT_MSG_SMS                   0

    #define BLINKER_EVENT_MSG_PUSH                  1

    #define BLINKER_EVENT_KEY_SIZE                  15

    #define BLINKER_EVENT_NUM_SIZE                  2

    #define BLINKER_EEP_ADDR_EVENT                  (BLINKER_EEP_ADDR_TIMER_END + BLINKER_TIMER_ERASE_SIZE)

    #define BLINKER_EEP_ADDR_EVENTDATA              BLINKER_EEP_ADDR_EVENT

    #define BLINKER_EVENTDATA_SIZE                  1

    // #define BLINKER_EEP_ADDR_EVENT_DATA1            (BLINKER_EEP_ADDR_EVENTDATA + BLINKER_EVENTDATA_SIZE)

    // #define BLINKER_EVENT_DATA1_SIZE                2

    #define BLINKER_EEP_ADDR_EVENT_DATA             (BLINKER_EEP_ADDR_EVENTDATA + BLINKER_EVENTDATA_SIZE)

    #define BLINKER_EVENT_DATA_SIZE                 4

    #define BLINKER_EEP_ADDR_EVENT_KEY1             (BLINKER_EEP_ADDR_EVENT_DATA + BLINKER_EVENT_DATA_SIZE)

    #define BLINKER_EVENT_KEY1_SIZE                 BLINKER_EVENT_KEY_SIZE

    // #define BLINKER_EEP_ADDR_EVENT_DATA2            (BLINKER_EEP_ADDR_EVENT_KEY1 + BLINKER_EVENT_KEY1_SIZE)

    #define BLINKER_EEP_ADDR_EVENT_KEY2             (BLINKER_EEP_ADDR_EVENT_KEY1 + BLINKER_EVENT_KEY1_SIZE)

    #define BLINKER_EVENT_KEY2_SIZE                 BLINKER_EVENT_KEY_SIZE

    #define BLINKER_EEP_ADDR_EVENT_MSG              (BLINKER_EEP_ADDR_EVENT_KEY2 + BLINKER_EVENT_KEY2_SIZE)

    #define BLINKER_EVENT_MSG_SIZE                  20

    #define BLINKER_EEP_ADDR_EVENT_ERASE            (BLINKER_EEP_ADDR_EVENT_MSG + BLINKER_EVENT_MSG_SIZE)

    #define BLINKER_EVENT_ERASE_SIZE                1

    #define BLINKER_EEP_ADDR_EVENT_END              (BLINKER_EEP_ADDR_EVENT_ERASE + BLINKER_EVENT_ERASE_SIZE)

    // 56

    #define BLINKER_EEP_ADDR_SERIALCFG          2432

    #define BLINKER_SERIALCFG_SIZE              4

#endif

#if defined(BLINKER_GPRS_AIR202) || defined(BLINKER_PRO_AIR202) || \
    defined(BLINKER_LOWPOWER_AIR202)

    // http://www.openluat.com/Product/file/rda8955/AirM2M%20%E6%97%A0%E7%BA%BF%E6%A8%A1%E5%9D%97AT%E5%91%BD%E4%BB%A4%E6%89%8B%E5%86%8CV3.90.pdf
    // http://www.openluat.com/Product/file/rda8955/%E8%BF%9E%E6%8E%A5%E9%98%BF%E9%87%8C%E4%BA%91AT%E6%B5%81%E7%A8%8B%E8%AF%B4%E6%98%8E%EF%BC%88%E4%B8%80%E6%9C%BA%E4%B8%80%E5%AF%86%E8%AE%A4%E8%AF%81%E6%96%B9%E6%A1%88%EF%BC%8CAT_S_TTS%E5%92%8CAT_S%E7%89%88%E6%9C%AC%E4%B8%8D%E6%94%AF%E6%8C%81%EF%BC%8CAT_S_SSL%E5%92%8CAT_S_NOTTS%E7%89%88%E6%9C%AC%E6%94%AF%E6%8C%81%EF%BC%89.pdf
    
    #define BLINKER_CMD_AT                      "AT"

    #define BLINKER_CMD_CGMMR_REQ               "AT+CGMR"

    #define BLINKER_CMD_CGMMR_RESP              "AirM2M_V5595_AT_S_SSL"

    #define BLINKER_CMD_CGATT                   "CGATT"

    #define BLINKER_CMD_CGQTT_REQ               "AT+CGATT?"

    #define BLINEKR_CMD_SAPBR_REQ               "AT+SAPBR"
    // AT+SAPBR=<cmd_type>,<cid>[,<ConParamTag>,<ConParamValue>]

    #define BLINKER_CMD_MCONFIG_REQ             "AT+MCONFIG"

    #define BLINKER_CMD_SSLMIPSTART             "AT+SSLMIPSTART"

    #define BLINKER_CMD_MCONNECT_REQ            "AT+MCONNECT"

    #define BLINKER_CMD_CONNECT_OK              "CONNECT OK"

    #define BLINKER_CMD_CONNACK_OK              "CONNACK OK"

    #define BLINKER_CMD_CONNACK                 "CONNACK"

    #define BLINKER_CMD_MSUB_REQ                "AT+MSUB"

    #define BLINKER_CMD_SUBACK                  "SUBACK"

    #define BLINKER_CMD_MQTTTMSGSET_REQ         "AT+MQTTMSGSET"

    #define BLINKER_CMD_MPUB_REQ                "AT+MPUB"

    #define BLINKER_CMD_MSUB                    "MSUB"

    #define BLINKER_CMD_PUBACK                  "PBACK"

    #define BLINKER_CMD_MQTTSTATU_REQ           "AT+MQTTSTATU"

    #define BLINKER_CMD_MQTTSTATUS              "MQTTSTATU "

    #define BLINKER_CMD_MIPCLOSE_REQ            "AT+MIPCLOSE"

    #define BLINKER_CMD_CIPSHUT_REQ             "AT+CIPSHUT"

    #define BLINKER_CMD_CSTT_REQ                "AT+CSTT"

    #define BLINKER_CMD_CMNET                   "CMNET"

    #define BLINKER_CMD_CIICR_REQ               "AT+CIICR"

    #define BLINKER_CMD_HTTPINIT_REQ            "AT+HTTPINIT"

    #define BLINKER_CMD_HTTPPARA_REQ            "AT+HTTPPARA"

    #define BLINKER_CMD_HTTPACTION_REQ          "AT+HTTPACTION"

    #define BLINKER_CMD_HTTPACTION              "HTTPACTION"

    #define BLINKER_CMD_HTTPREAD_REQ            "AT+HTTPREAD"

    #define BLINKER_CMD_HTTPREAD                "HTTPREAD"

    #define BLINKER_CMD_HTTPERM_REQ             "AT+HTTPTERM"

    #define BLINKER_CMD_HTTPDATA_REQ            "AT+HTTPDATA"

    #define BLINKER_CMD_DOWNLOAD                "DOWNLOAD"

    #define BLINEKR_CMD_CGSN_REQ                "AT+CGSN"

    #define BLINKER_CMD_ICCID_REQ               "AT+ICCID"

    #define BLINKER_CMD_ICCID                   "ICCID"

    #define BLINEKR_CMD_CGSN                    "CGSN"

    #define BLINKER_CMD_MDISCONNECT_REQ         "AT+MDISCONNECT"

    #define BLINKER_CMD_AMGSMLOC_REQ            "AT+AMGSMLOC"

    #define BLINKER_CMD_AMGSMLOC                "AMGSMLOC"

    #define BLINKER_CMD_CNTP_REQ                "AT+CNTP"

    #define BLINKER_CMD_CNTP                    "CNTP"

    #define BLINKER_CMD_CCLK_REQ                "AT+CCLK?"

    #define BLINKER_CMD_CCLK                    "CCLK"

#endif

#if defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_PRO_SIM7020) || \
    defined(BLINKER_PRO_AIR202)
    // http://www.simcom.com/upload/file/1547183713.pdf HTTP
    // http://www.simcom.com/upload/file/1547183663.pdf MQTT
    // http://www.simcom.com/upload/file/1547177595.pdf AT

    #define BLINKER_CMD_AT                      "AT"

    #define BLINKER_CMD_CPIN_REQ                "AT+CPIN?"

    #define BLINKER_CMD_CPIN                    "CPIN"

    #define BLINKER_CMD_READY                   "READY"

    #define BLINKER_CMD_CSQ_REQ                 "AT+CSQ"

    #define BLINKER_CMD_CSQ                     "CSQ"

    #define BLINKER_CMD_CGREG_REQ               "AT+CGREG?"

    #define BLINKER_CMD_CGREG                   "CGREG"

    #define BLINKER_CMD_CGACT_REQ               "AT+CGACT?"

    #define BLINKER_CMD_CGACT                   "CGACT"

    #define BLINEKR_CMD_GSN_REQ                 "AT+GSN"

    #define BLINKER_CMD_COPS_REQ                "AT+COPS?"

    #define BLINKER_CMD_COPS                    "COPS"

    #define BLINKER_CMD_CGCONTRDP_REQ           "AT+CGCONTRDP"

    #define BLINKER_CMD_CGCONTRDP               "CGCONTRDP"

    #define BLINKER_CMD_GSN_REQ                 "AT+GSN"

    #define BLINKER_CMD_CCID_REQ                "AT+CCID"

    #define BLINKER_CMD_CSNTPSTART_REQ          "AT+CSNTPSTART"

    #define BLINKER_CMD_CSNTPSTOP_REQ           "AT+CSNTPSTOP"

    #define BLINKER_CMD_CSNTP                   "CSNTP"

    #define BLINKER_CMD_CHTTPCREATE_REQ         "AT+CHTTPCREATE"

    #define BLINKER_CMD_CHTTPCREATE             "CHTTPCREATE"

    #define BLINEKR_CMD_CHTTPCON_REQ            "AT+CHTTPCON"

    #define BLINKER_CMD_CHTTPSEND_REQ           "AT+CHTTPSEND"

    #define BLINKER_CMD_CHTTPNMIH               "CHTTPNMIH"

    #define BLINKER_CMD_CHTTPNMIC               "CHTTPNMIC"

    #define BLINKER_CMD_CHTTPDISCON_REQ         "AT+CHTTPDISCON"

    #define BLINKER_CMD_CHTTPDESTROY_REQ        "AT+CHTTPDESTROY"

    #define BLINKER_CMD_CMQNEW_REQ              "AT+CMQNEW"

    #define BLINKER_CMD_CMQNEW                  "CMQNEW"

    #define BLINKER_CMD_CMQCON_REQ              "AT+CMQCON"

    #define BLINKER_CMD_CMQCON                  "CMQCON"

    #define BLINKER_CMD_CMQSUB_REQ              "AT+CMQSUB"

    #define BLINKER_CMD_CMQPUB_REQ              "AT+CMQPUB"

    #define BLINKER_CMD_CMQPUB                  "CMQPUB"

    #define BLINKER_CMD_CMQDISCON_RESQ          "AT+CMQDISCON"

    #define BLINKER_CMD_CMQDISCON               "CMQDISCON"

    #define BLINKER_CMD_CRESET_RESQ             "AT+CRESET"

    // AT+CMQNEW="public.iot-as-mqtt.cn-shanghai.aliyuncs.com","1883",12000,1024  
    // +CMQNEW: 0
    // OK
    // AT+CMQCON=0,3,"278669B20M25B642205N3CXP",600,0,0,"SFETga2UxKxN8jiqnNQg00105d5400","0ceed0bac52d4968b298e004be3dd496"  
    // OK
    // AT+CMQSUB=0,"/JgCGbHlndgz/278669B20M25B642205N3CXP/r",0
    // OK
    // +CMQPUB: 0,"/JgCGbHlndgz/278669B20M25B642205N3CXP/r",0,0,0,6,"313233"

#endif

#endif
