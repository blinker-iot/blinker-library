#ifndef BLINKER_PROTOCOL_BLE_MODE_H
#define BLINKER_PROTOCOL_BLE_MODE_H

#include "../../core/Result.h"
#include "../../core/Span.h"
#include "../SetupSession.h"
#include "../noise/NoiseSuite.h"

namespace blinker {
namespace ble {

static const uint8_t kModeAdvertisingFormatVersion = 1U;
static const uint8_t kProvisioningWireVersion = 1U;
static const uint8_t kDirectBbp2WireVersion = 2U;
static const size_t kModeServiceDataSize = 13U;
static const size_t kModeScanResponseSize = 31U;

enum class ApplicationMode : uint8_t {
    None = 0U,
    Provisioning = 1U,
    Direct = 2U
};

enum ModeCapability : uint16_t {
    ModeCapabilityNone = 0U,
    ModeCapabilityFragmentedRecords = 1U << 0U,
    ModeCapabilityNoiseNn = 1U << 1U,
    ModeCapabilityEnrollmentV1 = 1U << 2U,
    ModeCapabilityWifiConfigV1 = 1U << 3U,
    ModeCapabilityDirectBbp2 = 1U << 4U,
    ModeCapabilityNoiseNnPsk0 = 1U << 5U,
    ModeCapabilityLocalControllerPairingV1 = 1U << 6U
};

static const uint16_t kKnownModeCapabilities =
    ModeCapabilityFragmentedRecords | ModeCapabilityNoiseNn |
    ModeCapabilityEnrollmentV1 | ModeCapabilityWifiConfigV1 |
    ModeCapabilityDirectBbp2 | ModeCapabilityNoiseNnPsk0 |
    ModeCapabilityLocalControllerPairingV1;

struct ModeProfile {
    ApplicationMode mode;
    uint8_t wireVersion;
    uint16_t capabilities;
    uint8_t setupSessionLocator[kSetupSessionLocatorSize];

    ModeProfile()
        : mode(ApplicationMode::None), wireVersion(0U),
          capabilities(ModeCapabilityNone), setupSessionLocator() {}
};

ModeProfile makeDirectModeProfile();
ModeProfile makeProvisioningModeProfile(ByteView setupSessionLocator,
                                        bool acceptsWifiConfig);
ModeProfile makePskProvisioningModeProfile(ByteView setupSessionLocator,
                                           bool acceptsWifiConfig);
ModeProfile makeLocalControllerPairingModeProfile(
    ByteView setupSessionLocator);
ModeProfile makePskLocalControllerPairingModeProfile(
    ByteView setupSessionLocator);

Result provisioningNoisePattern(const ModeProfile& profile,
                                noise::NoiseNnPattern& pattern);

Result validateModeProfile(const ModeProfile& profile);
bool modeProfilesEqual(const ModeProfile& left, const ModeProfile& right);

// Service Data is exactly 13 bytes:
//   formatVersion | mode | wireVersion | capabilities(le16) |
//   setupSessionLocator
// The enclosing AD field uses the frozen 128-bit Blinker service UUID.
Result encodeModeServiceData(const ModeProfile& profile,
                             MutableByteSpan output,
                             ByteView& encoded);
Result decodeModeServiceData(ByteView encoded, ModeProfile& profile);

// Legacy advertising has a 31-byte scan-response ceiling. This produces the
// exact, full scan response containing one 128-bit Service Data AD structure.
Result encodeModeScanResponse(const ModeProfile& profile,
                              MutableByteSpan output,
                              ByteView& encoded);

} // namespace ble
} // namespace blinker

#endif
