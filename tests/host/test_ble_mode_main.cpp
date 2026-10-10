#include <BlinkerV2/core/Span.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/protocol/ble/Gatt.h>
#include <BlinkerV2/protocol/ble/Mode.h>
#include <BlinkerV2/protocol/ble/Presence.h>
#include <BlinkerV2/transport/BleFrameTransport.h>
#include <BlinkerV2/transport/BleModeCoordinator.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "CHECK failed at line " << __LINE__ << ": "          \
                      << #condition << std::endl;                               \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class ModeTestClock : public blinker::IClock {
public:
    uint32_t monotonicMillis() const override { return 0U; }
    bool unixTime(uint64_t&) const override { return false; }
};

class PresenceTestRandom final : public blinker::IRandom {
public:
    PresenceTestRandom(
        const uint8_t repeated[blinker::ble::kPresenceNonceSize],
        const uint8_t next[blinker::ble::kPresenceNonceSize],
        uint8_t repeatedCalls)
        : repeated_(), next_(), repeatedCalls_(repeatedCalls), calls_(0U) {
        std::memcpy(repeated_, repeated, sizeof(repeated_));
        std::memcpy(next_, next, sizeof(next_));
    }

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (output.data == nullptr || output.size != sizeof(repeated_)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        const uint8_t* source = calls_ < repeatedCalls_ ? repeated_ : next_;
        ++calls_;
        std::memcpy(output.data, source, output.size);
        return blinker::Result::success();
    }

    uint8_t calls() const { return calls_; }

private:
    uint8_t repeated_[blinker::ble::kPresenceNonceSize];
    uint8_t next_[blinker::ble::kPresenceNonceSize];
    uint8_t repeatedCalls_;
    uint8_t calls_;
};

class ModeTestCrypto final
    : public blinker::IX25519AesGcmCryptoProvider {
public:
    blinker::Result x25519PublicKey(
        blinker::ByteView,
        blinker::MutableByteSpan) override { return unsupported(); }
    blinker::Result x25519(
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan) override { return unsupported(); }
    blinker::Result aes256GcmEncrypt(
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan,
        size_t& written) override {
        written = 0U;
        return unsupported();
    }
    blinker::Result aes256GcmDecrypt(
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan,
        size_t& written) override {
        written = 0U;
        return unsupported();
    }

private:
    static blinker::Result unsupported() {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }
};

class ModeTestLink : public blinker::IBleModeLink {
public:
    ModeTestLink()
        : currentState(blinker::BleLinkState::Stopped),
          configured(),
          packetReceiver(nullptr),
          packetContext(nullptr),
          connectedHandler(nullptr),
          disconnectedHandler(nullptr),
          sessionContext(nullptr),
          configureCount(0U),
          refreshCount(0U),
          startCount(0U),
          stopCount(0U),
          refreshSupported(false),
          startsToFail(0U),
          configsToFail(0U),
          currentError(blinker::ErrorCode::Ok) {}

    blinker::Result configureBleProfile(
        const blinker::ble::ModeProfile& profile) override {
        ++configureCount;
        if (currentState != blinker::BleLinkState::Stopped) {
            return blinker::Result::failure(
                blinker::ErrorCode::StateConflict);
        }
        blinker::Result result = blinker::ble::validateModeProfile(profile);
        if (!result) return result;
        if (configsToFail != 0U) {
            --configsToFail;
            return blinker::Result::failure(
                blinker::ErrorCode::NotConfigured);
        }
        configured = profile;
        return blinker::Result::success();
    }

    blinker::ble::ModeProfile bleProfile() const override {
        return configured;
    }

    blinker::Result refreshBleProfile(
        const blinker::ble::ModeProfile& profile) override {
        ++refreshCount;
        if (!refreshSupported) {
            return blinker::Result::failure(
                blinker::ErrorCode::UnsupportedFeature);
        }
        if (currentState != blinker::BleLinkState::Ready) {
            return blinker::Result::failure(
                blinker::ErrorCode::StateConflict);
        }
        const blinker::Result result =
            blinker::ble::validateModeProfile(profile);
        if (result) configured = profile;
        return result;
    }

