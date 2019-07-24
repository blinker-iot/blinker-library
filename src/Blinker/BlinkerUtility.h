#ifndef BlinkerUtility_H
#define BlinkerUtility_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#define NUMBERIC_VALUE_SKIP                 2
#define ARRAY_NUMBERIC_VALUE_SKIP_START     3
#define ARRAY_NUMBERIC_VALUE_SKIP_IN        1
#define STRING_VALUE_SKIP                   3
#define NUMBERIC_VALUE_END_1                ","
#define NUMBERIC_VALUE_END_2                "}"
#define ARRAY_NUMBERIC_VALUE_END_1          ","
#define ARRAY_NUMBERIC_VALUE_END_2          "]"
#define STRING_VALUE_END                    "\""

#define FIND_KEY_VALUE_FAILED               -1000

#if defined(BLINKER_ARDUINOJSON) || defined(BLINKER_PRO) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP)
    #ifndef ARDUINOJSON_VERSION_MAJOR
    #include "modules/ArduinoJson/ArduinoJson.h"
    #endif
#endif

extern "C" {
    // typedef void (*callbackFunction)(void);

    typedef void (*blinker_callback_t)(void);
    typedef void (*blinker_callback_with_arg_t)(void*);
    // typedef void (*blinker_callback_with_char_arg_t)(char*);
    typedef void (*blinker_callback_with_string_arg_t)(const String & data);
    typedef void (*blinker_callback_with_string_uint8_arg_t)(const String & data, uint8_t num);
    typedef void (*blinker_callback_with_uint8_arg_t)(uint8_t data);
    typedef void (*blinker_callback_with_int32_arg_t)(int32_t data);
    typedef void (*blinker_callback_with_int32_uint8_arg_t)(int32_t data, uint8_t num);
    typedef void (*blinker_callback_with_rgb_arg_t)(uint8_t r_data, uint8_t g_data, uint8_t b_data, uint8_t bright_data);
    typedef void (*blinker_callback_with_joy_arg_t)(uint8_t x_data, uint8_t y_data);
    typedef void (*blinker_callback_with_table_arg_t)(uint8_t data);
    typedef String (*blinker_callback_return_string_t)(void);
    #if defined(BLINKER_PRO) || defined(BLINKER_PRO_SIM7020) || \
        defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
        defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY) || \
        defined(BLINKER_WIFI_SUBDEVICE)
        typedef bool (*blinker_callback_with_json_arg_t)(const JsonObject & data);
    #endif
    typedef int (*blinker_callback_return_int_t)(void);
    // typedef void (*callback_with_bool_arg_t)(bool state);
}

// enum blinker_at_aligenie_t
// {
//     ALI_NONE,
//     ALI_LIGHT,
//     ALI_OUTLET,
//     ALI_SENSOR
// };

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_GPRS_AIR202) || \
    defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_SUBDEVICE)
    class BlinkerSharer
    {
        public :
            BlinkerSharer(const String & _uuid)
            {
                name = (char*)malloc((_uuid.length()+1)*sizeof(char));
                strcpy(name, _uuid.c_str());
            }

            char * uuid() { return name; }

        private :
            char * name;
    };

    #if defined(BLINKER_WIFI_GATEWAY)
        class BlinkerMeshSub
        {
            public :
                BlinkerMeshSub(uint32_t nodeId)
                {
                    _id = nodeId;
                    _authState = false;
                    _new = true;
                }

                bool isNew() { return _new; }

                void state(bool st) { _new = st; }

                void auth(const String & name, const String & key, 
                    const String & type, uint16_t vas)
                {
                    _new = false;
                    _name = (char*)malloc((name.length()+1)*sizeof(char));
                    strcpy(_name, name.c_str());
                    _key = (char*)malloc((key.length()+1)*sizeof(char));
                    strcpy(_key, key.c_str());
                    _type = (char*)malloc((type.length()+1)*sizeof(char));
                    strcpy(_type, type.c_str());
                    _vas = vas;

                    BLINKER_LOG_ALL("auth msg, name: ", _name, 
                                    ", key: ", _key,
                                    ", type: ", _type,
                                    ", vas: ", _vas);
                }

                void authData(const String & key, const String & name)
                {
                    _auth = (char*)malloc((key.length()+1)*sizeof(char));
                    strcpy(_auth, key.c_str());
                    _dId = (char*)malloc((name.length()+1)*sizeof(char));
                    strcpy(_dId, name.c_str());

                    BLINKER_LOG_ALL("register msg, authKey: ", _auth, 
                                    ", deviceId: ", _dId);

                    _authState = true;
                }

                void freshAuth(bool authState) { _authState = authState; }

                bool isAuth()       { return _authState; }

                char *name()        { return _name; }

                char *key()         { return _key; }

                char *type()        { return _type; }

                char *authKey()     { return _auth; }

                String deviceName()  { return _dId; }

                uint32_t id()       { return _id; }

            private :
                uint32_t    _id;
                bool        _authState;
                bool        _new;
                char        *_name;
                char        *_key;
                char        *_type;
                uint16_t    _vas = 0;
                char        *_auth;
                char        *_dId;
        };
    #endif
#endif

template<class T>
String STRING_format(T p) { return String(p); }

#if defined(ESP8266) || defined(ESP32)
String macDeviceName();
// #elif defined(ESP32)
// String macDeviceName();
#endif

template<class T>
const T& BlinkerMin(const T& a, const T& b) { return (b < a) ? b : a; }

template<class T>
const T& BlinkerMax(const T& a, const T& b) { return (b < a) ? a : b; }

String STRING_find_string(const String & src, const String & targetStart, const String & targetEnd, uint8_t skipNum);

bool STRING_contains_string(const String & src, const String & key);

bool STRING_find_string_value(const String & src, String & dst, const String & key);

int32_t STRING_find_numberic_value(const String & src, const String & key);

float STRING_find_float_value(const String & src, const String & key);

int32_t STRING_find_array_numberic_value(const String & src, const String & key, uint8_t num);

float STRING_find_array_float_value(const String & src, const String & key, uint8_t num);

String STRING_find_array_string_value(const String & src, const String & key, uint8_t num);

#endif
