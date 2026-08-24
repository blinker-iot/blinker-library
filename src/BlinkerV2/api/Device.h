#ifndef BLINKER_API_DEVICE_H
#define BLINKER_API_DEVICE_H

#include "Client.h"
#include "DeviceSchema.h"
#include "Interactions.h"
#include "../core/ResourceProfile.h"

namespace blinker {

class Product;

// Schema revisions are application concerns. Product/model membership is
// server-owned enrollment metadata and must not be asserted by a Sketch or
// repeated in the data-plane HELLO. Protocol feature bits, frame sizes and
// scratch capacities are deliberately derived by Device from the active
// build-wide resource profile instead of being sketch magic numbers.
struct DeviceConfig {
    uint32_t manifestRevision;
    uint32_t initialStateRevision;

    constexpr DeviceConfig(
        uint32_t fieldManifestRevision = 1U,
        uint32_t restoredStateRevision = 0U)
        : manifestRevision(fieldManifestRevision),
          initialStateRevision(restoredStateRevision) {}
};

typedef ClientStorage<
    BLINKER_DEVICE_MAX_FIELDS,
    BLINKER_DEVICE_FRAME_SIZE,
    BLINKER_DEVICE_PATCH_SIZE,
    BLINKER_DEVICE_STATE_ARENA_SIZE>
    DeviceStorage;

// Owning public facade. FieldSpec tables remain immutable application
// descriptors (normally static const/Flash), while all mutable
// endpoint slots and protocol scratch are held here with profile capacities.
// Use Client + ClientStorage from BlinkerV2Advanced.h only when the active
// profile cannot describe a product's exceptional storage budget.
class Device {
public:
    template <size_t N>
    Device(
        const FieldSpec (&fields)[N],
        const DeviceConfig& config = DeviceConfig(),
        Diagnostics* diagnostics = nullptr)
        : storage_(),
          client_(
              storage_.buffers(fields),
              runtimeConfig(config),
              diagnostics),
          schemaBinder_(nullptr),
          schemaContext_(nullptr),
          interactionsBound_(true) {
        static_assert(
            N <= BLINKER_DEVICE_MAX_FIELDS,
            "FieldSpec table exceeds the active Device resource profile");
    }

    template <typename... Interactions>
    Device(
        const DeviceSchema<Interactions...>& schema,
        const DeviceConfig& config = DeviceConfig(),
        Diagnostics* diagnostics = nullptr)
        : storage_(),
          client_(
              storage_.buffers(schema.endpointTable()),
              runtimeConfig(config),
              diagnostics),
          schemaBinder_(&Device::bindSchema<Interactions...>),
          schemaContext_(&schema),
          interactionsBound_(false) {
        static_assert(
            DeviceSchema<Interactions...>::fieldCount > 0U,
            "DeviceSchema must contain at least one interaction field");
        static_assert(
            DeviceSchema<Interactions...>::fieldCount <=
                BLINKER_DEVICE_MAX_FIELDS,
            "DeviceSchema exceeds the active Device field profile");
    }

    // Device keeps zero-copy pointers into schema.
    // Reject inline makeDeviceSchema(...) temporaries at compile time.
    template <typename... Interactions>
    Device(
        DeviceSchema<Interactions...>&&,
        const DeviceConfig& = DeviceConfig()) = delete;
    template <typename... Interactions>
    Device(
        const DeviceSchema<Interactions...>&&,
        const DeviceConfig& = DeviceConfig()) = delete;

    EndpointCatalog& endpoints() { return client_.endpoints(); }
    const EndpointRegistry& endpointRegistry() const {
        return client_.endpointRegistry();
    }

