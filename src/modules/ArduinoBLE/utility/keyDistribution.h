#ifndef _KEY_DISTRIBUTION_H_
#define _KEY_DISTRIBUTION_H_
#include <Arduino.h>

class KeyDistribution{
public:
    KeyDistribution();
    KeyDistribution(uint8_t octet);
    void setOctet( uint8_t octet);
    uint8_t getOctet();
    // Ignored when SMP is on LE transport
    bool EncKey();
    // Device shall distribute IRK using Identity information command followed by its address using Identity address information
    bool IdKey();
    // Device shall distribute CSRK using signing information command
    bool SignKey();
    // Device would like to derive BR/EDR from LTK
    bool LinkKey();

    void setEncKey(bool state);
    void setIdKey(bool state);
    void setSignKey(bool state);
    void setLinkKey(bool state);
private:
    uint8_t _octet;
    // 1.   IRK by the slave2.   BD ADDR by the slave3.   CSRK by the slave4.   IRK by the master5.   BD_ADDR by the master6.   CSRK by the master
};

#endif