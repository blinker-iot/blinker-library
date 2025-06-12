#ifndef BLINKER_MIOT_H
#define BLINKER_MIOT_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BLINKERMIOT
{
    public :
        BLINKERMIOT() {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            _MIOTPowerStateFunc_m = newFunction;
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            _MIOTPowerStateFunc = newFunction;
        }

        void attachHSwing(blinker_callback_with_string_arg_t newFunction)
        {
            _MIOTHSwingStateFunc = newFunction;
        }

        void attachVSwing(blinker_callback_with_string_arg_t newFunction)
        {
            _MIOTVSwingStateFunc = newFunction;
        }

        void attachColor(blinker_callback_with_int32_arg_t newFunction)
        {
            _MIOTSetColorFunc = newFunction;
        }

        void attachMode(blinker_callback_with_string_string_arg_t newFunction)
        {
            _MIOTSetModeFunc_m = newFunction;
        }

        void attachMode(blinker_callback_with_uint8_arg_t newFunction)
        {
            _MIOTSetModeFunc = newFunction;
        }

        void attachLevel(blinker_callback_with_uint8_arg_t newFunction)
        {
            _MIOTSetLevelFunc = newFunction;
        }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            _MIOTSetBrightnessFunc = newFunction;
        }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            _MIOTSetColorTemperature = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            _MIOTQueryFunc_m = newFunction;
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            _MIOTQueryFunc = newFunction;
        }

        void powerState(const String & state, uint8_t num)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");
            
            if (num != 0)
            {
                payload += BLINKER_F(",\"num\":");
                payload += STRING_format(num);
            }
            else
            {
                payload += BLINKER_F("");
            }            

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

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void humi(int _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

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

        void level(uint8_t _lvl)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_LEVEL);
            payload += BLINKER_F("\":");
            payload += STRING_format(_lvl);
            payload += BLINKER_F("");

            if (_fresh >> 9 & 0x01) {
                free(aLevel);
            }

            aLevel = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aLevel, payload.c_str());

            _fresh |= 0x01 << 9;
        }

        void hswing(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 10 & 0x01) {
                free(ahState);
            }

            ahState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(ahState, payload.c_str());

            _fresh |= 0x01 << 10;
        }

        void vswing(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_VSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 11 & 0x01) {
                free(avState);
            }

            avState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(avState, payload.c_str());

            _fresh |= 0x01 << 11;
        }
        
        void eco(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_ECO);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 12 & 0x01) {
                free(ecoState);
            }

            ecoState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(ecoState, payload.c_str());

            _fresh |= 0x01 << 12;
        }
        
        void anion(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_ANION);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 13 & 0x01) {
                free(anionState);
            }

            anionState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(anionState, payload.c_str());

            _fresh |= 0x01 << 13;
        }
        
        void heater(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HEATER);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 14 & 0x01) {
                free(heaterState);
            }

            heaterState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(heaterState, payload.c_str());

            _fresh |= 0x01 << 14;
        }
        
        void dryer(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_DRYER);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 15 & 0x01) {
                free(dryerState);
            }

            dryerState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(dryerState, payload.c_str());

            _fresh |= 0x01 << 15;
        }
        
        void sleep(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_SLEEP);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 16 & 0x01) {
                free(sleepState);
            }

            sleepState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(sleepState, payload.c_str());

            _fresh |= 0x01 << 16;
        }
        
        void soft(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_SOFT);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 17 & 0x01) {
                free(softState);
            }

            softState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(softState, payload.c_str());

            _fresh |= 0x01 << 17;
        }
        
        void uv(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_UV);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 18 & 0x01) {
                free(uvState);
            }

            uvState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(uvState, payload.c_str());

            _fresh |= 0x01 << 18;
        }
        
        void unStraightBlow(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_UNSB);
            payload += BLINKER_F("\":\"");
            payload += state;
            payload += BLINKER_F("\"");

            if (_fresh >> 19 & 0x01) {
                free(unsbState);
            }

            unsbState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(unsbState, payload.c_str());

            _fresh |= 0x01 << 19;
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

            if (_fresh >> 9 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aLevel;
                
                free(aLevel);
            }

            if (_fresh >> 10 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += ahState;
                
                free(ahState);
            }

            if (_fresh >> 11 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += avState;
                
                free(avState);
            }

            if (_fresh >> 12 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += ecoState;
                
                free(ecoState);
            }

            if (_fresh >> 13 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += anionState;
                
                free(anionState);
            }

            if (_fresh >> 14 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += heaterState;
                
                free(heaterState);
            }

            if (_fresh >> 15 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += dryerState;
                
                free(dryerState);
            }

            if (_fresh >> 16 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += sleepState;
                
                free(sleepState);
            }

            if (_fresh >> 17 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += softState;
                
                free(softState);
            }

            if (_fresh >> 18 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += uvState;
                
                free(uvState);
            }

            if (_fresh >> 19 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += unsbState;
                
                free(unsbState);
            }

            miData += BLINKER_F("}");

            _fresh = 0;

            Blinker.miotPrint(miData);
        }

        void run()
        {
            if (Blinker.miotAvail())
            {
                miotParse(Blinker.lastRead());
            }
        }

    private :
        char *      aState;
        char *      aColor;
        char *      aMode;
        char *      aCtemp;
        char *      aBright;
        char *      aTemp;
        char *      aHumi;
        char *      aPm25;
        char *      aCO2;

        char *      aLevel;
        char *      ahState;
        char *      avState;
        char *      ecoState;
        char *      anionState;
        char *      heaterState;
        char *      dryerState;
        char *      sleepState;
        char *      softState;
        char *      uvState;
        char *      unsbState;
        uint16_t    _fresh = 0;

        blinker_callback_with_string_uint8_arg_t    _MIOTPowerStateFunc_m = NULL;
        blinker_callback_with_string_arg_t          _MIOTPowerStateFunc = NULL;
        blinker_callback_with_string_arg_t          _MIOTHSwingStateFunc = NULL;
        blinker_callback_with_string_arg_t          _MIOTVSwingStateFunc = NULL;
        blinker_callback_with_int32_arg_t           _MIOTSetColorFunc = NULL;
        blinker_callback_with_string_string_arg_t   _MIOTSetModeFunc_m = NULL;
        blinker_callback_with_uint8_arg_t           _MIOTSetModeFunc = NULL;
        blinker_callback_with_uint8_arg_t           _MIOTSetLevelFunc = NULL;
        blinker_callback_with_uint8_arg_t           _MIOTSetTempFunc = NULL;
        blinker_callback_with_uint8_arg_t           _MIOTSetHumiFunc = NULL;
        blinker_callback_with_string_arg_t          _MIOTSetBrightnessFunc = NULL;
        blinker_callback_with_int32_arg_t           _MIOTSetColorTemperature = NULL;
        blinker_callback_with_int32_uint8_arg_t     _MIOTQueryFunc_m = NULL;
        blinker_callback_with_int32_arg_t           _MIOTQueryFunc = NULL;

        void miotParse(const String & _data)
        {
            BLINKER_LOG_ALL(BLINKER_F("MIOT parse data: "), _data);

            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, _data);
            JsonObject root = jsonBuffer.as<JsonObject>();

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

                DynamicJsonDocument jsonBuffer(1024);
                DeserializationError error = deserializeJson(jsonBuffer, value);
                JsonObject rootSet = jsonBuffer.as<JsonObject>();

                if (error)
                {
                    return;
                }

                if (rootSet.containsKey(BLINKER_CMD_POWERSTATE1)) {
                    String setValue = rootSet[BLINKER_CMD_POWERSTATE1];
                    uint8_t setNum = rootSet[BLINKER_CMD_NUM];

                    if (_MIOTPowerStateFunc) _MIOTPowerStateFunc(setValue);
                    if (_MIOTPowerStateFunc_m) _MIOTPowerStateFunc_m(setValue, setNum);
                }
                else if (rootSet.containsKey(BLINKER_CMD_HSTATE)) {
                    String setValue = rootSet[BLINKER_CMD_HSTATE];

                    if (_MIOTHSwingStateFunc) _MIOTHSwingStateFunc(setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_VSTATE)) {
                    String setValue = rootSet[BLINKER_CMD_VSTATE];

                    if (_MIOTVSwingStateFunc) _MIOTVSwingStateFunc(setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_ECO)) {
                    String setValue = rootSet[BLINKER_CMD_ECO];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_ANION)) {
                    String setValue = rootSet[BLINKER_CMD_ANION];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_HEATER)) {
                    String setValue = rootSet[BLINKER_CMD_HEATER];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_DRYER)) {
                    String setValue = rootSet[BLINKER_CMD_DRYER];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_SLEEP)) {
                    String setValue = rootSet[BLINKER_CMD_SLEEP];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_SOFT)) {
                    String setValue = rootSet[BLINKER_CMD_SOFT];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_UV)) {
                    String setValue = rootSet[BLINKER_CMD_UV];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
                }
                else if (rootSet.containsKey(BLINKER_CMD_UNSB)) {
                    String setValue = rootSet[BLINKER_CMD_UNSB];

                    if (_MIOTSetModeFunc_m) _MIOTSetModeFunc_m(BLINKER_CMD_ECO, setValue);
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
                else if (rootSet.containsKey(BLINKER_CMD_COLORTEMP)) {
                    String setValue = rootSet[BLINKER_CMD_COLORTEMP];

                    if (_MIOTSetColorTemperature) _MIOTSetColorTemperature(setValue.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_MODE)) {
                    String setMode = rootSet[BLINKER_CMD_MODE];

                    if (_MIOTSetModeFunc) _MIOTSetModeFunc(setMode.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_LEVEL)) {
                    String setLevel = rootSet[BLINKER_CMD_LEVEL];

                    if (_MIOTSetLevelFunc) _MIOTSetLevelFunc(setLevel.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_TEMP)) {
                    String setTemp = rootSet[BLINKER_CMD_TEMP];

                    if (_MIOTSetTempFunc) _MIOTSetTempFunc(setTemp.toInt());
                }
                else if (rootSet.containsKey(BLINKER_CMD_HUMI)) {
                    String setHumi = rootSet[BLINKER_CMD_HUMI];

                    if (_MIOTSetHumiFunc) _MIOTSetHumiFunc(setHumi.toInt());
                }
            }
        }
};

#endif
