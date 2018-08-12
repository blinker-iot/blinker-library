/*-------------------------------------------------------------------------
  Arduino library to control a wide variety of WS2811- and WS2812-based RGB
  LED devices such as Adafruit FLORA RGB Smart Pixels and NeoPixel strips.
  Currently handles 400 and 800 KHz bitstreams on 8, 12 and 16 MHz ATmega
  MCUs, with LEDs wired for various color orders.  Handles most output pins
  (possible exception with upper PORT registers on the Arduino Mega).

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries,
  contributions by PJRC, Michael Miller and other members of the open
  source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  -------------------------------------------------------------------------
  This file is part of the Adafruit NeoPixel library.

  NeoPixel is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  NeoPixel is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NeoPixel.  If not, see
  <http://www.gnu.org/licenses/>.
  -------------------------------------------------------------------------*/

#include "Adafruit_NeoPixel.h"

#if defined(NRF52)
#include "nrf.h"

// Interrupt is only disabled if there is no PWM device available
// Note: Adafruit Bluefruit nrf52 does not use this option
//#define NRF52_DISABLE_INT
#endif

// Constructor when length, pin and type are known at compile-time:
Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, uint8_t p, neoPixelType t) :
  begun(false), brightness(0), pixels(NULL), endTime(0)  
{
  updateType(t);
  updateLength(n);
  setPin(p);
}

// via Michael Vogt/neophob: empty constructor is used when strand length
// isn't known at compile-time; situations where program config might be
// read from internal flash memory or an SD card, or arrive via serial
// command.  If using this constructor, MUST follow up with updateType(),
// updateLength(), etc. to establish the strand type, length and pin number!
Adafruit_NeoPixel::Adafruit_NeoPixel() :
#ifdef NEO_KHZ400
  is800KHz(true),
#endif
  begun(false), numLEDs(0), numBytes(0), pin(-1), brightness(0), pixels(NULL),
  rOffset(1), gOffset(0), bOffset(2), wOffset(1), endTime(0)
{
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  if(pixels)   free(pixels);
  if(pin >= 0) pinMode(pin, INPUT);
}

