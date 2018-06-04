#ifndef BlinkerConfig_H
#define BlinkerConfig_H

#define BLINKER_VERSION                 "0.1.2"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_NTP_TIMEOUT             1000

#define BLINKER_GPS_MSG_LIMIT           30000UL

#define BLINKER_MQTT_MSG_LIMIT          1000UL

#define BLINKER_LINK_MSG_LIMIT          60000UL

#define BLINKER_MQTT_KEEPALIVE          120000UL

#define BLINKER_MAX_WIDGET_SIZE         16

#define BLINKER_OBJECT_NOT_AVAIL        -1

#ifndef BLINKER_MAX_READ_SIZE
    #define BLINKER_MAX_READ_SIZE       256
#endif

#ifndef BLINKER_MAX_SEND_SIZE
    #define BLINKER_MAX_SEND_SIZE       128
#endif

#define  BLINKER_NO_LOGO

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

#define BLINKER_CMD_AUTODATA            "autoData"

#define BLINKER_CMD_LOGICDATA           "logicData"

#define BLINKER_CMD_LOGICTYPE           "logicType"

#define BLINKER_CMD_LESS                "less"

#define BLINKER_CMD_EQUAL               "equal"

#define BLINKER_CMD_GREATER             "greater"

#define BLINKER_CMD_NUMBERIC            "numberic"

#define BLINKER_CMD_COMPARETYPE         "compareType"

#define BLINKER_CMD_TRIGGER             "trigger"

#define BLINKER_CMD_DURATION            "duration"

#define BLINKER_CMD_TARGETSTATE         "targetState"

#define BLINKER_CMD_TARGETDATA          "targetData"

#define BLINKER_CMD_TIMESLOT            "timeSlot"

#define BLINKER_CMD_LINKDEVICE          "linkDevice"

#define BLINKER_CMD_LINKTYPE            "linkType"

#define BLINKER_CMD_LINKDATA            "linkData"

#define BLINKER_JOYSTICK_VALUE_DEFAULT  128

#if defined(BLINKER_MQTT)

    #define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"

    #define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

    #define BLINKER_MQTT_ALIYUN_PORT        1883

    #define BLINKER_MQTT_BORKER_QCLOUD      "qcloud"

    #define BLINKER_MQTT_QCLOUD_HOST        "iotcloud-mqtt.gz.tencentdevices.com"

    #define BLINKER_MQTT_QCLOUD_PORT        1883

    #define BLINKER_MQTT_PING_TIMEOUT       30000UL

    #define BLINKER_MQTT_HOST_SIZE          50

    #define BLINKER_MQTT_ID_SIZE            110

    #define BLINKER_MQTT_NAME_SIZE          80

    #define BLINKER_MQTT_KEY_SIZE           50

    #define BLINKER_MQTT_PINFO_SIZE         12

    #define BLINKER_MQTT_UUID_SIZE          40

    #define BLINKER_MQTT_DEVICENAME_SIZE    13

#endif

#define BLINKER_TYPE_STATE              0

#define BLINKER_TYPE_NUMERIC            1

#define BLINKER_COMPARE_LESS            0

#define BLINKER_COMPARE_EQUAL           1

#define BLINKER_COMPARE_GREATER         2

#define BLINKER_CHECK_DATA              170

#define BLINKER_EEP_SIZE                4096

#define BLINKER_EEP_ADDR_CHECK          0

#define BLINKER_CHECK_SIZE              1

#define BLINKER_EEP_ADDR_AUTO           (BLINKER_EEP_ADDR_CHECK + BLINKER_CHECK_SIZE)

#define BLINKER_AUTODATA_SIZE           4

#define BLINKER_EEP_ADDR_TARGGETDATA    (BLINKER_EEP_ADDR_AUTO + BLINKER_AUTODATA_SIZE)

#define BLINKER_TARGGETDATA_SIZE        4

#define BLINKER_EEP_ADDR_LINKDEVICE     (BLINKER_EEP_ADDR_TARGGETDATA + BLINKER_TARGGETDATA_SIZE)

#define BLINKER_LINKDEVICE_SIZE         26

#define BLINKER_EEP_ADDR_LINKTYPE       (BLINKER_EEP_ADDR_LINKDEVICE + BLINKER_LINKDEVICE_SIZE)

#define BLINKER_LINKTYPE_SIZE           18

#define BLINKER_EEP_ADDR_LINKDATA       (BLINKER_EEP_ADDR_LINKTYPE + BLINKER_LINKTYPE_SIZE)

#define BLINKER_LINKDATA_SIZE           128

#endif