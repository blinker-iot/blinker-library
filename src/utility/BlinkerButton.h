#ifndef BlinkerButton_H
#define BlinkerButton_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerButton
{
    public :
        BlinkerButton(const String & _name, callback_with_string_arg_t _func = NULL)
            // : buttonName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);

            buttonName = (char*)malloc((_name.length()+1)*sizeof(char));
            strcpy(buttonName, _name.c_str());

            // free(bicon);
            // free(iconClr);
            // free(bcon);
            // free(btext);
            // free(btext1);
            // free(textClr);
        }

        void attach(callback_with_string_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(buttonName, _func);
        }

        void icon(const String & _icon) {
            // bicon = _icon;
            // if (strlen(bicon)) free(bicon);

            bicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(bicon, _icon.c_str());
        }

        void color(const String & _clr) {
            // iconClr = _clr;
            // if (strlen(iconClr)) free(iconClr);

            iconClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(iconClr, _clr.c_str());
        }

        template <typename T>
        void content(T _con) {
            // bcon = STRING_format(_con);
            // if (strlen(bcon)) free(bcon);

            String _bcon = STRING_format(_con);
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());
        }

        template <typename T>
        void text(T _text) {
            // btext = STRING_format(_text);
            // if (strlen(btext)) free(btext);

            String _btext = STRING_format(_text);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());
        }

        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2) {
            // btext = STRING_format(_text1); btext1 = STRING_format(_text2);
            // if (strlen(btext)) free(btext);
            // if (strlen(btext1)) free(btext1);

            String _btext = STRING_format(_text1);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _btext = STRING_format(_text2);
            btext1 = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext1, _btext.c_str());
        }

        void textColor(const String & _clr) {
            // textClr = _clr;
            // if (strlen(textClr)) free(textClr);

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());
        }

        void print() { print(""); }

        void print(const String & _state)
        {
            if (!registered) {
                return;
            }

            // String buttonData = "{\"" + _state + "\",\"" + bicon + \
            //                     "\",\"" + iconClr + "\",\"" + btext + \
            //                     "\",\"" + textClr + "\"}";

            String buttonData;
            buttonData += BLINKER_F("{\""BLINKER_CMD_SWITCH"\":\"");
            buttonData += (_state);
            buttonData += BLINKER_F("\"");
            
            // char _buttonData[BLINKER_MAX_SEND_SIZE];

            // strcat(_buttonData, "{\""BLINKER_CMD_SWITCH"\":\"");
            // strcat(_buttonData, _state.c_str());
            // strcat(_buttonData, "\"");

            // if (bicon.length()) {
            //     buttonData += ",\""BLINKER_CMD_ICON"\":\"" + (bicon) + "\"";
            if (strlen(bicon)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_ICON"\":\"");
                buttonData += (bicon);
                buttonData += BLINKER_F("\"");
                free(bicon);
            }

            // if (iconClr.length()) {
            //     buttonData += ",\""BLINKER_CMD_COLOR"\":\"" + (iconClr) + "\"";
            if (strlen(iconClr)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_COLOR"\":\"");
                buttonData += (iconClr);
                buttonData += BLINKER_F("\"");
                free(iconClr);
            }

            // if (bcon.length()) {
            //     buttonData += ",\""BLINKER_CMD_CONTENT"\":\"" + (bcon) + "\"";
            if (strlen(bcon)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_CONTENT"\":\"");
                buttonData += (bcon);
                buttonData += BLINKER_F("\"");
                free(bcon);
            }

            // if (btext.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXT"\":\"" + (btext) + "\"";
            if (strlen(btext)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_TEXT"\":\"");
                buttonData += (btext);
                buttonData += BLINKER_F("\"");
                free(btext);
            }

            // if (btext1.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXT1"\":\"" + (btext1) + "\"";
            if (strlen(btext1)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_TEXT1"\":\"");
                buttonData += (btext1);
                buttonData += BLINKER_F("\"");
                free(btext1);
            }

            // if (textClr.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXTCOLOR"\":\"" + (textClr) + "\"";
            if (strlen(textClr)) {
                buttonData += BLINKER_F(",\""BLINKER_CMD_TEXTCOLOR"\":\"");
                buttonData += (textClr);
                buttonData += BLINKER_F("\"");
                free(textClr);
            }

            buttonData += BLINKER_F("}");

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("buttonData: "), buttonData);

//             BLINKER_LOG4(BLINKER_F("bicon: "), bicon, " ,", bicon.length());
//             BLINKER_LOG4(BLINKER_F("iconClr: "), iconClr, " ,", iconClr.length());
//             BLINKER_LOG4(BLINKER_F("bcon: "), bcon, " ,", bcon.length());
//             BLINKER_LOG4(BLINKER_F("btext: "), btext, " ,", btext.length());
//             BLINKER_LOG4(BLINKER_F("btext1: "), btext1, " ,", btext1.length());
//             BLINKER_LOG4(BLINKER_F("textClr: "), textClr, " ,", textClr.length());
#endif

            Blinker.printArray(buttonName, buttonData);

            // bicon = "";
            // iconClr = "";
            // bcon = "";
            // btext = "";
            // btext1 = "";
            // textClr = "";

            // free(bicon);
            // free(iconClr);
            // free(bcon);
            // free(btext);
            // free(btext1);
            // free(textClr);
        }
        // bool checkName(String name) { return ((buttonName == name) ? true : false); }

    private :
        // String buttonName;
        char * buttonName;
        // callback_with_string_arg_t _bfunc = NULL;
        bool registered = false;
        // String bicon = "";
        // String iconClr = "";
        // String bcon = "";
        // String btext = "";
        // String btext1 = "";
        // String textClr = "";
        char * bicon = "";
        char * iconClr = "";
        char * bcon = "";
        char * btext = "";
        char * btext1 = "";
        char * textClr = "";
};

#endif