void Adafruit_NeoPixel::begin(void) {
  if(pin >= 0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  begun = true;

}

void Adafruit_NeoPixel::updateLength(uint16_t n) {
  if(pixels) free(pixels); // Free existing data (if any)

  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  numBytes = n * ((wOffset == rOffset) ? 3 : 4);
  if((pixels = (uint8_t *)malloc(numBytes))) {
    memset(pixels, 0, numBytes);
    numLEDs = n;
  } else {
    numLEDs = numBytes = 0;
  }
}

void Adafruit_NeoPixel::updateType(neoPixelType t) {
  boolean oldThreeBytesPerPixel = (wOffset == rOffset); // false if RGBW

  wOffset = (t >> 6) & 0b11; // See notes in header file
  rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
  gOffset = (t >> 2) & 0b11;
  bOffset =  t       & 0b11;
#ifdef NEO_KHZ400
  is800KHz = (t < 256);      // 400 KHz flag is 1<<8
#endif

  // If bytes-per-pixel has changed (and pixel data was previously
  // allocated), re-allocate to new size.  Will clear any data.
  if(pixels) {
    boolean newThreeBytesPerPixel = (wOffset == rOffset);
    if(newThreeBytesPerPixel != oldThreeBytesPerPixel) updateLength(numLEDs);
  }
}

#if defined(ESP8266) 
// ESP8266 show() is external to enforce ICACHE_RAM_ATTR execution
extern "C" void ICACHE_RAM_ATTR espShow(
  uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t type);
#elif defined(ESP32)
extern "C" void espShow(
  uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t type);
#endif // ESP8266

void Adafruit_NeoPixel::show(void) {

  if(!pixels) return;

  // Data latch = 300+ microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  while(!canShow());
  // endTime is a private member (rather than global var) so that multiple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).

  // In order to make this code runtime-configurable to work with any pin,
  // SBI/CBI instructions are eschewed in favor of full PORT writes via the
  // OUT or ST instructions.  It relies on two facts: that peripheral
  // functions (such as PWM) take precedence on output pins, so our PORT-
  // wide writes won't interfere, and that interrupts are globally disabled
  // while data is being issued to the LEDs, so no other code will be
  // accessing the PORT.  The code takes an initial 'snapshot' of the PORT
  // state, computes 'pin high' and 'pin low' values, and writes these back
  // to the PORT register as needed.

  // NRF52 may use PWM + DMA (if available), may not need to disable interrupt
#ifndef NRF52
  noInterrupts(); // Need 100% focus on instruction timing
#endif

#ifdef __AVR__
// AVR MCUs -- ATmega & ATtiny (no XMEGA) ---------------------------------

  volatile uint16_t
    i   = numBytes; // Loop counter
  volatile uint8_t
   *ptr = pixels,   // Pointer to next byte
    b   = *ptr++,   // Current byte value
    hi,             // PORT w/output bit set high
    lo;             // PORT w/output bit set low

  // Hand-tuned assembly code issues data to the LED drivers at a specific
  // rate.  There's separate code for different CPU speeds (8, 12, 16 MHz)
  // for both the WS2811 (400 KHz) and WS2812 (800 KHz) drivers.  The
  // datastream timing for the LED drivers allows a little wiggle room each
  // way (listed in the datasheets), so the conditions for compiling each
  // case are set up for a range of frequencies rather than just the exact
  // 8, 12 or 16 MHz values, permitting use with some close-but-not-spot-on
  // devices (e.g. 16.5 MHz DigiSpark).  The ranges were arrived at based
  // on the datasheet figures and have not been extensively tested outside
  // the canonical 8/12/16 MHz speeds; there's no guarantee these will work
  // close to the extremes (or possibly they could be pushed further).
  // Keep in mind only one CPU speed case actually gets compiled; the
  // resulting program isn't as massive as it might look from source here.

// 8 MHz(ish) AVR ---------------------------------------------------------
#if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif

    volatile uint8_t n1, n2 = 0;  // First, next bits out

    // Squeezing an 800 KHz stream out of an 8 MHz chip requires code
    // specific to each PORT register.

    // 10 instruction clocks per bit: HHxxxxxLLL
    // OUT instructions:              ^ ^    ^   (T=0,2,7)

    // PORTD OUTPUT ----------------------------------------------------

#if defined(PORTD)
 #if defined(PORTB) || defined(PORTC) || defined(PORTF)
    if(port == &PORTD) {
 #endif // defined(PORTB/C/F)

      hi = PORTD |  pinMask;
      lo = PORTD & ~pinMask;
      n1 = lo;
      if(b & 0x80) n1 = hi;

      // Dirty trick: RJMPs proceeding to the next instruction are used
      // to delay two clock cycles in one instruction word (rather than
      // using two NOPs).  This was necessary in order to squeeze the
      // loop down to exactly 64 words -- the maximum possible for a
      // relative branch.

      asm volatile(
       "headD:"                   "\n\t" // Clk  Pseudocode
        // Bit 7:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
        "out  %[port] , %[n1]"    "\n\t" // 1    PORT = n1
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 6"        "\n\t" // 1-2  if(b & 0x40)
         "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 6:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
        "out  %[port] , %[n2]"    "\n\t" // 1    PORT = n2
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 5"        "\n\t" // 1-2  if(b & 0x20)
         "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 5:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
        "out  %[port] , %[n1]"    "\n\t" // 1    PORT = n1
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 4"        "\n\t" // 1-2  if(b & 0x10)
         "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 4:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
        "out  %[port] , %[n2]"    "\n\t" // 1    PORT = n2
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 3"        "\n\t" // 1-2  if(b & 0x08)
         "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 3:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
        "out  %[port] , %[n1]"    "\n\t" // 1    PORT = n1
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 2"        "\n\t" // 1-2  if(b & 0x04)
         "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 2:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
        "out  %[port] , %[n2]"    "\n\t" // 1    PORT = n2
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 1"        "\n\t" // 1-2  if(b & 0x02)
         "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "rjmp .+0"                "\n\t" // 2    nop nop
        // Bit 1:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
        "out  %[port] , %[n1]"    "\n\t" // 1    PORT = n1
        "rjmp .+0"                "\n\t" // 2    nop nop
        "sbrc %[byte] , 0"        "\n\t" // 1-2  if(b & 0x01)
         "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "sbiw %[count], 1"        "\n\t" // 2    i-- (don't act on Z flag yet)
        // Bit 0:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi
        "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
        "out  %[port] , %[n2]"    "\n\t" // 1    PORT = n2
        "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++
        "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 0x80)
         "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo
        "brne headD"              "\n"   // 2    while(i) (Z flag set above)
      : [byte]  "+r" (b),
        [n1]    "+r" (n1),
        [n2]    "+r" (n2),
        [count] "+w" (i)
      : [port]   "I" (_SFR_IO_ADDR(PORTD)),
        [ptr]    "e" (ptr),
        [hi]     "r" (hi),
        [lo]     "r" (lo));

 #if defined(PORTB) || defined(PORTC) || defined(PORTF)
    } else // other PORT(s)
 #endif // defined(PORTB/C/F)
#endif // defined(PORTD)

    // PORTB OUTPUT ----------------------------------------------------

#if defined(PORTB)
 #if defined(PORTD) || defined(PORTC) || defined(PORTF)
    if(port == &PORTB) {
 #endif // defined(PORTD/C/F)

      // Same as above, just switched to PORTB and stripped of comments.
      hi = PORTB |  pinMask;
      lo = PORTB & ~pinMask;
      n1 = lo;
      if(b & 0x80) n1 = hi;

      asm volatile(
       "headB:"                   "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 6"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 5"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 4"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 3"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 2"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 1"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 0"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "brne headB"              "\n"
      : [byte] "+r" (b), [n1] "+r" (n1), [n2] "+r" (n2), [count] "+w" (i)
      : [port] "I" (_SFR_IO_ADDR(PORTB)), [ptr] "e" (ptr), [hi] "r" (hi),
        [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTC) || defined(PORTF)
    }
 #endif
 #if defined(PORTC) || defined(PORTF)
    else
 #endif // defined(PORTC/F)
#endif // defined(PORTB)

    // PORTC OUTPUT ----------------------------------------------------

#if defined(PORTC)
 #if defined(PORTD) || defined(PORTB) || defined(PORTF)
    if(port == &PORTC) {
 #endif // defined(PORTD/B/F)

      // Same as above, just switched to PORTC and stripped of comments.
      hi = PORTC |  pinMask;
      lo = PORTC & ~pinMask;
      n1 = lo;
      if(b & 0x80) n1 = hi;

      asm volatile(
       "headC:"                   "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 6"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 5"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 4"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 3"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 2"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 1"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 0"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "brne headC"              "\n"
      : [byte] "+r" (b), [n1] "+r" (n1), [n2] "+r" (n2), [count] "+w" (i)
      : [port] "I" (_SFR_IO_ADDR(PORTC)), [ptr] "e" (ptr), [hi] "r" (hi),
        [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTB) || defined(PORTF)
    }
 #endif // defined(PORTD/B/F)
 #if defined(PORTF)
    else
 #endif
#endif // defined(PORTC)

    // PORTF OUTPUT ----------------------------------------------------

#if defined(PORTF)
 #if defined(PORTD) || defined(PORTB) || defined(PORTC)
    if(port == &PORTF) {
 #endif // defined(PORTD/B/C)

      hi = PORTF |  pinMask;
      lo = PORTF & ~pinMask;
      n1 = lo;
      if(b & 0x80) n1 = hi;

      asm volatile(
       "headF:"                   "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 6"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 5"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 4"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 3"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 2"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 1"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n2]   , %[lo]"    "\n\t"
        "out  %[port] , %[n1]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "sbrc %[byte] , 0"        "\n\t"
         "mov %[n2]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "mov  %[n1]   , %[lo]"    "\n\t"
        "out  %[port] , %[n2]"    "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[n1]   , %[hi]"    "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "brne headF"              "\n"
      : [byte] "+r" (b), [n1] "+r" (n1), [n2] "+r" (n2), [count] "+w" (i)
      : [port] "I" (_SFR_IO_ADDR(PORTF)), [ptr] "e" (ptr), [hi] "r" (hi),
        [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTB) || defined(PORTC)
    }
 #endif // defined(PORTD/B/C)
#endif // defined(PORTF)

#ifdef NEO_KHZ400
  } else { // end 800 KHz, do 400 KHz

    // Timing is more relaxed; unrolling the inner loop for each bit is
    // not necessary.  Still using the peculiar RJMPs as 2X NOPs, not out
    // of need but just to trim the code size down a little.
    // This 400-KHz-datastream-on-8-MHz-CPU code is not quite identical
    // to the 800-on-16 code later -- the hi/lo timing between WS2811 and
    // WS2812 is not simply a 2:1 scale!

    // 20 inst. clocks per bit: HHHHxxxxxxLLLLLLLLLL
    // ST instructions:         ^   ^     ^          (T=0,4,10)

    volatile uint8_t next, bit;

    hi   = *port |  pinMask;
    lo   = *port & ~pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head20:"                  "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
       "mov  %[next], %[hi]"    "\n\t" // 0-1   next = hi    (T =  4)
      "st   %a[port], %[next]"  "\n\t" // 2    PORT = next   (T =  6)
      "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo     (T =  7)
      "dec  %[bit]"             "\n\t" // 1    bit--         (T =  8)
      "breq nextbyte20"         "\n\t" // 1-2  if(bit == 0)
      "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 10)
      "st   %a[port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 12)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 16)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 18)
      "rjmp head20"             "\n\t" // 2    -> head20 (next bit out)
     "nextbyte20:"              "\n\t" //                    (T = 10)
      "st   %a[port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 12)
      "nop"                     "\n\t" // 1    nop           (T = 13)
      "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 14)
      "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 16)
      "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 18)
      "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
      : [port]  "+e" (port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [hi]    "r" (hi),
        [lo]    "r" (lo),
        [ptr]   "e" (ptr));
  }
#endif // NEO_KHZ400

// 12 MHz(ish) AVR --------------------------------------------------------
#elif (F_CPU >= 11100000UL) && (F_CPU <= 14300000UL)

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif

    // In the 12 MHz case, an optimized 800 KHz datastream (no dead time
    // between bytes) requires a PORT-specific loop similar to the 8 MHz
    // code (but a little more relaxed in this case).

    // 15 instruction clocks per bit: HHHHxxxxxxLLLLL
    // OUT instructions:              ^   ^     ^     (T=0,4,10)

    volatile uint8_t next;

    // PORTD OUTPUT ----------------------------------------------------

#if defined(PORTD)
 #if defined(PORTB) || defined(PORTC) || defined(PORTF)
    if(port == &PORTD) {
 #endif // defined(PORTB/C/F)

      hi   = PORTD |  pinMask;
      lo   = PORTD & ~pinMask;
      next = lo;
      if(b & 0x80) next = hi;

      // Don't "optimize" the OUT calls into the bitTime subroutine;
      // we're exploiting the RCALL and RET as 3- and 4-cycle NOPs!
      asm volatile(
       "headD:"                   "\n\t" //        (T =  0)
        "out   %[port], %[hi]"    "\n\t" //        (T =  1)
        "rcall bitTimeD"          "\n\t" // Bit 7  (T = 15)
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 6
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 5
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 4
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 3
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 2
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeD"          "\n\t" // Bit 1
        // Bit 0:
        "out  %[port] , %[hi]"    "\n\t" // 1    PORT = hi    (T =  1)
        "rjmp .+0"                "\n\t" // 2    nop nop      (T =  3)
        "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++   (T =  5)
        "out  %[port] , %[next]"  "\n\t" // 1    PORT = next  (T =  6)
        "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo    (T =  7)
        "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 0x80) (T =  8)
         "mov %[next] , %[hi]"    "\n\t" // 0-1    next = hi  (T =  9)
        "nop"                     "\n\t" // 1                 (T = 10)
        "out  %[port] , %[lo]"    "\n\t" // 1    PORT = lo    (T = 11)
        "sbiw %[count], 1"        "\n\t" // 2    i--          (T = 13)
        "brne headD"              "\n\t" // 2    if(i != 0) -> (next byte)
         "rjmp doneD"             "\n\t"
        "bitTimeD:"               "\n\t" //      nop nop nop     (T =  4)
         "out  %[port], %[next]"  "\n\t" // 1    PORT = next     (T =  5)
         "mov  %[next], %[lo]"    "\n\t" // 1    next = lo       (T =  6)
         "rol  %[byte]"           "\n\t" // 1    b <<= 1         (T =  7)
         "sbrc %[byte], 7"        "\n\t" // 1-2  if(b & 0x80)    (T =  8)
          "mov %[next], %[hi]"    "\n\t" // 0-1   next = hi      (T =  9)
         "nop"                    "\n\t" // 1                    (T = 10)
         "out  %[port], %[lo]"    "\n\t" // 1    PORT = lo       (T = 11)
         "ret"                    "\n\t" // 4    nop nop nop nop (T = 15)
         "doneD:"                 "\n"
        : [byte]  "+r" (b),
          [next]  "+r" (next),
          [count] "+w" (i)
        : [port]   "I" (_SFR_IO_ADDR(PORTD)),
          [ptr]    "e" (ptr),
          [hi]     "r" (hi),
          [lo]     "r" (lo));

 #if defined(PORTB) || defined(PORTC) || defined(PORTF)
    } else // other PORT(s)
 #endif // defined(PORTB/C/F)
#endif // defined(PORTD)

    // PORTB OUTPUT ----------------------------------------------------

#if defined(PORTB)
 #if defined(PORTD) || defined(PORTC) || defined(PORTF)
    if(port == &PORTB) {
 #endif // defined(PORTD/C/F)

      hi   = PORTB |  pinMask;
      lo   = PORTB & ~pinMask;
      next = lo;
      if(b & 0x80) next = hi;

      // Same as above, just set for PORTB & stripped of comments
      asm volatile(
       "headB:"                   "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeB"          "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "out  %[port] , %[next]"  "\n\t"
        "mov  %[next] , %[lo]"    "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[next] , %[hi]"    "\n\t"
        "nop"                     "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "brne headB"              "\n\t"
         "rjmp doneB"             "\n\t"
        "bitTimeB:"               "\n\t"
         "out  %[port], %[next]"  "\n\t"
         "mov  %[next], %[lo]"    "\n\t"
         "rol  %[byte]"           "\n\t"
         "sbrc %[byte], 7"        "\n\t"
          "mov %[next], %[hi]"    "\n\t"
         "nop"                    "\n\t"
         "out  %[port], %[lo]"    "\n\t"
         "ret"                    "\n\t"
         "doneB:"                 "\n"
        : [byte] "+r" (b), [next] "+r" (next), [count] "+w" (i)
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [ptr] "e" (ptr), [hi] "r" (hi),
          [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTC) || defined(PORTF)
    }
 #endif
 #if defined(PORTC) || defined(PORTF)
    else
 #endif // defined(PORTC/F)
#endif // defined(PORTB)

    // PORTC OUTPUT ----------------------------------------------------

#if defined(PORTC)
 #if defined(PORTD) || defined(PORTB) || defined(PORTF)
    if(port == &PORTC) {
 #endif // defined(PORTD/B/F)

      hi   = PORTC |  pinMask;
      lo   = PORTC & ~pinMask;
      next = lo;
      if(b & 0x80) next = hi;

      // Same as above, just set for PORTC & stripped of comments
      asm volatile(
       "headC:"                   "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "out  %[port] , %[next]"  "\n\t"
        "mov  %[next] , %[lo]"    "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[next] , %[hi]"    "\n\t"
        "nop"                     "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "brne headC"              "\n\t"
         "rjmp doneC"             "\n\t"
        "bitTimeC:"               "\n\t"
         "out  %[port], %[next]"  "\n\t"
         "mov  %[next], %[lo]"    "\n\t"
         "rol  %[byte]"           "\n\t"
         "sbrc %[byte], 7"        "\n\t"
          "mov %[next], %[hi]"    "\n\t"
         "nop"                    "\n\t"
         "out  %[port], %[lo]"    "\n\t"
         "ret"                    "\n\t"
         "doneC:"                 "\n"
        : [byte] "+r" (b), [next] "+r" (next), [count] "+w" (i)
        : [port] "I" (_SFR_IO_ADDR(PORTC)), [ptr] "e" (ptr), [hi] "r" (hi),
          [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTB) || defined(PORTF)
    }
 #endif // defined(PORTD/B/F)
 #if defined(PORTF)
    else
 #endif
#endif // defined(PORTC)

    // PORTF OUTPUT ----------------------------------------------------

#if defined(PORTF)
 #if defined(PORTD) || defined(PORTB) || defined(PORTC)
    if(port == &PORTF) {
 #endif // defined(PORTD/B/C)

      hi   = PORTF |  pinMask;
      lo   = PORTF & ~pinMask;
      next = lo;
      if(b & 0x80) next = hi;

      // Same as above, just set for PORTF & stripped of comments
      asm volatile(
       "headF:"                   "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out   %[port], %[hi]"    "\n\t"
        "rcall bitTimeC"          "\n\t"
        "out  %[port] , %[hi]"    "\n\t"
        "rjmp .+0"                "\n\t"
        "ld   %[byte] , %a[ptr]+" "\n\t"
        "out  %[port] , %[next]"  "\n\t"
        "mov  %[next] , %[lo]"    "\n\t"
        "sbrc %[byte] , 7"        "\n\t"
         "mov %[next] , %[hi]"    "\n\t"
        "nop"                     "\n\t"
        "out  %[port] , %[lo]"    "\n\t"
        "sbiw %[count], 1"        "\n\t"
        "brne headF"              "\n\t"
         "rjmp doneC"             "\n\t"
        "bitTimeC:"               "\n\t"
         "out  %[port], %[next]"  "\n\t"
         "mov  %[next], %[lo]"    "\n\t"
         "rol  %[byte]"           "\n\t"
         "sbrc %[byte], 7"        "\n\t"
          "mov %[next], %[hi]"    "\n\t"
         "nop"                    "\n\t"
         "out  %[port], %[lo]"    "\n\t"
         "ret"                    "\n\t"
         "doneC:"                 "\n"
        : [byte] "+r" (b), [next] "+r" (next), [count] "+w" (i)
        : [port] "I" (_SFR_IO_ADDR(PORTF)), [ptr] "e" (ptr), [hi] "r" (hi),
          [lo] "r" (lo));

 #if defined(PORTD) || defined(PORTB) || defined(PORTC)
    }
 #endif // defined(PORTD/B/C)
#endif // defined(PORTF)

#ifdef NEO_KHZ400
  } else { // 400 KHz

    // 30 instruction clocks per bit: HHHHHHxxxxxxxxxLLLLLLLLLLLLLLL
    // ST instructions:               ^     ^        ^    (T=0,6,15)

    volatile uint8_t next, bit;

    hi   = *port |  pinMask;
    lo   = *port & ~pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head30:"                  "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
       "mov  %[next], %[hi]"    "\n\t" // 0-1   next = hi    (T =  4)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T =  6)
      "st   %a[port], %[next]"  "\n\t" // 2    PORT = next   (T =  8)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 10)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 12)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
      "nop"                     "\n\t" // 1    nop           (T = 15)
      "st   %a[port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 17)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 19)
      "dec  %[bit]"             "\n\t" // 1    bit--         (T = 20)
      "breq nextbyte30"         "\n\t" // 1-2  if(bit == 0)
      "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 22)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 24)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 26)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 28)
      "rjmp head30"             "\n\t" // 2    -> head30 (next bit out)
     "nextbyte30:"              "\n\t" //                    (T = 22)
      "nop"                     "\n\t" // 1    nop           (T = 23)
      "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 24)
      "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 26)
      "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 28)
      "brne head30"             "\n"   // 1-2  if(i != 0) -> (next byte)
      : [port]  "+e" (port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [hi]     "r" (hi),
        [lo]     "r" (lo),
        [ptr]    "e" (ptr));
  }
#endif // NEO_KHZ400

// 16 MHz(ish) AVR --------------------------------------------------------
#elif (F_CPU >= 15400000UL) && (F_CPU <= 19000000L)

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif

    // WS2811 and WS2812 have different hi/lo duty cycles; this is
    // similar but NOT an exact copy of the prior 400-on-8 code.

    // 20 inst. clocks per bit: HHHHHxxxxxxxxLLLLLLL
    // ST instructions:         ^   ^        ^       (T=0,5,13)

    volatile uint8_t next, bit;

    hi   = *port |  pinMask;
    lo   = *port & ~pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head20:"                   "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
       "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
      "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
      "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
      "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
      "breq nextbyte20"          "\n\t" // 1-2  if(bit == 0) (from dec above)
      "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
      "nop"                      "\n\t" // 1    nop           (T = 13)
      "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
      "nop"                      "\n\t" // 1    nop           (T = 16)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
      "rjmp head20"              "\n\t" // 2    -> head20 (next bit out)
     "nextbyte20:"               "\n\t" //                    (T = 10)
      "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
      "ld   %[byte] ,  %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 13)
      "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
      "nop"                      "\n\t" // 1    nop           (T = 16)
      "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
       "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
      : [port]  "+e" (port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [ptr]    "e" (ptr),
        [hi]     "r" (hi),
        [lo]     "r" (lo));

#ifdef NEO_KHZ400
  } else { // 400 KHz

    // The 400 KHz clock on 16 MHz MCU is the most 'relaxed' version.

    // 40 inst. clocks per bit: HHHHHHHHxxxxxxxxxxxxLLLLLLLLLLLLLLLLLLLL
    // ST instructions:         ^       ^           ^         (T=0,8,20)

    volatile uint8_t next, bit;

    hi   = *port |  pinMask;
    lo   = *port & ~pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head40:"                  "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
       "mov  %[next] , %[hi]"   "\n\t" // 0-1   next = hi    (T =  4)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T =  6)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T =  8)
      "st   %a[port], %[next]"  "\n\t" // 2    PORT = next   (T = 10)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 12)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 16)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 18)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 20)
      "st   %a[port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 22)
      "nop"                     "\n\t" // 1    nop           (T = 23)
      "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo     (T = 24)
      "dec  %[bit]"             "\n\t" // 1    bit--         (T = 25)
      "breq nextbyte40"         "\n\t" // 1-2  if(bit == 0)
      "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 27)
      "nop"                     "\n\t" // 1    nop           (T = 28)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 30)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 32)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 34)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 36)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 38)
      "rjmp head40"             "\n\t" // 2    -> head40 (next bit out)
     "nextbyte40:"              "\n\t" //                    (T = 27)
      "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 28)
      "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 30)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 32)
      "st   %a[port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 34)
      "rjmp .+0"                "\n\t" // 2    nop nop       (T = 36)
      "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 38)
      "brne head40"             "\n"   // 1-2  if(i != 0) -> (next byte)
      : [port]  "+e" (port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [ptr]    "e" (ptr),
        [hi]     "r" (hi),
        [lo]     "r" (lo));
  }
#endif // NEO_KHZ400

#else
 #error "CPU SPEED NOT SUPPORTED"
#endif // end F_CPU ifdefs on __AVR__

// END AVR ----------------------------------------------------------------


#elif defined(__arm__)

// ARM MCUs -- Teensy 3.0, 3.1, LC, Arduino Due ---------------------------

#if defined(TEENSYDUINO) && defined(KINETISK) // Teensy 3.0, 3.1, 3.2, 3.5, 3.6
#define CYCLES_800_T0H  (F_CPU / 4000000)
#define CYCLES_800_T1H  (F_CPU / 1250000)
#define CYCLES_800      (F_CPU /  800000)
#define CYCLES_400_T0H  (F_CPU / 2000000)
#define CYCLES_400_T1H  (F_CPU /  833333)
#define CYCLES_400      (F_CPU /  400000)

  uint8_t          *p   = pixels,
                   *end = p + numBytes, pix, mask;
  volatile uint8_t *set = portSetRegister(pin),
                   *clr = portClearRegister(pin);
  uint32_t          cyc;

  ARM_DEMCR    |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif
    cyc = ARM_DWT_CYCCNT + CYCLES_800;
    while(p < end) {
      pix = *p++;
      for(mask = 0x80; mask; mask >>= 1) {
        while(ARM_DWT_CYCCNT - cyc < CYCLES_800);
        cyc  = ARM_DWT_CYCCNT;
        *set = 1;
        if(pix & mask) {
          while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T1H);
        } else {
          while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H);
        }
        *clr = 1;
      }
    }
    while(ARM_DWT_CYCCNT - cyc < CYCLES_800);
#ifdef NEO_KHZ400
  } else { // 400 kHz bitstream
    cyc = ARM_DWT_CYCCNT + CYCLES_400;
    while(p < end) {
      pix = *p++;
      for(mask = 0x80; mask; mask >>= 1) {
        while(ARM_DWT_CYCCNT - cyc < CYCLES_400);
        cyc  = ARM_DWT_CYCCNT;
        *set = 1;
        if(pix & mask) {
          while(ARM_DWT_CYCCNT - cyc < CYCLES_400_T1H);
        } else {
          while(ARM_DWT_CYCCNT - cyc < CYCLES_400_T0H);
        }
        *clr = 1;
      }
    }
    while(ARM_DWT_CYCCNT - cyc < CYCLES_400);
  }
#endif // NEO_KHZ400

#elif defined(TEENSYDUINO) && defined(__MKL26Z64__) // Teensy-LC

#if F_CPU == 48000000
  uint8_t          *p   = pixels,
		   pix, count, dly,
                   bitmask = digitalPinToBitMask(pin);
  volatile uint8_t *reg = portSetRegister(pin);
  uint32_t         num = numBytes;
  asm volatile(
	"L%=_begin:"				"\n\t"
	"ldrb	%[pix], [%[p], #0]"		"\n\t"
	"lsl	%[pix], #24"			"\n\t"
	"movs	%[count], #7"			"\n\t"
	"L%=_loop:"				"\n\t"
	"lsl	%[pix], #1"			"\n\t"
	"bcs	L%=_loop_one"			"\n\t"
	"L%=_loop_zero:"
	"strb	%[bitmask], [%[reg], #0]"	"\n\t"
	"movs	%[dly], #4"			"\n\t"
	"L%=_loop_delay_T0H:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_loop_delay_T0H"		"\n\t"
	"strb	%[bitmask], [%[reg], #4]"	"\n\t"
	"movs	%[dly], #13"			"\n\t"
	"L%=_loop_delay_T0L:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_loop_delay_T0L"		"\n\t"
	"b	L%=_next"			"\n\t"
	"L%=_loop_one:"
	"strb	%[bitmask], [%[reg], #0]"	"\n\t"
	"movs	%[dly], #13"			"\n\t"
	"L%=_loop_delay_T1H:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_loop_delay_T1H"		"\n\t"
	"strb	%[bitmask], [%[reg], #4]"	"\n\t"
	"movs	%[dly], #4"			"\n\t"
	"L%=_loop_delay_T1L:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_loop_delay_T1L"		"\n\t"
	"nop"					"\n\t"
	"L%=_next:"				"\n\t"
	"sub	%[count], #1"			"\n\t"
	"bne	L%=_loop"			"\n\t"
	"lsl	%[pix], #1"			"\n\t"
	"bcs	L%=_last_one"			"\n\t"
	"L%=_last_zero:"
	"strb	%[bitmask], [%[reg], #0]"	"\n\t"
	"movs	%[dly], #4"			"\n\t"
	"L%=_last_delay_T0H:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_last_delay_T0H"		"\n\t"
	"strb	%[bitmask], [%[reg], #4]"	"\n\t"
	"movs	%[dly], #10"			"\n\t"
	"L%=_last_delay_T0L:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_last_delay_T0L"		"\n\t"
	"b	L%=_repeat"			"\n\t"
	"L%=_last_one:"
	"strb	%[bitmask], [%[reg], #0]"	"\n\t"
	"movs	%[dly], #13"			"\n\t"
	"L%=_last_delay_T1H:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_last_delay_T1H"		"\n\t"
	"strb	%[bitmask], [%[reg], #4]"	"\n\t"
	"movs	%[dly], #1"			"\n\t"
	"L%=_last_delay_T1L:"			"\n\t"
	"sub	%[dly], #1"			"\n\t"
	"bne	L%=_last_delay_T1L"		"\n\t"
	"nop"					"\n\t"
	"L%=_repeat:"				"\n\t"
	"add	%[p], #1"			"\n\t"
	"sub	%[num], #1"			"\n\t"
	"bne	L%=_begin"			"\n\t"
	"L%=_done:"				"\n\t"
	: [p] "+r" (p),
	  [pix] "=&r" (pix),
	  [count] "=&r" (count),
	  [dly] "=&r" (dly),
	  [num] "+r" (num)
	: [bitmask] "r" (bitmask),
	  [reg] "r" (reg)
  );
#else
#error "Sorry, only 48 MHz is supported, please set Tools > CPU Speed to 48 MHz"
#endif // F_CPU == 48000000

// Begin of support for NRF52832 based boards  -------------------------

#elif defined(NRF52)
// [[[Begin of the Neopixel NRF52 EasyDMA implementation
//                                    by the Hackerspace San Salvador]]]
// This technique uses the PWM peripheral on the NRF52. The PWM uses the
// EasyDMA feature included on the chip. This technique loads the duty 
// cycle configuration for each cycle when the PWM is enabled. For this 
// to work we need to store a 16 bit configuration for each bit of the
// RGB(W) values in the pixel buffer.
// Comparator values for the PWM were hand picked and are guaranteed to
// be 100% organic to preserve freshness and high accuracy. Current 
// parameters are:
//   * PWM Clock: 16Mhz
//   * Minimum step time: 62.5ns
//   * Time for zero in high (T0H): 0.31ms
//   * Time for one in high (T1H): 0.75ms
//   * Cycle time:  1.25us
//   * Frequency: 800Khz
// For 400Khz we just double the calculated times.
// ---------- BEGIN Constants for the EasyDMA implementation -----------
// The PWM starts the duty cycle in LOW. To start with HIGH we
// need to set the 15th bit on each register.

// WS2812 (rev A) timing is 0.35 and 0.7us
//#define MAGIC_T0H               5UL | (0x8000) // 0.3125us
//#define MAGIC_T1H              12UL | (0x8000) // 0.75us

// WS2812B (rev B) timing is 0.4 and 0.8 us
#define MAGIC_T0H               6UL | (0x8000) // 0.375us
#define MAGIC_T1H              13UL | (0x8000) // 0.8125us

// WS2811 (400 khz) timing is 0.5 and 1.2
#define MAGIC_T0H_400KHz        8UL  | (0x8000) // 0.5us
#define MAGIC_T1H_400KHz        19UL | (0x8000) // 1.1875us

// For 400Khz, we double value of CTOPVAL
#define CTOPVAL                20UL            // 1.25us
#define CTOPVAL_400KHz         40UL            // 2.5us

// ---------- END Constants for the EasyDMA implementation -------------
// 
// If there is no device available an alternative cycle-counter
// implementation is tried.
// The nRF52832 runs with a fixed clock of 64Mhz. The alternative
// implementation is the same as the one used for the Teensy 3.0/1/2 but
// with the Nordic SDK HAL & registers syntax.
// The number of cycles was hand picked and is guaranteed to be 100% 
// organic to preserve freshness and high accuracy.
// ---------- BEGIN Constants for cycle counter implementation ---------
#define CYCLES_800_T0H  18  // ~0.36 uS
#define CYCLES_800_T1H  41  // ~0.76 uS
#define CYCLES_800      71  // ~1.25 uS

#define CYCLES_400_T0H  26  // ~0.50 uS
#define CYCLES_400_T1H  70  // ~1.26 uS
#define CYCLES_400      156 // ~2.50 uS
// ---------- END of Constants for cycle counter implementation --------

  // To support both the SoftDevice + Neopixels we use the EasyDMA
  // feature from the NRF25. However this technique implies to
  // generate a pattern and store it on the memory. The actual
  // memory used in bytes corresponds to the following formula:
  //              totalMem = numBytes*8*2+(2*2)
  // The two additional bytes at the end are needed to reset the
  // sequence.
  //
  // If there is not enough memory, we will fall back to cycle counter
  // using DWT
  uint32_t  pattern_size   = numBytes*8*sizeof(uint16_t)+2*sizeof(uint16_t);
  uint16_t* pixels_pattern = NULL;

  NRF_PWM_Type* pwm = NULL;

  // Try to find a free PWM device, which is not enabled
  // and has no connected pins
  NRF_PWM_Type* PWM[3] = {NRF_PWM0, NRF_PWM1, NRF_PWM2};
  for(int device = 0; device<3; device++) {
    if( (PWM[device]->ENABLE == 0)                            &&
        (PWM[device]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk)
    ) {
      pwm = PWM[device];
      break;
    }
  }
  
  // only malloc if there is PWM device available
  if ( pwm != NULL ) {
    #ifdef ARDUINO_FEATHER52 // use thread-safe malloc
      pixels_pattern = (uint16_t *) rtos_malloc(pattern_size);
    #else
      pixels_pattern = (uint16_t *) malloc(pattern_size);
    #endif
  }

  // Use the identified device to choose the implementation
  // If a PWM device is available use DMA
  if( (pixels_pattern != NULL) && (pwm != NULL) ) {
    uint16_t pos = 0; // bit position

    for(uint16_t n=0; n<numBytes; n++) {
      uint8_t pix = pixels[n];

      for(uint8_t mask=0x80, i=0; mask>0; mask >>= 1, i++) {
        #ifdef NEO_KHZ400
        if( !is800KHz ) {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H_400KHz : MAGIC_T0H_400KHz;
        }else
        #endif
        {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
        }

        pos++;
      }
    }

    // Zero padding to indicate the end of que sequence
    pixels_pattern[++pos] = 0 | (0x8000); // Seq end
    pixels_pattern[++pos] = 0 | (0x8000); // Seq end

    // Set the wave mode to count UP
    pwm->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);

    // Set the PWM to use the 16MHz clock
    pwm->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);

    // Setting of the maximum count
    // but keeping it on 16Mhz allows for more granularity just
    // in case someone wants to do more fine-tuning of the timing.