    blinker::Result start() override {
        ++startCount;
        if (currentState != blinker::BleLinkState::Stopped) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        if (startsToFail != 0U) {
            --startsToFail;
            // Model a platform which fails after partially initializing its
            // stack. BleRecordTransport must stop it before rollback can
            // reconfigure the link.
            currentState = blinker::BleLinkState::Error;
            return blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
        currentState = blinker::BleLinkState::Ready;
        currentError = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }

    void stop() override {
        ++stopCount;
        currentState = blinker::BleLinkState::Stopped;
        currentError = blinker::ErrorCode::Ok;
    }

    void poll(uint32_t) override {}
    blinker::BleLinkState state() const override { return currentState; }
    blinker::ErrorCode lastError() const override {
        return currentError;
    }
    size_t sessionCount() const override { return 0U; }
    blinker::Result sessionAt(
        size_t,
        blinker::BleSessionInfo&) const override {
        return blinker::Result::failure(blinker::ErrorCode::NotFound);
    }
    blinker::Result sendPacket(
        uint32_t,
        blinker::ByteView) override {
        return blinker::Result::failure(blinker::ErrorCode::NotConnected);
    }
    void setPacketReceiver(
        blinker::BlePacketReceiver receiver,
        void* context) override {
        packetReceiver = receiver;
        packetContext = context;
    }
    void setSessionHandlers(
        blinker::BleSessionHandler connected,
        blinker::BleSessionHandler disconnected,
        void* context) override {
        connectedHandler = connected;
        disconnectedHandler = disconnected;
        sessionContext = context;
    }

    blinker::BleLinkState currentState;
    blinker::ble::ModeProfile configured;
    blinker::BlePacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleSessionHandler connectedHandler;
    blinker::BleSessionHandler disconnectedHandler;
    void* sessionContext;
    uint32_t configureCount;
    uint32_t refreshCount;
    uint32_t startCount;
    uint32_t stopCount;
    bool refreshSupported;
    uint8_t startsToFail;
    uint8_t configsToFail;
    blinker::ErrorCode currentError;
};

class ModeTestPreparation : public blinker::IBleModePreparation {
public:
    ModeTestPreparation()
        : callCount(0U), rollbackCount(0U), callsToFail(0U), lastProfile() {}

    blinker::Result prepareBleMode(
        const blinker::ble::ModeProfile& profile,
        bool rollback) override {
        ++callCount;
        if (rollback) ++rollbackCount;
        lastProfile = profile;
        if (callsToFail != 0U) {
            --callsToFail;
            return blinker::Result::failure(
                blinker::ErrorCode::NotConfigured);
        }
        return blinker::Result::success();
    }

    uint32_t callCount;
    uint32_t rollbackCount;
    uint8_t callsToFail;
    blinker::ble::ModeProfile lastProfile;
};

struct ModeFixture {
    ModeTestClock clock;
    ModeTestLink link;
    ModeTestCrypto crypto;
    uint8_t provisioningRx[88U * BLINKER_BLE_MAX_SESSIONS];
    uint8_t provisioningTx[88U * BLINKER_BLE_MAX_TX_FRAMES];
    uint8_t provisioningPacket[20];
    uint8_t directRx[88U * BLINKER_BLE_MAX_SESSIONS];
    uint8_t directTx[88U * BLINKER_BLE_MAX_TX_FRAMES];
    uint8_t directPacket[20];
    uint8_t plaintext[64U];
    blinker::BleFrameTransport provisioning;
    blinker::BleFrameTransport direct;
    ModeTestPreparation preparation;
    blinker::BleModeCoordinator coordinator;

