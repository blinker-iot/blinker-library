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

extern "C" {
    typedef void (*callbackFunction)(void);

    typedef void (*callback_t)(void);
    typedef void (*callback_with_arg_t)(void*);
    typedef void (*callback_with_string_arg_t)(const String & data);
    typedef void (*callback_with_int32_arg_t)(int32_t data);
    typedef void (*callback_with_rgb_arg_t)(uint8_t r_data, uint8_t g_data, uint8_t b_data, uint8_t bright_data);
    typedef void (*callback_with_joy_arg_t)(uint8_t x_data, uint8_t y_data);
    typedef String (*callback_return_string_t)(void);
#if defined(ESP8266) || defined(ESP32)
    // typedef bool (*callback_with_json_arg_t)(const JsonObject & data);
#endif
    // typedef void (*callback_with_bool_arg_t)(bool state);
}

template<class T>
String STRING_format(T p);

#if defined(ESP8266)
String macDeviceName();
#elif defined(ESP32)
String macDeviceName();
#endif

template<class T>
const T& BlinkerMin(const T& a, const T& b);

template<class T>
const T& BlinkerMax(const T& a, const T& b);

String STRING_find_string(const String & src, const String & targetStart, const String & targetEnd, uint8_t skipNum);

bool STRING_contains_string(const String & src, const String & key);

bool STRING_find_string_value(const String & src, String & dst, const String & key);

int32_t STRING_find_numberic_value(const String & src, const String & key);

float STRING_find_float_value(const String & src, const String & key);

int32_t STRING_find_array_numberic_value(const String & src, const String & key, uint8_t num);

float STRING_find_array_float_value(const String & src, const String & key, uint8_t num);

String STRING_find_array_string_value(const String & src, const String & key, uint8_t num);

#endif