#ifdef NEO_KHZ400
    if( !is800KHz ) {
      pwm->COUNTERTOP = (CTOPVAL_400KHz << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }else
#endif
    {
      pwm->COUNTERTOP = (CTOPVAL << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }

    // Disable loops, we want the sequence to repeat only once
    pwm->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);

    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    pwm->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) |
                   (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    // Pointer to the memory storing the patter
    pwm->SEQ[0].PTR = (uint32_t)(pixels_pattern) << PWM_SEQ_PTR_PTR_Pos;

    // Calculation of the number of steps loaded from memory.
    pwm->SEQ[0].CNT = (pattern_size/sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos;

    // The following settings are ignored with the current config.
    pwm->SEQ[0].REFRESH  = 0;
    pwm->SEQ[0].ENDDELAY = 0;

    // The Neopixel implementation is a blocking algorithm. DMA
    // allows for non-blocking operation. To "simulate" a blocking
    // operation we enable the interruption for the end of sequence
    // and block the execution thread until the event flag is set by
    // the peripheral.
//    pwm->INTEN |= (PWM_INTEN_SEQEND0_Enabled<<PWM_INTEN_SEQEND0_Pos);

    // PSEL must be configured before enabling PWM
    pwm->PSEL.OUT[0] = g_ADigitalPinMap[pin];

    // Enable the PWM
    pwm->ENABLE = 1;

    // After all of this and many hours of reading the documentation
    // we are ready to start the sequence...
    pwm->EVENTS_SEQEND[0]  = 0;
    pwm->TASKS_SEQSTART[0] = 1;

    // But we have to wait for the flag to be set.
    while(!pwm->EVENTS_SEQEND[0])
    {
      #ifdef ARDUINO_FEATHER52
      yield();
      #endif
    }

    // Before leave we clear the flag for the event.
    pwm->EVENTS_SEQEND[0] = 0;

    // We need to disable the device and disconnect
    // all the outputs before leave or the device will not
    // be selected on the next call.
    // TODO: Check if disabling the device causes performance issues.
    pwm->ENABLE = 0;

    pwm->PSEL.OUT[0] = 0xFFFFFFFFUL;

    #ifdef ARDUINO_FEATHER52  // use thread-safe free
      rtos_free(pixels_pattern);
    #else
      free(pixels_pattern);
    #endif
  }// End of DMA implementation
  // ---------------------------------------------------------------------
  else{
    // Fall back to DWT
    #ifdef ARDUINO_FEATHER52
      // Bluefruit Feather 52 uses freeRTOS
      // Critical Section is used since it does not block SoftDevice execution
      taskENTER_CRITICAL();
    #elif defined(NRF52_DISABLE_INT)
      // If you are using the Bluetooth SoftDevice we advise you to not disable
      // the interrupts. Disabling the interrupts even for short periods of time
      // causes the SoftDevice to stop working.
      // Disable the interrupts only in cases where you need high performance for
      // the LEDs and if you are not using the EasyDMA feature.
      __disable_irq();
    #endif

    uint32_t pinMask = 1UL << g_ADigitalPinMap[pin];

    uint32_t CYCLES_X00     = CYCLES_800;
    uint32_t CYCLES_X00_T1H = CYCLES_800_T1H;
    uint32_t CYCLES_X00_T0H = CYCLES_800_T0H;

#ifdef NEO_KHZ400
    if( !is800KHz )
    {
      CYCLES_X00     = CYCLES_400;
      CYCLES_X00_T1H = CYCLES_400_T1H;
      CYCLES_X00_T0H = CYCLES_400_T0H;
    }
#endif

    // Enable DWT in debug core
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Tries to re-send the frame if is interrupted by the SoftDevice.
    while(1) {
      uint8_t *p = pixels;

      uint32_t cycStart = DWT->CYCCNT;
      uint32_t cyc = 0;

      for(uint16_t n=0; n<numBytes; n++) {
        uint8_t pix = *p++;

        for(uint8_t mask = 0x80; mask; mask >>= 1) {
          while(DWT->CYCCNT - cyc < CYCLES_X00);
          cyc  = DWT->CYCCNT;

          NRF_GPIO->OUTSET |= pinMask;

          if(pix & mask) {
            while(DWT->CYCCNT - cyc < CYCLES_X00_T1H);
          } else {
            while(DWT->CYCCNT - cyc < CYCLES_X00_T0H);
          }

          NRF_GPIO->OUTCLR |= pinMask;
        }
      }
      while(DWT->CYCCNT - cyc < CYCLES_X00);


      // If total time longer than 25%, resend the whole data.
      // Since we are likely to be interrupted by SoftDevice
      if ( (DWT->CYCCNT - cycStart) < ( 8*numBytes*((CYCLES_X00*5)/4) ) ) {
        break;
      }

      // re-send need 300us delay
      delayMicroseconds(300);
    }

    // Enable interrupts again
    #ifdef ARDUINO_FEATHER52
      taskEXIT_CRITICAL();
    #elif defined(NRF52_DISABLE_INT)
      __enable_irq();
    #endif
  }
// END of NRF52 implementation

#elif defined (__SAMD21E17A__) || defined(__SAMD21G18A__)  || defined(__SAMD21E18A__) || defined(__SAMD21J18A__) // Arduino Zero, Gemma/Trinket M0, SODAQ Autonomo and others
  // Tried this with a timer/counter, couldn't quite get adequate
  // resolution.  So yay, you get a load of goofball NOPs...

  uint8_t  *ptr, *end, p, bitMask, portNum;
  uint32_t  pinMask;

  portNum =  g_APinDescription[pin].ulPort;
  pinMask =  1ul << g_APinDescription[pin].ulPin;
  ptr     =  pixels;
  end     =  ptr + numBytes;
  p       = *ptr++;
  bitMask =  0x80;

  volatile uint32_t *set = &(PORT->Group[portNum].OUTSET.reg),
                    *clr = &(PORT->Group[portNum].OUTCLR.reg);

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif
    for(;;) {
      *set = pinMask;
      asm("nop; nop; nop; nop; nop; nop; nop; nop;");
      if(p & bitMask) {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop;");
        *clr = pinMask;
      } else {
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop;");
      }
      if(bitMask >>= 1) {
        asm("nop; nop; nop; nop; nop; nop; nop; nop; nop;");
      } else {
        if(ptr >= end) break;
        p       = *ptr++;
        bitMask = 0x80;
      }
    }
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    for(;;) {
      *set = pinMask;
      asm("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
      if(p & bitMask) {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop;");
        *clr = pinMask;
      } else {
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop;");
      }
      asm("nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;");
      if(bitMask >>= 1) {
        asm("nop; nop; nop; nop; nop; nop; nop;");
      } else {
        if(ptr >= end) break;
        p       = *ptr++;
        bitMask = 0x80;
      }
    }
  }
#endif

#elif defined (__SAMD51__) // M4 @ 120mhz
  // Tried this with a timer/counter, couldn't quite get adequate
  // resolution.  So yay, you get a load of goofball NOPs...

  uint8_t  *ptr, *end, p, bitMask, portNum;
  uint32_t  pinMask;

  portNum =  g_APinDescription[pin].ulPort;
  pinMask =  1ul << g_APinDescription[pin].ulPin;
  ptr     =  pixels;
  end     =  ptr + numBytes;
  p       = *ptr++;
  bitMask =  0x80;

  volatile uint32_t *set = &(PORT->Group[portNum].OUTSET.reg),
                    *clr = &(PORT->Group[portNum].OUTCLR.reg);

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif
    for(;;) {
      if(p & bitMask) { // ONE
        // High 800ns
        *set = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
        // Low 450ns
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop;");
      } else { // ZERO
        // High 400ns
        *set = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop;");
        // Low 850ns
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
      }
      if(bitMask >>= 1) {
        // Move on to the next pixel
        asm("nop;");
      } else {
        if(ptr >= end) break;
        p       = *ptr++;
        bitMask = 0x80;
      }
    }
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    // ToDo!
  }
