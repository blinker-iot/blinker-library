#ifndef BLINKER_API_BASE_H
#define BLINKER_API_BASE_H

#include "BlinkerDebug.h"
#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

template <class T>
int8_t checkNum(const char* name, T * c, uint8_t count)
{
    // BLINKER_LOG_ALL(BLINKER_F("checkNum count: "), count);
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        // BLINKER_LOG_ALL(BLINKER_F("checkName: "), name, BLINKER_F(", name: "), c[cNum]->getName());
        // BLINKER_LOG_ALL(BLINKER_F("is strcmp: "), strcmp(name, c[cNum]->getName()) == 0);
        // BLINKER_LOG_ALL(BLINKER_F("is equal: "), name == c[cNum]->getName());
        // if (c[cNum]->checkName(name)) return cNum;
        if(strncmp(name, c[cNum]->getName(), strlen(name)) == 0) return cNum;
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

class BlinkerWidgets_num
{
    public :
        BlinkerWidgets_num(const char* _name)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            autoUpdate = true;
        }

        const char* getName() { return wName; }

        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

        void setState(bool state) { autoUpdate = state; }

        bool state() { return autoUpdate; }

    private :
        const char* wName;
        bool autoUpdate = false;
};

class BlinkerWidgets_string
{
    public :
        BlinkerWidgets_string(const char* _name, blinker_callback_with_string_arg_t _func = NULL)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            wfunc = _func;
        }

        const char* getName() { return wName; }
        void setFunc(blinker_callback_with_string_arg_t _func) { wfunc = _func; }
        blinker_callback_with_string_arg_t getFunc() { return wfunc; }
        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

    private :
        const char* wName;
        blinker_callback_with_string_arg_t wfunc;
};

class BlinkerWidgets_int32
{
    public :
        BlinkerWidgets_int32(const char* _name, blinker_callback_with_int32_arg_t _func = NULL)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            wfunc = _func;
        }

        const char* getName() { return wName; }
        void setFunc(blinker_callback_with_int32_arg_t _func) { wfunc = _func; }
        blinker_callback_with_int32_arg_t getFunc() { return wfunc; }
        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

    private :
        const char* wName;
        blinker_callback_with_int32_arg_t wfunc;
};

class BlinkerWidgets_rgb
{
    public :
        BlinkerWidgets_rgb(const char* _name, blinker_callback_with_rgb_arg_t _func = NULL)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            wfunc = _func;
        }

        const char* getName() { return wName; }
        void setFunc(blinker_callback_with_rgb_arg_t _func) { wfunc = _func; }
        blinker_callback_with_rgb_arg_t getFunc() { return wfunc; }
        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

    private :
        const char* wName;
        blinker_callback_with_rgb_arg_t wfunc;
};

class BlinkerWidgets_joy
{
    public :
        BlinkerWidgets_joy(const char* _name, blinker_callback_with_joy_arg_t _func = NULL)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            wfunc = _func;
        }

        const char* getName() { return wName; }
        void setFunc(blinker_callback_with_joy_arg_t _func) { wfunc = _func; }
        blinker_callback_with_joy_arg_t getFunc() { return wfunc; }
        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

    private :
        const char* wName;
        blinker_callback_with_joy_arg_t wfunc;
};

class BlinkerWidgets_table
{
    public :
        BlinkerWidgets_table(const char* _name, 
            blinker_callback_with_table_arg_t _func = NULL,
            blinker_callback_t _func2 = NULL)
        {
            // wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(wName, _name);
            wName = _name;

            wfunc = _func;
            wfunc2 = _func2;
        }

        const char* getName() { return wName; }
        void setFunc(blinker_callback_with_table_arg_t _func, blinker_callback_t _func2) 
        { wfunc = _func; wfunc2 = _func2; }
        blinker_callback_with_table_arg_t getFunc() { return wfunc; }
        blinker_callback_t getFunc2() { return wfunc2; }
        // bool checkName(const char* name) {
        //     return strncmp(name, wName, strlen(name)) == 0;
        // }

