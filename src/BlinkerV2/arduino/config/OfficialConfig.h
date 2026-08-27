#ifndef BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H
#define BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H

#include <stdint.h>

#ifndef BLINKER_OFFICIAL_CONTROL_HOST
#define BLINKER_OFFICIAL_CONTROL_HOST "replace-with-control-host.example"
#endif

#ifndef BLINKER_OFFICIAL_CONTROL_PORT
#define BLINKER_OFFICIAL_CONTROL_PORT 80U
#endif

#ifndef BLINKER_OFFICIAL_FIRMWARE_VERSION
#define BLINKER_OFFICIAL_FIRMWARE_VERSION "device-v2-0.1.0"
#endif

#ifndef BLINKER_OFFICIAL_CLOUD_SESSION_TLS
#define BLINKER_OFFICIAL_CLOUD_SESSION_TLS 0
#endif

#ifndef BLINKER_OFFICIAL_MQTT_TLS
#define BLINKER_OFFICIAL_MQTT_TLS 0
#endif

namespace blinker {
namespace official {

// These values are library release configuration, not Sketch credentials.
// Release builds set the BLINKER_OFFICIAL_* macros without changing user code.
static const char controlHost[] = BLINKER_OFFICIAL_CONTROL_HOST;
static const uint16_t controlPort = BLINKER_OFFICIAL_CONTROL_PORT;
static const char firmwareVersion[] = BLINKER_OFFICIAL_FIRMWARE_VERSION;
static const char cloudSessionCaPem[] = "";
static const char mqttCaPem[] = "";
static const bool cloudSessionTls = BLINKER_OFFICIAL_CLOUD_SESSION_TLS != 0;
static const bool mqttTls = BLINKER_OFFICIAL_MQTT_TLS != 0;
static const bool allowPlainStorage = true;

} // namespace official
} // namespace blinker

#endif