#endif

#elif defined (ARDUINO_STM32_FEATHER) // FEATHER WICED (120MHz)

  // Tried this with a timer/counter, couldn't quite get adequate
  // resolution.  So yay, you get a load of goofball NOPs...

  uint8_t  *ptr, *end, p, bitMask;
  uint32_t  pinMask;

  pinMask =  BIT(PIN_MAP[pin].gpio_bit);
  ptr     =  pixels;
  end     =  ptr + numBytes;
  p       = *ptr++;
  bitMask =  0x80;

  volatile uint16_t *set = &(PIN_MAP[pin].gpio_device->regs->BSRRL);
  volatile uint16_t *clr = &(PIN_MAP[pin].gpio_device->regs->BSRRH);

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif
    for(;;) {
      if(p & bitMask) { // ONE
        // High 800ns
        *set = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop;");
        // Low 450ns
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop;");
      } else { // ZERO
        // High 400ns
        *set = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop;");
        // Low 850ns
        *clr = pinMask;
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop;");
      }
      if(bitMask >>= 1) {
        // Move on to the next pixel
        asm("nop;");
      } else {
        if(ptr >= end) break;
        p       = *ptr++;
        bitMask = 0x80;
      }
    }
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    // ToDo!
  }
#endif

#elif defined (NRF51)
  uint8_t          *p   = pixels,
                    pix, count, mask;
  int32_t         num = numBytes;
  unsigned int bitmask = ( 1 << g_ADigitalPinMap[pin] );
