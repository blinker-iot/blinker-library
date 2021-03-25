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

        void freshAttachWidget(char _name[], blinker_callback_with_table_arg_t _func, blinker_callback_t _func2)
        {
            int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
            if(num >= 0 ) _Widgets_tab[num]->setFunc(_func, _func2);
            else attachWidget(_name, _func, _func2);
        }

        void strWidgetsParse(const char* _wName, const JsonObject& data);
        void intWidgetsParse(const char* _wName, const JsonObject& data);
        void rgbWidgetsParse(const char* _wName, const JsonObject& data);
        void tabWidgetsParse(const char* _wName, const JsonObject& data);

        uint8_t attachWidget(const char* _name, blinker_callback_with_string_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_table_arg_t _func, blinker_callback_t _func2);
    #endif

    #if defined(BLINKER_MIOT)
        void attachMIOTSetPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        { _MIOTPowerStateFunc_m = newFunction; }
        void attachMIOTSetPowerState(blinker_callback_with_string_arg_t newFunction)
        { _MIOTPowerStateFunc = newFunction; }
        void attachMIOTSetColor(blinker_callback_with_int32_arg_t newFunction)
        { _MIOTSetColorFunc = newFunction; }
        void attachMIOTSetMode(blinker_callback_with_uint8_arg_t newFunction)
        { _MIOTSetModeFunc = newFunction; }
        // void attachMIOTSetcMode(blinker_callback_with_string_arg_t newFunction)
        // { _MIOTSetcModeFunc = newFunction; }
        void attachMIOTSetBrightness(blinker_callback_with_string_arg_t newFunction)
        { _MIOTSetBrightnessFunc = newFunction; }
        // void attachMIOTRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        // { _MIOTSetRelativeBrightnessFunc = newFunction; }
        void attachMIOTSetColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        { _MIOTSetColorTemperature = newFunction; }
        // void attachMIOTRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        // { _MIOTSetRelativeColorTemperature = newFunction; }
        void attachMIOTQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        { _MIOTQueryFunc_m = newFunction; }
        void attachMIOTQuery(blinker_callback_with_int32_arg_t newFunction)
        { _MIOTQueryFunc = newFunction; }
    #endif

    private :
        bool        _fresh = false;

    #if defined(BLINKER_WIDGET)
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_tab = 0;

        class BlinkerWidgets_string *       _Widgets_str[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_int32 *        _Widgets_int[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_rgb *          _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_table *        _Widgets_tab[BLINKER_MAX_WIDGET_SIZE];
    #endif

        void heartBeat(const JsonObject& data);

        blinker_callback_with_string_arg_t _availableFunc = NULL;

    protected :
        void parse(char _data[], bool ex_data = false);

    #if defined(BLINKER_MIOT)
        void miotParse(const String & _data);

        blinker_callback_with_string_uint8_arg_t _MIOTPowerStateFunc_m = NULL;
        blinker_callback_with_string_arg_t  _MIOTPowerStateFunc = NULL;
        blinker_callback_with_int32_arg_t   _MIOTSetColorFunc = NULL;
        blinker_callback_with_uint8_arg_t   _MIOTSetModeFunc = NULL;
        // blinker_callback_with_string_arg_t  _MIOTSetcModeFunc = NULL;
        blinker_callback_with_string_arg_t  _MIOTSetBrightnessFunc = NULL;
        // blinker_callback_with_int32_arg_t   _MIOTSetRelativeBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t   _MIOTSetColorTemperature = NULL;
        // blinker_callback_with_int32_arg_t   _MIOTSetRelativeColorTemperature = NULL;
        blinker_callback_with_int32_uint8_arg_t _MIOTQueryFunc_m = NULL;
        blinker_callback_with_int32_arg_t   _MIOTQueryFunc = NULL;
    #endif
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
        for (uint8_t wNum = 0; wNum < _wCount_tab; wNum++) {
            tabWidgetsParse(_Widgets_tab[wNum]->getName(), root);
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
uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_table_arg_t _func,
        blinker_callback_t _func2)
{
    int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_tab < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_tab[_wCount_tab] = new BlinkerWidgets_table(_name, _func, _func2);
            _wCount_tab++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_tab: "), _wCount_tab);

            return _wCount_tab;
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

template <class Proto>
void BlinkerApi<Proto>::tabWidgetsParse(const char* _wName, const JsonObject& data)
{
    int8_t num = checkNum(_wName, _Widgets_tab, _wCount_tab);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    if (data.containsKey(_wName)) {
        String _setData = data[_wName];

        uint8_t _number = 0;

        blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();
                
        for (uint8_t num = 0; num < 5; num++)
        {
            if (strcmp(_setData.substring(num, num + 1).c_str(), "1") == 0)
            {
                if (wFunc) {
                    switch (num)
                    {
                        case 0:
                            wFunc(BLINKER_CMD_TAB_0);
                            break;
                        case 1:
                            wFunc(BLINKER_CMD_TAB_1);
                            break;
                        case 2:
                            wFunc(BLINKER_CMD_TAB_2);
                            break;
                        case 3:
                            wFunc(BLINKER_CMD_TAB_3);
                            break;
                        case 4:
                            wFunc(BLINKER_CMD_TAB_4);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        
        BLINKER_LOG_ALL(BLINKER_F("tabWidgetsParse isParsed"));
        _fresh = true;

        blinker_callback_t wFunc2 = _Widgets_tab[num]->getFunc2();
        if (wFunc2) {
            wFunc2();
        }
    }
}

#if defined(BLINKER_MIOT)
    template <class Proto>
    void BlinkerApi<Proto>::miotParse(const String & _data)
    {
        BLINKER_LOG_ALL(BLINKER_F("MIOT parse data: "), _data);

        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(_data);
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, _data);
        JsonObject root = jsonBuffer.as<JsonObject>();

        // if (!root.success()) return;
        if (error) return;

        if (root.containsKey(BLINKER_CMD_GET))
        {
            String value = root[BLINKER_CMD_GET];

            bool query_set = false;

            if(_MIOTQueryFunc) query_set = true;
            if(_MIOTQueryFunc_m) query_set = true;

            if (!query_set)
            {
                BLINKER_ERR_LOG("None query function set!");
            }

            if (value == BLINKER_CMD_STATE){
                uint8_t setNum = root[BLINKER_CMD_NUM];
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_ALL_NUMBER);
                if (_MIOTQueryFunc_m) _MIOTQueryFunc_m(BLINKER_CMD_QUERY_ALL_NUMBER, setNum);
            }
            else if (value == BLINKER_CMD_POWERSTATE) {
                uint8_t setNum = root[BLINKER_CMD_NUM];
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_POWERSTATE_NUMBER);
                if (_MIOTQueryFunc_m) _MIOTQueryFunc_m(BLINKER_CMD_QUERY_POWERSTATE_NUMBER, setNum);
            }
            else if (value == BLINKER_CMD_COLOR) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
            }
            else if (value == BLINKER_CMD_COLOR_) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
            }
            else if (value == BLINKER_CMD_COLORTEMP) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLORTEMP_NUMBER);
            }
            else if (value == BLINKER_CMD_BRIGHTNESS) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER);
            }
            else if (value == BLINKER_CMD_TEMP) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
            }
            else if (value == BLINKER_CMD_HUMI) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
            }
            else if (value == BLINKER_CMD_PM25) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
            }
            else if (value == BLINKER_CMD_MODE) {
                if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
            }
        }
        else if (root.containsKey(BLINKER_CMD_SET)) {
            String value = root[BLINKER_CMD_SET];

            // DynamicJsonBuffer jsonBufferSet;
            // JsonObject& rootSet = jsonBufferSet.parseObject(value);
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, value);
            JsonObject rootSet = jsonBuffer.as<JsonObject>();

            // if (!rootSet.success())
            if (error)
            {
                // BLINKER_ERR_LOG_ALL("Json error");
                return;
            }

            // BLINKER_LOG_ALL("Json parse success");

            if (rootSet.containsKey(BLINKER_CMD_POWERSTATE)) {
                String setValue = rootSet[BLINKER_CMD_POWERSTATE];
                uint8_t setNum = rootSet[BLINKER_CMD_NUM];

                if (setValue == "true") setValue = "on";
                else setValue = "off";                

                if (_MIOTPowerStateFunc) _MIOTPowerStateFunc(setValue);
                if (_MIOTPowerStateFunc_m) _MIOTPowerStateFunc_m(setValue, setNum);
            }
            else if (rootSet.containsKey(BLINKER_CMD_COLOR)) {
                String setValue = rootSet[BLINKER_CMD_COLOR];

                if (_MIOTSetColorFunc) _MIOTSetColorFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_COLOR_)) {
                String setValue = rootSet[BLINKER_CMD_COLOR_];

                if (_MIOTSetColorFunc) _MIOTSetColorFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_BRIGHTNESS)) {
                String setValue = rootSet[BLINKER_CMD_BRIGHTNESS];

                if (_MIOTSetBrightnessFunc) _MIOTSetBrightnessFunc(setValue);
            }
            // else if (rootSet.containsKey(BLINKER_CMD_UPBRIGHTNESS)) {
            //     String setValue = rootSet[BLINKER_CMD_UPBRIGHTNESS];

            //     if (_MIOTSetRelativeBrightnessFunc) _MIOTSetRelativeBrightnessFunc(setValue.toInt());
            // }
            // else if (rootSet.containsKey(BLINKER_CMD_DOWNBRIGHTNESS)) {
            //     String setValue = rootSet[BLINKER_CMD_DOWNBRIGHTNESS];

            //     if (_MIOTSetRelativeBrightnessFunc) _MIOTSetRelativeBrightnessFunc(- setValue.toInt());
            // }
            else if (rootSet.containsKey(BLINKER_CMD_COLORTEMP)) {
                String setValue = rootSet[BLINKER_CMD_COLORTEMP];

                if (_MIOTSetColorTemperature) _MIOTSetColorTemperature(setValue.toInt());
            }
            // else if (rootSet.containsKey(BLINKER_CMD_UPCOLORTEMP)) {
            //     String setValue = rootSet[BLINKER_CMD_UPCOLORTEMP];

            //     if (_MIOTSetRelativeColorTemperature) _MIOTSetRelativeColorTemperature(setValue.toInt());
            // }
            // else if (rootSet.containsKey(BLINKER_CMD_DOWNCOLORTEMP)) {
            //     String setValue = rootSet[BLINKER_CMD_DOWNCOLORTEMP];

            //     if (_MIOTSetRelativeColorTemperature) _MIOTSetRelativeColorTemperature(- setValue.toInt());
            // }
            else if (rootSet.containsKey(BLINKER_CMD_MODE)) {
                String setMode = rootSet[BLINKER_CMD_MODE];

                if (_MIOTSetModeFunc) _MIOTSetModeFunc(setMode.toInt());
            }
            // else if (rootSet.containsKey(BLINKER_CMD_CANCELMODE)) {
            //     String setcMode = rootSet[BLINKER_CMD_CANCELMODE];

            //     if (_MIOTSetcModeFunc) _MIOTSetcModeFunc(setcMode);
            // }
        }
    }
#endif

#endif

#endif
