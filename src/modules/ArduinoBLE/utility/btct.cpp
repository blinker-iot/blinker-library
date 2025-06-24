#if defined(ARDUINO_ARCH_RENESAS)

#include "btct.h"
#include <Arduino.h>
#include "HCI.h"
#include "../ArduinoBLE.h"
BluetoothCryptoToolbox::BluetoothCryptoToolbox(){}
//    In step 1, AES-128 with key K is applied to an all-zero input block.
//    In step 2, K1 is derived through the following operation:
//    If the most significant bit of L is equal to 0, K1 is the left-shift
//    of L by 1 bit.
//    Otherwise, K1 is the exclusive-OR of const_Rb and the left-shift of L
//    by 1 bit.
//    In step 3, K2 is derived through the following operation:
//    If the most significant bit of K1 is equal to 0, K2 is the left-shift
//    of K1 by 1 bit.
//    Otherwise, K2 is the exclusive-OR of const_Rb and the left-shift of
//    K1 by 1 bit.
//    In step 4, (K1,K2) := Generate_Subkey(K) is returned.
unsigned char const_Rb[16] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
  };

#define DHKEY_LENGTH 32
#define N_LEN 16
#define ADDR_LEN 6
#define LEN_LTK 16
#define LEN_MAC_KEY 16

void BluetoothCryptoToolbox::printBytes(uint8_t bytes[], uint8_t length){
#ifdef _BLE_TRACE_
    for(int i=0; i<length; i++){
        if(i>0){
            Serial.print(", 0x");
        }else{
            Serial.print("0x");
        }
        Serial.print(bytes[i],HEX);
    }
    Serial.print('\n');
#endif
}


int BluetoothCryptoToolbox::f5(uint8_t DHKey[],uint8_t N_master[], uint8_t N_slave[],
            uint8_t BD_ADDR_master[], uint8_t BD_ADDR_slave[], uint8_t MacKey[], uint8_t LTK[])
{
    uint8_t SALT[16] = {0x6C, 0x88, 0x83, 0x91, 0xAA, 0xF5, 0xA5, 0x38, 0x60, 0x37, 0x0B, 0xDB, 0x5A, 0x60, 0x83, 0xBE};
    uint8_t keyID[4] = {0x62, 0x74, 0x6c, 0x65};
    uint8_t length[2];
    length[0] = 0x01;
    length[1] = 0x00;
#ifdef _BLE_TRACE_
    Serial.print("Starting f5 calculation");
    Serial.print("Using DHKey:  ");
    printBytes(DHKey, DHKEY_LENGTH);
    Serial.print("Using N_Master: ");
    printBytes(N_master, N_LEN);
    Serial.print("Using N_Slave:  ");
    printBytes(N_slave, N_LEN);

    Serial.println("Using BD_ADDR_MASTER: ");
    printBytes(BD_ADDR_master, ADDR_LEN);
    Serial.println("Using BD_ADDR_SLAVE:  ");
    printBytes(BD_ADDR_slave, ADDR_LEN);
#endif

    uint8_t T[16];

    struct __attribute__ ((packed)) CmacInput
    {
        uint8_t counter;
        uint8_t keyID[4];
        uint8_t N1[16];
        uint8_t N2[16];
        uint8_t A1[7];
        uint8_t A2[7];
        uint8_t length[2];
    } cmacInput = {0,{0},{0},{0},{0},{0},{0}};
    cmacInput.counter = 0;
    memcpy(cmacInput.keyID, keyID, 4);
    memcpy(cmacInput.N1,N_master,16);
    memcpy(cmacInput.N2,N_slave,16);
    memcpy(cmacInput.A1,BD_ADDR_master,7);
    memcpy(cmacInput.A2,BD_ADDR_slave,7);
    memcpy(cmacInput.length,length,2);
    AES_CMAC(SALT, DHKey, 32, T);

    AES_CMAC(T, (uint8_t*)&cmacInput,sizeof(cmacInput), MacKey);
    cmacInput.counter=1;
    AES_CMAC(T, (uint8_t*)&cmacInput, sizeof(cmacInput), LTK);

    return 1;
}
int BluetoothCryptoToolbox::f6(uint8_t W[], uint8_t N1[],uint8_t N2[],uint8_t R[], uint8_t IOCap[], uint8_t A1[], uint8_t A2[], uint8_t Ex[])
{
    struct __attribute__ ((packed)) F6Input
    {
        uint8_t N1[16];
        uint8_t N2[16];
        uint8_t R[16];
        uint8_t IOCap[3];
        uint8_t A1[7];
        uint8_t A2[7];
    } f6Input = {{0},{0},{0},{0},{0},{0}};

    memcpy(f6Input.N1, N1, 16);
    memcpy(f6Input.N2, N2, 16);
    memcpy(f6Input.R, R, 16);
    memcpy(f6Input.IOCap, IOCap, 3);
    memcpy(f6Input.A1, A1, 7);
    memcpy(f6Input.A2, A2, 7);


    AES_CMAC(W, (uint8_t*)&f6Input, sizeof(f6Input),Ex);
    return 1;
}
// AES_CMAC from RFC
int BluetoothCryptoToolbox::ah(uint8_t k[16], uint8_t r[3], uint8_t* result)
{
    uint8_t r_[16];
    int i=0;
    for(i=0; i<16; i++) r_[i] = 0;
    for(i=0; i<3; i++)  r_[i+13] = r[i];
    uint8_t intermediate[16];
    AES_128(k,r_,intermediate);
    for(i=0; i<3; i++){
        result[i] = intermediate[i+13];
    }
    return 1;
}
void BluetoothCryptoToolbox::testAh()
{
    uint8_t irk[16] = {0xec,0x02,0x34,0xa3,0x57,0xc8,0xad,0x05,0x34,0x10,0x10,0xa6,0x0a,0x39,0x7d,0x9b};         
    uint8_t expected_final[3] = {0x0d,0xfb,0xaa};
    uint8_t ourResult[3];
    ah(irk, expected_final, ourResult);

#ifdef _BLE_TRACE_
    Serial.print("Expected   : ");
    printBytes(&expected_final[3], 3);
    Serial.print("Actual     : ");
    printBytes(ourResult, 3);
#endif
}