// https://github.com/sandeepmistry/arduino-nRF5/blob/dc53980c8bac27898fca90d8ecb268e11111edc1/variants/BBCmicrobit/variant.cpp

  volatile unsigned int *reg = (unsigned int *) (0x50000000UL + 0x508);

// https://github.com/sandeepmistry/arduino-nRF5/blob/dc53980c8bac27898fca90d8ecb268e11111edc1/cores/nRF5/SDK/components/device/nrf51.h
// http://www.iot-programmer.com/index.php/books/27-micro-bit-iot-in-c/chapters-micro-bit-iot-in-c/47-micro-bit-iot-in-c-fast-memory-mapped-gpio?showall=1
// https://github.com/Microsoft/pxt-neopixel/blob/master/sendbuffer.asm

  asm volatile(
    // "cpsid i" ; disable irq

    //    b .start
    "b  L%=_start"                    "\n\t"
    // .nextbit:               ;            C0
    "L%=_nextbit:"                    "\n\t"          //;            C0
    //    str r1, [r3, #0]    ; pin := hi  C2
    "strb %[bitmask], [%[reg], #0]"   "\n\t"          //; pin := hi  C2
    //    tst r6, r0          ;            C3
    "tst %[mask], %[pix]"             "\n\t"//          ;            C3
    //    bne .islate         ;            C4
    "bne L%=_islate"                  "\n\t"          //;            C4
    //    str r1, [r2, #0]    ; pin := lo  C6
    "strb %[bitmask], [%[reg], #4]"   "\n\t"          //; pin := lo  C6
    // .islate:
    "L%=_islate:"                     "\n\t"
    //    lsrs r6, r6, #1     ; r6 >>= 1   C7
    "lsr %[mask], %[mask], #1"       "\n\t"          //; r6 >>= 1   C7
    //    bne .justbit        ;            C8
    "bne L%=_justbit"                 "\n\t"          //;            C8

    //    ; not just a bit - need new byte
    //    adds r4, #1         ; r4++       C9
    "add %[p], #1"                   "\n\t"          //; r4++       C9
    //    subs r5, #1         ; r5--       C10
    "sub %[num], #1"                 "\n\t"          //; r5--       C10
    //    bcc .stop           ; if (r5<0) goto .stop  C11
    "bcc L%=_stop"                    "\n\t"          //; if (r5<0) goto .stop  C11
    // .start:
    "L%=_start:"
    //    movs r6, #0x80      ; reset mask C12
    "movs %[mask], #0x80"             "\n\t"          //; reset mask C12
    //    nop                 ;            C13
    "nop"                             "\n\t"          //;            C13

    // .common:               ;             C13
    "L%=_common:"                     "\n\t"          //;            C13
    //    str r1, [r2, #0]   ; pin := lo   C15
    "strb %[bitmask], [%[reg], #4]"   "\n\t"          //; pin := lo  C15
    //    ; always re-load byte - it just fits with the cycles better this way
    //    ldrb r0, [r4, #0]  ; r0 := *r4   C17
    "ldrb  %[pix], [%[p], #0]"        "\n\t"          //; r0 := *r4   C17
    //    b .nextbit         ;             C20
    "b L%=_nextbit"                   "\n\t"          //;             C20

    // .justbit: ; C10
    "L%=_justbit:"                    "\n\t"          //; C10
    //    ; no nops, branch taken is already 3 cycles
    //    b .common ; C13
    "b L%=_common"                    "\n\t"          //; C13

    // .stop:
    "L%=_stop:"                       "\n\t"
    //    str r1, [r2, #0]   ; pin := lo
    "strb %[bitmask], [%[reg], #4]"   "\n\t"          //; pin := lo
    //    cpsie i            ; enable irq

    : [p] "+r" (p),
    [pix] "=&r" (pix),
    [count] "=&r" (count),
    [mask] "=&r" (mask),
    [num] "+r" (num)
    : [bitmask] "r" (bitmask),
    [reg] "r" (reg)
  );

