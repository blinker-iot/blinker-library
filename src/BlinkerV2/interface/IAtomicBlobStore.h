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
