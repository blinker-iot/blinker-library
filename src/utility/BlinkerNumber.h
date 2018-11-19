#ifndef BlinkerNUM_H
#define BlinkerNUM_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerNumber
{
    public :
        BlinkerNumber(const String & _name)
            // : numName(_name)
        {
            numName = (char*)malloc((_name.length()+1)*sizeof(char));
            strcpy(numName, _name.c_str());
        }
        
        void icon(const String & _icon) {
            if (_fresh >> 0 & 0x01) {
                free(nicon);
            }

            nicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(nicon, _icon.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & _clr) {
            if (_fresh >> 1 & 0x01) {
                free(ncolor);
            }

            ncolor = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(ncolor, _clr.c_str());

            _fresh |= 0x01 << 1;
        }

        void unit(const String & _unit) {
            if (_fresh >> 2 & 0x01) {
                free(nunit);
            }

            nunit = (char*)malloc((_unit.length()+1)*sizeof(char));
            strcpy(nunit, _unit.c_str());

            _fresh |= 0x01 << 2;
        }

        template <typename T>
        void text(T _text) {
            if (_fresh >> 3 & 0x01) {
                free(ntext);
            }

            String _ntext = STRING_format(_text);
            ntext = (char*)malloc((_ntext.length()+1)*sizeof(char));
            strcpy(ntext, _ntext.c_str());

            _fresh |= 0x01 << 3;
        }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
        void print()                        { _print(""); }
    
    private :
        char * numName;
        char * nicon;// = "";
        char * ncolor;// = "";
        char * nunit;// = "";
        char * ntext;
        uint8_t _fresh = 0;

        void _print(const String & value) {
            if (_fresh == 0 && value.length() == 0) {
                return;
            }

            String numberData = "";

            if (value.length()) {
                numberData += BLINKER_F("{\""BLINKER_CMD_VALUE"\":");
                numberData += value;
            }

            // if (nicon && (_fresh >> 0 & 0x01)) {
            if (_fresh >> 0 & 0x01) {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\""BLINKER_CMD_ICON"\":\"");
                numberData += nicon;
                numberData += BLINKER_F("\"");

                free(nicon);
            }

            // if (ncolor && (_fresh >> 1 & 0x01)) {
            if (_fresh >> 1 & 0x01) {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\""BLINKER_CMD_COLOR"\":\"");
                numberData += ncolor;
                numberData += BLINKER_F("\"");

                free(ncolor);
            }

            // if (nunit && (_fresh >> 2 & 0x01)) {
            if (_fresh >> 2 & 0x01) {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\""BLINKER_CMD_UNIT"\":\"");
                numberData += nunit;
                numberData += BLINKER_F("\"");

                free(nunit);
            }

            // if (ntext && (_fresh >> 3 & 0x01)) {
            if (_fresh >> 3 & 0x01) {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\""BLINKER_CMD_TEXT"\":\"");
                numberData += (ntext);
                numberData += BLINKER_F("\"");

                free(ntext);
            }

            numberData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(numName, numberData);
        }
};

#endif