#elif defined(__SAM3X8E__) // Arduino Due

  #define SCALE      VARIANT_MCK / 2UL / 1000000UL
  #define INST       (2UL * F_CPU / VARIANT_MCK)
  #define TIME_800_0 ((int)(0.40 * SCALE + 0.5) - (5 * INST))
  #define TIME_800_1 ((int)(0.80 * SCALE + 0.5) - (5 * INST))
  #define PERIOD_800 ((int)(1.25 * SCALE + 0.5) - (5 * INST))
  #define TIME_400_0 ((int)(0.50 * SCALE + 0.5) - (5 * INST))
  #define TIME_400_1 ((int)(1.20 * SCALE + 0.5) - (5 * INST))
  #define PERIOD_400 ((int)(2.50 * SCALE + 0.5) - (5 * INST))

  int             pinMask, time0, time1, period, t;
  Pio            *port;
  volatile WoReg *portSet, *portClear, *timeValue, *timeReset;
  uint8_t        *p, *end, pix, mask;

  pmc_set_writeprotect(false);
  pmc_enable_periph_clk((uint32_t)TC3_IRQn);
  TC_Configure(TC1, 0,
    TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK1);
  TC_Start(TC1, 0);

  pinMask   = g_APinDescription[pin].ulPin; // Don't 'optimize' these into
  port      = g_APinDescription[pin].pPort; // declarations above.  Want to
  portSet   = &(port->PIO_SODR);            // burn a few cycles after
  portClear = &(port->PIO_CODR);            // starting timer to minimize
  timeValue = &(TC1->TC_CHANNEL[0].TC_CV);  // the initial 'while'.
  timeReset = &(TC1->TC_CHANNEL[0].TC_CCR);
  p         =  pixels;
  end       =  p + numBytes;
  pix       = *p++;
  mask      = 0x80;

