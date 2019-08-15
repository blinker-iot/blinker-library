#ifndef BlinkerNUM_H
#define BlinkerNUM_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerNumber
{
    public :
        BlinkerNumber(char _name[])
        {
            numName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(numName, _name);
        }
        
        void icon(const String & _icon)
        {
            if (_fresh >> 0 & 0x01) free(nicon);

            nicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(nicon, _icon.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & _clr)
        {
            if (_fresh >> 1 & 0x01) free(ncolor);

            ncolor = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(ncolor, _clr.c_str());

            _fresh |= 0x01 << 1;
        }

        void unit(const String & _unit)
        {
            if (_fresh >> 2 & 0x01) free(nunit);

            nunit = (char*)malloc((_unit.length()+1)*sizeof(char));
            strcpy(nunit, _unit.c_str());

            _fresh |= 0x01 << 2;
        }

        template <typename T>
        void text(T _text)
        {
            if (isnan(_text)) return;

            if (_fresh >> 3 & 0x01) free(ntext);

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

        void _print(const String & value)
        {
            if (_fresh == 0 && value.length() == 0) return;

            String numberData = "";

            if (value.length())
            {
                numberData += BLINKER_F("{\"");
                numberData += BLINKER_F(BLINKER_CMD_VALUE);
                numberData += BLINKER_F("\":");
                numberData += value;
                
                Blinker.printNumArray(numName, value);
            }

            if (_fresh >> 0 & 0x01)
            {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\"");
                numberData += BLINKER_F(BLINKER_CMD_ICON);
                numberData += BLINKER_F("\":\"");
                numberData += nicon;
                numberData += BLINKER_F("\"");

                free(nicon);
            }
            
            if (_fresh >> 1 & 0x01)
            {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\"");
                numberData += BLINKER_F(BLINKER_CMD_COLOR);
                numberData += BLINKER_F("\":\"");
                numberData += ncolor;
                numberData += BLINKER_F("\"");

                free(ncolor);
            }
            
            if (_fresh >> 2 & 0x01)
            {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\"");
                numberData += BLINKER_F(BLINKER_CMD_UNIT);
                numberData += BLINKER_F("\":\"");
                numberData += nunit;
                numberData += BLINKER_F("\"");

                free(nunit);
            }

            if (_fresh >> 3 & 0x01)
            {
                if (numberData.length()) numberData += BLINKER_F(",");
                else numberData += BLINKER_F("{");

                numberData += BLINKER_F("\"");
                numberData += BLINKER_F(BLINKER_CMD_TEXT);
                numberData += BLINKER_F("\":\"");
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
