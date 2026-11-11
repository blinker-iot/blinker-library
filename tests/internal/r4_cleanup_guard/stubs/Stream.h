#ifndef BLINKER_TEST_R4_CLEANUP_STREAM_H
#define BLINKER_TEST_R4_CLEANUP_STREAM_H
#include "Print.h"
class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
};
#endif