#ifdef NEO_KHZ400 // 800 KHz check needed only if 400 KHz support enabled
  if(is800KHz) {
#endif
    time0  = TIME_800_0;
    time1  = TIME_800_1;
    period = PERIOD_800;
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    time0  = TIME_400_0;
    time1  = TIME_400_1;
    period = PERIOD_400;
  }
#endif

  for(t = time0;; t = time0) {
    if(pix & mask) t = time1;
    while(*timeValue < period);
    *portSet   = pinMask;
    *timeReset = TC_CCR_CLKEN | TC_CCR_SWTRG;
    while(*timeValue < t);
    *portClear = pinMask;
    if(!(mask >>= 1)) {   // This 'inside-out' loop logic utilizes
      if(p >= end) break; // idle time to minimize inter-byte delays.
      pix = *p++;
      mask = 0x80;
    }
  }
  while(*timeValue < period); // Wait for last bit
  TC_Stop(TC1, 0);

#endif // end Due

// END ARM ----------------------------------------------------------------


#elif defined(ESP8266) || defined(ESP32)

// ESP8266 ----------------------------------------------------------------

  // ESP8266 show() is external to enforce ICACHE_RAM_ATTR execution
  espShow(pin, pixels, numBytes, is800KHz);

#elif defined(__ARDUINO_ARC__)

// Arduino 101  -----------------------------------------------------------

#define NOPx7 { __builtin_arc_nop(); \
  __builtin_arc_nop(); __builtin_arc_nop(); \
  __builtin_arc_nop(); __builtin_arc_nop(); \
  __builtin_arc_nop(); __builtin_arc_nop(); }

  PinDescription *pindesc = &g_APinDescription[pin];
  register uint32_t loop = 8 * numBytes; // one loop to handle all bytes and all bits
  register uint8_t *p = pixels;
  register uint32_t currByte = (uint32_t) (*p);
  register uint32_t currBit = 0x80 & currByte;
  register uint32_t bitCounter = 0;
  register uint32_t first = 1;

  // The loop is unusual. Very first iteration puts all the way LOW to the wire -
  // constant LOW does not affect NEOPIXEL, so there is no visible effect displayed.
  // During that very first iteration CPU caches instructions in the loop.
  // Because of the caching process, "CPU slows down". NEOPIXEL pulse is very time sensitive
  // that's why we let the CPU cache first and we start regular pulse from 2nd iteration
  if (pindesc->ulGPIOType == SS_GPIO) {
    register uint32_t reg = pindesc->ulGPIOBase + SS_GPIO_SWPORTA_DR;
    uint32_t reg_val = __builtin_arc_lr((volatile uint32_t)reg);
    register uint32_t reg_bit_high = reg_val | (1 << pindesc->ulGPIOId);
    register uint32_t reg_bit_low  = reg_val & ~(1 << pindesc->ulGPIOId);

    loop += 1; // include first, special iteration
    while(loop--) {
      if(!first) {
        currByte <<= 1;
        bitCounter++;
      }

      // 1 is >550ns high and >450ns low; 0 is 200..500ns high and >450ns low
      __builtin_arc_sr(first ? reg_bit_low : reg_bit_high, (volatile uint32_t)reg);
      if(currBit) { // ~400ns HIGH (740ns overall)
        NOPx7
        NOPx7
      }
      // ~340ns HIGH
      NOPx7
     __builtin_arc_nop();

      // 820ns LOW; per spec, max allowed low here is 5000ns */
      __builtin_arc_sr(reg_bit_low, (volatile uint32_t)reg);
      NOPx7
      NOPx7

      if(bitCounter >= 8) {
        bitCounter = 0;
        currByte = (uint32_t) (*++p);
      }

      currBit = 0x80 & currByte;
      first = 0;
    }
  } else if(pindesc->ulGPIOType == SOC_GPIO) {
    register uint32_t reg = pindesc->ulGPIOBase + SOC_GPIO_SWPORTA_DR;
    uint32_t reg_val = MMIO_REG_VAL(reg);
    register uint32_t reg_bit_high = reg_val | (1 << pindesc->ulGPIOId);
    register uint32_t reg_bit_low  = reg_val & ~(1 << pindesc->ulGPIOId);

    loop += 1; // include first, special iteration
    while(loop--) {
      if(!first) {
        currByte <<= 1;
        bitCounter++;
      }
      MMIO_REG_VAL(reg) = first ? reg_bit_low : reg_bit_high;
      if(currBit) { // ~430ns HIGH (740ns overall)
        NOPx7
        NOPx7
        __builtin_arc_nop();
      }
      // ~310ns HIGH
      NOPx7

      // 850ns LOW; per spec, max allowed low here is 5000ns */
      MMIO_REG_VAL(reg) = reg_bit_low;
      NOPx7
      NOPx7

      if(bitCounter >= 8) {
        bitCounter = 0;
        currByte = (uint32_t) (*++p);
      }

      currBit = 0x80 & currByte;
      first = 0;
    }
  }