    ModeFixture()
        : clock(),
          link(),
          crypto(),
          provisioningRx(),
          provisioningTx(),
          provisioningPacket(),
          directRx(),
          directTx(),
          directPacket(),
          plaintext(),
          provisioning(
              link,
              clock,
              crypto,
              blinker::MutableByteSpan(
                  provisioningRx,
                  sizeof(provisioningRx)),
              blinker::MutableByteSpan(
                  provisioningTx,
                  sizeof(provisioningTx)),
              blinker::MutableByteSpan(
                  provisioningPacket,
                  sizeof(provisioningPacket)),
              blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
              transportConfig()),
          direct(
              link,
              clock,
              crypto,
              blinker::MutableByteSpan(directRx, sizeof(directRx)),
              blinker::MutableByteSpan(directTx, sizeof(directTx)),
              blinker::MutableByteSpan(directPacket, sizeof(directPacket)),
              blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
              transportConfig()),
          preparation(),
          coordinator(link, provisioning, direct, &preparation) {}

    static blinker::BleFrameTransportConfig transportConfig() {
        blinker::BleFrameTransportConfig config;
        config.maxFrameSize = 64U;
        config.reassemblyTimeoutMillis = 1000U;
        config.maxPacketsPerPoll = 2U;
        return config;
    }
};

blinker::ble::ModeProfile provisioningProfile(uint8_t first) {
    uint8_t setupSessionLocator[8] = {};
    for (size_t index = 0U;
         index < sizeof(setupSessionLocator);
         ++index) {
        setupSessionLocator[index] = static_cast<uint8_t>(first + index);
    }
    return blinker::ble::makeProvisioningModeProfile(
        blinker::ByteView(
            setupSessionLocator,
            sizeof(setupSessionLocator)),
        true);
}

void testAdvertisingContract() {
    const blinker::ble::ModeProfile provisioning = provisioningProfile(1U);
    uint8_t encodedBytes[blinker::ble::kModeServiceDataSize] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        provisioning,
        blinker::MutableByteSpan(encodedBytes, sizeof(encodedBytes)),
        encoded));
    const uint8_t expected[] = {
        0x01U, 0x01U, 0x01U, 0x0FU, 0x00U,
        0x01U, 0x02U, 0x03U, 0x04U,
        0x05U, 0x06U, 0x07U, 0x08U
    };
    CHECK_EQ(sizeof(expected), encoded.size);
    CHECK_TRUE(std::memcmp(expected, encoded.data, sizeof(expected)) == 0);

    blinker::ble::ModeProfile decoded;
    CHECK_TRUE(blinker::ble::decodeModeServiceData(encoded, decoded));
    CHECK_TRUE(blinker::ble::modeProfilesEqual(provisioning, decoded));
    blinker::noise::NoiseNnPattern pattern;
    CHECK_TRUE(blinker::ble::provisioningNoisePattern(
        provisioning,
        pattern));
    CHECK_EQ(blinker::noise::NoiseNnPattern::Nn, pattern);

    const blinker::ble::ModeProfile enhanced =
        blinker::ble::makePskProvisioningModeProfile(
            blinker::ByteView(
                provisioning.modeLocator,
                sizeof(provisioning.modeLocator)),
            true);
    CHECK_TRUE(blinker::ble::validateModeProfile(enhanced));
    CHECK_TRUE(blinker::ble::provisioningNoisePattern(enhanced, pattern));
    CHECK_EQ(blinker::noise::NoiseNnPattern::NnPsk0, pattern);
    CHECK_EQ(static_cast<uint16_t>(0x002DU), enhanced.capabilities);

    uint8_t scanBytes[blinker::ble::kModeScanResponseSize] = {};
    blinker::ByteView scan;
    CHECK_TRUE(blinker::ble::encodeModeScanResponse(
        provisioning,
        blinker::MutableByteSpan(scanBytes, sizeof(scanBytes)),
        scan));
    CHECK_EQ(static_cast<size_t>(31U), scan.size);
    CHECK_EQ(static_cast<uint8_t>(30U), scan.data[0]);
    CHECK_EQ(static_cast<uint8_t>(0x21U), scan.data[1]);
    CHECK_TRUE(std::memcmp(
        scan.data + 2U,
        blinker::ble::kServiceUuidLittleEndian,
        16U) == 0);
    CHECK_TRUE(std::memcmp(scan.data + 18U, expected, sizeof(expected)) == 0);

    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        direct,
        blinker::MutableByteSpan(encodedBytes, sizeof(encodedBytes)),
        encoded));
    CHECK_EQ(static_cast<uint8_t>(2U), encoded.data[1]);
    CHECK_EQ(static_cast<uint8_t>(2U), encoded.data[2]);
    CHECK_EQ(static_cast<uint8_t>(0x11U), encoded.data[3]);
    for (size_t index = 5U; index < encoded.size; ++index) {
        CHECK_EQ(static_cast<uint8_t>(0U), encoded.data[index]);
    }

    const uint8_t presenceLocator[] = {
        0xA0U, 0xA1U, 0xA2U, 0xA3U,
        0x59U, 0x44U, 0xBBU, 0x16U
    };
    const blinker::ble::ModeProfile authorized =
        blinker::ble::makeAuthorizedDirectModeProfile(blinker::ByteView(
            presenceLocator,
            sizeof(presenceLocator)));
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        authorized,
        blinker::MutableByteSpan(encodedBytes, sizeof(encodedBytes)),
        encoded));
    const uint8_t expectedAuthorized[] = {
        0x01U, 0x02U, 0x03U, 0x51U, 0x00U,
        0xA0U, 0xA1U, 0xA2U, 0xA3U,
        0x59U, 0x44U, 0xBBU, 0x16U
    };
    CHECK_TRUE(std::memcmp(
        expectedAuthorized,
        encoded.data,
        sizeof(expectedAuthorized)) == 0);
    CHECK_TRUE(blinker::ble::decodeModeServiceData(encoded, decoded));
    CHECK_TRUE(blinker::ble::modeProfilesEqual(authorized, decoded));

    encodedBytes[0] = 2U;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedVersion,
        blinker::ble::decodeModeServiceData(
            blinker::ByteView(encodedBytes, sizeof(encodedBytes)),
            decoded).code());
    blinker::ble::ModeProfile invalid = provisioning;
    std::memset(
        invalid.modeLocator,
        0,
        sizeof(invalid.modeLocator));
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::ble::validateModeProfile(invalid).code());
    invalid = direct;
    invalid.capabilities = static_cast<uint16_t>(
        invalid.capabilities | 0x8000U);
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        blinker::ble::validateModeProfile(invalid).code());
    invalid = provisioning;
    invalid.capabilities = static_cast<uint16_t>(
        invalid.capabilities |
        blinker::ble::ModeCapabilityNoiseNnPsk0);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::ble::validateModeProfile(invalid).code());
}

