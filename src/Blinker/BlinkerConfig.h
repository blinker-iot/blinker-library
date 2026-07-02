#ifndef BlinkerConfig_H
#define BlinkerConfig_H

#include "BlinkerPlatform.h"

// #include "Blinker/BlinkerDebug.h"
// #include "Blinker/BlinkerUtility.h"
#include "../Server/BlinkerServer.h"

#define BLINKER_VERSION                 "0.3.12"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_NTP_TIMEOUT             1000UL


#define BLINKER_MQTT_CONNECT_TIMESLOT   5000UL

#define BLINKER_CONFIG_UPDATE_LIMIT     10000UL

#define BLINKER_CONFIG_GET_LIMIT        10000UL

#define BLINKER_WIFI_INIT_TIMEOUT       20000UL

#define BLINKER_SERVER_CONNECT_LIMIT    3

#define BLINKER_DEVICE_HEARTBEAT_TIME   600

#define BLINKER_MDNS_SERVICE_BLINKER    "blinker"

#define BLINKER_ERROR                   0x00

#define BLINKER_SUCCESS                 0x01

#define BLINKER_MSG_FROM_MQTT           0

#define BLINKER_MSG_FROM_WS             1

#define BLINKER_INT_DATA                0

#define BLINKER_UINT_DATA               1

#define BLINKER_FLOAT_DATA              2

// #if defined(BLINKER_MQTT)
//     #define BLINKER_MSG_AUTOFORMAT_TIMEOUT  1000
// #else
    #define BLINKER_MSG_AUTOFORMAT_TIMEOUT  100
// #endif

#if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
    #define BLINKER_PRESSTIME_POWERDOWN     2000UL
    
    #define BLINKER_PRESSTIME_RESET         10000UL
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
    #define BLINKER_MAX_WIDGET_SIZE         16
#else
    #define BLINKER_MAX_WIDGET_SIZE         6
#endif

#define BLINKER_OBJECT_NOT_AVAIL        -1

#ifndef BLINKER_MAX_READ_SIZE
    #if defined(BLINKER_NATIVE_WIFI)
        #define BLINKER_MAX_READ_SIZE       1024
    #else
        #define BLINKER_MAX_READ_SIZE       128
    #endif
#endif

#ifndef BLINKER_MAX_SEND_SIZE
    #if defined(BLINKER_NATIVE_WIFI)
        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
            #define BLINKER_MAX_SEND_SIZE       1024
        #else
            #define BLINKER_MAX_SEND_SIZE       512
        #endif
    #else
        #define BLINKER_MAX_SEND_SIZE       128
    #endif
#endif

#ifndef BLINKER_MAX_SEND_BUFFER_SIZE
    #if defined(BLINKER_NATIVE_WIFI)
        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
            #define BLINKER_MAX_SEND_BUFFER_SIZE       BLINKER_MAX_SEND_SIZE - 128
        #else
            #define BLINKER_MAX_SEND_BUFFER_SIZE       BLINKER_MAX_SEND_SIZE
        #endif
    #else
        #define BLINKER_MAX_SEND_BUFFER_SIZE       128
    #endif
#endif

#define BLINKER_AUTHKEY_SIZE            14

#if defined(BLINKER_NATIVE_WIFI)
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

#define BLINKER_CMD_TRIGGER             "triggers"

#define BLINKER_CMD_SOURCE              "source"

#define BLINKER_CMD_OPERATOR            "operator"

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

#define BLINKER_CMD_POWERSTATE1         "pstate"

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

#define BLINKER_CMD_LEVEL               "level"

#define BLINKER_CMD_LEVEL_UP            "upLevel"

#define BLINKER_CMD_LEVEL_DW            "downLevel"

#define BLINKER_CMD_TEMP_UP             "upTemp"

#define BLINKER_CMD_TEMP_DW             "downTemp"

#define BLINKER_CMD_HSTATE              "hsState"

#define BLINKER_CMD_VSTATE              "vsState"

#define BLINKER_CMD_ECO                 "eco"

#define BLINKER_CMD_ANION               "anion"

#define BLINKER_CMD_HEATER              "heater"

#define BLINKER_CMD_DRYER               "dryer"

#define BLINKER_CMD_SOFT                "soft"

#define BLINKER_CMD_UV                  "uv"

#define BLINKER_CMD_UNSB                "unsb"





























#define BLINKER_CMD_SHARE               "share"

#define BLINKER_CMD_AUTO_UPDATE_KEY     "upKey"

#define BLINKER_CMD_CANCEL_UPDATE_KEY   "cKey"




#define BLINKER_CMD_SERVERCLIENT        "serverClient"

#define BLINKER_CMD_HELLO               "hello"

// #define BLINKER_CMD_WHOIS               "whois"

#define BLINKER_CMD_GATE                "gate"

#define BLINKER_CMD_CONTROL             "ctrl"

#define BLINKER_CMD_DEVICEINFO          "dInf"

#define BLINKER_CMD_NEW                 "{\"hello\":\"new\"}"

