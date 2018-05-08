#ifndef BlinkerConfig_H
#define BlinkerConfig_H

#define BLINKER_VERSION                 "0.1.1"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_GPS_MSG_LIMIT           30000UL

#define BLINKER_MQTT_MSG_LIMIT          1000UL

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

#define BLINKER_CMD_JOYSTICK            "joy"

#define BLINKER_CMD_GYRO                "gyro"

#define BLINKER_CMD_AHRS                "ahrs"

#define BLINKER_CMD_GPS                 "gps"

#define BLINKER_CMD_RGB                 "rgb"

#define BLINKER_CMD_VIBRATE             "vibrate"

#define BLINKER_CMD_BUTTON_TAP          "tap"

#define BLINKER_CMD_BUTTON_PRESSED      "pressed"

#define BLINKER_CMD_BUTTON_RELEASED     "released"

#define BLINKER_CMD_NEWLINE             "\n"

#define BLINKER_CMD_INTERSPACE          " "

#define BLINKER_CMD_GET                 "get"

#define BLINKER_CMD_STATE               "state"

#define BLINKER_CMD_ONLINE              "online"

#define BLINKER_CMD_CONNECTED           "connected"

#define BLINKER_CMD_VERSION             "version"

#define BLINKER_CMD_NOTICE              "notice"

#define BLINKER_JOYSTICK_VALUE_DEFAULT  128

#if defined(BLINKER_MQTT)

    #define BLINKER_MQTT_BORKER_ALIYUN  "aliyun"

    #define BLINKER_MQTT_ALIYUN_HOST    "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

    #define BLINKER_MQTT_ALIYUN_PORT    1883

    #define BLINKER_MQTT_BORKER_QCLOUD  "qcloud"

    #define BLINKER_MQTT_QCLOUD_HOST    "iotcloud-mqtt.gz.tencentdevices.com"

    #define BLINKER_MQTT_QCLOUD_PORT    1883

    #define BLINKER_MQTT_PING_TIMEOUT       30000UL

    #define BLINKER_MQTT_HOST_SIZE          50

    #define BLINKER_MQTT_ID_SIZE            110

    #define BLINKER_MQTT_NAME_SIZE          80

    #define BLINKER_MQTT_KEY_SIZE           50

    #define BLINKER_MQTT_PINFO_SIZE         12

    #define BLINKER_MQTT_UUID_SIZE          40

    #define BLINKER_MQTT_DEVICENAME_SIZE    13

#endif

#endif