int BluetoothCryptoToolbox::g2(uint8_t U[], uint8_t V[], uint8_t X[], uint8_t Y[], uint8_t out[4])
{
    struct __attribute__ ((packed)) CmacInput {
        uint8_t U[32];
        uint8_t V[32];
        uint8_t Y[16];
    } cmacInput= {{0},{0},{0}};
    memcpy(cmacInput.U,U,32);
    memcpy(cmacInput.V,V,32);
    memcpy(cmacInput.Y,Y,16);
    uint8_t intermediate[16];
    AES_CMAC(X,(uint8_t*)&cmacInput,sizeof(CmacInput),intermediate);
    memcpy(out,&intermediate[12],4);
    return 1;
}
void BluetoothCryptoToolbox::testg2(){
    uint8_t U[32] = {0x20,0xb0,0x03,0xd2,0xf2,0x97,0xbe,0x2c,0x5e,0x2c,0x83,0xa7,0xe9,0xf9,0xa5,0xb9,0xef,0xf4,0x91,0x11,0xac,0xf4,0xfd,0xdb,0xcc,0x03,0x01,0x48,0x0e,0x35,0x9d,0xe6};
    uint8_t V[32] = {0x55,0x18,0x8b,0x3d,0x32,0xf6,0xbb,0x9a,0x90,0x0a,0xfc,0xfb,0xee,0xd4,0xe7,0x2a,0x59,0xcb,0x9a,0xc2,0xf1,0x9d,0x7c,0xfb,0x6b,0x4f,0xdd,0x49,0xf4,0x7f,0xc5,0xfd};
    uint8_t X[16] = {0xd5,0xcb,0x84,0x54,0xd1,0x77,0x73,0x3e,0xff,0xff,0xb2,0xec,0x71,0x2b,0xae,0xab};
    uint8_t Y[16] = {0xa6,0xe8,0xe7,0xcc,0x25,0xa7,0x5f,0x6e,0x21,0x65,0x83,0xf7,0xff,0x3d,0xc4,0xcf};
    uint8_t out[4];

    uint32_t expected = 0;
    g2(U,V,X,Y,out);
    uint32_t result = 0;
    for(int i=0; i<4; i++) result += out[i] << 8*i;

#ifdef _BLE_TRACE_
    Serial.print("Expected :     ");
    Serial.println(expected);
    Serial.print("Result   : ");
    Serial.println(result);
    Serial.println();
#endif
}

