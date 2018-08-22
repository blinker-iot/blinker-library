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

            nicon = (char*)malloc(1*sizeof(char));
            nicon[0] = '\0';
            ncolor = (char*)malloc(1*sizeof(char));
            ncolor[0] = '\0';
            nunit = (char*)malloc(1*sizeof(char));
            nunit[0] = '\0';
            ntext = (char*)malloc(1*sizeof(char));
            ntext[0] = '\0';
        }
        
        void icon(const String & _icon) { 
            // nicon = _icon; 
            // if (strlen(nicon)) free(nicon);

            // nicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            nicon = (char*)realloc(nicon, (_icon.length()+1)*sizeof(char));
            strcpy(nicon, _icon.c_str());
        }

        void color(const String & _clr) { 
            // ncolor = _clr; 
            // if (strlen(ncolor)) free(ncolor);

            // ncolor = (char*)malloc((_clr.length()+1)*sizeof(char));
            ncolor = (char*)realloc(ncolor, (_clr.length()+1)*sizeof(char));
            strcpy(ncolor, _clr.c_str());
        }

        void unit(const String & _unit) { 
            // nunit = _unit; 
            // if (strlen(nunit)) free(nunit);

            // nunit = (char*)malloc((_unit.length()+1)*sizeof(char));
            nunit = (char*)realloc(nunit, (_unit.length()+1)*sizeof(char));
            strcpy(nunit, _unit.c_str());
        }

        template <typename T>
        void text(T _text) {
            // ntext = STRING_format(_text);

            String _ntext = STRING_format(_text);
            ntext = (char*)realloc(ntext, (_ntext.length()+1)*sizeof(char));
            strcpy(ntext, _ntext.c_str());
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
        char * nicon;// = "";
        char * ncolor;// = "";
        char * nunit;// = "";
        char * ntext;

        void _print(const String & value) {

            String numberData;
            numberData += BLINKER_F("{\""BLINKER_CMD_VALUE"\":");
            numberData += value;

            // if (nicon.length()) {
            if (strlen(nicon)) {
                numberData += BLINKER_F(",\""BLINKER_CMD_ICON"\":\"");
                numberData += nicon;
                numberData += BLINKER_F("\"");
                // free(nicon);

                nicon = (char*)realloc(nicon, 1*sizeof(char));
                nicon[0] = '\0';
            }

            // if (ncolor.length()) {
            if (strlen(ncolor)) {
                numberData += BLINKER_F(",\""BLINKER_CMD_COLOR"\":\"");
                numberData += ncolor;
                numberData += BLINKER_F("\"");
                // free(ncolor);

                ncolor = (char*)realloc(ncolor, 1*sizeof(char));
                ncolor[0] = '\0';
            }

            // if (nunit.length()) {
            if (strlen(nunit)) {
                numberData += BLINKER_F(",\""BLINKER_CMD_UNIT"\":\"");
                numberData += nunit;
                numberData += BLINKER_F("\"");
                // free(nunit);

                nunit = (char*)realloc(nunit, 1*sizeof(char));
                nunit[0] = '\0';
            }

            if (strlen(ntext)) {
                numberData += BLINKER_F(",\""BLINKER_CMD_TEXT"\":\"");
                numberData += (ntext);
                numberData += BLINKER_F("\"");

                ntext = (char*)realloc(ntext, 1*sizeof(char));
                ntext[0] = '\0';
            }

            numberData += BLINKER_F("}");

            Blinker.printArray(numName, numberData);

            // nicon = "";
            // ncolor = "";
            // nunit = "";
        }
};

#endif
