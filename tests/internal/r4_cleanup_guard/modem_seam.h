#ifndef BLINKER_TEST_R4_CLEANUP_MODEM_SEAM_H
#define BLINKER_TEST_R4_CLEANUP_MODEM_SEAM_H
#include <stdint.h>
#include <string>
#include <vector>
namespace cleanup_seam {
struct Calls {
    unsigned begin = 0U, write = 0U, nowait = 0U, passthrough = 0U, end = 0U;
    std::vector<std::string> commands;
    unsigned total() const { return begin + write + nowait + passthrough + end; }
};
extern Calls calls;
extern uint32_t now;
}
#endif
