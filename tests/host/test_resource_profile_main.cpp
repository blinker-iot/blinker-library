#include <BlinkerV2/api/Client.h>
#include <BlinkerV2/api/Device.h>
#include <BlinkerV2/runtime/WifiCloudLifecycle.h>

#include <cstdlib>
#include <iostream>

namespace {

void check(bool result) {
    if (!result) {
        std::cerr << "Resource profile admission check failed" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

class FramePort final : public blinker::IFrameTransport {
public:
    FramePort() : started(false), polls(0U), sends(0U), lastTransport(255U) {}
    blinker::Result start() override {
        started = true;
        return blinker::Result::success();
    }
    void stop() override { started = false; }
    void poll(uint32_t) override { ++polls; }
    blinker::TransportState state() const override {
        return started ? blinker::TransportState::Online
                       : blinker::TransportState::Stopped;
    }
    blinker::TransportCapabilities capabilities() const override {
        blinker::TransportCapabilities result;
        result.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
        result.features = blinker::TransportFeatureBinary;
        return result;
    }
    blinker::Result send(
        blinker::ByteView frame, const blinker::SendTarget& target) override {
        check(started && !frame.empty());
        check(frame.size <= BLINKER_DEVICE_FRAME_SIZE);
        ++sends;
        lastTransport = target.transportId;
        return blinker::Result::success();
    }
    void setReceiver(blinker::FrameReceiver, void*) override {}

    bool started;
    uint32_t polls;
    uint32_t sends;
    uint8_t lastTransport;
};

} // namespace

int main() {
#if BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL
    static_assert(BLINKER_DEVICE_FRAME_SIZE == 256U,
                  "A transport capacity change must not select Standard frames");
    static_assert(BLINKER_DEVICE_MAX_FIELDS == 24U,
                  "A transport capacity change must not enlarge the schema");
#endif
#if UINTPTR_MAX <= UINT32_MAX && \
    BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL && \
    BLINKER_MAX_TRANSPORTS == 3 && BLINKER_MAX_PEER_SESSIONS == 3
    static_assert(sizeof(blinker::Device) <= 1760U,
                  "Small three-path Device exceeded its measured ABI budget");
#endif
    // Use the real Client + Runtime + TransportHub, compiled with the same
    // profile. These are admission/ABI probes, not LAN wire/security mocks.
    FramePort ports[BLINKER_MAX_TRANSPORTS + 1U];
    const blinker::EndpointDescriptor field(
        blinker::StringView("power", 5U), blinker::EndpointKind::Property,
        blinker::ValueType::Boolean, blinker::AccessRead);
    typedef blinker::ClientStorage<24U, 256U, 192U, 32U> FixedStorage;
    blinker::ClientStorage<1U, BLINKER_DEVICE_FRAME_SIZE, 64U, 8U> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    config.maxReassemblySize = BLINKER_DEVICE_FRAME_SIZE;
    blinker::Client client(storage.buffers(&field, 1U), config);
    for (size_t index = 0U; index < BLINKER_MAX_TRANSPORTS; ++index) {
        check(client.addTransport(ports[index]).ok());
    }
    check(client.addTransport(ports[BLINKER_MAX_TRANSPORTS]).code() ==
          blinker::ErrorCode::CapacityExceeded);
    check(client.begin().ok());
    client.poll(1000U);
    for (size_t index = 0U; index < BLINKER_MAX_TRANSPORTS; ++index) {
        check(ports[index].polls == 1U && ports[index].sends == 1U);
        check(ports[index].lastTransport == index);
    }
    client.end();
    for (size_t index = 0U; index < BLINKER_MAX_TRANSPORTS; ++index) {
        check(!ports[index].started);
    }
    std::cout << "pointer=" << sizeof(void*)
              << " profile=" << BLINKER_RESOURCE_PROFILE
              << " transports=" << BLINKER_MAX_TRANSPORTS
              << " peers=" << BLINKER_MAX_PEER_SESSIONS
              << " frame=" << BLINKER_DEVICE_FRAME_SIZE
              << " Client=" << sizeof(blinker::Client)
              << " Runtime=" << sizeof(blinker::DeviceRuntime)
              << " Device=" << sizeof(blinker::Device)
              << " TransportHub=" << sizeof(blinker::TransportHub)
              << " fixedStorage24=" << sizeof(FixedStorage)
              << " WifiLifecycle=" << sizeof(blinker::WifiCloudLifecycle)
              << std::endl;
    return EXIT_SUCCESS;
}