void BluetoothCryptoToolbox::AES_CMAC ( unsigned char *key, unsigned char *input, int length,
                  unsigned char *mac )
{
    unsigned char       X[16],Y[16], M_last[16], padded[16];
    unsigned char       K1[16], K2[16];
    int         n, i, flag;
    generateSubkey(key,K1,K2);

    n = (length+15) / 16;       /* n is number of rounds */

    if ( n == 0 ) {
        n = 1;
        flag = 0;
    } else {
        if ( (length%16) == 0 ) { /* last block is a complete block */
            flag = 1;
        } else { /* last block is not complete block */
            flag = 0;
        }
    }

    if ( flag ) { /* last block is complete block */
        xor_128(&input[16*(n-1)],K1,M_last);
    } else {
        padding(&input[16*(n-1)],padded,length%16);
        xor_128(padded,K2,M_last);
    }

    for ( i=0; i<16; i++ ) X[i] = 0;
    for ( i=0; i<n-1; i++ ) {
        xor_128(X,&input[16*i],Y); /* Y := Mi (+) X  */
        AES_128(key,Y,X);      /* X := AES-128(KEY, Y); */
    }

    xor_128(X,M_last,Y);
    AES_128(key,Y,X);

    for ( i=0; i<16; i++ ) {
        mac[i] = X[i];
    }
}
// Paddinng function from RFC
void BluetoothCryptoToolbox::padding( unsigned char *lastb, unsigned char *pad, int length )
{
    int j;
    /* original last block */
    for ( j=0; j<16; j++ ) {
        if ( j < length ) {
            pad[j] = lastb[j];
        } else if ( j == length ) {
            pad[j] = 0x80;
        } else {
            pad[j] = 0x00;
        }
    }
}
// Generate subkey from RFC
void BluetoothCryptoToolbox::generateSubkey(uint8_t* key, uint8_t* K1, uint8_t* K2){
    unsigned char L[16];
    unsigned char Z[16];
    unsigned char tmp[16];
    int i;

    for ( i=0; i<16; i++ ) Z[i] = 0;

    AES_128(key,Z,L);

    if ( (L[0] & 0x80) == 0 ) { /* If MSB(L) = 0, then K1 = L << 1 */
        leftshift_onebit(L,K1);
    } else {    /* Else K1 = ( L << 1 ) (+) Rb */

        leftshift_onebit(L,tmp);
        xor_128(tmp,const_Rb,K1);
    }

    if ( (K1[0] & 0x80) == 0 ) {
        leftshift_onebit(K1,K2);
    } else {
        leftshift_onebit(K1,tmp);
        xor_128(tmp,const_Rb,K2);
    }
    return;
}
// Use BLE AES function - restart bluetooth if crash
int BluetoothCryptoToolbox::AES_128(uint8_t* key, uint8_t* data_in, uint8_t* data_out){
    uint8_t status = 0;
    int n = 0;
    int tries = 30;
    while(HCI.leEncrypt(key,data_in, &status, data_out)!=1&&n<tries){
#ifdef _BLE_TRACE_
        Serial.print("AES failed... retrying: ");
        Serial.println(n);
#endif
        BLE.end();
        delay(200);
        BLE.begin();
        n++;
        delay(100*n);
    }
    if(n==tries){
#ifdef _BLE_TRACE_
        Serial.println("something went wrong with AES.");
#endif
        return 0;
    }
    return 1;
}
// Tests AES CMAC
#ifdef _BLE_TRACE_
void BluetoothCryptoToolbox::test(){
    unsigned char L[16];
    unsigned char Z[16];
    unsigned char tmp[16];
    int i;

    for ( i=0; i<16; i++ ) Z[i] = 0x00;
    uint8_t k[16]            = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

    Serial.println("AES Plaintext:");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(Z[i],HEX);
    }
    Serial.println(".");
    uint8_t expected_aes[16] = {0x7d, 0xf7, 0x6b, 0x0c, 0x1a, 0xb8, 0x99, 0xb3, 0x3e, 0x42, 0xf0, 0x47, 0xb9, 0x1b, 0x54, 0x6f};

    AES_128(k, Z, L);
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(L[i],HEX);
    }
    Serial.println(".");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(expected_aes[i],HEX);
    }
    Serial.println(".");

    uint8_t k1[16];
    uint8_t k2[16];
    generateSubkey(k,k1,k2);
    uint8_t expected_k1[16]  = {0xfb, 0xee, 0xd6, 0x18, 0x35, 0x71, 0x33, 0x66, 0x7c, 0x85, 0xe0, 0x8f, 0x72, 0x36, 0xa8, 0xde};
    uint8_t expected_k2[16]  = {0xf7, 0xdd, 0xac, 0x30, 0x6a, 0xe2, 0x66, 0xcc, 0xf9, 0x0b, 0xc1, 0x1e, 0xe4, 0x6d, 0x51, 0x3b};

    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(k2[i],HEX);
    }
    Serial.println(".");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(expected_k2[i],HEX);
    }
    Serial.println(".");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(k1[i],HEX);
    }
    Serial.println(".");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(expected_k1[i],HEX);
    }
    Serial.println(".");

    uint8_t m[40] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11};
    uint8_t mac[16];
    uint8_t expected_mac[16] = {0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30, 0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27};
    AES_CMAC(k,m,40,mac);

    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(mac[i],HEX);
    }
    Serial.println(".");
    for(int i=0; i<16; i++){
        Serial.print(" 0x");
        Serial.print(expected_mac[i],HEX);
    }
    Serial.println(".");
}
#endif //_BLE_TRACE_
// From RFC
void BluetoothCryptoToolbox::leftshift_onebit(unsigned char *input,unsigned char *output)
{
    int i;
    unsigned char overflow = 0;

    for ( i=15; i>=0; i-- ) {
        output[i] = input[i] << 1;
        output[i] |= overflow;
        overflow = (input[i] & 0x80)?1:0;
    }
    return;
}
// From RFC
void BluetoothCryptoToolbox::xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
    int i;
    for (i=0;i<16; i++)
    {
        out[i] = a[i] ^ b[i];
    }
}
BluetoothCryptoToolbox btct;

#endif