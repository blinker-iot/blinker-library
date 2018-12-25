#ifndef Ultrasonic_h
#define Ultrasonic_h

#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

class Ultrasonic
{
    public:
        Ultrasonic(uint8_t TP, uint8_t EP, uint32_t TO = 30000);
        uint32_t cm();
        uint32_t inch();

    private:
        uint8_t Trig_pin;
        uint8_t Echo_pin;
        uint32_t Time_out;
};

#endif