void testAuthorizedPresenceGolden() {
    const uint8_t key[blinker::ble::kPresenceKeySize] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
        0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU
    };
    blinker::DeviceInstanceId instance;
    for (size_t index = 0U; index < sizeof(instance.bytes); ++index) {
        instance.bytes[index] = static_cast<uint8_t>(0x10U + index);
    }
    const uint8_t nonce[blinker::ble::kPresenceNonceSize] = {
        0xA0U, 0xA1U, 0xA2U, 0xA3U
    };
    const uint8_t expected[blinker::ble::kPresenceLocatorSize] = {
        0xA0U, 0xA1U, 0xA2U, 0xA3U,
        0x59U, 0x44U, 0xBBU, 0x16U
    };
    uint8_t locator[blinker::ble::kPresenceLocatorSize] = {};
    CHECK_TRUE(blinker::ble::derivePresenceLocator(
        blinker::ByteView(key, sizeof(key)),
        instance,
        0x01020304U,
        0x05060708U,
        blinker::ByteView(nonce, sizeof(nonce)),
        blinker::MutableByteSpan(locator, sizeof(locator))));
    CHECK_TRUE(std::memcmp(locator, expected, sizeof(expected)) == 0);

    bool matches = false;
    CHECK_TRUE(blinker::ble::verifyPresenceLocator(
        blinker::ByteView(key, sizeof(key)),
        instance,
        0x01020304U,
        0x05060708U,
        blinker::ByteView(locator, sizeof(locator)),
        matches));
    CHECK_TRUE(matches);
    locator[7] ^= 0x01U;
    CHECK_TRUE(blinker::ble::verifyPresenceLocator(
        blinker::ByteView(key, sizeof(key)),
        instance,
        0x01020304U,
        0x05060708U,
        blinker::ByteView(locator, sizeof(locator)),
        matches));
    CHECK_TRUE(!matches);
}

