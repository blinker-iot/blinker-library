#ifndef BLINKER_DUEROS_H
#define BLINKER_DUEROS_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BLINKERDUEROS
{
    public :
        BLINKERDUEROS(Functions& nFunc) : func(nFunc) {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            _DuerOSPowerStateFunc_m = newFunction;
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            _DuerOSPowerStateFunc = newFunction;
        }

        void attachColor(blinker_callback_with_int32_arg_t newFunction)
        {
            _DuerOSSetColorFunc = newFunction;
        }

        void attachMode(blinker_callback_with_string_arg_t newFunction)
        {
            _DuerOSSetModeFunc = newFunction;
        }

        void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        {
            _DuerOSSetcModeFunc = newFunction;
        }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            _DuerOSSetBrightnessFunc = newFunction;
        }

        void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        {
            _DuerOSSetRelativeBrightnessFunc = newFunction;
        }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            _DuerOSSetColorTemperature = newFunction;
        }

        void attachRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            _DuerOSSetRelativeColorTemperature = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            _DuerOSQueryFunc_m = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            _DuerOSQueryFunc = newFunction;
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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(int32_t clr)
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

            // func.DuerOSPrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(const String & now_md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":[\"\",\"");
            payload += now_md;
            payload += BLINKER_F("\"]");

            // func.DuerOSPrint(payload);

            if (_fresh >> 2 & 0x01) {
                free(aMode);
            }

            aMode = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aMode, payload.c_str());

            _fresh |= 0x01 << 2;
        }

        void mode(const String & pre_md, const String & now_md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":[\"");
            payload += pre_md;
            payload += BLINKER_F("\",\"");
            payload += now_md;
            payload += BLINKER_F("\"]");

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

            if (_fresh >> 3 & 0x01) {
                free(aCtemp);
            }

            aCtemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCtemp, payload.c_str());

            _fresh |= 0x01 << 3;
        }

        void brightness(int now_bright)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":[\"\",");
            payload += STRING_format(now_bright);
            payload += BLINKER_F("]");

            // func.DuerOSPrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aBright);
            }

            aBright = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aBright, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void brightness(int pre_bright, int now_bright)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":[");
            payload += STRING_format(pre_bright);
            payload += BLINKER_F(",");
            payload += STRING_format(now_bright);
            payload += BLINKER_F("]");

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

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

            // func.DuerOSPrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm10(double _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 8 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void pm10(float _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 8 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void pm10(int _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 8 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void co2(double _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 9 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 9;
        }

        void co2(float _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 9 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 9;
        }

        void co2(int _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 9 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 9;
        }

        void aqi(int _aqi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_AQI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_aqi);
            payload += BLINKER_F("\"");

            // func.DuerOSPrint(payload);

            if (_fresh >> 10 & 0x01) {
                free(aAQI);
            }

            aAQI = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aAQI, payload.c_str());

            _fresh |= 0x01 << 10;
        }        

        void time(uint32_t _time)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TIME_ALL);
            payload += BLINKER_F("\":");
            payload += STRING_format(_time/1000);

            // func.DuerOSPrint(payload);

            if (_fresh >> 11 & 0x01) {
                free(aTIME);
            }

            aTIME = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTIME, payload.c_str());

            _fresh |= 0x01 << 11;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String duerData;

            if (_fresh >> 0 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aCtemp;
                
                free(aCtemp);
            }

            if (_fresh >> 4 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 5 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 6 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 7 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aPm25;
                
                free(aPm25);
            }

            if (_fresh >> 8 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aPm10;
                
                free(aPm10);
            }

            if (_fresh >> 9 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aCO2;
                
                free(aCO2);
            }

            if (_fresh >> 10 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aAQI;
                
                free(aAQI);
            }

            if (_fresh >> 11 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aTIME;
                
                free(aTIME);
            }

            duerData += BLINKER_F("}");

            _fresh = 0;

            func.duerPrint(duerData);
        }

        void report()
        {
            if (_fresh == 0) return;
            
            String duerData;
            bool isCheck = false;

            if (_fresh >> 0 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aState;

                isCheck = true;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                
                    duerData += aColor;
                    
                    isCheck = true;
                }
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aMode;
                    
                    isCheck = true;
                }
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aBright;
                    
                    isCheck = true;
                }
                
                free(aBright);
            }

            if (_fresh >> 4 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aTemp;
                    
                    isCheck = true;
                }
                
                free(aTemp);
            }

            if (_fresh >> 5 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aHumi;
                    
                    isCheck = true;
                }
                
                free(aHumi);
            }

            if (_fresh >> 6 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aPm25;
                    
                    isCheck = true;
                }
                
                free(aPm25);
            }

            if (_fresh >> 7 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aPm10;
                    
                    isCheck = true;
                }
                
                free(aPm10);
            }

            if (_fresh >> 8 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aCO2;
                    
                    isCheck = true;
                }
                
                free(aCO2);
            }

            if (_fresh >> 9 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aAQI;
                    
                    isCheck = true;
                }
                
                free(aAQI);
            }

            if (_fresh >> 10 & 0x01) {
                if (!isCheck)
                {
                    if (duerData.length()) duerData += BLINKER_F(",");
                    else duerData += BLINKER_F("{");
                    
                    duerData += aTIME;
                    
                    isCheck = true;
                }
                
                free(aTIME);
            }

            duerData += BLINKER_F("}");

            _fresh = 0;

            func.duerPrint(duerData, true);
        }

        void run()
        {
            if (func.duerAvail())
            {
                duerParse(func.lastRead());
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
        char * aPm10;
        char * aCO2;        
        char * aAQI;
        char * aTIME;
        uint16_t _fresh = 0;

        blinker_callback_with_string_uint8_arg_t    _DuerOSPowerStateFunc_m = NULL;
        blinker_callback_with_string_arg_t      _DuerOSPowerStateFunc = NULL;
        blinker_callback_with_int32_arg_t       _DuerOSSetColorFunc = NULL;
        blinker_callback_with_string_arg_t      _DuerOSSetModeFunc = NULL;
        blinker_callback_with_string_arg_t      _DuerOSSetcModeFunc = NULL;
        blinker_callback_with_string_arg_t      _DuerOSSetBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t       _DuerOSSetRelativeBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t       _DuerOSSetColorTemperature = NULL;
        blinker_callback_with_int32_arg_t       _DuerOSSetRelativeColorTemperature = NULL;
        blinker_callback_with_int32_uint8_arg_t _DuerOSQueryFunc_m = NULL;
        blinker_callback_with_int32_arg_t       _DuerOSQueryFunc = NULL;

        void duerParse(const String & _data)
        {
            BLINKER_LOG_ALL(BLINKER_F("DuerOS parse data: "), _data);

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

                if(_DuerOSQueryFunc) query_set = true;
                if(_DuerOSQueryFunc_m) query_set = true;

                if (!query_set)
                {
                    BLINKER_ERR_LOG("None query function set!");
                }

                if (value == BLINKER_CMD_POWERSTATE) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_POWERSTATE_NUMBER);
                }
                else if (value == BLINKER_CMD_AQI) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_AQI_NUMBER);
                }
                else if (value == BLINKER_CMD_PM25) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
                }
                else if (value == BLINKER_CMD_PM10) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_PM10_NUMBER);
                }
                else if (value == BLINKER_CMD_CO2) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_CO2_NUMBER);
                }
                else if (value == BLINKER_CMD_TEMP) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
                }
                else if (value == BLINKER_CMD_HUMI) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
                }
                else if (value == BLINKER_CMD_MODE) {
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
                }
                else if (value == BLINKER_CMD_TIME_ALL) {
                    uint8_t setNum = root[BLINKER_CMD_NUM];
                    if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_TIME_NUMBER);
                    if (_DuerOSQueryFunc_m) _DuerOSQueryFunc_m(BLINKER_CMD_QUERY_TIME_NUMBER, setNum);
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

                    if (_DuerOSPowerStateFunc) _DuerOSPowerStateFunc(setValue);
                    if (_DuerOSPowerStateFunc_m) _DuerOSPowerStateFunc_m(setValue, setNum);
                }
                else if (rootSet.containsKey(BLINKER_CMD_COLOR)) {
                    String setValue = rootSet[BLINKER_CMD_COLOR];

                    if (_DuerOSSetColorFunc) _DuerOSSetColorFunc(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_COLOR_)) {
                    String setValue = rootSet[BLINKER_CMD_COLOR_];

                    if (_DuerOSSetColorFunc) _DuerOSSetColorFunc(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_BRIGHTNESS)) {
                    String setValue = rootSet[BLINKER_CMD_BRIGHTNESS];

                    if (_DuerOSSetBrightnessFunc) _DuerOSSetBrightnessFunc(setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_UPBRIGHTNESS)) {
                    String setValue = rootSet[BLINKER_CMD_UPBRIGHTNESS];

                    if (_DuerOSSetRelativeBrightnessFunc) _DuerOSSetRelativeBrightnessFunc(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_DOWNBRIGHTNESS)) {
                    String setValue = rootSet[BLINKER_CMD_DOWNBRIGHTNESS];

                    if (_DuerOSSetRelativeBrightnessFunc) _DuerOSSetRelativeBrightnessFunc(- setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_COLORTEMP)) {
                    String setValue = rootSet[BLINKER_CMD_COLORTEMP];

                    if (_DuerOSSetColorTemperature) _DuerOSSetColorTemperature(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_UPCOLORTEMP)) {
                    String setValue = rootSet[BLINKER_CMD_UPCOLORTEMP];

                    if (_DuerOSSetRelativeColorTemperature) _DuerOSSetRelativeColorTemperature(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_DOWNCOLORTEMP)) {
                    String setValue = rootSet[BLINKER_CMD_DOWNCOLORTEMP];

                    if (_DuerOSSetRelativeColorTemperature) _DuerOSSetRelativeColorTemperature(- setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_MODE)) {
                    String setMode = rootSet[BLINKER_CMD_MODE];

                    if (_DuerOSSetModeFunc) _DuerOSSetModeFunc(setMode);
                }
                else if (rootSet.containsKey(BLINKER_CMD_CANCELMODE)) {
                    String setcMode = rootSet[BLINKER_CMD_CANCELMODE];

                    if (_DuerOSSetcModeFunc) _DuerOSSetcModeFunc(setcMode);
                }
            }
        }
};

#endif
