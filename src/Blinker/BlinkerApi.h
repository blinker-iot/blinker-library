#ifndef BlinkerApi_H
#define BlinkerApi_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerDebug.h>
#include <utility/BlinkerUtility.h>

enum b_widgettype_t {
    W_BUTTON,
    W_SLIDER,
    W_TOGGLE
};

enum b_joystickaxis_t {
    J_Xaxis,
    J_Yaxis
};

enum b_ahrsattitude_t {
    Yaw,
    Pitch,
    Roll
};

enum b_gps_t {
    LONG,
    LAT
};

static class BlinkerButton * _Button[BLINKER_MAX_WIDGET_SIZE];
static class BlinkerSlider * _Slider[BLINKER_MAX_WIDGET_SIZE];
static class BlinkerToggle * _Toggle[BLINKER_MAX_WIDGET_SIZE];

class BlinkerButton
{
    public :
        BlinkerButton()
            : buttonName(NULL), buttonState(false)
        {}
        
        void name(String name) { buttonName = name; }
        String getName() { return buttonName; }
        void freshState(bool state) { buttonState = state; }
        bool getState() { return buttonState; }
        bool checkName(String name) { return ((buttonName == name) ? true : false); }
    
    private :
        String  buttonName;
        bool    buttonState;
};

class BlinkerSlider
{
    public :
        BlinkerSlider()
            : sliderName(NULL), sliderValue(0)
        {}
        
        void name(String name) { sliderName = name; }
        String getName() { return sliderName; }
        void freshValue(uint8_t value) { sliderValue = value; }
        uint8_t getValue() { return sliderValue; }
        bool checkName(String name) { return ((sliderName == name) ? true : false); }
    
    private :
        String  sliderName;
        uint8_t sliderValue;
};

class BlinkerToggle
{
    public :
        BlinkerToggle()
            : toggleName(NULL), toggleState(false)
        {}
        
        void name(String name) { toggleName = name; }
        String getName() { return toggleName; }
        void freshState(bool state) { toggleState = state; }
        bool getState() { return toggleState; }
        bool checkName(String name) { return ((toggleName == name) ? true : false); }
    
    private :
        String  toggleName;
        bool    toggleState;
};