void testPresenceNonceGeneration() {
    const uint8_t previous[blinker::ble::kPresenceNonceSize] = {
        1U, 2U, 3U, 4U};
    const uint8_t next[blinker::ble::kPresenceNonceSize] = {
        5U, 6U, 7U, 8U};
    uint8_t output[blinker::ble::kPresenceNonceSize] = {};
    PresenceTestRandom retrying(previous, next, 2U);
    CHECK_TRUE(blinker::ble::generatePresenceNonce(
        retrying,
        blinker::ByteView(previous, sizeof(previous)),
        blinker::MutableByteSpan(output, sizeof(output))));
    CHECK_EQ(static_cast<uint8_t>(3U), retrying.calls());
    CHECK_TRUE(std::memcmp(output, next, sizeof(next)) == 0);

    PresenceTestRandom stuck(previous, next, 4U);
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        blinker::ble::generatePresenceNonce(
            stuck,
            blinker::ByteView(previous, sizeof(previous)),
            blinker::MutableByteSpan(output, sizeof(output))).code());
    const uint8_t zero[blinker::ble::kPresenceNonceSize] = {};
    CHECK_TRUE(std::memcmp(output, zero, sizeof(zero)) == 0);
}

void testConstructionDoesNotStealCallbacks() {
    ModeFixture fixture;
    CHECK_TRUE(fixture.link.packetReceiver == nullptr);
    CHECK_TRUE(fixture.link.connectedHandler == nullptr);
    CHECK_TRUE(fixture.provisioning.start());
    void* firstOwner = fixture.link.packetContext;
    CHECK_TRUE(firstOwner != nullptr);
    CHECK_EQ(
        blinker::ErrorCode::AlreadyExists,
        fixture.direct.start().code());
    fixture.direct.stop();
    CHECK_TRUE(fixture.link.packetContext == firstOwner);
    CHECK_TRUE(fixture.link.packetReceiver != nullptr);
    CHECK_EQ(blinker::BleLinkState::Ready, fixture.link.state());
    fixture.provisioning.stop();
    CHECK_TRUE(fixture.link.packetReceiver == nullptr);
    CHECK_TRUE(fixture.direct.start());
    CHECK_TRUE(fixture.link.packetContext != nullptr);
    CHECK_TRUE(fixture.link.packetContext != firstOwner);
    fixture.direct.stop();
}

void testDirectFrameTransportDoesNotClaimAuthentication() {
    ModeFixture fixture;
    const blinker::TransportCapabilities capabilities =
        fixture.direct.capabilities();
    CHECK_TRUE(
        (capabilities.features & blinker::TransportFeatureLocal) != 0U);
    CHECK_TRUE(
        (capabilities.features & blinker::TransportFeatureFragmented) != 0U);
    CHECK_TRUE(
        (capabilities.features &
         blinker::TransportFeatureAuthenticated) == 0U);
}