    private :
        const char* wName;
        blinker_callback_with_table_arg_t wfunc;
        blinker_callback_t                wfunc2;
};

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_WIFI_AT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_GPRS_AIR202) || \
    defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
    defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_LOWPOWER_AIR202) || defined(BLINKER_WIFI_SUBDEVICE) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7000) || defined(BLINKE_HTTP) || \
    defined(BLINKER_WIFI)
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
    };

    enum BlinkerDataValueType {
        BLINKER_DATA_TYPE_UINT8 = 0,
        BLINKER_DATA_TYPE_INT8,
        BLINKER_DATA_TYPE_UINT16,
        BLINKER_DATA_TYPE_INT16,
        BLINKER_DATA_TYPE_UINT32,
        BLINKER_DATA_TYPE_INT32,
        BLINKER_DATA_TYPE_FLOAT
    };

    union BlinkerDataValue
    {
        uint8_t     uint8_data;
        int8_t      int8_data;
        uint16_t    uint16_data;
        int16_t     int16_data;
        uint32_t    uint32_data;
        int32_t     int32_data;
        float       float_data;
    };

    // union BlinkerDataType
    // {
    //     int32_t     int_data;
    //     uint32_t    uint_data;
    //     float       float_data;
    // };

    // class BlinkerTimeSlotData
    // {
    //     public :
    //         BlinkerTimeSlotData() 
    //             : time_data(0)
    //             , key_count(0)
    //         {}

    //         int8_t checkNum(char name[], uint8_t count)
    //         {
    //             BLINKER_LOG_ALL(BLINKER_F("checkNum count: "), count);
    //             for (uint8_t cNum = 0; cNum < count; cNum++)
    //             {
    //                 // BLINKER_LOG_ALL(BLINKER_F("checkName: "), name, BLINKER_F(", name: "), c[cNum]->getName());
    //                 // BLINKER_LOG_ALL(BLINKER_F("is strcmp: "), strcmp(name, c[cNum]->getName()) == 0);
    //                 // BLINKER_LOG_ALL(BLINKER_F("is equal: "), name == c[cNum]->getName());
    //                 if (strcmp(name, keys[cNum]) == 0) return cNum;
    //             }

    //             return BLINKER_OBJECT_NOT_AVAIL;
    //         }

    //         void saveData(char key[], int32_t data, time_t now_time)
    //         {
    //             if (key_count == 0) time_data = now_time;

    //             int8_t num = checkNum(key, key_count);

    //             // int32_t format_data;

    //             if (num == BLINKER_OBJECT_NOT_AVAIL)
    //             {
    //                 if (key_count < 6)
    //                 {
    //                     strcpy(keys[key_count], key);
    //                     data_type[key_count] = BLINKER_INT_DATA;
    //                     datas[key_count].int_data = data;
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
    //                     // }
    //                     // datas[key_count][4] = BLINKER_INT_DATA;

    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
    //                                     BLINKER_F(" key_count: "), key_count, \
    //                                     BLINKER_F(" data: "), datas[key_count].int_data, \
    //                                     BLINKER_F(" data_type: "), data_type[key_count]);
                        
    //                     key_count++;
    //                 }
    //             }
    //         }

    //         void saveData(char key[], uint32_t data, time_t now_time)
    //         {
    //             if (key_count == 0) time_data = now_time;

    //             int8_t num = checkNum(key, key_count);

    //             uint32_t format_data;

    //             if (num == BLINKER_OBJECT_NOT_AVAIL)
    //             {
    //                 if (key_count < 6)
    //                 {
    //                     strcpy(keys[key_count], key);
    //                     data_type[key_count] = BLINKER_UINT_DATA;
    //                     datas[key_count].uint_data = data;
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
    //                     // }
    //                     // datas[key_count][4] = BLINKER_INT_DATA;

    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
    //                                     BLINKER_F(" key_count: "), key_count, \
    //                                     BLINKER_F(" data: "), datas[key_count].uint_data, \
    //                                     BLINKER_F(" data_type: "), data_type[key_count]);
                        
    //                     key_count++;
    //                 }
    //             }
    //         }

    //         void saveData(char key[], float data, time_t now_time)
    //         {
    //             if (key_count == 0) time_data = now_time;

    //             int8_t num = checkNum(key, key_count);

    //             float format_data;

    //             if (num == BLINKER_OBJECT_NOT_AVAIL)
    //             {
    //                 if (key_count < 6)
    //                 {
    //                     strcpy(keys[key_count], key);
    //                     data_type[key_count] = BLINKER_FLOAT_DATA;
    //                     datas[key_count].float_data = data;
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *(&datas[key_count][0] + i) = *((uint8_t *)&data + i);
    //                     // }
    //                     // datas[key_count][4] = BLINKER_INT_DATA;

    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&format_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     BLINKER_LOG_ALL(BLINKER_F("new key: "), key, \
    //                                     BLINKER_F(" key_count: "), key_count, \
    //                                     BLINKER_F(" data: "), datas[key_count].float_data, \
    //                                     BLINKER_F(" data_type: "), data_type[key_count]);
                        
    //                     key_count++;
    //                 }
    //             }
    //         }

    //         String getData()
    //         {
    //             int32_t     int_data;
    //             uint32_t    uint_data;
    //             float       float_data;

    //             BLINKER_LOG_FreeHeap_ALL();

    //             String _data = BLINKER_F("{\"date\":");
    //             #if defined(ESP8266) || defined(ESP32)
    //             _data += STRING_format(time_data);
    //             #elif defined(ARDUINO_ARCH_RENESAS_UNO)
    //             _data += STRING_format((unsigned long)time_data);
    //             #endif

    //             for(uint8_t num = 0; num < key_count; num++)
    //             {
    //                 _data += BLINKER_F(",\"");
    //                 _data += keys[num];
    //                 _data += BLINKER_F("\":");
    //                 // _data += keys[num];
    //                 BLINKER_LOG_ALL(BLINKER_F("datas["), num,
    //                                 BLINKER_F("].data_type: "), 
    //                                 data_type[num]);

    //                 if (data_type[num] == BLINKER_INT_DATA)
    //                 {
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&int_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     _data += STRING_format(datas[num].int_data);

    //                     BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].int_data);
    //                 }
    //                 else if (data_type[num] == BLINKER_UINT_DATA)
    //                 {
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&uint_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     _data += STRING_format(datas[num].uint_data);

    //                     BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].uint_data);
    //                 }
    //                 else if (data_type[num] == BLINKER_FLOAT_DATA)
    //                 {
    //                     // for(uint8_t i=0; i<4; i++){
    //                     //     *((uint8_t *)&float_data + i) = *(&datas[key_count][0] + i);
    //                     // }

    //                     _data += STRING_format(datas[num].float_data);

    //                     BLINKER_LOG_ALL(BLINKER_F("datas[num].int_data: "), datas[num].float_data);
    //                 }
    //             }

    //             _data += STRING_format("}");

    //             return _data;
    //         }

    //         void flush()
    //         {
    //             key_count = 0;
    //             time_data = 0;
    //         }

    //         // for(uint8_t i=0;i<4;i++){
    //         //     *(&test[0] + i) = *((uint8_t *)&testt + i); // 逐个字节单元进行复制
    //         //     Serial.println(*((uint8_t *)&testt + i));
    //         // }

    //     private :
    //         time_t  time_data;
    //         uint8_t key_count;
    //         uint8_t data_type[6];
    //         char    keys[6][15];
    //         BlinkerDataType datas[6];
    // };

    class BlinkerData
    {
        public :
            BlinkerData()
            {
                dataCount = 0;
                latest_time = 0;
                latest_index = 0;
                time_data[0] = 0;
                data_types[0] = BLINKER_DATA_TYPE_UINT8;
                values[0].uint8_data = 0;
            }

            void name(const String & name) { _dname = name; }

            const char *getName() { return _dname.c_str(); }

            bool saveDataValue(const BlinkerDataValue& dataValue, BlinkerDataValueType dataType, time_t now_time, uint32_t _limit) {
                if (dataCount > 0 && (now_time - latest_time < _limit)) {
                    return false;
                }

                latest_time = now_time;

                uint8_t storeIndex;
                if (dataCount >= BLINKER_MAX_DATA_COUNT) {
                    storeIndex = (latest_index + 1) % BLINKER_MAX_DATA_COUNT;
                    latest_index = storeIndex;
                } else {
                    storeIndex = dataCount;
                    latest_index = dataCount;
                    dataCount++;
                }

                time_data[storeIndex] = now_time;
                data_types[storeIndex] = dataType;
                values[storeIndex] = dataValue;

                BLINKER_LOG_ALL(BLINKER_F("saveDataValue: type="), dataType, 
                                BLINKER_F(" index="), storeIndex, BLINKER_F(" count="), dataCount);
                return true;
            }

            String getData() {
                if (dataCount == 0) {
                    return BLINKER_F("[]");
                }

                String _data_;
                _data_.reserve(dataCount * 25);
                _data_ = BLINKER_F("[");
                
                for (uint8_t i = 0; i < dataCount; i++) {
                    uint8_t index = (dataCount < BLINKER_MAX_DATA_COUNT) ? 
                                   i : (latest_index + 1 + i) % BLINKER_MAX_DATA_COUNT;
                    
                    _data_ += BLINKER_F("[");
                    _data_ += String((unsigned long)time_data[index]);
                    _data_ += BLINKER_F(",");
                    _data_ += formatDataValue(values[index], data_types[index]);
                    _data_ += BLINKER_F("]");
                    
                    if (i + 1 < dataCount) {
                        _data_ += BLINKER_F(",");
                    }
                }
                _data_ += BLINKER_F("]");

                BLINKER_LOG_ALL(BLINKER_F("getData: "), _data_.length(), BLINKER_F(" bytes"));
                return _data_;
            }

            bool checkName(const String & name) { return (_dname == name); }

            void flush()
            {
                dataCount = 0;
                latest_index = 0;
                latest_time = 0;
                values[0].uint8_data = 0;
            }

            size_t getMemoryUsage() const {
                size_t baseSize = sizeof(*this) + _dname.length();
                
                size_t dataSize = 0;
                for (uint8_t i = 0; i < dataCount; i++) {
                    dataSize += sizeof(time_data[0]) + sizeof(data_types[0]);
                    
                    switch (data_types[i]) {
                        case BLINKER_DATA_TYPE_UINT8:
                        case BLINKER_DATA_TYPE_INT8:
                            dataSize += 1; break;
                        case BLINKER_DATA_TYPE_UINT16:
                        case BLINKER_DATA_TYPE_INT16:
                            dataSize += 2; break;
                        case BLINKER_DATA_TYPE_UINT32:
                        case BLINKER_DATA_TYPE_INT32:
                        case BLINKER_DATA_TYPE_FLOAT:
                            dataSize += 4; break;
                    }
                }
                
                return baseSize + dataSize;
            }

            float getMemoryEfficiency() const {
                if (dataCount == 0) return 1.0f;
                
                size_t actualSize = getMemoryUsage();
                size_t originalSize = sizeof(*this) + _dname.length() + 
                                     dataCount * (sizeof(time_data[0]) + 10);
                
                return (float)actualSize / originalSize;
            }

            String formatDataValue(const BlinkerDataValue& value, BlinkerDataValueType type) {
                switch (type) {
                    case BLINKER_DATA_TYPE_UINT8:   return String(value.uint8_data);
                    case BLINKER_DATA_TYPE_INT8:    return String(value.int8_data);
                    case BLINKER_DATA_TYPE_UINT16:  return String(value.uint16_data);
                    case BLINKER_DATA_TYPE_INT16:   return String(value.int16_data);
                    case BLINKER_DATA_TYPE_UINT32:  return String(value.uint32_data);
                    case BLINKER_DATA_TYPE_INT32:   return String(value.int32_data);
                    case BLINKER_DATA_TYPE_FLOAT:   return String(value.float_data);
                    default:                        return BLINKER_F("0");
                }
            }

        private :
            uint8_t dataCount = 0;
            uint8_t latest_index = 0;
            time_t  latest_time = 0;
            String _dname;
            
            time_t                  time_data[BLINKER_MAX_DATA_COUNT];
            BlinkerDataValueType    data_types[BLINKER_MAX_DATA_COUNT];
            BlinkerDataValue        values[BLINKER_MAX_DATA_COUNT];
    };
#endif

#endif