    Result addTransport(IFrameTransport& transport) {
        return client_.addTransport(transport);
    }
    Result begin() {
        Result result = prepare();
        return result ? client_.begin() : result;
    }
    void end() { client_.end(); }
    void run(uint32_t totalBudgetMicros = 1000U) {
        client_.poll(totalBudgetMicros);
    }
    void poll(uint32_t totalBudgetMicros) {
        client_.poll(totalBudgetMicros);
    }
    ClientStatus status() const { return client_.status(); }
    const OutboundSchedulerCounters& outboundCounters() const {
        return client_.outboundCounters();
    }
    const TelemetryCounters& telemetryCounters() const {
        return client_.telemetryCounters();
    }
    Result transportStatus(
        size_t index,
        TransportState& state,
        TransportCapabilities& capabilities) const {
        return client_.transportStatus(index, state, capabilities);
    }

private:
    Result prepare() {
        if (!interactionsBound_ && schemaBinder_ != nullptr) {
            Result result = schemaBinder_(
                schemaContext_,
                *this);
            if (!result) return result;
            interactionsBound_ = true;
        }
        return client_.prepare();
    }

    Client& client() { return client_; }

    typedef Result (*SchemaBinder)(const void* context, Device& device);

    class AutoBinder {
    public:
        explicit AutoBinder(Device& device) : device_(device) {}

        template <typename Interaction>
        Result bind(Interaction& interaction) {
            return device_.autoBind(interaction);
        }

        template <typename Interaction>
        Result bind(const Interaction&) {
            return Result::failure(ErrorCode::InvalidArgument);
        }

    private:
        Device& device_;
    };

    template <typename... Interactions>
    static Result bindSchema(const void* context, Device& device) {
        if (context == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        const DeviceSchema<Interactions...>* schema =
            static_cast<const DeviceSchema<Interactions...>*>(context);
        AutoBinder binder(device);
        return schema->bindAll(binder);
    }

    template <typename Interaction>
    Result autoBind(Interaction& interaction) {
        return interaction.bind(client_);
    }

    static DeviceRuntimeConfig runtimeConfig(const DeviceConfig& config) {
        DeviceRuntimeConfig runtime;
        runtime.maxFrameSize =
            static_cast<uint16_t>(BLINKER_DEVICE_FRAME_SIZE);
        runtime.maxReassemblySize = BLINKER_DEVICE_FRAME_SIZE;
        runtime.manifestRevision = config.manifestRevision;
        runtime.initialStateRevision = config.initialStateRevision;
        return runtime;
    }

    DeviceStorage storage_;
    Client client_;
    SchemaBinder schemaBinder_;
    const void* schemaContext_;
    bool interactionsBound_;

    Device(const Device&);
    Device& operator=(const Device&);

    friend class Product;
};

#if UINTPTR_MAX <= UINT32_MAX
#if BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL && \
    BLINKER_DEVICE_MAX_FIELDS == 24 && BLINKER_DEVICE_FRAME_SIZE == 256 && \
    BLINKER_DEVICE_PATCH_SIZE == 192 && \
    BLINKER_DEVICE_STATE_ARENA_SIZE == 192
static_assert(
    sizeof(Device) <= 1664U,
    "Small public Device exceeds its 32-bit default profile gate");
#elif BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_STANDARD && \
    BLINKER_DEVICE_MAX_FIELDS == 32 && BLINKER_DEVICE_FRAME_SIZE == 1024 && \
    BLINKER_DEVICE_PATCH_SIZE == 512 && \
    BLINKER_DEVICE_STATE_ARENA_SIZE == 512
static_assert(
    sizeof(Device) <= 3448U,
    "Standard public Device exceeds its 32-bit default profile gate");
#elif BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_LARGE && \
    BLINKER_DEVICE_MAX_FIELDS == 64 && BLINKER_DEVICE_FRAME_SIZE == 4096 && \
    BLINKER_DEVICE_PATCH_SIZE == 2048 && \
    BLINKER_DEVICE_STATE_ARENA_SIZE == 2048
static_assert(
    sizeof(Device) <= 10980U,
    "Large public Device exceeds its 32-bit default profile gate");
#endif
#endif

} // namespace blinker

#endif
