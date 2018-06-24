#ifndef BlinkerConfig_H
#define BlinkerConfig_H

#include <utility/BlinkerDebug.h>

#define BLINKER_VERSION                 "0.1.5"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_NTP_TIMEOUT             1000UL

#define BLINKER_GPS_MSG_LIMIT           30000UL

#define BLINKER_PRINT_MSG_LIMIT         20

#define BLINKER_MQTT_MSG_LIMIT          1000UL

#define BLINKER_LINK_MSG_LIMIT          60000UL

#define BLINKER_MQTT_KEEPALIVE          120000UL

#define BLINKER_SMS_MSG_LIMIT           60000UL

#define BLINKER_SMS_MAX_SEND_SIZE       128

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
    #define BLINKER_MAX_WIDGET_SIZE         16
#else
    #define BLINKER_MAX_WIDGET_SIZE         16
#endif

#define BLINKER_OBJECT_NOT_AVAIL        -1

#ifndef BLINKER_MAX_READ_SIZE
#if defined(ESP8266) || defined(ESP32)
    #define BLINKER_MAX_READ_SIZE       512
#else
    #define BLINKER_MAX_READ_SIZE       256
#endif
#endif

#ifndef BLINKER_MAX_SEND_SIZE
#if defined(ESP8266) || defined(ESP32)
    #define BLINKER_MAX_SEND_SIZE       512
#else
    #define BLINKER_MAX_SEND_SIZE       128
#endif
#endif

#define BLINKER_AUTHKEY_SIZE            14

#define BLINKER_NO_LOGO

//#define BLINKER_LOGO_3D

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

#define BLINKER_CMD_NEWLINE             "\n"

#define BLINKER_CMD_INTERSPACE          " "

#define BLINKER_CMD_DATA                "data"

#define BLINKER_CMD_GET                 "get"

#define BLINKER_CMD_SET                 "set"

#define BLINKER_CMD_STATE               "state"

#define BLINKER_CMD_ONLINE              "online"

#define BLINKER_CMD_CONNECTED           "connected"

#define BLINKER_CMD_VERSION             "version"

#define BLINKER_CMD_NOTICE              "notice"

#define BLINKER_CMD_NOTFOUND            "device not found"

#define BLINKER_CMD_AUTO                "auto"

#define BLINKER_CMD_AUTOID              "autoId"

#define BLINKER_CMD_AUTODATA            "autoData"

#define BLINKER_CMD_DELETID             "deletId"

#define BLINKER_CMD_LOGICDATA           "logicData"

#define BLINKER_CMD_LOGICTYPE           "logicType"

#define BLINKER_CMD_LESS                "less"

#define BLINKER_CMD_EQUAL               "equal"

#define BLINKER_CMD_GREATER             "greater"

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

#define BLINKER_CMD_LINKDEVICE          "linkDevice"

#define BLINKER_CMD_LINKTYPE            "linkType"

#define BLINKER_CMD_LINKDATA            "linkData"

#define BLINKER_CMD_COUNTDOWN           "countdown"

#define BLINKER_CMD_COUNTDOWNDATA       "countdownData"

#define BLINKER_CMD_TOTALTIME           "totalTime"

#define BLINKER_CMD_RUNTIME             "runTime"

#define BLINKER_CMD_ACTION              "action"

#define BLINKER_CMD_ACTION1             "action1"

#define BLINKER_CMD_ACTION2             "action2"

#define BLINKER_CMD_LOOP                "loop"

#define BLINKER_CMD_LOOPDATA            "loopData"

#define BLINKER_CMD_TIMES               "times"

#define BLINKER_CMD_TIME1               "time1"

#define BLINKER_CMD_TIME2               "time2"

#define BLINKER_CMD_TIMING              "timing"

#define BLINKER_CMD_TIMINGDATA          "timingData"

#define BLINKER_CMD_DAY                 "day"

#define BLINKER_CMD_TASK                "task"

#define BLINKER_CMD_DETAIL              "detail"

