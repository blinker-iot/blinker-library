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
            // nicon = _icon; 
            // if (strlen(nicon)) free(nicon);

            nicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(nicon, _icon.c_str());
        }
        void color(const String & _clr) { 
            // ncolor = _clr; 
            // if (strlen(ncolor)) free(ncolor);

            ncolor = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(ncolor, _clr.c_str());
        }
        void unit(const String & _unit) { 
            // nunit = _unit; 
            // if (strlen(nunit)) free(nunit);

            nunit = (char*)malloc((_unit.length()+1)*sizeof(char));
            strcpy(nunit, _unit.c_str());
        }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
    
    private :
        // String numName;
        // String nicon = "";
        // String ncolor = "";
        // String nunit = "";

        char * numName;
        char * nicon = "";
        char * ncolor = "";
        char * nunit = "";

        void _print(const String & value) {

            String buttonData;
            buttonData += BLINKER_F("{\""BLINKER_CMD_VALUE"\":");
            buttonData += value;

            // if (nicon.length()) {
            if (strlen(nicon)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_ICON"\":\"");
                buttonData += nicon;
                buttonData += BLINKER_F("\"");
                free(nicon);
            }

            // if (ncolor.length()) {
            if (strlen(ncolor)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_COLOR"\":\"");
                buttonData += ncolor;
                buttonData += BLINKER_F("\"");
                free(ncolor);
            }

            // if (nunit.length()) {
            if (strlen(nunit)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_UNIT"\":\"");
                buttonData += nunit;
                buttonData += BLINKER_F("\"");
                free(nunit);
            }

            buttonData += BLINKER_F("}");

            Blinker.printArray(numName, buttonData);

            // nicon = "";
            // ncolor = "";
            // nunit = "";
        }
};

#endif
