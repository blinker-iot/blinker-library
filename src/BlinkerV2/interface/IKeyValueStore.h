#ifndef BLINKER_INTERFACE_IKEYVALUESTORE_H
#define BLINKER_INTERFACE_IKEYVALUESTORE_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

class IKeyValueStore {
public:
    virtual ~IKeyValueStore() {}

    virtual Result get(StringView key, MutableByteSpan output, size_t& written) = 0;
    virtual Result set(StringView key, ByteView value) = 0;
    virtual Result remove(StringView key) = 0;
    virtual Result commit() = 0;
};

} // namespace blinker

#endif