void testRestartActiveRepreparesExactProfile() {
    ModeFixture fixture;
    const blinker::ble::ModeProfile provisioning =
        provisioningProfile(0x11U);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        fixture.coordinator.restartActive().code());
    CHECK_TRUE(fixture.coordinator.start(provisioning));
    CHECK_TRUE(fixture.coordinator.restartActive());
    CHECK_EQ(
        blinker::BleModeCoordinatorState::Provisioning,
        fixture.coordinator.state());
    CHECK_TRUE(blinker::ble::modeProfilesEqual(
        provisioning,
        fixture.coordinator.activeProfile()));
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.configureCount);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.startCount);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.preparation.callCount);
    CHECK_EQ(static_cast<uint32_t>(0U), fixture.preparation.rollbackCount);

    fixture.preparation.callsToFail = 1U;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        fixture.coordinator.restartActive().code());
    CHECK_EQ(
        blinker::BleModeCoordinatorState::Error,
        fixture.coordinator.state());
    CHECK_EQ(
        blinker::ble::ApplicationMode::None,
        fixture.coordinator.activeMode());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        fixture.coordinator.lastTransitionError());
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        fixture.coordinator.restartActive().code());
}

void testSwitchAndSuccessfulRollback() {
    ModeFixture fixture;
    const blinker::ble::ModeProfile provisioning = provisioningProfile(1U);
    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    CHECK_TRUE(fixture.coordinator.start(provisioning));
    CHECK_EQ(
        blinker::BleModeCoordinatorState::Provisioning,
        fixture.coordinator.state());
    CHECK_EQ(blinker::ble::ApplicationMode::Provisioning,
             fixture.coordinator.activeMode());
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.startCount);
    void* provisioningOwner = fixture.link.packetContext;
    CHECK_TRUE(provisioningOwner != nullptr);

    // Exact profile replay is idempotent and does not disconnect.
    CHECK_TRUE(fixture.coordinator.switchTo(provisioning));
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.startCount);
    CHECK_EQ(static_cast<uint32_t>(0U), fixture.link.stopCount);

    CHECK_TRUE(fixture.coordinator.switchTo(direct));
    CHECK_EQ(blinker::BleModeCoordinatorState::Direct,
             fixture.coordinator.state());
    CHECK_TRUE(fixture.link.packetContext != nullptr);
    CHECK_TRUE(fixture.link.packetContext != provisioningOwner);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.startCount);

    fixture.link.startsToFail = 1U;
    const blinker::ble::ModeProfile newProvisioning =
        provisioningProfile(0x21U);
    CHECK_EQ(
        blinker::ErrorCode::InternalError,
        fixture.coordinator.switchTo(newProvisioning).code());
    CHECK_EQ(blinker::BleModeCoordinatorState::Direct,
             fixture.coordinator.state());
    CHECK_EQ(blinker::ble::ApplicationMode::Direct,
             fixture.coordinator.activeMode());
    CHECK_EQ(static_cast<uint32_t>(1U),
             fixture.coordinator.rollbackCount());
    CHECK_EQ(blinker::ErrorCode::InternalError,
             fixture.coordinator.lastTransitionError());
    CHECK_TRUE(blinker::ble::modeProfilesEqual(
        direct,
        fixture.link.bleProfile()));
    CHECK_EQ(static_cast<uint32_t>(4U), fixture.preparation.callCount);
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.preparation.rollbackCount);
    CHECK_TRUE(blinker::ble::modeProfilesEqual(
        direct,
        fixture.preparation.lastProfile));
    fixture.coordinator.stop();
    CHECK_EQ(blinker::BleModeCoordinatorState::Stopped,
             fixture.coordinator.state());
}

void testIdleProfileRefreshAvoidsRadioRestart() {
    ModeFixture fixture;
    fixture.link.refreshSupported = true;
    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    CHECK_TRUE(fixture.coordinator.start(direct));

    uint8_t locator[blinker::kSetupSessionLocatorSize] = {
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    const blinker::ble::ModeProfile refreshed =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    CHECK_TRUE(fixture.coordinator.refreshActive(refreshed));
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.refreshCount);
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.startCount);
    CHECK_EQ(static_cast<uint32_t>(0U), fixture.link.stopCount);
    CHECK_TRUE(blinker::ble::modeProfilesEqual(
        refreshed, fixture.coordinator.activeProfile()));

    // Ports without a live update use the existing safe mode switch.
    fixture.link.refreshSupported = false;
    locator[0] = 9U;
    const blinker::ble::ModeProfile fallback =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    CHECK_TRUE(fixture.coordinator.refreshActive(fallback));
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.refreshCount);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.startCount);
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.stopCount);
}

