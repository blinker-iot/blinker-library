#ifndef BLINKER_TEST_R4_CLEANUP_SERVER_H
#define BLINKER_TEST_R4_CLEANUP_SERVER_H
#include "Print.h"
class Server : public Print { public: virtual void begin() = 0; };
#endif
