#ifndef BLINKER_TEST_R4_MODEM_FAKE_ARDUINO_H
#define BLINKER_TEST_R4_MODEM_FAKE_ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern uint32_t fakeNow;
extern uint32_t fakeWriteCost, fakeReadCost, fakePollCost;
extern size_t fakeWriteLimit;
extern bool fakePollFault;
extern "C" void blinker_r4_rpc_fault();

class Stream {
public:
    template <typename T> void print(const T&) {}
    template <typename T> void println(const T&) {}
    void println() {}
    void write(const uint8_t*, size_t) {}
};

class UART : public Stream {
public:
    UART(int = 0, int = 0) : reads(0U), writes(0U), starts(0U), flushes(0U), checks(0U), input(), output() {}
    void begin(int) { ++starts; }
    void end() {}
    void flush() { ++flushes; }
    int available() {
        ++checks; fakeNow += fakePollCost;
        if (fakePollFault) blinker_r4_rpc_fault();
        return !input.empty();
    }
    int read() {
        ++reads;
        fakeNow += fakeReadCost;
        if (input.empty()) return -1;
        const unsigned char value = static_cast<unsigned char>(input[0]);
        input.erase(0U, 1U);
        return value;
    }
    size_t write(const uint8_t* bytes, size_t size) {
        ++writes;
        fakeNow += fakeWriteCost;
        const size_t count = size < fakeWriteLimit ? size : fakeWriteLimit;
        output.append(reinterpret_cast<const char*>(bytes), count);
        return count;
    }
    unsigned reads, writes, starts, flushes, checks;
    std::string input, output;
};

extern UART Serial2;
unsigned long millis();
#endif
