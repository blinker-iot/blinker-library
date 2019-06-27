#ifndef _PAINLESS_MESH_LOGGER_HPP_
#define _PAINLESS_MESH_LOGGER_HPP_
namespace painlessmesh {
namespace logger {

#include <stdarg.h>
#include "Arduino.h"

typedef enum {
  ERROR = 1 << 0,
  STARTUP = 1 << 1,
  MESH_STATUS = 1 << 2,
  CONNECTION = 1 << 3,
  SYNC = 1 << 4,
  S_TIME = 1 << 5,
  COMMUNICATION = 1 << 6,
  GENERAL = 1 << 7,
  MSG_TYPES = 1 << 8,
  REMOTE = 1 << 9,  // not yet implemented
  APPLICATION = 1 << 10,
  DEBUG = 1 << 11
} LogLevel;

class LogClass {
 public:
  void setLogLevel(uint16_t newTypes) {
    // set the different kinds of debug messages you want to generate.
    types = newTypes;
    Serial.print(F("\nsetLogLevel:"));
    if (types & ERROR) {
      Serial.print(F(" ERROR |"));
    }
    if (types & STARTUP) {
      Serial.print(F(" STARTUP |"));
    }
    if (types & MESH_STATUS) {
      Serial.print(F(" MESH_STATUS |"));
    }
    if (types & CONNECTION) {
      Serial.print(F(" CONNECTION |"));
    }
    if (types & SYNC) {
      Serial.print(F(" SYNC |"));
    }
    if (types & S_TIME) {
      Serial.print(F(" S_TIME |"));
    }
    if (types & COMMUNICATION) {
      Serial.print(F(" COMMUNICATION |"));
    }
    if (types & GENERAL) {
      Serial.print(F(" GENERAL |"));
    }
    if (types & MSG_TYPES) {
      Serial.print(F(" MSG_TYPES |"));
    }
    if (types & REMOTE) {
      Serial.print(F(" REMOTE |"));
    }
    if (types & APPLICATION) {
      Serial.print(F(" APPLICATION |"));
    }
    if (types & DEBUG) {
      Serial.print(F(" DEBUG |"));
    }
    Serial.println();
    return;
  }
  void operator()(LogLevel type, const char* format...) {
    if (type & types) {  // Print only the message types set for output
      va_list args;
      va_start(args, format);

      vsnprintf(str, 200, format, args);

      if (types) {
        switch (type) {
          case ERROR:
            Serial.print(F("ERROR: "));
            break;
          case STARTUP:
            Serial.print(F("STARTUP: "));
            break;
          case MESH_STATUS:
            Serial.print(F("MESH_STATUS: "));
            break;
          case CONNECTION:
            Serial.print(F("CONNECTION: "));
            break;
          case SYNC:
            Serial.print(F("SYNC: "));
            break;
          case S_TIME:
            Serial.print(F("S_TIME: "));
            break;
          case COMMUNICATION:
            Serial.print(F("COMMUNICATION: "));
            break;
          case GENERAL:
            Serial.print(F("GENERAL: "));
            break;
          case MSG_TYPES:
            Serial.print(F("MSG_TYPES: "));
            break;
          case REMOTE:
            Serial.print(F("REMOTE: "));
            break;
          case APPLICATION:
            Serial.print(F("APPLICATION: "));
            break;
          case DEBUG:
            Serial.print(F("DEBUG: "));
            break;
        }
      }

      Serial.print(str);

      va_end(args);
    }
  }

 private:
  uint16_t types = 0;
  char str[200];
};

}  // namespace logger
}  // namespace painlessmesh
#endif

