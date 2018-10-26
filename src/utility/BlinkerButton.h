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

            // bicon = (char*)malloc(1*sizeof(char));
            // bicon[0] = '\0';
            // iconClr = (char*)malloc(1*sizeof(char));
            // iconClr[0] = '\0';
            // bcon = (char*)malloc(1*sizeof(char));
            // bcon[0] = '\0';
            // btext = (char*)malloc(1*sizeof(char));
            // btext[0] = '\0';
            // btext1 = (char*)malloc(1*sizeof(char));
            // btext1[0] = '\0';
            // textClr = (char*)malloc(1*sizeof(char));
            // textClr[0] = '\0';
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

            // bicon = (char*)realloc(bicon, (_icon.length()+1)*sizeof(char));
            bicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(bicon, _icon.c_str());
        }

        void color(const String & _clr) {
            // iconClr = _clr;

            // iconClr = (char*)realloc(iconClr, (_clr.length()+1)*sizeof(char));
            iconClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(iconClr, _clr.c_str());
        }

        template <typename T>
        void content(T _con) {
            // bcon = STRING_format(_con);

            String _bcon = STRING_format(_con);
            // bcon = (char*)realloc(bcon, (_bcon.length()+1)*sizeof(char));
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());
        }

        template <typename T>
        void text(T _text) {
            // btext = STRING_format(_text);

            String _btext = STRING_format(_text);
            // btext = (char*)realloc(btext, (_btext.length()+1)*sizeof(char));
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());
        }

        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2) {
            // btext = STRING_format(_text1); btext1 = STRING_format(_text2);

            String _btext = STRING_format(_text1);
            // btext = (char*)realloc(btext, (_btext.length()+1)*sizeof(char));
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _btext = STRING_format(_text2);
            // btext1 = (char*)realloc(btext1, (_btext.length()+1)*sizeof(char));
            btext1 = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext1, _btext.c_str());
        }

        void textColor(const String & _clr) {
            // textClr = _clr;

            // textClr = (char*)realloc(textClr, (_clr.length()+1)*sizeof(char));
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
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("button _state: "), _state);

            // BLINKER_LOG2(BLINKER_F("bicon: "), bicon == NULL);
            // BLINKER_LOG2(BLINKER_F("iconClr: "), iconClr == NULL);

            // BLINKER_LOG2(BLINKER_F("strlen(bicon): "), strlen(bicon));
            // BLINKER_LOG2(BLINKER_F("strlen(iconClr): "), strlen(iconClr));
            // BLINKER_LOG2(BLINKER_F("strlen(bcon): "), strlen(bcon));
            // BLINKER_LOG2(BLINKER_F("strlen(btext): "), strlen(btext));
            // BLINKER_LOG2(BLINKER_F("strlen(btext1): "), strlen(btext1));
            // BLINKER_LOG2(BLINKER_F("strlen(textClr): "), strlen(textClr));
#endif
            String buttonData;

            if (_state.length()) {
                buttonData += BLINKER_F("{\""BLINKER_CMD_SWITCH"\":\"");
                buttonData += (_state);
                buttonData += BLINKER_F("\"");
            }
            
            // char _buttonData[BLINKER_MAX_SEND_SIZE];

            // strcat(_buttonData, "{\""BLINKER_CMD_SWITCH"\":\"");
            // strcat(_buttonData, _state.c_str());
            // strcat(_buttonData, "\"");

            // if (bicon.length()) {
            //     buttonData += ",\""BLINKER_CMD_ICON"\":\"" + (bicon) + "\"";
            // if (strlen(bicon) > 1) {
            if (bicon) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button bicon: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_ICON"\":\"");
                buttonData += (bicon);
                buttonData += BLINKER_F("\"");

                // bicon = (char*)realloc(bicon, 1*sizeof(char));
                free(bicon);
                // bicon[0] = '\0';
            }

            // if (iconClr.length()) {
            //     buttonData += ",\""BLINKER_CMD_COLOR"\":\"" + (iconClr) + "\"";
            // if (strlen(iconClr) > 1) {
            if (iconClr) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button iconClr: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_COLOR"\":\"");
                buttonData += (iconClr);
                buttonData += BLINKER_F("\"");

                // iconClr = (char*)realloc(iconClr, 1*sizeof(char));
                free(iconClr);
                // iconClr[0] = '\0';
            }

            // if (bcon.length()) {
            //     buttonData += ",\""BLINKER_CMD_CONTENT"\":\"" + (bcon) + "\"";
            // if (strlen(bcon) > 1) {
            if (bcon) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button bcon: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_CONTENT"\":\"");
                buttonData += (bcon);
                buttonData += BLINKER_F("\"");

                // bcon = (char*)realloc(bcon, 1*sizeof(char));
                free(bcon);
                // bcon[0] = '\0';
            }

            // if (btext.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXT"\":\"" + (btext) + "\"";
            // if (strlen(btext) > 1) {
            if (btext) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button btext: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_TEXT"\":\"");
                buttonData += (btext);
                buttonData += BLINKER_F("\"");

                // btext = (char*)realloc(btext, 1*sizeof(char));
                free(btext);
                // btext[0] = '\0';
            }

            // if (btext1.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXT1"\":\"" + (btext1) + "\"";
            // if (strlen(btext1) > 1) {
            if (btext1) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button btext1: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_TEXT1"\":\"");
                buttonData += (btext1);
                buttonData += BLINKER_F("\"");

                // btext1 = (char*)realloc(btext1, 1*sizeof(char));
                free(btext1);
                // btext1[0] = '\0';
            }

            // if (textClr.length()) {
            //     buttonData += ",\""BLINKER_CMD_TEXTCOLOR"\":\"" + (textClr) + "\"";
            // if (strlen(textClr) > 1) {
            if (textClr) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData = BLINKER_F("{");
                // BLINKER_LOG1(BLINKER_F("button textClr: "));
                buttonData += BLINKER_F("\""BLINKER_CMD_TEXTCOLOR"\":\"");
                buttonData += (textClr);
                buttonData += BLINKER_F("\"");

                // textClr = (char*)realloc(textClr, 1*sizeof(char));
                free(textClr);
                // textClr[0] = '\0';
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
        char * bicon;
        char * iconClr;
        char * bcon;
        char * btext;
        char * btext1;
        char * textClr;
};

#endif
