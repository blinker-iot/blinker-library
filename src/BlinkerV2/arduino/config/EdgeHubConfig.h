#ifndef BLINKER_V2_ARDUINO_EDGEHUBCONFIG_H
#define BLINKER_V2_ARDUINO_EDGEHUBCONFIG_H

#ifndef BLINKER_EDGE_HUB_NVS_PARTITION
#define BLINKER_EDGE_HUB_NVS_PARTITION "nvs"
#endif

#ifndef BLINKER_EDGE_HUB_NTP_PRIMARY
#define BLINKER_EDGE_HUB_NTP_PRIMARY "pool.ntp.org"
#endif

#ifndef BLINKER_EDGE_HUB_NTP_SECONDARY
#define BLINKER_EDGE_HUB_NTP_SECONDARY "time.cloudflare.com"
#endif

namespace blinker {
namespace official {

static const char edgeHubNvsPartition[] =
    BLINKER_EDGE_HUB_NVS_PARTITION;
static const char edgeHubNtpPrimary[] = BLINKER_EDGE_HUB_NTP_PRIMARY;
static const char edgeHubNtpSecondary[] = BLINKER_EDGE_HUB_NTP_SECONDARY;

} // namespace official
} // namespace blinker

#endif
