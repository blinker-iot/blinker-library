#ifndef BLINKER_INTERFACE_IATOMICBLOBSTORE_H
#define BLINKER_INTERFACE_IATOMICBLOBSTORE_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class StorageProtection : uint8_t {
    PlainFlash = 0,
    EncryptedFlash,
    HardwareBackedSealed
};

// One crash-consistent blob. replace()/clear() must expose either the complete
// previous value or the complete requested value after reset/power loss.
// Success is the durable commit boundary. Failure may leave either value;
// callers must not assume rollback or perform an external action on that basis.
// clear() may return NotFound for an already absent blob ONLY after durable
// absence is established (including any required commit); this one result is
// an idempotent clear, not an uncertain write failure.
// A load on the same failed handle is not by itself proof of durability (e.g.
// an uncommitted NVS view). Recovery must re-establish the port's commit state.
class IAtomicBlobStore {
public:
    virtual ~IAtomicBlobStore() {}

    virtual Result load(MutableByteSpan output, size_t& written) = 0;
    virtual Result replace(ByteView value) = 0;
    virtual Result clear() = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
