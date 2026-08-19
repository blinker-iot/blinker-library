#ifndef BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H
#define BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H

#include <stdint.h>

namespace blinker {
namespace official {

// These values are library release configuration, not Sketch credentials.
// Production publication must replace the placeholder host in one place.
static const char controlHost[] = "replace-with-control-host.example";
static const uint16_t controlPort = 80U;
static const char firmwareVersion[] = "device-v2-0.1.0";
static const char cloudSessionCaPem[] = "";
static const char mqttCaPem[] = "";
static const bool cloudSessionTls = false;
static const bool mqttTls = false;
static const bool allowPlainStorage = true;

} // namespace official
} // namespace blinker

#define BLINKER_OFFICIAL_CLOUD_SESSION_TLS 0
#define BLINKER_OFFICIAL_MQTT_TLS 0

#endif
