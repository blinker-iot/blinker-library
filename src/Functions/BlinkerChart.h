#ifndef BLINKER_REAL_TIME_DATA_H
#define BLINKER_REAL_TIME_DATA_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerRealTimeData
{
    public :
        BlinkerRealTimeData(char _name[], blinker_callback_t _func = NULL)
        {
            Blinker.attachRTData(_name, _func);
        }

        void saveData(char key[], int32_t data)
        {
            if (_count = 0)
            {
                strcpy(_keys, key);
            }

            _datas[_count] = data;
            _times[_count] = Blinker.time();

            _count++;
        }

        void print()
        {
            String rtData;

            if (_count > 0)
            {
                rtData = BLINKER_F("{\"rt\":\"{\"");
                rtData += STRING_format(_keys);
                rtData += BLINKER_F("\":[");

                for (uint8_t i = 0; i < _count; i++)
                {
                    rtData += BLINKER_F("{\"value\":");
                    rtData += STRING_format(_datas[i]);
                    rtData += BLINKER_F(",\"date\":");
                    rtData += STRING_format(_times[i]);
                    rtData += BLINKER_F("}");

                    if (i < _count - 1)
                    {
                        rtData += BLINKER_F(",");
                    }
                }

                rtData += BLINKER_F("]}}");

                Blinker.printObject(Blinker.widgetName_str(wNum), buttonData);

                _count = 0;
            }
        }

    private :
        uint8_t wNum;

        uint8_t _count = 0;
        int32_t _datas[12];
        time_t  _times[12];
        char    _keys[12];
};

#endif
