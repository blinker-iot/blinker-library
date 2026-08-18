#ifndef BLINKER_OFFICIAL_ESP32_STORAGE_H
#define BLINKER_OFFICIAL_ESP32_STORAGE_H

#include <BlinkerEsp32StorageAdapter.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline Esp32NvsBlobStoreConfig esp32BlobStorageConfig(
    const char* namespaceName,
    const char* key,
    size_t maximumSize) {
    Esp32NvsBlobStoreConfig config;
    config.namespaceName = namespaceName;
    config.key = key;
    config.maximumSize = maximumSize;
    return config;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
