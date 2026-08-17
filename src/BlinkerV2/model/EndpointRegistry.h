#ifndef BLINKER_MODEL_ENDPOINT_REGISTRY_H
#define BLINKER_MODEL_ENDPOINT_REGISTRY_H

#include "../core/Result.h"
#include "Endpoint.h"

namespace blinker {

Result validateEndpointDescriptor(const EndpointDescriptor& descriptor);

typedef const EndpointDescriptor* (*EndpointDescriptorAccessor)(
    const void* context,
    size_t index);

// A zero-copy immutable descriptor source. It allows a public DeviceSchema to
// compose const interaction-local FieldSpec arrays without copying them into a
// contiguous SRAM table. The context and every returned descriptor must
// outlive the registry.
struct EndpointDescriptorTable {
    const void* context;
    EndpointDescriptorAccessor accessor;
    size_t count;

    constexpr EndpointDescriptorTable()
        : context(nullptr), accessor(nullptr), count(0U) {}

    constexpr EndpointDescriptorTable(
        const void* tableContext,
        EndpointDescriptorAccessor tableAccessor,
        size_t descriptorCount)
        : context(tableContext),
          accessor(tableAccessor),
          count(descriptorCount) {}
};

class EndpointRegistry {
public:
    // Preferred device-v2 path: descriptors are a contiguous immutable table
    // owned by the application (normally const/ROM). No per-field pointer
    // table is retained in SRAM.
    EndpointRegistry(const EndpointDescriptor* descriptors, size_t count);

    // Public interaction-schema path. Descriptor order remains the canonical
    // field-ID order and is sealed with the same validation as a contiguous
    // table.
    explicit EndpointRegistry(const EndpointDescriptorTable& table);

    // Advanced builder for applications that construct a schema before
    // begin(). Canonical Client does not retain this per-field pointer table.
    EndpointRegistry(const EndpointDescriptor** storage, size_t capacity);

    Result add(const EndpointDescriptor& descriptor);
    // Endpoint order defines endpoint IDs and contributes to the manifest
    // fingerprint. Once sealed, the registry stays immutable for the rest of
    // its lifetime, including across Runtime stop/start cycles.
    Result seal();
    const EndpointDescriptor* find(StringView key) const;
    const EndpointDescriptor* findById(uint16_t id) const;
    Result idOf(StringView key, uint16_t& id) const;
    const EndpointDescriptor* at(size_t index) const;

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool sealed() const { return sealed_; }

private:
    static const EndpointDescriptor* contiguousAt(
        const void* context,
        size_t index);

    const void* descriptorContext_;
    EndpointDescriptorAccessor descriptorAccessor_;
    const EndpointDescriptor** storage_;
    size_t capacity_;
    size_t size_;
    bool sealed_;
    bool staticTable_;
};

} // namespace blinker

#endif
