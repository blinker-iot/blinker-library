#ifndef BLINKER_WIDGET_API_H
#define BLINKER_WIDGET_API_H

#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerConfig.h"
#include "utility/BlinkerUtility.h"

template <class T>
int8_t checkNum(char name[], T * c, uint8_t count)
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
        BlinkerWidgets_string(char _name[], blinker_callback_with_string_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_string_arg_t _func) { wfunc = _func; }
        blinker_callback_with_string_arg_t getFunc() { return wfunc; }
        bool checkName(char name[]) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_string_arg_t wfunc;
};

class BlinkerWidgets_int32
{
    public :
        BlinkerWidgets_int32(char _name[], blinker_callback_with_int32_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_int32_arg_t _func) { wfunc = _func; }
        blinker_callback_with_int32_arg_t getFunc() { return wfunc; }
        bool checkName(char name[]) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_int32_arg_t wfunc;
};

class BlinkerWidgets_rgb
{
    public :
        BlinkerWidgets_rgb(char _name[], blinker_callback_with_rgb_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_rgb_arg_t _func) { wfunc = _func; }
        blinker_callback_with_rgb_arg_t getFunc() { return wfunc; }
        bool checkName(char name[]) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_rgb_arg_t wfunc;
};

class BlinkerWidgets_joy
{
    public :
        BlinkerWidgets_joy(char _name[], blinker_callback_with_joy_arg_t _func = NULL)
        {
            wName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(wName, _name);

            wfunc = _func;
        }

        char * getName() { return wName; }
        void setFunc(blinker_callback_with_joy_arg_t _func) { wfunc = _func; }
        blinker_callback_with_joy_arg_t getFunc() { return wfunc; }
        bool checkName(char name[]) {
            return strcmp(name, wName) == 0;
        }

    private :
        char *wName;
        blinker_callback_with_joy_arg_t wfunc;
};

#endif
