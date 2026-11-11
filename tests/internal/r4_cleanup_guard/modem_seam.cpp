// Keep the actual pinned SDK declaration. This deliberately permissive seam
// NEVER checks shared owner/fault state: only the production guard may stop it.
#include "Modem.h"
#include "modem_seam.h"
#include <cstdarg>
#include <cstdio>

namespace cleanup_seam { Calls calls; uint32_t now = 123U; }
unsigned long millis() { return cleanup_seam::now; }
void yield() { ++cleanup_seam::now; }
void delay(unsigned long milliseconds) { cleanup_seam::now += static_cast<uint32_t>(milliseconds); }

ModemClass::ModemClass(UART* uart)
    : beginned(false), delete_serial(false), _serial(uart), _timeout(MODEM_TIMEOUT),
      tx_buff{}, trim_results(true), _serial_debug(nullptr), _debug_level(0U) {}
ModemClass::~ModemClass() {}
void ModemClass::begin(int, int) { ++cleanup_seam::calls.begin; beginned = true; }
void ModemClass::end() { ++cleanup_seam::calls.end; beginned = false; }

namespace {
void record(const char* format, va_list arguments) {
    char command[256] = {};
    const int count = std::vsnprintf(command, sizeof(command), format, arguments);
    if (count < 0 || static_cast<size_t>(count) >= sizeof(command)) std::abort();
    cleanup_seam::calls.commands.push_back(command);
}
}
bool ModemClass::write(const std::string& prompt, std::string& response, const char* format, ...) {
    ++cleanup_seam::calls.write;
    va_list arguments; va_start(arguments, format); record(format, arguments); va_end(arguments);
    response = prompt == PROMPT(_BEGINCLIENT) ? "7" :
        prompt == PROMPT(_CLIENTRECEIVE) ? "AB" :
        prompt == PROMPT(_GETSTATUS) ? "3" : "1";
    return true;
}
void ModemClass::write_nowait(const std::string&, std::string&, const char* format, ...) {
    ++cleanup_seam::calls.nowait;
    va_list arguments; va_start(arguments, format); record(format, arguments); va_end(arguments);
}
bool ModemClass::passthrough(const uint8_t*, size_t) { ++cleanup_seam::calls.passthrough; return true; }
ModemClass modem(static_cast<UART*>(nullptr));
