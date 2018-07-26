#ifndef BlinkerNUM_H
#define BlinkerNUM_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerNUM
{
    public :
        BlinkerNUM(const String & _name)
            : numName(_name)
        {}
        
        void icon(const String & _icon) { nicon = _icon; }
        void color(const String & _clr) { ncolor = _clr; }
        void unit(const String & _unit) { nunit = _unit; }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
    
    private :
        String numName;
        String nicon = "";
        String ncolor = "";
        String nunit = "";

        void _print(const String & value) {

            String buttonData = "{\"val\":" + value;

            if (nicon.length()) {
                buttonData += ",\"ico\":\"" + nicon + "\"";
            }
            if (ncolor.length()) {
                buttonData += ",\"col\":\"" + ncolor + "\"";
            }
            if (nunit.length()) {
                buttonData += ",\"uin\":\"" + nunit + "\"";
            }

            buttonData += "}";

            Blinker.printArray(numName, buttonData);
        }
};

#endif
