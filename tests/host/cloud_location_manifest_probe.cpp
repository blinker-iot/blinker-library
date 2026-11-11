#include <cstdio>

#include "../arduino/CompileCloudLocation/Schema.h"
#include "BlinkerV2/core/Sha256.h"
#include "BlinkerV2/model/EndpointRegistry.h"
#include "BlinkerV2/model/Manifest.h"

namespace {

bool require(blinker::Result result, const char* stage) {
    if (result) return true;
    std::fprintf(stderr, "%s failed (code=%u)\n", stage,
                 static_cast<unsigned>(result.code()));
    return false;
}

void printHex(blinker::ByteView bytes) {
    for (size_t index = 0U; index < bytes.size; ++index) {
        std::printf("%02x", static_cast<unsigned>(bytes.data[index]));
    }
}

} // namespace

int main() {
    namespace candidate = blinker::cloud_location_candidate;
    // The MCU sketch includes this exact immutable schema; no duplicate field
    // model or independent CBOR implementation is used to derive admission.
    blinker::EndpointRegistry registry(candidate::fields, candidate::fieldCount);
    if (!require(registry.seal(), "seal")) return 1;
    uint16_t endpointId = 0U;
    if (!require(registry.idOf(blinker::StringView(candidate::locationKey), endpointId), "location id")) return 1;
    if (endpointId != candidate::locationEndpointId) {
        std::fprintf(stderr, "location endpoint differs from the candidate contract\n");
        return 1;
    }
    uint8_t fingerprint[blinker::kSha256Size] = {};
    if (!require(blinker::computeManifestFingerprint(candidate::manifestRevision, registry,
            blinker::MutableByteSpan(fingerprint, sizeof(fingerprint))), "fingerprint")) return 1;
    uint8_t storage[1024] = {};
    blinker::ByteView manifest;
    if (!require(blinker::encodeManifest(candidate::manifestRevision, registry,
            blinker::MutableByteSpan(storage, sizeof(storage)), manifest), "manifest")) return 1;
    std::printf("{\"revision\":%lu,\"fieldCount\":%zu,\"endpointId\":%u,\"manifestFingerprint\":\"",
                static_cast<unsigned long>(candidate::manifestRevision), registry.size(),
                static_cast<unsigned>(endpointId));
    printHex(blinker::ByteView(fingerprint, sizeof(fingerprint)));
    std::printf("\",\"canonicalManifestHex\":\"");
    printHex(manifest);
    std::printf("\"}\n");
    return 0;
}
