#ifndef BLINKER_ALIGENIE_H
#define BLINKER_ALIGENIE_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BLINKERALIGENIE
{
    public :
        BLINKERALIGENIE(Functions& nFunc) : func(nFunc) {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            _AliGeniePowerStateFunc_m = newFunction;
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGeniePowerStateFunc = newFunction;
        }

        void attachHSwing(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieHSwingStateFunc = newFunction;
        }

        void attachVSwing(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieVSwingStateFunc = newFunction;
        }

        void attachColor(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieSetColorFunc = newFunction;
        }

        void attachMode(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieSetModeFunc = newFunction;
        }

        void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieSetcModeFunc = newFunction;
        }

        void attachLevel(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieSetLevelFunc_str = newFunction;
        }

        void attachLevel(blinker_callback_with_uint8_arg_t newFunction)
        {
            _AliGenieSetLevelFunc = newFunction;
        }

        void attachRelativeLevel(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieSetRelativeLevelFunc = newFunction;
        }

        void attachTemp(blinker_callback_with_uint8_arg_t newFunction)
        {
            _AliGenieSetTempFunc = newFunction;
        }

        void attachRelativeTemp(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieSetRelativeTempFunc = newFunction;
        }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            _AliGenieSetBrightnessFunc = newFunction;
        }

        void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieSetRelativeBrightnessFunc = newFunction;
        }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieSetColorTemperature = newFunction;
        }

        void attachRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieSetRelativeColorTemperature = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            _AliGenieQueryFunc_m = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            _AliGenieQueryFunc = newFunction;
        }

        void powerState(const String & state, uint8_t num)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            if (num != 0)
            {
                payload += BLINKER_F("\",\"num\":");
                payload += STRING_format(num);
            }
            else
            {
                payload += BLINKER_F("\"");
            }            

            // func.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void powerState(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & clr)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR_);
            payload += BLINKER_F("\":\"");
            payload += clr;
            payload += BLINKER_F("\",");
            payload += BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR);
            payload += BLINKER_F("\":\"");
            payload += clr;
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(const String & md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":\"");
            payload += md;
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 2 & 0x01) {
                free(aMode);
            }

            aMode = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aMode, payload.c_str());

            _fresh |= 0x01 << 2;
        }

        void colorTemp(int clrTemp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLORTEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(clrTemp);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 3 & 0x01) {
                free(aCtemp);
            }

            aCtemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCtemp, payload.c_str());

            _fresh |= 0x01 << 3;
        }

        void brightness(int bright)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(bright);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aBright);
            }

            aBright = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aBright, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void temp(double _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void temp(float _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void temp(int _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void humi(double _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void humi(float _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void humi(int _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void pm25(double _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm25(float _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm25(int _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void level(const String & _level)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_LEVEL);
            payload += BLINKER_F("\":\"");
            payload += _level;
            payload += BLINKER_F("\"");

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 8 & 0x01) {
                free(aLevel);
            }

            aLevel = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aLevel, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void hswing(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 9 & 0x01) {
                free(ahState);
            }

            ahState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(ahState, payload.c_str());

            _fresh |= 0x01 << 9;
        }

        void vswing(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_VSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 10 & 0x01) {
                free(avState);
            }

            avState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(avState, payload.c_str());

            _fresh |= 0x01 << 10;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String aliData;

            if (_fresh >> 0 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aCtemp;
                
                free(aCtemp);
            }

            if (_fresh >> 4 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 5 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 6 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 7 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aPm25;
                
                free(aPm25);
            }

            if (_fresh >> 8 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aLevel;
                
                free(aLevel);
            }

            if (_fresh >> 9 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += ahState;
                
                free(ahState);
            }

            if (_fresh >> 10 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += avState;
                
                free(avState);
            }

            aliData += BLINKER_F("}");

            _fresh = 0;

            func.aliPrint(aliData);
        }

        void run()
        {
            if (func.aliAvail())
            {
                aliParse(func.lastRead());
            }
        }

    private :
        Functions&  func;
        char * aState;
        char * aColor;
        char * aMode;
        char * aCtemp;
        char * aBright;
        char * aTemp;
        char * aHumi;
        char * aPm25;
        
        char * aLevel;
        char * ahState;
        char * avState;
        uint16_t _fresh = 0;

        blinker_callback_with_string_uint8_arg_t    _AliGeniePowerStateFunc_m = NULL;
        blinker_callback_with_string_arg_t          _AliGeniePowerStateFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieHSwingStateFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieVSwingStateFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieSetColorFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieSetModeFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieSetcModeFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieSetLevelFunc_str = NULL;
        blinker_callback_with_uint8_arg_t           _AliGenieSetLevelFunc = NULL;
        blinker_callback_with_uint8_arg_t           _AliGenieSetTempFunc = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieSetRelativeLevelFunc = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieSetRelativeTempFunc = NULL;
        blinker_callback_with_string_arg_t          _AliGenieSetBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieSetRelativeBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieSetColorTemperature = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieSetRelativeColorTemperature = NULL;
        blinker_callback_with_int32_uint8_arg_t     _AliGenieQueryFunc_m = NULL;
        blinker_callback_with_int32_arg_t           _AliGenieQueryFunc = NULL;

        void aliParse(const String & _data)
    {
        BLINKER_LOG_ALL(BLINKER_F("AliGenie parse data: "), _data);

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

            if(_AliGenieQueryFunc) query_set = true;
            if(_AliGenieQueryFunc_m) query_set = true;

            if (!query_set)
            {
                BLINKER_ERR_LOG("None query function set!");
            }

            if (value == BLINKER_CMD_STATE){
                uint8_t setNum = root[BLINKER_CMD_NUM];
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_ALL_NUMBER);
                if (_AliGenieQueryFunc_m) _AliGenieQueryFunc_m(BLINKER_CMD_QUERY_ALL_NUMBER, setNum);
            }
            else if (value == BLINKER_CMD_POWERSTATE) {
                uint8_t setNum = root[BLINKER_CMD_NUM];
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_POWERSTATE_NUMBER);
                if (_AliGenieQueryFunc_m) _AliGenieQueryFunc_m(BLINKER_CMD_QUERY_POWERSTATE_NUMBER, setNum);
            }
            else if (value == BLINKER_CMD_COLOR) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
            }
            else if (value == BLINKER_CMD_COLOR_) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
            }
            else if (value == BLINKER_CMD_COLORTEMP) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLORTEMP_NUMBER);
            }
            else if (value == BLINKER_CMD_BRIGHTNESS) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER);
            }
            else if (value == BLINKER_CMD_TEMP) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
            }
            else if (value == BLINKER_CMD_HUMI) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
            }
            else if (value == BLINKER_CMD_PM25) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
            }
            else if (value == BLINKER_CMD_MODE) {
                if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
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

                if (_AliGeniePowerStateFunc) _AliGeniePowerStateFunc(setValue);
                if (_AliGeniePowerStateFunc_m) _AliGeniePowerStateFunc_m(setValue, setNum);
            }
            else if (rootSet.containsKey(BLINKER_CMD_HSTATE)) {
                String setValue = rootSet[BLINKER_CMD_HSTATE];

                if (_AliGenieHSwingStateFunc) _AliGenieHSwingStateFunc(setValue);
            }
            else if (rootSet.containsKey(BLINKER_CMD_VSTATE)) {
                String setValue = rootSet[BLINKER_CMD_VSTATE];

                if (_AliGenieVSwingStateFunc) _AliGenieVSwingStateFunc(setValue);
            }
            else if (rootSet.containsKey(BLINKER_CMD_COLOR)) {
                String setValue = rootSet[BLINKER_CMD_COLOR];

                if (_AliGenieSetColorFunc) _AliGenieSetColorFunc(setValue);
            }
            else if (rootSet.containsKey(BLINKER_CMD_COLOR_)) {
                String setValue = rootSet[BLINKER_CMD_COLOR_];

                if (_AliGenieSetColorFunc) _AliGenieSetColorFunc(setValue);
            }
            else if (rootSet.containsKey(BLINKER_CMD_BRIGHTNESS)) {
                String setValue = rootSet[BLINKER_CMD_BRIGHTNESS];

                if (_AliGenieSetBrightnessFunc) _AliGenieSetBrightnessFunc(setValue);
            }
            else if (rootSet.containsKey(BLINKER_CMD_UPBRIGHTNESS)) {
                String setValue = rootSet[BLINKER_CMD_UPBRIGHTNESS];

                if (_AliGenieSetRelativeBrightnessFunc) _AliGenieSetRelativeBrightnessFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_DOWNBRIGHTNESS)) {
                String setValue = rootSet[BLINKER_CMD_DOWNBRIGHTNESS];

                if (_AliGenieSetRelativeBrightnessFunc) _AliGenieSetRelativeBrightnessFunc(- setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_COLORTEMP)) {
                String setValue = rootSet[BLINKER_CMD_COLORTEMP];

                if (_AliGenieSetColorTemperature) _AliGenieSetColorTemperature(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_UPCOLORTEMP)) {
                String setValue = rootSet[BLINKER_CMD_UPCOLORTEMP];

                if (_AliGenieSetRelativeColorTemperature) _AliGenieSetRelativeColorTemperature(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_DOWNCOLORTEMP)) {
                String setValue = rootSet[BLINKER_CMD_DOWNCOLORTEMP];

                if (_AliGenieSetRelativeColorTemperature) _AliGenieSetRelativeColorTemperature(- setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_MODE)) {
                String setMode = rootSet[BLINKER_CMD_MODE];

                if (_AliGenieSetModeFunc) _AliGenieSetModeFunc(setMode);
            }
            else if (rootSet.containsKey(BLINKER_CMD_CANCELMODE)) {
                String setcMode = rootSet[BLINKER_CMD_CANCELMODE];

                if (_AliGenieSetcModeFunc) _AliGenieSetcModeFunc(setcMode);
            }
            else if (rootSet.containsKey(BLINKER_CMD_LEVEL)) {
                String setLevel = rootSet[BLINKER_CMD_LEVEL];

                if (_AliGenieSetLevelFunc_str) _AliGenieSetLevelFunc_str(setLevel);
                if (_AliGenieSetLevelFunc) _AliGenieSetLevelFunc(setLevel.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_TEMP)) {
                String setValue = rootSet[BLINKER_CMD_TEMP];

                if (_AliGenieSetTempFunc) _AliGenieSetTempFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_LEVEL_UP)) {
                String setValue = rootSet[BLINKER_CMD_LEVEL_UP];

                if (_AliGenieSetRelativeLevelFunc) _AliGenieSetRelativeLevelFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_LEVEL_DW)) {
                String setValue = rootSet[BLINKER_CMD_LEVEL_DW];

                if (_AliGenieSetRelativeLevelFunc) _AliGenieSetRelativeLevelFunc(- setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_TEMP_UP)) {
                String setValue = rootSet[BLINKER_CMD_TEMP_UP];

                if (_AliGenieSetRelativeTempFunc) _AliGenieSetRelativeTempFunc(setValue.toInt());
            }
            else if (rootSet.containsKey(BLINKER_CMD_TEMP_DW)) {
                String setValue = rootSet[BLINKER_CMD_TEMP_DW];

                if (_AliGenieSetRelativeTempFunc) _AliGenieSetRelativeTempFunc(- setValue.toInt());
            }
        }
    }
};

#endif