#define BLINKER_CMD_WHOIS               "{\"hello\":\"whois\"}"

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

#define BLINKER_MAX_BLINKER_DATA_SIZE   8

#define BLINKER_MAX_RTDATA_DATA_SIZE    10

#define BLINKER_MAX_RTDATA_SIZE         4

#define BLINKER_MAX_DATA_COUNT          4

#define BLINKER_DATA_UPDATE_COUNT       2

#define BLINKER_CMD_COMCONFIG_NUM           0

#define BLINKER_CMD_SMARTCONFIG_NUM         1



#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

    #define BLINKER_CMD_CONFIG_UPDATE_NUMBER    7

    #define BLINKER_CMD_CONFIG_GET_NUMBER       8

    #define BLINKER_CMD_CONFIG_DELETE_NUMBER    9

    #define BLINKER_CMD_DATA_STORAGE_NUMBER     10

    #define BLINKER_CMD_DATA_GET_NUMBER         11

    #define BLINKER_CMD_DATA_DELETE_NUMBER      12

    #define BLINKER_CMD_AUTO_PULL_NUMBER        13



    #define BLINKER_CMD_FRESH_SHARERS_NUMBER    16

    #define BLINKER_CMD_LOWPOWER_FREQ_GET_NUM   17

    #define BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER 18

    #define BLINKER_CMD_LOWPOWER_DATA_GET_NUM   19

    #define BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER 20

    #define BLINKER_CMD_EVENT_DATA_NUMBER       21


    #define BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER 23

    #define BLINKER_CMD_EVENT_WARNING_NUMBER    24

    #define BLINKER_CMD_EVENT_ERROR_NUMBER      25

    #define BLINKER_CMD_EVENT_MSG_NUMBER        26

    #define BLINKER_CMD_TIME_SLOT_DATA_NUMBER   28

    #define BLINKER_CMD_TEXT_DATA_NUMBER        29

    #define BLINKER_CMD_JSON_DATA_NUMBER        30



    #define BLINKER_CMD_JSON_DATA_GET_NUMBER    33

    #define BLINKER_CMD_DEFAULT_NUMBER          0

#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
    #define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"

    #define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

    #define BLINKER_MQTT_ALIYUN_PORT        1883

    #define BLINKER_MQTT_BORKER_QCLOUD      "qcloud"

    #define BLINKER_MQTT_QCLOUD_HOST        "iotcloud-mqtt.gz.tencentdevices.com"

    #define BLINKER_MQTT_QCLOUD_PORT        8883

    #define BLINKER_MQTT_BORKER_ONENET      "onenet"

    #define BLINKER_MQTT_ONENET_HOST        "mqtt.heclouds.com"

    #define BLINKER_MQTT_ONENET_PORT        6002

    #define BLINKER_MQTT_BORKER_BLINKER     "blinker"

    #define BLINKER_MQTT_BLINKER_HOST       "broker.clz.me"

    #define BLINKER_MQTT_BLINKER_PORT       21303

    #define BLINKER_MQTT_PING_TIMEOUT       30000UL

    #define BLINKER_MQTT_HOST_SIZE          50

    #define BLINKER_MQTT_ID_SIZE            110

    #define BLINKER_MQTT_NAME_SIZE          80

    #define BLINKER_MQTT_KEY_SIZE           50

    #define BLINKER_MQTT_PINFO_SIZE         12

    #define BLINKER_MQTT_UUID_SIZE          40

    #define BLINKER_MQTT_DEVICEID_SIZE      26

    #define BLINKER_MQTT_DEVICENAME_SIZE    14

    #define BLINKER_MQTT_USER_UUID_SIZE     24

    #define BLINKER_MQTT_MAX_SHARERS_NUM    9

    #define BLINKER_MQTT_FROM_AUTHER        BLINKER_MQTT_MAX_SHARERS_NUM

    #define BLINKER_MQTT_FORM_SERVER        BLINKER_MQTT_MAX_SHARERS_NUM + 1

#endif

