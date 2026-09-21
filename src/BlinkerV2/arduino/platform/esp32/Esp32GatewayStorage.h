#ifndef BLINKER_V2_ARDUINO_ESP32_GATEWAY_STORAGE_H
#define BLINKER_V2_ARDUINO_ESP32_GATEWAY_STORAGE_H

#include "../../ports/esp32/Esp32Storage.h"
#include <BlinkerV2/identity/GatewayChildRecords.h>

namespace blinker {

// Durable bindings, NOT native connections. Keep slot zero's deployed names
// and codec; no migration, partition erase, dynamic allocation or secret cache.
// Two bindings are the first bounded target. BLE workspace capacity is separate.
template <typename AccessBlob = Esp32NvsAtomicBlobStore, typename RenewalBlob = AccessBlob>
class BasicEsp32GatewayStorage {
public:
    enum : size_t { capacity = 2U };

    explicit BasicEsp32GatewayStorage(const Esp32NvsBlobStoreConfig& config)
        : slots_{{config, "child", "pending"}, {config, "child_1", "pending_1"}} {}

    Result begin() {
        for (size_t i = 0U; i < capacity; ++i) {
            slots_[i].records.invalidate();
            Result result = slots_[i].access.begin();
            if (result) result = slots_[i].renewal.begin();
            if (!result) { end(); return result; }
        }
        return Result::success();
    }

    void end() {
        for (size_t i = 0U; i < capacity; ++i) {
            slots_[i].records.invalidate();
            slots_[i].renewal.end();
            slots_[i].access.end();
        }
    }

    GatewayChildRecords* records(size_t index) {
        return index < capacity ? &slots_[index].records : nullptr;
    }

    // Explicit Hub access reset. The caller may retire the Hub root ONLY after
    // all journals and active/tombstone records are durably removed. A partial
    // failure is returned, not rolled back; retry includes already-empty slots.
    Result clear() {
        for (size_t i = 0U; i < capacity; ++i) {
            Result result = slots_[i].records.renewal.clear();
            if (result) result = slots_[i].records.access.clear();
            if (!result) return result;
        }
        return Result::success();
    }

    BasicEsp32GatewayStorage(const BasicEsp32GatewayStorage&) = delete;
    BasicEsp32GatewayStorage& operator=(const BasicEsp32GatewayStorage&) = delete;

private:
    static Esp32NvsBlobStoreConfig blobConfig(Esp32NvsBlobStoreConfig config,
                                             const char* name, const char* key, size_t size) {
        config.namespaceName = name; config.key = key; config.maximumSize = size;
        return config;
    }

    struct Slot {
        AccessBlob access;
        RenewalBlob renewal;
        GatewayChildRecords records;
        Slot(const Esp32NvsBlobStoreConfig& config, const char* accessKey, const char* renewalKey)
            : access(blobConfig(config, "bl_eh_access", accessKey, GatewayAccessStore::serializedSize)),
              renewal(blobConfig(config, "bl_eh_renew", renewalKey, GatewayCredentialRenewalStore::serializedSize)),
              records(access, renewal) {}
    };
    Slot slots_[capacity];
};

} // namespace blinker
#endif
