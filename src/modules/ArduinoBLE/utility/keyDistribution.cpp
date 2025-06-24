#include "keyDistribution.h"

KeyDistribution::KeyDistribution():_octet(0){}
KeyDistribution::KeyDistribution(uint8_t octet):_octet(octet){}

#define ENCKEY  0b00000001
#define IDKEY   0b00000010
#define SIGNKEY 0b00000100
#define LINKKEY 0b00001000
void KeyDistribution::setOctet( uint8_t octet){_octet = octet;}
uint8_t KeyDistribution::getOctet() {return _octet;}
// Ignored when SMP is on LE transport
bool KeyDistribution::EncKey(){ return (_octet & ENCKEY)>0;}
// Device shall distribute IRK using Identity information command followed by its address using Identity address information
bool KeyDistribution::IdKey(){ return  (_octet & IDKEY)>0;}
// Device shall distribute CSRK using signing information command
bool KeyDistribution::SignKey(){ return (_octet & SIGNKEY)>0;}
// Device would like to derive BR/EDR from LTK
bool KeyDistribution::LinkKey(){ return (_octet & LINKKEY)>0;}

void KeyDistribution::setEncKey(bool state) { _octet= state? _octet|ENCKEY  : _octet&~ENCKEY;}
void KeyDistribution::setIdKey(bool state)  { _octet= state? _octet|IDKEY   : _octet&~IDKEY;}
void KeyDistribution::setSignKey(bool state){ _octet= state? _octet|SIGNKEY : _octet&~SIGNKEY;}
void KeyDistribution::setLinkKey(bool state){ _octet= state? _octet|LINKKEY : _octet&~LINKKEY;}