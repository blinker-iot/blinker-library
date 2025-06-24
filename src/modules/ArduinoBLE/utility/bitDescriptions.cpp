#include "bitDescriptions.h"


#define BONDING_BIT  0b00000001
#define MITM_BIT     0b00000100
#define SC_BIT       0b00001000
#define KEYPRESS_BIT 0b00010000
#define CT2_BIT      0b00100000


AuthReq::AuthReq(){}
AuthReq::AuthReq(uint8_t octet):_octet(octet){}
bool AuthReq::Bonding(){ return (_octet & BONDING_BIT)>0;}
bool AuthReq::MITM(){ return (_octet & MITM_BIT)>0;}
bool AuthReq::SC(){ return (_octet & SC_BIT)>0;}
bool AuthReq::KeyPress(){ return (_octet & KEYPRESS_BIT)>0;}
bool AuthReq::CT2(){ return (_octet & CT2_BIT)>0;}


void AuthReq::setBonding(bool state) { _octet= state? _octet|BONDING_BIT  : _octet&~BONDING_BIT;}
void AuthReq::setMITM(bool state)  { _octet= state? _octet|MITM_BIT   : _octet&~MITM_BIT;}
void AuthReq::setSC(bool state){ _octet= state? _octet|SC_BIT : _octet&~SC_BIT;}
void AuthReq::setKeyPress(bool state){ _octet= state? _octet|KEYPRESS_BIT : _octet&~KEYPRESS_BIT;}
void AuthReq::setCT2(bool state){ _octet= state? _octet|CT2_BIT : _octet&~CT2_BIT;}

uint8_t _octet;


void AuthReq::setOctet( uint8_t octet){_octet = octet;}
uint8_t AuthReq::getOctet() {return _octet;}