template <class T>
int8_t checkNum(String name, T * c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++) {
        if (c[cNum]->checkName(name))
            return cNum;
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

template <class Proto>
class BlinkerApi
{
    public :
        BlinkerApi() {
            joyValue[J_Xaxis] = BLINKER_JOYSTICK_VALUE_DEFAULT;
            joyValue[J_Yaxis] = BLINKER_JOYSTICK_VALUE_DEFAULT;
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
            gpsValue[LONG] = "0.000000"
            gpsValue[LAT] = "0.000000"
        }

        void wInit(const String & _name, b_widgettype_t _type) {
            switch (_type) {
                case W_BUTTON :
                    if (checkNum(_name, _Button, _bCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Button[_bCount] = new BlinkerButton();
                            _Button[_bCount]->name(_name);
                            _bCount++;
                        }
                    }
                    break;
                case W_SLIDER :
                    if (checkNum(_name, _Slider, _sCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Slider[_sCount] = new BlinkerSlider();
                            _Slider[_sCount]->name(_name);
                            _sCount++;
                        }
                    }
                    break;
                case W_TOGGLE :
                    if (checkNum(_name, _Toggle, _tCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Toggle[_tCount] = new BlinkerToggle();
                            _Toggle[_tCount]->name(_name);
                            _tCount++;
                        }
                    }
                    break;
                default :
                    break;
            }
        }

        void parse()
        {
            if (static_cast<Proto*>(this)->parseState() ) {
                _fresh = false;

                for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                    buttonParse(_Button[bNum]->getName());
                }
                for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                    slider(_Slider[sNum]->getName());
                }
                for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                    toggle(_Toggle[kNum]->getName());
                }

                joystick(J_Xaxis);
                ahrs(Yaw);

                if (_fresh) {
                    static_cast<Proto*>(this)->isParsed();
                }
            }
        }

        bool button(const String & _bName)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _bName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_BUTTON_PRESSED || state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                bool _state = _Button[num]->getState();
                _Button[num]->freshState(false);

                return _state;
            }
        }

        bool toggle(const String & _tName)
        {
            int8_t num = checkNum(_tName, _Toggle, _tCount);
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _tName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_ON) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(true);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_OFF) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(false);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _tCount++;
                    }
                    return false;
                }

                return _Toggle[num]->getState();
            }
        }

        uint8_t slider(const String & _sName)
        {
            int8_t num = checkNum(_sName, _Slider, _sCount);
            int16_t value = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), _sName);

            if (value != FIND_KEY_VALUE_FAILED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _Slider[_sCount]->freshValue(value);
                        _sCount++;
                    }
                }
                else {
                    _Slider[num]->freshValue(value);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _sCount++;
                    }
                    return 0;
                }
                
                return _Slider[num]->getValue();
            }
        }

        uint8_t joystick(b_joystickaxis_t axis)
        {
            int16_t jAxisValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, axis);

            if (jAxisValue != FIND_KEY_VALUE_FAILED) {
                joyValue[J_Xaxis] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, J_Xaxis);
                joyValue[J_Yaxis] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, J_Yaxis);

                _fresh = true;
                return jAxisValue;
            }
            else {
                return joyValue[axis];
            }
        }

        int16_t ahrs(b_ahrsattitude_t attitude)
        {
            int16_t aAttiValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, attitude);

            if (aAttiValue != FIND_KEY_VALUE_FAILED) {
                ahrsValue[Yaw] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Yaw);
                ahrsValue[Roll] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Roll);
                ahrsValue[Pitch] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Pitch);

                _fresh = true;

                return aAttiValue;
            }
            else {
                return ahrsValue[attitude];
            }
        }
        
        void attachAhrs()
        {
            bool state = false;
            uint32_t startTime = millis();
            static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
            while (!state) {
                while (!static_cast<Proto*>(this)->connected()) {
                    static_cast<Proto*>(this)->run();
                    if (static_cast<Proto*>(this)->connect()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                        break;
                    }
                }
                
                ::delay(100);

                if (static_cast<Proto*>(this)->checkAvail()) {
                    BLINKER_LOG2("GET: ", static_cast<Proto*>(this)->dataParse());
                    if (STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS)) {
                        BLINKER_LOG1("AHRS attach sucessed...");
                        parse();
                        state = true;
                        break;
                    }
                    else {
                        BLINKER_LOG1("AHRS attach failed...Try again");
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                    }
                }
                else {
                    if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS) {
                        BLINKER_LOG1("AHRS attach failed...Try again");
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                    }
                }
            }
        }

        void detachAhrs()
        {
            static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_OFF);
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
        }

        String gps(b_gps_t axis) {
            String axisValue = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, axis);

            if (axisValue != "") {
                gpsValue[LONG] = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, LONG);
                gpsValue[LAT] = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, LAT);

                _fresh = true;

                return gpsValue[axis];
            }
            else {
                return gpsValue[axis];
            }
        }

        void attachGPS()
        {
            bool state = false;
            uint32_t startTime = millis();
            static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, BLINKER_CMD_ON);
            while (!state) {
                while (!static_cast<Proto*>(this)->connected()) {
                    static_cast<Proto*>(this)->run();
                    if (static_cast<Proto*>(this)->connect()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, BLINKER_CMD_ON);
                        break;
                    }
                }
                
                ::delay(100);

                if (static_cast<Proto*>(this)->checkAvail()) {
                    BLINKER_LOG2("GET: ", static_cast<Proto*>(this)->dataParse());
                    if (STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS)) {
                        BLINKER_LOG1("GPS attach sucessed...");
                        parse();
                        state = true;
                        break;
                    }
                    else {
                        BLINKER_LOG1("GPS attach failed...Try again");
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, BLINKER_CMD_ON);
                    }
                }
                else {
                    if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS) {
                        BLINKER_LOG1("GPS attach failed...Try again");
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, BLINKER_CMD_ON);
                    }
                }
            }
        }

        void detachGPS()
        {
            static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, BLINKER_CMD_OFF);
        }

        void vibrate(uint16_t ms = 500)
        {
            if (ms > 1000) {
                ms = 1000;
            }

            static_cast<Proto*>(this)->print(BLINKER_CMD_VIBRATE, ms);
        }

        void delay(unsigned long ms)
        {
            uint32_t start = micros();
            while (ms > 0) {
                static_cast<Proto*>(this)->run();
                
                if ((micros() - start)/1000 >= ms) {
                    ms = 0;
                }
            }
        }
    
    private :
        uint8_t _bCount = 0;
        uint8_t _sCount = 0;
        uint8_t _tCount = 0;
        uint8_t joyValue[2];
        int16_t ahrsValue[3];
        String  gpsValue[2];
        bool    _fresh = false;

        bool buttonParse(const String & _bName)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _bName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_BUTTON_PRESSED || state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                return _Button[num]->getState();
            }
        }
};

#endif