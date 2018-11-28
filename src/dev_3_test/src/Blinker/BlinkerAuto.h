#ifndef BLINKER_AUTO_H
#define BLINKER_AUTO_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

class BlinkerAUTO
{
    public :
        BlinkerAUTO()
            : _haveAuto(false)
            , _autoState(false)
            , _trigged(false)
        {}

        void run(const String & key, float data, int32_t nowTime);
        void run(const String & key, const String & state, int32_t nowTime);
        void manager(const String & data);
        void deserialization();
        void serialization();
        void setNum(uint8_t num) { a_num = num; }
        bool isTrigged() { return _trigged; }
        void fresh();
        uint32_t id() { return _autoId; }

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
        bool        _haveAuto;
        bool        _autoState;
        uint32_t    _autoId = 0;
        uint8_t     _logicType;
        uint8_t     _typeState;
        uint32_t    _autoData[2];

        uint8_t     _targetNum = 0;

        uint8_t     logic_type[2];
        char        _targetKey[2][12];
        float       _targetData[2];
        uint8_t     _compareType[2];
        bool        _targetState[2];
        uint32_t    _time1[2];
        uint32_t    _time2[2];
        uint32_t    _duration[2];
        uint32_t    _treTime[2];
        bool        isRecord[2];
        bool        _isTrigged[2];
        bool        _trigged;

        void triggerCheck(const String & state, uint8_t num);
};

#endif
