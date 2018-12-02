#if defined(ESP8266) || defined(ESP32)

#define BLINKER_AT_MQTT
#define BLINKER_ESP_AT

#include "Blinker/BlinkerMQTTATBase.h"

uint32_t serialSet = BLINKER_SERIAL_DEFAULT;

#if defined(ESP8266)
SerialConfig ss_cfg;
#elif defined(ESP32)
uint32_t    ss_cfg;
#endif

#if defined(ESP8266)
SerialConfig serConfig()
#elif defined(ESP32)
uint32_t    serConfig()
#endif
{
    uint8_t _bitSet = serialSet & 0xFF;

    switch (_bitSet)
    {
        case BLINKER_SERIAL_5N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5N1"));
            return SERIAL_5N1;
        case BLINKER_SERIAL_6N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6N1"));
            return SERIAL_6N1;
        case BLINKER_SERIAL_7N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7N1"));
            return SERIAL_7N1;
        case BLINKER_SERIAL_8N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
        case BLINKER_SERIAL_5N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5N2"));
            return SERIAL_5N2;
        case BLINKER_SERIAL_6N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6N2"));
            return SERIAL_6N2;
        case BLINKER_SERIAL_7N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7N2"));
            return SERIAL_7N2;
        case BLINKER_SERIAL_8N2 :
            BLINKER_LOG(("SerialConfig SERIAL_8N2"));
            return SERIAL_8N2;
        case BLINKER_SERIAL_5E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5E1"));
            return SERIAL_5E1;
        case BLINKER_SERIAL_6E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6E1"));
            return SERIAL_6E1;
        case BLINKER_SERIAL_7E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7E1"));
            return SERIAL_7E1;
        case BLINKER_SERIAL_8E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8E1"));
            return SERIAL_8E1;
        case BLINKER_SERIAL_5E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5E2"));
            return SERIAL_5E2;
        case BLINKER_SERIAL_6E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6E2"));
            return SERIAL_6E2;
        case BLINKER_SERIAL_7E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7E2"));
            return SERIAL_7E2;
        case BLINKER_SERIAL_8E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8E2"));
            return SERIAL_8E2;
        case BLINKER_SERIAL_5O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5O1"));
            return SERIAL_5O1;
        case BLINKER_SERIAL_6O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6O1"));
            return SERIAL_6O1;
        case BLINKER_SERIAL_7O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7O1"));
            return SERIAL_7O1;
        case BLINKER_SERIAL_8O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8O1"));
            return SERIAL_8O1;
        case BLINKER_SERIAL_5O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5O2"));
            return SERIAL_5O2;
        case BLINKER_SERIAL_6O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6O2"));
            return SERIAL_6O2;
        case BLINKER_SERIAL_7O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7O2"));
            return SERIAL_7O2;
        case BLINKER_SERIAL_8O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8O2"));
            return SERIAL_8O2;
        default :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
    }
}

uint8_t parseMode(uint8_t _mode, uint8_t _pullState)
{
    uint8_t _modes = _mode << 4 | _pullState;

    switch(_modes)
    {
        case 0 << 4 | 0 :
            return INPUT;
        case 1 << 4 | 0 :
            return OUTPUT;
        case 0 << 4 | 1 :
            return INPUT_PULLUP;
        case 0 << 4 | 2 :
#if defined(ESP8266)
            return INPUT_PULLDOWN_16;
#elif defined(ESP32)
            return INPUT_PULLDOWN;
#endif
        default :
            return INPUT;
    }
}

#endif
