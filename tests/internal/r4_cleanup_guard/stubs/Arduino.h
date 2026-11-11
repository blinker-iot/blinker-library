#ifndef BLINKER_TEST_R4_CLEANUP_ARDUINO_H
#define BLINKER_TEST_R4_CLEANUP_ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <cstring>
#include <cstdlib>
#include <string>

typedef bool boolean;
typedef uint8_t byte;
typedef std::string String;
#define PROGMEM
#define pgm_read_byte(address) (*reinterpret_cast<const uint8_t*>(address))
#define pgm_read_byte_near(address) pgm_read_byte(address)
#define pgm_read_word(address) (*reinterpret_cast<const uint16_t*>(address))

// Original FifoBuffer's critical sections are single-threaded Host seams.
// They do not model IRQ masking, scheduling, timing or the ARM ABI.
#define synchronized if (true)

class Stream;
class UART;
unsigned long millis();
void yield();
void delay(unsigned long milliseconds);
namespace arduino { template <typename T, uint32_t Size> class FifoBuffer; }
using arduino::FifoBuffer;
#endif
