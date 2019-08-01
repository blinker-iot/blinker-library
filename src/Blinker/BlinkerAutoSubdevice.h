#ifndef BLINKER_AUTO_SUBDEVICE_H
#define BLINKER_AUTO_SUBDEVICE_H

#if defined(BLINKER_WIFI_SUBDEVICE)

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include <EEPROM.h>

// #include "Blinker/BlinkerAuto.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerUtility.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif

class BlinkerAutoSubdevice
{
    private :
        uint8_t     a_num;

        // {
        //     "auto":{
        //         "ena":1,//_autoState
        //         "id":123456,//_autoId
        //         "logic":"numberic",//_logicType
        //         "data":
        //         [
        //             {
        //                 "key":"humi",
        //                 "value":40,
        //                 "type":"<",//_targetState|_compareType
        //                 "dur":10
        //             }
        //         ],
        //         "range":[540, 1260],
        //     }
        // }

        // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
        // | | | | |            | _time1 0-1440min 11  | _time2 0-1440min 11                   
        // | | | | | _duration 0-60min 6
        // | | | | _targetState|_compareType on/off|less/equal/greater 2
        // | | | _targetState|_compareType on/off|less/equal/greater
        // |  
        // | logic_type state/numberic 2
        // autoData

        // | _linkNum
        // - - - - - - - -
        // | | |_logicType state/numberic/and/or 2    
        // | | _autoState true/false 1    
        // | _haveAuto     
        // | 
        // typestate
};

#endif

#endif
