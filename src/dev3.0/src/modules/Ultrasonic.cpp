#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include "Ultrasonic.h"

Ultrasonic::Ultrasonic(uint8_t TP, uint8_t EP, uint32_t TO)
{
    pinMode(TP,OUTPUT);
    pinMode(EP,INPUT);
    Trig_pin = TP;
    Echo_pin = EP;
    Time_out = TO;  // 3000 µs = 50cm // 30000 µs = 5 m 
}

uint32_t Ultrasonic::cm()
{
    uint32_t duration, distance_cm;

    digitalWrite(Trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(Trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig_pin, LOW);
    duration = pulseIn(Echo_pin,HIGH,Time_out);
    if ( duration == 0 ) {
        duration = Time_out;
    }

    distance_cm = duration /29 / 2 ;
    return distance_cm;
}

uint32_t Ultrasonic::inch()
{
    uint32_t duration, distance_inc;

    digitalWrite(Trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(Trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig_pin, LOW);
    duration = pulseIn(Echo_pin,HIGH,Time_out);
    if ( duration == 0 ) {
        duration = Time_out;
    }

    distance_inc = duration / 74 / 2;
    return distance_inc;
}