#else 
#error Architecture not supported
#endif


// END ARCHITECTURE SELECT ------------------------------------------------

#ifndef NRF52
  interrupts();
#endif

  endTime = micros(); // Save EOD time for latch on next call
}

// Set the output pin number
void Adafruit_NeoPixel::setPin(uint8_t p) {
  if(begun && (pin >= 0)) pinMode(pin, INPUT);
    pin = p;
    if(begun) {
      pinMode(p, OUTPUT);
      digitalWrite(p, LOW);
    }
#ifdef __AVR__
    port    = portOutputRegister(digitalPinToPort(p));
    pinMask = digitalPinToBitMask(p);
#endif
}

// Set pixel color from separate R,G,B components:
void Adafruit_NeoPixel::setPixelColor(
 uint16_t n, uint8_t r, uint8_t g, uint8_t b) {

  if(n < numLEDs) {
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
    }
    uint8_t *p;
    if(wOffset == rOffset) { // Is an RGB-type strip
      p = &pixels[n * 3];    // 3 bytes per pixel
    } else {                 // Is a WRGB-type strip
      p = &pixels[n * 4];    // 4 bytes per pixel
      p[wOffset] = 0;        // But only R,G,B passed -- set W to 0
    }
    p[rOffset] = r;          // R,G,B always stored
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

void Adafruit_NeoPixel::setPixelColor(
 uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {

  if(n < numLEDs) {
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
      w = (w * brightness) >> 8;
    }
    uint8_t *p;
    if(wOffset == rOffset) { // Is an RGB-type strip
      p = &pixels[n * 3];    // 3 bytes per pixel (ignore W)
    } else {                 // Is a WRGB-type strip
      p = &pixels[n * 4];    // 4 bytes per pixel
      p[wOffset] = w;        // Store W
    }
    p[rOffset] = r;          // Store R,G,B
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if(n < numLEDs) {
    uint8_t *p,
      r = (uint8_t)(c >> 16),
      g = (uint8_t)(c >>  8),
      b = (uint8_t)c;
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
    }
    if(wOffset == rOffset) {
      p = &pixels[n * 3];
    } else {
      p = &pixels[n * 4];
      uint8_t w = (uint8_t)(c >> 24);
      p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
    }
    p[rOffset] = r;
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Convert separate R,G,B,W into packed 32-bit WRGB color.
// Packed format is always WRGB, regardless of LED strand color order.
uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
uint32_t Adafruit_NeoPixel::getPixelColor(uint16_t n) const {
  if(n >= numLEDs) return 0; // Out of bounds, return no color.

  uint8_t *p;

  if(wOffset == rOffset) { // Is RGB-type device
    p = &pixels[n * 3];
    if(brightness) {
      // Stored color was decimated by setBrightness().  Returned value
      // attempts to scale back to an approximation of the original 24-bit
      // value used when setting the pixel color, but there will always be
      // some error -- those bits are simply gone.  Issue is most
      // pronounced at low brightness levels.
      return (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
             (((uint32_t)(p[gOffset] << 8) / brightness) <<  8) |
             ( (uint32_t)(p[bOffset] << 8) / brightness       );
    } else {
      // No brightness adjustment has been made -- return 'raw' color
      return ((uint32_t)p[rOffset] << 16) |
             ((uint32_t)p[gOffset] <<  8) |
              (uint32_t)p[bOffset];
    }
  } else {                 // Is RGBW-type device
    p = &pixels[n * 4];
    if(brightness) { // Return scaled color
      return (((uint32_t)(p[wOffset] << 8) / brightness) << 24) |
             (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
             (((uint32_t)(p[gOffset] << 8) / brightness) <<  8) |
             ( (uint32_t)(p[bOffset] << 8) / brightness       );
    } else { // Return raw color
      return ((uint32_t)p[wOffset] << 24) |
             ((uint32_t)p[rOffset] << 16) |
             ((uint32_t)p[gOffset] <<  8) |
              (uint32_t)p[bOffset];
    }
  }
}

// Returns pointer to pixels[] array.  Pixel data is stored in device-
// native format and is not translated here.  Application will need to be
// aware of specific pixel data format and handle colors appropriately.
uint8_t *Adafruit_NeoPixel::getPixels(void) const {
  return pixels;
}

uint16_t Adafruit_NeoPixel::numPixels(void) const {
  return numLEDs;
}

// Adjust output brightness; 0=darkest (off), 255=brightest.  This does
// NOT immediately affect what's currently displayed on the LEDs.  The
// next call to show() will refresh the LEDs at this level.  However,
// this process is potentially "lossy," especially when increasing
// brightness.  The tight timing in the WS2811/WS2812 code means there
// aren't enough free cycles to perform this scaling on the fly as data
// is issued.  So we make a pass through the existing color data in RAM
// and scale it (subsequent graphics commands also work at this
// brightness level).  If there's a significant step up in brightness,
// the limited number of steps (quantization) in the old data will be
// quite visible in the re-scaled version.  For a non-destructive
// change, you'll need to re-render the full strip data.  C'est la vie.
void Adafruit_NeoPixel::setBrightness(uint8_t b) {
  // Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
  uint8_t newBrightness = b + 1;
  if(newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM
    uint8_t  c,
            *ptr           = pixels,
             oldBrightness = brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if(oldBrightness == 0) scale = 0; // Avoid /0
    else if(b == 255) scale = 65535 / oldBrightness;
    else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
    for(uint16_t i=0; i<numBytes; i++) {
      c      = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    brightness = newBrightness;
  }
}

//Return the brightness value
uint8_t Adafruit_NeoPixel::getBrightness(void) const {
  return brightness - 1;
}

void Adafruit_NeoPixel::clear() {
  memset(pixels, 0, numBytes);
}

/* A PROGMEM (flash mem) table containing 8-bit unsigned sine wave (0-255).
   Copy & paste this snippet into a Python REPL to regenerate:
import math
for x in range(256):
    print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
    if x&15 == 15: print
*/
static const uint8_t PROGMEM _sineTable[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
   79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
   37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
   10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
   10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
   37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
   79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124};

/* Similar to above, but for an 8-bit gamma-correction table.
   Copy & paste this snippet into a Python REPL to regenerate:
import math
gamma=2.6
for x in range(256):
    print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
    if x&15 == 15: print
*/
static const uint8_t PROGMEM _gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
    3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
    7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
   13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
   20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
   30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
   42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
   76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
   97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
  122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
  150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
  182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
  218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255};

uint8_t Adafruit_NeoPixel::sine8(uint8_t x) const {
  return pgm_read_byte(&_sineTable[x]); // 0-255 in, 0-255 out
}

uint8_t Adafruit_NeoPixel::gamma8(uint8_t x) const {
  return pgm_read_byte(&_gammaTable[x]); // 0-255 in, 0-255 out
}

