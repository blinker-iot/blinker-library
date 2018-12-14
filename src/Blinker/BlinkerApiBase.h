#ifndef BLINKER_API_BASE_H
#define BLINKER_API_BASE_H

#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerConfig.h"
#include "utility/BlinkerUtility.h"

template <class T>
int8_t checkNum(char * name, T * c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        if (c[cNum]->checkName(name))
            return cNum;
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

class BlinkerWidgets_string
{
    public :
        BlinkerWidgets_string(char * _name, blinker_callback_with_string_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_string_arg_t _func) { wfunc = _func; }
        blinker_callback_with_string_arg_t getFunc() { return wfunc; }
        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_string_arg_t wfunc;
};

class BlinkerWidgets_int32
{
    public :
        BlinkerWidgets_int32(char * _name, blinker_callback_with_int32_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_int32_arg_t _func) { wfunc = _func; }
        blinker_callback_with_int32_arg_t getFunc() { return wfunc; }
        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_int32_arg_t wfunc;
};

class BlinkerWidgets_rgb
{
    public :
        BlinkerWidgets_rgb(char * _name, blinker_callback_with_rgb_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_rgb_arg_t _func) { wfunc = _func; }
        blinker_callback_with_rgb_arg_t getFunc() { return wfunc; }
        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_rgb_arg_t wfunc;
};

class BlinkerWidgets_joy
{
    public :
        BlinkerWidgets_joy(char * _name, blinker_callback_with_joy_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_joy_arg_t _func) { wfunc = _func; }
        blinker_callback_with_joy_arg_t getFunc() { return wfunc; }
        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_joy_arg_t wfunc;
};

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
    class BlinkerBridge_key
    {
        public :
            BlinkerBridge_key(char * _key, blinker_callback_with_string_arg_t _func = NULL)
            {
                bKey = (char*)malloc((strlen(_key)+1)*sizeof(char));
                strcpy(bKey, _key);

                wfunc = _func;
            }

            char * getKey() { return bKey; }
            void setFunc(blinker_callback_with_string_arg_t _func) { wfunc = _func; }
            blinker_callback_with_string_arg_t getFunc() { return wfunc; }
            bool checkName(char * _key) { return strcmp(_key, bKey) == 0; }
            char * getName()
            {
                if (_register) return bName;
                else return NULL;
            }
            void name(const String & name)
            {
                _register = true;
                bName = (char*)malloc((name.length()+1)*sizeof(char));
                strcpy(bName, name.c_str());
            }

        private :
            char *bKey;
            char *bName;
            bool _register = false;
            blinker_callback_with_string_arg_t wfunc;
        // public :
        //     BlinkerBridge() {}

        //     void name(char name[])
        //     {
        //         _bName = (char*)malloc((strlen(name)+1)*sizeof(char));
        //         strcpy(_bName, name);
        //     }
        //     char * getName() { return _bName; }
        //     void freshBridge(const String & name)
        //     {
        //         bridgeName = (char*)malloc((name.length()+1)*sizeof(char));
        //         strcpy(bridgeName, name.c_str());
        //     }
        //     char * getBridge() { return bridgeName; }
        //     bool checkName(char name[]) { return strcmp(_bName, name) == 0; }

        // private :
        //     char *_bName;
        //     char *bridgeName;
    };

    class BlinkerData
    {
        public :
            BlinkerData()
                // : _dname(NULL)
            {
                memcpy(data,"\0",256);
            }

            void name(const String & name) { _dname = name; }

            String getName() { return _dname; }

            void saveData(const String & _data) {
                String _data_;

                if (strlen(data)) _data_ = STRING_format(data);

                if (dataCount == 6) {
                    _data_ += BLINKER_F(",[");
                    _data_ += STRING_format(millis());
                    _data_ += BLINKER_F(",");
                    _data_ += _data;
                    _data_ += BLINKER_F("]");

                    _data_ = "{\"data\":[" + STRING_format(_data_);
                    _data_ += BLINKER_F("]}");

                    DynamicJsonBuffer jsonDataBuffer;
                    JsonObject& dataArray = jsonDataBuffer.parseObject(_data_);

                    for (uint8_t num = 0; num < dataCount; num++) {
                        dataArray["data"][num][0] = dataArray["data"][num+1][0];
                        dataArray["data"][num][1] = dataArray["data"][num+1][1];
                    }
                    
                    _data_ = BLINKER_F("");
                    for (uint8_t num = 0; num < dataCount; num++)
                    {
                        String data_get = dataArray["data"][num];

                        _data_ += data_get;

                        if (num != dataCount - 1) _data_ += BLINKER_F(",");
                    }

                    if (_data_.length() < 256)
                    {
                        strcpy(data, _data_.c_str());
                    }
                }
                else
                {
                    if (_data_.length())
                    {
                        _data_ += BLINKER_F(",");
                    }

                    _data_ += BLINKER_F("[");
                    _data_ += STRING_format(millis());
                    _data_ += BLINKER_F(",");
                    _data_ += _data;
                    _data_ += BLINKER_F("]");

                    if (_data_.length() < 256) {
                        strcpy(data, _data_.c_str());
                        dataCount++;
                    }
                }
                BLINKER_LOG_ALL(BLINKER_F("saveData: "), data);
                BLINKER_LOG_ALL(BLINKER_F("saveData dataCount: "), dataCount);
            }

            String getData(time_t now_time) {
                BLINKER_LOG_ALL(BLINKER_F("getData data: "), data);
                
                String _data_ = BLINKER_F("{\"data\":[");
                _data_ += STRING_format(data);
                _data_ += BLINKER_F("]}");

                DynamicJsonBuffer jsonDataBuffer;
                JsonObject& dataArray = jsonDataBuffer.parseObject(_data_);

                uint32_t now_millis = millis();

                for (uint8_t num = dataCount; num > 0; num--) {
                    uint32_t data_time = dataArray["data"][num-1][0];
                    uint32_t real_time = now_time - (now_millis - data_time)/1000;
                    dataArray["data"][num-1][0] = real_time;
                    
                    BLINKER_LOG_ALL(BLINKER_F("data_time: "), data_time, \
                                    BLINKER_F(" now_time: "), now_time, \
                                    BLINKER_F(" real_time: "), real_time);
                }

                String _data_decode = dataArray["data"];

                BLINKER_LOG_ALL(BLINKER_F("getData _data_: "), _data_decode);

                return _data_decode;
            }

            bool checkName(const String & name) { return ((_dname == name) ? true : false); }

        private :
            uint8_t dataCount = 0;
            String _dname;
            char data[256];
    };
#endif

#endif
