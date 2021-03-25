#ifndef BLINKER_API_H
#define BLINKER_API_H

#include "Blinker/BlinkerApiBase.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerUtility.h"

enum b_rgb_t {
    BLINKER_R,
    BLINKER_G,
    BLINKER_B,
    BLINKER_BRIGHT
};

template <class Proto>
class BlinkerApi
{
    public :
        void attachData(blinker_callback_with_string_arg_t newFunction)
        { _availableFunc = newFunction; }
    
    #if defined(BLINKER_WIDGET)
        void freshAttachWidget(const char* _name, blinker_callback_with_string_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
            if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
            else attachWidget(_name, _func);            
        }
        
        void freshAttachWidget(const char* _name, blinker_callback_with_int32_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
            if(num >= 0 ) _Widgets_int[num]->setFunc(_func);
            else attachWidget(_name, _func);            
        }
        
        void freshAttachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
            if(num >= 0 ) _Widgets_rgb[num]->setFunc(_func);
            else attachWidget(_name, _func);            
        }

        void strWidgetsParse(const char* _wName, const JsonObject& data);
        void intWidgetsParse(const char* _wName, const JsonObject& data);
        void rgbWidgetsParse(const char* _wName, const JsonObject& data);

        uint8_t attachWidget(const char* _name, blinker_callback_with_string_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func);
    #endif

    private :
        bool        _fresh = false;

    #if defined(BLINKER_WIDGET)
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_rgb = 0;

        class BlinkerWidgets_string *       _Widgets_str[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_int32 *        _Widgets_int[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_rgb *          _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
    #endif

        void heartBeat(const JsonObject& data);

        blinker_callback_with_string_arg_t _availableFunc = NULL;

    protected :
        void parse(char _data[], bool ex_data = false);
};

template <class Proto>
void BlinkerApi<Proto>::parse(char _data[], bool ex_data)
{
    BLINKER_LOG_ALL(BLINKER_F("parse data: "), _data);
    
    if (!ex_data)
    {
        _fresh = false;

        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, STRING_format(_data));
        JsonObject root = jsonBuffer.as<JsonObject>();

        if (error)
        {
            return;
        }

        heartBeat(root);

    #if defined(BLINKER_WIDGET)
        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), root);
        }
        for (uint8_t wNum = 0; wNum < _wCount_int; wNum++) {
            intWidgetsParse(_Widgets_int[wNum]->getName(), root);
        }
        for (uint8_t wNum = 0; wNum < _wCount_rgb; wNum++) {
            rgbWidgetsParse(_Widgets_rgb[wNum]->getName(), root);
        }
    #endif

        if (_fresh)
        {
            static_cast<Proto*>(this)->flush();
        }
        else
        {
            if (_availableFunc)
            {
                _availableFunc(_data);
            }
            static_cast<Proto*>(this)->flush();
        }
    }
    else
    {}
}

template <class Proto>
void BlinkerApi<Proto>::heartBeat(const JsonObject& data)
{
    String state = data[BLINKER_CMD_GET];

    // if (state.length())
    if (data.containsKey(BLINKER_CMD_GET))
    {
        if (state == BLINKER_CMD_STATE)
        {
            static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
            static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_OTA_VERSION_CODE);
            static_cast<Proto*>(this)->checkState(false);
            static_cast<Proto*>(this)->printNow();

            _fresh = true;
        }
    }
}

#if defined(BLINKER_WIDGET)
template <class Proto>
uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_str <= BLINKER_MAX_WIDGET_SIZE)
        {
            _Widgets_str[_wCount_str] = new BlinkerWidgets_string(_name, _func);
            _wCount_str++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_str: "), _wCount_str);
            return _wCount_str;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

template <class Proto>
uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_int32_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_int < BLINKER_MAX_WIDGET_SIZE)
        {
            _Widgets_int[_wCount_int] = new BlinkerWidgets_int32(_name, _func);
            _wCount_int++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_int: "), _wCount_int);

            return _wCount_int;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

template <class Proto>
uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE/2)
        {
            _Widgets_rgb[_wCount_rgb] = new BlinkerWidgets_rgb(_name, _func);
            _wCount_rgb++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_rgb: "), _wCount_rgb);

            return _wCount_rgb;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

template <class Proto>
void BlinkerApi<Proto>::strWidgetsParse(const char* _wName, const JsonObject& data)
{
    int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    if (data.containsKey(_wName))
    {
        String state = data[_wName];
        BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse isParsed"));
        _fresh = true;

        BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse: "), _wName);

        blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();

        if (nbFunc) nbFunc(state);
    }
}

template <class Proto>
void BlinkerApi<Proto>::intWidgetsParse(const char* _wName, const JsonObject& data)
{
    int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    if (data.containsKey(_wName)) {
        int _number = data[_wName];
        BLINKER_LOG_ALL(BLINKER_F("intWidgetsParse isParsed"));
        _fresh = true;

        blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
        if (wFunc) {
            wFunc(_number);
        }
    }
}

template <class Proto>
void BlinkerApi<Proto>::rgbWidgetsParse(const char* _wName, const JsonObject& data)
{
    int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    if (data.containsKey(_wName))
    {
        uint8_t _rValue = data[_wName][BLINKER_R];
        uint8_t _gValue = data[_wName][BLINKER_G];
        uint8_t _bValue = data[_wName][BLINKER_B];
        uint8_t _brightValue = data[_wName][BLINKER_BRIGHT];
        BLINKER_LOG_ALL(BLINKER_F("rgbWidgetsParse isParsed"));
        _fresh = true;

        blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
        if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
    }
}
#endif

#endif