#define BLINKER_CMD_MESSAGE             "message"

#define BLINKER_CMD_DEVICENAME          "deviceName"

#define BLINKER_CMD_IOTID               "iotId"

#define BLINKER_CMD_IOTTOKEN            "iotToken"

#define BLINKER_CMD_PRODUCTKEY          "productKey"

#define BLINKER_CMD_BROKER              "broker"

#define BLINKER_CMD_UUID                "uuid"

#define BLINKER_JOYSTICK_VALUE_DEFAULT  128

#define BLINKER_ONE_DAY_TIME            86400UL

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)

    #define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"

    #define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

    #define BLINKER_MQTT_ALIYUN_PORT        1883

    #define BLINKER_MQTT_BORKER_QCLOUD      "qcloud"

    #define BLINKER_MQTT_QCLOUD_HOST        "iotcloud-mqtt.gz.tencentdevices.com"

    #define BLINKER_MQTT_QCLOUD_PORT        1883

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

#endif

#if defined(ESP8266) || defined(ESP32)

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

    #define BLINKER_AUTONUM_SIZ             1

    #define BLINKER_EEP_ADDR_AUTO_START     (BLINKER_EEP_ADDR_AUTONUM + BLINKER_AUTONUM_SIZ)

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

    // #define BLINKER_TARGET_DATA_SIZE        (BLINKER_AUTOID_SIZE + BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + BLINKER_TARGETDATA_SIZE)

    #define BLINKER_EEP_ADDR_LINKDEVICE1    (BLINKER_EEP_ADDR_TARGETDATA2 + BLINKER_TARGETDATA_SIZE)

    #define BLINKER_LINKDEVICE_SIZE         26

    #define BLINKER_EEP_ADDR_LINKTYPE1      (BLINKER_EEP_ADDR_LINKDEVICE1 + BLINKER_LINKDEVICE_SIZE)

    #define BLINKER_LINKTYPE_SIZE           18

    #define BLINKER_EEP_ADDR_LINKDATA1      (BLINKER_EEP_ADDR_LINKTYPE1 + BLINKER_LINKTYPE_SIZE)

    #define BLINKER_LINKDATA_SIZE           212

    #define BLINKER_EEP_ADDR_LINKDEVICE2    (BLINKER_EEP_ADDR_LINKDATA1 + BLINKER_LINKDATA_SIZE)

    #define BLINKER_LINKDEVICE_SIZE         26

    #define BLINKER_EEP_ADDR_LINKTYPE2      (BLINKER_EEP_ADDR_LINKDEVICE2 + BLINKER_LINKDEVICE_SIZE)

    #define BLINKER_LINKTYPE_SIZE           18

    #define BLINKER_EEP_ADDR_LINKDATA2      (BLINKER_EEP_ADDR_LINKTYPE2 + BLINKER_LINKTYPE_SIZE)

    #define BLINKER_LINKDATA_SIZE           212

    #define BLINKER_ONE_AUTO_DATA_SIZE      (BLINKER_AUTOID_SIZE + BLINKER_TYPESTATE_SIZE + (BLINKER_AUTODATA_SIZE + BLINKER_TARGETKEY_SIZE + BLINKER_TARGETDATA_SIZE + BLINKER_LINKDEVICE_SIZE + BLINKER_LINKTYPE_SIZE + BLINKER_LINKDATA_SIZE) * 2)

#endif

#if defined(BLINKER_PRO)

    #ifndef BLINKER_BUTTON_PIN
        #define BLINKER_BUTTON_PIN              2
    #endif

    #define BLINKER_CMD_REGISTER            "register"

    #define BLINKER_AIR_DETECTOR            "OwnAirdetector"

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

    #define BLINKER_PRO_DATA_SIZE           (BLINKER_SSID_SIZE + BLINKER_PSWD_SIZE + BLINKER_WLAN_CHECK_SIZE + BLINKER_AUUID_SIZE + BLINKER_AUTH_CHECK_SIZE)

#endif

#endif