#if defined(BLINKER_NATIVE_WIFI)

    #define BLINKER_TIMING_TIMER_SIZE       10

    // #define BLINKER_TYPE_STATE              0

    // #define BLINKER_TYPE_NUMERIC            1

    #define BLINKER_TYPE_OR                 0

    #define BLINKER_TYPE_AND                1

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

    #define BLINKER_EEP_ADDR_AUTODATA       (BLINKER_EEP_ADDR_AUTOID + BLINKER_AUTOID_SIZE)

    #define BLINKER_AUTODATA_SIZE           6

    #define BLINKER_EEP_ADDR_SOURCE         (BLINKER_EEP_ADDR_AUTODATA + BLINKER_AUTODATA_SIZE)

    #define BLINKER_SOURCE_SIZE             12

    #define BLINKER_EEP_ADDR_VALUE          (BLINKER_EEP_ADDR_SOURCE + BLINKER_SOURCE_SIZE)

    #define BLINKER_VALUE_SIZE              4

    #define BLINKER_ONE_AUTO_DATA_SIZE      (BLINKER_AUTOID_SIZE + BLINKER_AUTODATA_SIZE + BLINKER_SOURCE_SIZE + BLINKER_VALUE_SIZE)

    // #define BLINKER_EEP_ADDR_TYPESTATE      (BLINKER_EEP_ADDR_AUTOID + BLINKER_AUTOID_SIZE)

    // #define BLINKER_TYPESTATE_SIZE          1

    // #define BLINKER_EEP_ADDR_AUTO1          (BLINKER_EEP_ADDR_TYPESTATE + BLINKER_TYPESTATE_SIZE)

    // #define BLINKER_AUTODATA_SIZE           4

    // #define BLINKER_EEP_ADDR_TARGETKEY1     (BLINKER_EEP_ADDR_AUTO1 + BLINKER_AUTODATA_SIZE)

    // #define BLINKER_TARGETKEY_SIZE          12

    // #define BLINKER_EEP_ADDR_TARGETDATA1    (BLINKER_EEP_ADDR_TARGETKEY1 + BLINKER_TARGETKEY_SIZE)

    // #define BLINKER_TARGETDATA_SIZE         4

    // #define BLINKER_EEP_ADDR_AUTO2          (BLINKER_EEP_ADDR_TARGETDATA1 + BLINKER_TARGETDATA_SIZE)

    // #define BLINKER_AUTODATA_SIZE           4

    // #define BLINKER_EEP_ADDR_TARGETKEY2     (BLINKER_EEP_ADDR_AUTO2 + BLINKER_AUTODATA_SIZE)

    // #define BLINKER_TARGETKEY_SIZE          12

    // #define BLINKER_EEP_ADDR_TARGETDATA2    (BLINKER_EEP_ADDR_TARGETKEY2 + BLINKER_TARGETKEY_SIZE)

    // #define BLINKER_TARGETDATA_SIZE         4

    // // // #define BLINKER_TARGET_DATA_SIZE        (BLINKER_AUTOID_SIZE + BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + BLINKER_TARGETDATA_SIZE)

    // // #define BLINKER_EEP_ADDR_LINKDEVICE1    (BLINKER_EEP_ADDR_TARGETDATA2 + BLINKER_TARGETDATA_SIZE)

    // // #define BLINKER_LINKDEVICE_SIZE         26

    // // #define BLINKER_EEP_ADDR_LINKTYPE1      (BLINKER_EEP_ADDR_LINKDEVICE1 + BLINKER_LINKDEVICE_SIZE)

    // // #define BLINKER_LINKTYPE_SIZE           18

    // // #define BLINKER_EEP_ADDR_LINKDATA1      (BLINKER_EEP_ADDR_LINKTYPE1 + BLINKER_LINKTYPE_SIZE)

    // // #define BLINKER_LINKDATA_SIZE           212

    // // #define BLINKER_EEP_ADDR_LINKDEVICE2    (BLINKER_EEP_ADDR_LINKDATA1 + BLINKER_LINKDATA_SIZE)

    // // #define BLINKER_LINKDEVICE_SIZE         26

    // // #define BLINKER_EEP_ADDR_LINKTYPE2      (BLINKER_EEP_ADDR_LINKDEVICE2 + BLINKER_LINKDEVICE_SIZE)

    // // #define BLINKER_LINKTYPE_SIZE           18

    // // #define BLINKER_EEP_ADDR_LINKDATA2      (BLINKER_EEP_ADDR_LINKTYPE2 + BLINKER_LINKTYPE_SIZE)

    // // #define BLINKER_LINKDATA_SIZE           212

    // #define BLINKER_ONE_AUTO_DATA_SIZE      (BLINKER_AUTOID_SIZE + BLINKER_TYPESTATE_SIZE + \
    //                                         (BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + \
    //                                         BLINKER_TARGETDATA_SIZE) * 2)// + BLINKER_LINKDEVICE_SIZE + BLINKER_LINKTYPE_SIZE + BLINKER_LINKDATA_SIZE) * 2)


    // #define BLINKER_AUTONUM_SUB_SIZE        1

    // #define BLINKER_EEP_ADDR_AUTO_START_SUB (BLINKER_EEP_ADDR_AUTONUM_SUB + BLINKER_AUTONUM_SUB_SIZE)

    // ////////////////////////////////////////////////////////////////////////////////////////

    // #define BLINKER_EEP_ADDR_AUTOID_SUB     0

    // #define BLINKER_AUTOID_SUB_SIZE         4
    
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

    #ifndef BLINKER_BUTTON_PIN
        #define BLINKER_BUTTON_PIN              2
    #endif

    #define BLINKER_CMD_REGISTER            "register"

    #define BLINKER_DIY_DEVICE              "DiyArduino"

    #define BLINKER_SMART_PLUGIN            "OwnPlug"

    #define BLINKER_SMART_LAMP              "OwnLight"





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




#endif


#if defined(BLINKER_NATIVE_WIFI)

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



#endif
