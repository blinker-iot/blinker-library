#ifndef BLINKER_API_BASE_H
#define BLINKER_API_BASE_H

#include "BlinkerDebug.h"
#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

template <class T>
int8_t checkNum(char * name, T * c, uint8_t count)
{
    BLINKER_LOG_ALL(BLINKER_F("checkNum count: "), count);
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        // BLINKER_LOG_ALL(BLINKER_F("checkName: "), name, BLINKER_F(", name: "), c[cNum]->getName());
        // BLINKER_LOG_ALL(BLINKER_F("is strcmp: "), strcmp(name, c[cNum]->getName()) == 0);
        // BLINKER_LOG_ALL(BLINKER_F("is equal: "), name == c[cNum]->getName());
        if (c[cNum]->checkName(name)) return cNum;
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
    defined(BLINKER_LOWPOWER_AIR202) || defined(BLINKER_WIFI_SUBDEVICE) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7000) || defined(BLINKE_HTTP)
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

    union BlinkerDataType
    {
        int32_t     int_data;
        uint32_t    uint_data;
        float       float_data;
    };

    class BlinkerTimeSlotData
    {
        public :
            BlinkerTimeSlotData() 
                : time_data(0)
                , key_count(0)
            {}

            int8_t checkNum(char name[], uint8_t count)
            {
                BLINKER_LOG_ALL(BLINKER_F("checkNum count: "), count);
                for (uint8_t cNum = 0; cNum < count; cNum++)
                {
                    // BLINKER_LOG_ALL(BLINKER_F("checkName: "), name, BLINKER_F(", name: "), c[cNum]->getName());
                    // BLINKER_LOG_ALL(BLINKER_F("is strcmp: "), strcmp(name, c[cNum]->getName()) == 0);
                    // BLINKER_LOG_ALL(BLINKER_F("is equal: "), name == c[cNum]->getName());
                    if (strcmp(name, keys[cNum]) == 0) return cNum;
                }

                return BLINKER_OBJECT_NOT_AVAIL;
            }

            void saveData(char key[], int32_t data, time_t now_time)
            {
                if (key_count == 0) time_data = now_time;

                int8_t num = checkNum(key, key_count);

                // int32_t format_data;

                if (num == BLINKER_OBJECT_NOT_AVAIL)
                {
                    if (key_count < 6)
                    {
                        strcpy(keys[key_count], key);
                        data_type[key_count] = BLINKER_INT_DATA;
                        datas[key_count].int_data = data;
                        // for(uint8_t i=0; i<4; i++){
                        //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
                        // }
                        // datas[key_count][4] = BLINKER_INT_DATA;

                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
                        // }

                        BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
                                        BLINKER_F(" key_count: "), key_count, \
                                        BLINKER_F(" data: "), datas[key_count].int_data, \
                                        BLINKER_F(" data_type: "), data_type[key_count]);
                        
                        key_count++;
                    }
                }
            }

            void saveData(char key[], uint32_t data, time_t now_time)
            {
                if (key_count == 0) time_data = now_time;

                int8_t num = checkNum(key, key_count);

                uint32_t format_data;

                if (num == BLINKER_OBJECT_NOT_AVAIL)
                {
                    if (key_count < 6)
                    {
                        strcpy(keys[key_count], key);
                        data_type[key_count] = BLINKER_UINT_DATA;
                        datas[key_count].uint_data = data;
                        // for(uint8_t i=0; i<4; i++){
                        //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
                        // }
                        // datas[key_count][4] = BLINKER_INT_DATA;

                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
                        // }

                        BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
                                        BLINKER_F(" key_count: "), key_count, \
                                        BLINKER_F(" data: "), datas[key_count].uint_data, \
                                        BLINKER_F(" data_type: "), data_type[key_count]);
                        
                        key_count++;
                    }
                }
            }

            void saveData(char key[], float data, time_t now_time)
            {
                if (key_count == 0) time_data = now_time;

                int8_t num = checkNum(key, key_count);

                float format_data;

                if (num == BLINKER_OBJECT_NOT_AVAIL)
                {
                    if (key_count < 6)
                    {
                        strcpy(keys[key_count], key);
                        data_type[key_count] = BLINKER_FLOAT_DATA;
                        datas[key_count].float_data = data;
                        // for(uint8_t i=0; i<4; i++){
                        //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
                        // }
                        // datas[key_count][4] = BLINKER_INT_DATA;

                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
                        // }

                        BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
                                        BLINKER_F(" key_count: "), key_count, \
                                        BLINKER_F(" data: "), datas[key_count].float_data, \
                                        BLINKER_F(" data_type: "), data_type[key_count]);
                        
                        key_count++;
                    }
                }
            }

            String getData()
            {
                int32_t     int_data;
                uint32_t    uint_data;
                float       float_data;

                BLINKER_LOG_FreeHeap_ALL();

                String _data = BLINKER_F("{\"date\":");
                _data += STRING_format(time_data);

                for(uint8_t num = 0; num < key_count; num++)
                {
                    _data += BLINKER_F(",\"");
                    _data += keys[num];
                    _data += BLINKER_F("\":");
                    // _data += keys[num];
                    BLINKER_LOG_ALL(BLINKER_F("datas["), num,
                                    BLINKER_F("].data_type: "), 
                                    data_type[num]);

                    if (data_type[num] == BLINKER_INT_DATA)
                    {
                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&int_data + i) = *(&datas[key_count][0] + i);
                        // }

                        _data += STRING_format(datas[num].int_data);

                        BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].int_data);
                    }
                    else if (data_type[num] == BLINKER_UINT_DATA)
                    {
                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&uint_data + i) = *(&datas[key_count][0] + i);
                        // }

                        _data += STRING_format(datas[num].uint_data);

                        BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].uint_data);
                    }
                    else if (data_type[num] == BLINKER_FLOAT_DATA)
                    {
                        // for(uint8_t i=0; i<4; i++){
                        //     *((uint8_t *)&float_data + i) = *(&datas[key_count][0] + i);
                        // }

                        _data += STRING_format(datas[num].float_data);

                        BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].float_data);
                    }
                }

                _data += STRING_format("}");

                return _data;
            }

            void flush()
            {
                key_count = 0;
                time_data = 0;
            }

            // for(uint8_t i=0;i<4;i++){
            //     *(&test[0] + i) = *((uint8_t *)&testt + i); // 逐个字节单元进行复制
            //     Serial.println(*((uint8_t *)&testt + i));
            // }

        private :
            time_t  time_data;
            uint8_t key_count;
            uint8_t data_type[6];
            char    keys[6][15];
            BlinkerDataType datas[6];
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
                    memset(data[num], '\0', 10);
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
                    memset(data[num], '\0', 10);
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

    class BlinkerRTData
    {
        public :
            BlinkerRTData()
                // : _dname(NULL)
            {
                // data = (char*)malloc((256)*sizeof(char));
                // memcpy(data,"\0",128);
                for(uint8_t num = 0; num < 15; num++)
                {
                    // time_data[num] = 0;
                    // data[num] = 0;
                }
            }

            void name(const char* _name) { _dname = _name; }

            const char* getName() { return _dname; }

            // bool saveData(int _data, time_t now_time) {
            //     // if (dataCount > 0)
            //     // {
            //     //     if (now_time - latest_time < _limit) return false;
            //     // }

            //     latest_time = now_time;

            //     if (dataCount >= BLINKER_MAX_RTDATA_DATA_SIZE)
            //     {
            //         full = true;

            //         dataCount = dataCount % BLINKER_MAX_RTDATA_DATA_SIZE;

            //         time_data[dataCount] = now_time;
            //         data[dataCount] = _data;

            //         dataCount++;
            //         // for (uint8_t num = 0; num < dataCount - 1; num++) {
            //         //     time_data[num] = time_data[num + 1];
            //         //     data[num] = data[num+1];
            //         // }
            //         // time_data[dataCount - 1] = now_time;
            //         // data[dataCount - 1] = _data;
            //     }
            //     else
            //     {
            //         time_data[dataCount] = now_time;
            //         data[dataCount] = _data;
            //         dataCount++;
            //     }
            //     BLINKER_LOG_ALL(BLINKER_F("saveData: "), _data);
            //     BLINKER_LOG_ALL(BLINKER_F("saveData dataCount: "), dataCount);

            //     return true;
            // }

            bool available() {
                // if (full)
                // {
                //     if (printCount >= BLINKER_MAX_RTDATA_DATA_SIZE)
                //     {
                //         return false;
                //     } 
                // } else {
                //     if (printCount >= dataCount)
                //     {
                //         return false;
                //     } 
                // }

                if (is_fresh) return true;

                return false;
            }

            // String getData() {
            //     // full = false;
                
            //     String _data_ = BLINKER_F("");
                
            //     // if (full)
            //     // {
            //     //     if (printCount >= BLINKER_MAX_RTDATA_DATA_SIZE)
            //     //     {
            //     //         _data_ = "null";
            //     //     } 
            //     // } else {
            //     //     if (printCount >= dataCount)
            //     //     {
            //     //         _data_ = "null";
            //     //     } 
            //     // }

            //     if (available())
            //     {
            //         _data_ += "\"";
            //         _data_ += _dname;
            //         _data_ += "\":{\"date\":";
            //         _data_ += String(time_data[printCount]);
            //         _data_ += ",\"value\":";
            //         _data_ += data[printCount];
            //         _data_ += "}";

            //         printCount++;
            //     }
            //     else
            //     {
            //         _data_ = "null";
            //     }

            //     // for (uint8_t num = 0; num < dataCount; num++) {
            //         // _data_ += "{\"date\":";
            //         // _data_ += String(time_data[printCount]);
            //         // _data_ += ",\"value\":";
            //         // _data_ += data[printCount];
            //         // _data_ += "}";

            //     // BLINKER_LOG_ALL(BLINKER_F("getData data: "), data);

            //     // printCount++;
            //     //     if (num + 1 < dataCount)
            //     //     {
            //     //         _data_ += ",";
            //     //     }
            //     // }
            //     // _data_ += BLINKER_F("]");

            //     BLINKER_LOG_ALL(BLINKER_F("getData _data_: "), _data_);

            //     return _data_;
            // }

            bool checkName(const char* name) { return strncmp(name, _dname, strlen(name)) == 0; }

            // void flush()
            // {
            //     for (uint8_t num = 0; num < dataCount; num++) {
            //         time_data[num] = latest_time;
            //         data[num] = 0;
            //     }

            //     dataCount = 0;
            //     printCount = 0;
            //     full = false;
            // }

            void state(bool fresh_state)
            {
                is_fresh = fresh_state;
            }

        private :
            bool    is_fresh = false;
            // uint8_t dataCount = 0;
            // uint8_t printCount = 0;
            // time_t  latest_time = 0;
            // char * data;
            // time_t  time_data[15];
            // int     data[15];
            // bool    full = false;
            const char*   _dname;
    };
#endif

#endif