void testPreparationFailureRollsBackBeforeAdvertising() {
    ModeFixture fixture;
    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    const blinker::ble::ModeProfile provisioning =
        provisioningProfile(0x31U);
    CHECK_TRUE(fixture.coordinator.start(direct));
    fixture.preparation.callsToFail = 1U;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        fixture.coordinator.switchTo(provisioning).code());
    CHECK_EQ(blinker::BleModeCoordinatorState::Direct,
             fixture.coordinator.state());
    CHECK_EQ(static_cast<uint32_t>(1U),
             fixture.coordinator.rollbackCount());
    CHECK_EQ(static_cast<uint32_t>(1U),
             fixture.preparation.rollbackCount);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.link.startCount);
    CHECK_TRUE(blinker::ble::modeProfilesEqual(
        direct,
        fixture.link.bleProfile()));
}

void testInvalidTargetAndFailedRollback() {
    ModeFixture fixture;
    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    CHECK_TRUE(fixture.coordinator.start(direct));
    const uint32_t stopsBeforeInvalid = fixture.link.stopCount;
    blinker::ble::ModeProfile invalid = provisioningProfile(1U);
    invalid.modeLocator[0] = 0U;
    for (size_t index = 1U;
         index < sizeof(invalid.modeLocator);
         ++index) {
        invalid.modeLocator[index] = 0U;
    }
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        fixture.coordinator.switchTo(invalid).code());
    CHECK_EQ(stopsBeforeInvalid, fixture.link.stopCount);
    CHECK_EQ(blinker::BleModeCoordinatorState::Direct,
             fixture.coordinator.state());

    fixture.link.startsToFail = 2U;
    CHECK_EQ(
        blinker::ErrorCode::InternalError,
        fixture.coordinator.switchTo(provisioningProfile(0x41U)).code());
    CHECK_EQ(blinker::BleModeCoordinatorState::Error,
             fixture.coordinator.state());
    CHECK_EQ(blinker::ble::ApplicationMode::None,
             fixture.coordinator.activeMode());
    CHECK_EQ(static_cast<uint32_t>(1U),
             fixture.coordinator.rollbackCount());
    fixture.coordinator.stop();
    CHECK_EQ(blinker::BleModeCoordinatorState::Stopped,
             fixture.coordinator.state());
}

void testDirectLinkFailureBecomesCoordinatorError() {
    ModeFixture fixture;
    CHECK_TRUE(fixture.coordinator.start(
        blinker::ble::makeDirectModeProfile()));
    fixture.link.currentState = blinker::BleLinkState::Error;
    fixture.link.currentError = blinker::ErrorCode::AuthenticationRequired;
    fixture.coordinator.poll(1000U);
    CHECK_EQ(
        blinker::BleModeCoordinatorState::Error,
        fixture.coordinator.state());
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.coordinator.lastTransitionError());
    fixture.coordinator.stop();
    CHECK_EQ(
        blinker::BleModeCoordinatorState::Stopped,
        fixture.coordinator.state());
}

} // namespace

int main() {
    testAdvertisingContract();
    testAuthorizedPresenceGolden();
    testPresenceNonceGeneration();
    testConstructionDoesNotStealCallbacks();
    testDirectFrameTransportDoesNotClaimAuthentication();
    testRestartActiveRepreparesExactProfile();
    testSwitchAndSuccessfulRollback();
    testIdleProfileRefreshAvoidsRadioRestart();
    testPreparationFailureRollsBackBeforeAdvertising();
    testInvalidTargetAndFailedRollback();
    testDirectLinkFailureBecomesCoordinatorError();
    std::cout << "BLE mode ownership tests passed; coordinator="
              << sizeof(blinker::BleModeCoordinator)
              << " B; profile=" << sizeof(blinker::ble::ModeProfile)
              << " B" << std::endl;
    return 0;
}
