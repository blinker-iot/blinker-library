#ifndef BLINKER_API_BASE_H
#define BLINKER_API_BASE_H

#include "BlinkerDebug.h"
#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

template <class T>
int8_t checkNum(char * name, T * c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        // BLINKER_LOG_ALL("checkName: ", name, ", name: ", c[cNum]->getName());
        // BLINKER_LOG_ALL("is strcmp: ", strcmp(name, c[cNum]->getName()) == 0);
        // BLINKER_LOG_ALL("is equal: ", name == c[cNum]->getName());
        if (c[cNum]->checkName(name))
            return cNum;
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

class BlinkerWidgets_num
{
    public :
        BlinkerWidgets_num(char * _name)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            autoUpdate = true;
        }

        char * getName() { return wName; }

        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

        void setState(bool state) { autoUpdate = state; }

        bool state() { return autoUpdate; }

    private :
        char *wName;
        bool autoUpdate = false;
};

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

class BlinkerWidgets_table
{
    public :
        BlinkerWidgets_table(char * _name, 
            blinker_callback_with_table_arg_t _func = NULL,
            blinker_callback_t _func2 = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
            wfunc2 = _func2;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_table_arg_t _func, blinker_callback_t _func2) 
        { wfunc = _func; wfunc2 = _func2; }
        blinker_callback_with_table_arg_t getFunc() { return wfunc; }
        blinker_callback_t getFunc2() { return wfunc2; }
        bool checkName(char * name) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_table_arg_t wfunc;
        blinker_callback_t                wfunc2;
};

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_GPRS_AIR202) || \
    defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_LOWPOWER_AIR202) || defined(BLINKER_WIFI_SUBDEVICE)
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
                else return "false";
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
                // data = (char*)malloc((256)*sizeof(char));
                // memcpy(data,"\0",128);
                for(uint8_t num = 0; num < BLINKER_MAX_DATA_COUNT; num++)
                {
                    time_data[num] = 0;
                    memcpy(data[num], "\0", 10);
                }
            }

            void name(const String & name) { _dname = name; }

            String getName() { return _dname; }

            bool saveData(const String & _data, time_t now_time, uint32_t _limit) {
                if (dataCount > 0)
                {
                    if (now_time - latest_time < _limit) return false;
                }

                latest_time = now_time;

                if (dataCount >= BLINKER_MAX_DATA_COUNT)
                {
                    dataCount = BLINKER_MAX_DATA_COUNT;

                    for (uint8_t num = 0; num < dataCount - 1; num++) {
                        time_data[num] = time_data[num + 1];
                        strcpy(data[num], data[num+1]);
                    }
                    time_data[dataCount - 1] = now_time;
                    strcpy(data[dataCount - 1], _data.c_str());
                }
                else
                {
                    time_data[dataCount] = now_time;
                    strcpy(data[dataCount], _data.c_str());
                    dataCount++;
                }

                // String _data_;

                // if (strlen(data)) _data_ = STRING_format(data);

                // if (dataCount == 6) {
                //     _data_ += BLINKER_F(",[");
                //     _data_ += STRING_format(now_time);
                //     _data_ += BLINKER_F(",");
                //     _data_ += _data;
                //     _data_ += BLINKER_F("]");

                //     _data_ = "{\"data\":[" + STRING_format(_data_);
                //     _data_ += BLINKER_F("]}");

                //     DynamicJsonBuffer jsonDataBuffer;
                //     JsonObject& dataArray = jsonDataBuffer.parseObject(_data_);

                //     for (uint8_t num = 0; num < dataCount; num++) {
                //         dataArray["data"][num][0] = dataArray["data"][num+1][0];
                //         dataArray["data"][num][1] = dataArray["data"][num+1][1];
                //     }
                    
                //     _data_ = BLINKER_F("");
                //     for (uint8_t num = 0; num < dataCount; num++)
                //     {
                //         String data_get = dataArray["data"][num];

                //         _data_ += data_get;

                //         if (num != dataCount - 1) _data_ += BLINKER_F(",");
                //     }

                //     if (_data_.length() < 128)
                //     {
                //         strcpy(data, _data_.c_str());
                //     }
                // }
                // else
                // {
                //     if (_data_.length())
                //     {
                //         _data_ += BLINKER_F(",");
                //     }

                //     _data_ += BLINKER_F("[");
                //     _data_ += STRING_format(now_time);
                //     _data_ += BLINKER_F(",");
                //     _data_ += _data;
                //     _data_ += BLINKER_F("]");

                //     if (_data_.length() < 128) {
                //         strcpy(data, _data_.c_str());
                //         dataCount++;
                //     }
                // }
                BLINKER_LOG_ALL(BLINKER_F("saveData: "), _data);
                BLINKER_LOG_ALL(BLINKER_F("saveData dataCount: "), dataCount);

                return true;
            }

            String getData() {
                // BLINKER_LOG_ALL(BLINKER_F("getData data: "), data);
                
                String _data_ = BLINKER_F("[");//{\"data\":
                // _data_ += STRING_format(data);
                for (uint8_t num = 0; num < dataCount; num++) {
                    _data_ += "[";
                    _data_ += String(time_data[num]);
                    _data_ += ",";
                    _data_ += data[num];
                    _data_ += "]";
                    if (num + 1 < dataCount)
                    {
                        _data_ += ",";
                    }

                    // time_data[num] = latest_time;
                    // memcpy(data[num], "\0", 10);
                }
                _data_ += BLINKER_F("]");//}

                // dataCount = 0;

                // DynamicJsonBuffer jsonDataBuffer;
                // JsonObject& dataArray = jsonDataBuffer.parseObject(_data_);

                // uint32_t now_millis = millis();

                // for (uint8_t num = dataCount; num > 0; num--) {
                //     uint8_t data_time = dataArray["data"][num][0];
                //     uint32_t real_time = now_time - (dataCount - data_time - 1)*60;
                //     dataArray["data"][num-1][0] = real_time;
                    
                //     BLINKER_LOG_ALL(BLINKER_F("data_time: "), data_time, 
                //                     BLINKER_F(" now_time: "), now_time, 
                //                     BLINKER_F(" real_time: "), real_time);
                // }

                // String _data_decode = dataArray["data"];

                BLINKER_LOG_ALL(BLINKER_F("getData _data_: "), _data_);

                return _data_;
            }

            bool checkName(const String & name) { return ((_dname == name) ? true : false); }

            void flush()
            {
                for (uint8_t num = 0; num < dataCount; num++) {
                    time_data[num] = latest_time;
                    memcpy(data[num], "\0", 10);
                }

                dataCount = 0;
            }

        private :
            uint8_t dataCount = 0;
            time_t  latest_time = 0;
            String _dname;
            // char * data;
            time_t  time_data[BLINKER_MAX_DATA_COUNT];
            char    data[BLINKER_MAX_DATA_COUNT][10];
    };
#endif

#endif
