#ifndef BLINKER_MIOT_H
#define BLINKER_MIOT_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BLINKERMIOT
{
    public :
        BLINKERMIOT(Functions& nFunc) : func(nFunc) {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            // func.attachMIOTSetPowerState(newFunction);
            _MIOTPowerStateFunc_m = newFunction;
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            // func.attachMIOTSetPowerState(newFunction);
            _MIOTPowerStateFunc = newFunction;
        }

        void attachColor(blinker_callback_with_int32_arg_t newFunction)
        {
            // func.attachMIOTSetColor(newFunction);
            _MIOTSetColorFunc = newFunction;
        }

        void attachMode(blinker_callback_with_uint8_arg_t newFunction)
        {
            // func.attachMIOTSetMode(newFunction);
            _MIOTSetModeFunc = newFunction;
        }

        // void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        // {
        //     func.attachMIOTSetcMode(newFunction);
        // }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            // func.attachMIOTSetBrightness(newFunction);
            _MIOTSetBrightnessFunc = newFunction;
        }

        // void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        // {
        //     func.attachMIOTRelativeBrightness(newFunction);
        // }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            // func.attachMIOTSetColorTemperature(newFunction);
            _MIOTSetColorTemperature = newFunction;
        }

        // void attachRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        // {
        //     func.attachMIOTRelativeColorTemperature(newFunction);
        // }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            // func.attachMIOTQuery(newFunction);
            _MIOTQueryFunc_m = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            // func.attachMIOTQuery(newFunction);
            _MIOTQueryFunc = newFunction;
        }

        void powerState(const String & state, uint8_t num)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":");

            if (state == "on")
            {
                payload += "true";
            }
            else
            {
                payload += "false";
            }
            
            if (num != 0)
            {
                payload += BLINKER_F(",\"num\":");
                payload += STRING_format(num);
            }
            else
            {
                payload += BLINKER_F("");
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
            payload += BLINKER_F("\":");

            if (state == "on")
            {
                payload += "true";
            }
            else
            {
                payload += "false";
            }
            
            // payload += BLINKER_F("\"");

            // func.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(uint32_t clr)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR_);
            payload += BLINKER_F("\":");
            payload += STRING_format(clr);
            payload += BLINKER_F(",");
            payload += BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR);
            payload += BLINKER_F("\":");
            payload += STRING_format(clr);

            // func.aligeniePrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(uint8_t md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":");
            payload += STRING_format(md);

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

        void co2(double _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void co2(float _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void co2(int _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String miData;

            if (_fresh >> 0 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aCtemp;
                
                free(aCtemp);
            }

            if (_fresh >> 4 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 5 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 6 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 7 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aPm25;
                
                free(aPm25);
            }

            if (_fresh >> 8 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aCO2;
                
                free(aCO2);
            }

            miData += BLINKER_F("}");

            _fresh = 0;

            func.miotPrint(miData);
        }

        void run()
        {
            if (func.miotAvail())
            {
                miotParse(func.lastRead());
            }
        }

    private :
        Functions&  func;
        char *      aState;
        char *      aColor;
        char *      aMode;
        char *      aCtemp;
        char *      aBright;
        char *      aTemp;
        char *      aHumi;
        char *      aPm25;
        char *      aCO2;  
        uint16_t    _fresh = 0;

        blinker_callback_with_string_uint8_arg_t    _MIOTPowerStateFunc_m = NULL;
        blinker_callback_with_string_arg_t          _MIOTPowerStateFunc = NULL;
        blinker_callback_with_int32_arg_t           _MIOTSetColorFunc = NULL;
        blinker_callback_with_uint8_arg_t           _MIOTSetModeFunc = NULL;
        // blinker_callback_with_string_arg_t       _MIOTSetcModeFunc = NULL;
        blinker_callback_with_string_arg_t          _MIOTSetBrightnessFunc = NULL;
        // blinker_callback_with_int32_arg_t        _MIOTSetRelativeBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t           _MIOTSetColorTemperature = NULL;
        // blinker_callback_with_int32_arg_t        _MIOTSetRelativeColorTemperature = NULL;
        blinker_callback_with_int32_uint8_arg_t     _MIOTQueryFunc_m = NULL;
        blinker_callback_with_int32_arg_t           _MIOTQueryFunc = NULL;

        void miotParse(const String & _data)
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
};

#endif
