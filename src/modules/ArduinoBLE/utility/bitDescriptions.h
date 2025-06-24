#ifndef _BIT_DESCRIPTIONS_H_
#define _BIT_DESCRIPTIONS_H_
#include <Arduino.h>

class AuthReq{
public:
    AuthReq();
    AuthReq(uint8_t octet);
    void setOctet( uint8_t octet);
    uint8_t getOctet();


    // The Bonding_Flags field is a 2-bit field that indicates the type of bonding being requested by the initiating device
    bool Bonding();
    // The MITM field is a 1-bit flag that is set to one if the device is requesting MITM protection
    bool MITM();
    // The SC field is a 1 bit flag. If LE Secure Connections pairing is supported by the device, then the SC field shall be set to 1, otherwise it shall be set to 0.
    bool SC();
    // The keypress field is a 1-bit flag that is used only in the Passkey Entry protocol and shall be ignored in other protocols.
    bool KeyPress();
    // The CT2 field is a 1-bit flag that shall be set to 1 upon transmission to indicate support for the h7 function.
    bool CT2();

    void setBonding(bool state);
    void setMITM(bool state);
    void setSC(bool state);
    void setKeyPress(bool state);
    void setCT2(bool state);
private:
    uint8_t _octet;
};

enum IOCap {
    DisplayOnly,
    DisplayYesNo,
    KeyboardOnly,
    NoInputNoOutput,
    KeyboardDisplay
};

#endif