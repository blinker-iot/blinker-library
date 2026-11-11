#ifndef BLINKER_TEST_R4_STORAGE_GATE_H
#define BLINKER_TEST_R4_STORAGE_GATE_H

#include "StorageGateCommon.h"
#include <Preferences.h>

namespace storage_gate {
namespace port {

#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
enum : size_t { keyCount = 12U, maximumSnapshot = 2640U, maximumRecord = 664U };
static const char* const snapshotMagic = "R4K2";
#else
enum : size_t { keyCount = 10U, maximumSnapshot = 1306U, maximumRecord = 280U };
static const char* const snapshotMagic = "R4K1";
#endif
static const char* const name = "uno-r4-owned-preferences";
static const char* const keys[keyCount] = {
    "ws0", "ws1", "w00", "w01", "w10", "w11", "di0", "di1", "ac0", "ac1"
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    , "ts0", "ts1"
#endif
};
static const uint16_t payloadLimits[keyCount / 2U] = {16U, 116U, 116U, 28U, 260U
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    , 644U
#endif
};
static const uint8_t legacyIds[keyCount / 2U] = {1U, 2U, 3U, 6U, 0xffU
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    , 0xffU
#endif
};

struct Slot {
    const uint8_t* bytes;
    uint16_t size;
    bool present;
};

// Preferences::getType() folds transport failure into PT_INVALID. Confirm the
// actual AT transaction before interpreting an absent key; never guess absence.
inline bool decimal(const std::string& text, uint32_t maximum, uint32_t& value) {
    value = 0U;
    if (text.empty()) return false;
    for (size_t i = 0U; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        const uint32_t digit = static_cast<uint32_t>(text[i] - '0');
        if (digit > maximum || value > (maximum - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    return true;
}
inline bool confirmedType(const char* key, uint32_t& type) {
    std::string response;
    return modem.write(std::string(PROMPT(_PREF_TYPE)), response, "%s%s\r\n",
                       CMD_WRITE(_PREF_TYPE), key) && decimal(response, PT_INVALID, type);
}
inline bool confirmedLength(const char* key, uint32_t maximum, uint32_t& size) {
    std::string response;
    return modem.write(std::string(PROMPT(_PREF_LEN)), response, "%s%s\r\n",
                       CMD_WRITE(_PREF_LEN), key) && decimal(response, maximum, size);
}
inline bool readSlot(Preferences& preferences, size_t index,
                     uint8_t* output, uint16_t& size, bool& present) {
    uint32_t type = 0U, length = 0U;
    size = 0U; present = false;
    if (!confirmedType(keys[index], type)) return false;
    if (type == PT_INVALID) return true;
    if (type != PT_BLOB || !confirmedLength(keys[index], payloadLimits[index / 2U] + 20U, length) ||
        length < 20U || preferences.getBytes(keys[index], output, maximumRecord) != length) return false;
    size = static_cast<uint16_t>(length); present = true;
    return true;
}
inline bool validEnvelope(const Slot& slot, size_t index) {
    if (!slot.present) return slot.size == 0U;
    if (slot.size < 20U || slot.size > payloadLimits[index / 2U] + 20U) return false;
    const uint8_t* bytes = slot.bytes;
    const uint16_t payload = big16(bytes + 12U);
    const uint8_t id = static_cast<uint8_t>(index / 2U);
    return memcmp(bytes, "BRJ2", 4U) == 0 && bytes[4] == 1U &&
           (bytes[5] == id || (id < 5U && bytes[5] == legacyIds[id])) && bytes[7] == 0U &&
           bytes[14] == 0U && bytes[15] == 0U && big32(bytes + 8U) != 0U &&
           payload <= payloadLimits[id] && slot.size == payload + 20U &&
           ((bytes[6] == 1U && payload != 0U) || (bytes[6] == 2U && payload == 0U)) &&
           big32(bytes + 16U + payload) == crc32(bytes, 16U + payload);
}
inline bool decode(const uint8_t* data, size_t size, Slot (&slots)[keyCount], uint8_t* identity) {
    if (size < 4U + 3U * keyCount || size > maximumSnapshot || memcmp(data, snapshotMagic, 4U) != 0) return false;
    size_t cursor = 4U;
    for (size_t index = 0U; index < keyCount; ++index) {
        if (cursor > size || size - cursor < 3U || data[cursor] > 1U) return false;
        slots[index].present = data[cursor] == 1U;
        slots[index].size = little16(data + cursor + 1U);
        cursor += 3U;
        if (slots[index].size > size - cursor) return false;
        slots[index].bytes = data + cursor;
        if (!validEnvelope(slots[index], index)) return false;
        cursor += slots[index].size;
    }
    if (cursor != size) return false;
    for (size_t pair = 0U; pair < keyCount; pair += 2U) {
        if (slots[pair].present && slots[pair + 1U].present &&
            big32(slots[pair].bytes + 8U) == big32(slots[pair + 1U].bytes + 8U)) return false;
    }
    const Slot* anchor = nullptr;
    for (size_t index = 6U; index < 8U; ++index) {
        const Slot& item = slots[index];
        if (!item.present) continue;
        if (anchor && big32(item.bytes + 8U) == big32(anchor->bytes + 8U)) return false;
        if (!anchor || big32(item.bytes + 8U) > big32(anchor->bytes + 8U)) anchor = &item;
    }
    return anchor != nullptr && anchor->bytes[6] == 1U &&
           instanceAnchor(anchor->bytes + 16U, big16(anchor->bytes + 12U), identity);
}
inline bool snapshot(uint8_t* output, size_t& size) {
    Preferences preferences;
    modem.noDebug();
    if (!preferences.begin("bl_v2", true)) return false;
    modem.timeout(1000U);
    memcpy(output, snapshotMagic, 4U);
    size = 4U;
    bool success = true;
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    // Maintenance-only optional workspace; keep multi-kilobyte raw snapshots
    // off the SDK's nominal 1 KiB main stack. No product includes this gate.
    static uint8_t bytes[maximumRecord] = {};
#else
    uint8_t bytes[maximumRecord] = {};
#endif
    for (size_t index = 0U; index < keyCount && success; ++index) {
        uint16_t length = 0U; bool present = false;
        success = readSlot(preferences, index, bytes, length, present);
        if (!success || size + 3U + length > maximumSnapshot) { success = false; break; }
        output[size] = present ? 1U : 0U;
        putLittle16(output + size + 1U, length);
        if (length != 0U) memcpy(output + size + 3U, bytes, length);
        size += 3U + length;
        wipe(bytes, sizeof(bytes));
    }
    wipe(bytes, sizeof(bytes));
    preferences.end();
    Slot slots[keyCount]; uint8_t identity[16U] = {};
    success = success && decode(output, size, slots, identity);
    wipe(identity, sizeof(identity));
    return success;
}
inline bool matchesSnapshot(const uint8_t* expected, size_t expectedSize) {
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    static uint8_t second[maximumSnapshot] = {};
#else
    uint8_t second[maximumSnapshot] = {};
#endif
    size_t size = 0U;
    const bool result = snapshot(second, size) && size == expectedSize &&
                        memcmp(second, expected, size) == 0;
    wipe(second, sizeof(second));
    return result;
}
inline bool validateTarget(const uint8_t* desired, size_t desiredSize,
                           const uint8_t* current, size_t currentSize) {
    Slot slots[keyCount]; uint8_t desiredIdentity[16U] = {}, currentIdentity[16U] = {};
    const bool result = decode(desired, desiredSize, slots, desiredIdentity) &&
                        decode(current, currentSize, slots, currentIdentity) &&
                        memcmp(desiredIdentity, currentIdentity, 16U) == 0;
    wipe(desiredIdentity, sizeof(desiredIdentity)); wipe(currentIdentity, sizeof(currentIdentity));
    return result;
}
inline bool restore(const uint8_t* desired, size_t desiredSize,
                    const uint8_t* current, size_t currentSize) {
    Slot wanted[keyCount], before[keyCount]; uint8_t identity[16U] = {};
    const bool valid = validateTarget(desired, desiredSize, current, currentSize) &&
                       decode(desired, desiredSize, wanted, identity) &&
                       decode(current, currentSize, before, identity);
    wipe(identity, sizeof(identity));
    if (!valid) return false;
    Preferences preferences;
    modem.noDebug();
    if (!preferences.begin("bl_v2", false)) return false;
    modem.timeout(1000U);
    bool success = true;
#if defined(BLINKER_INTERNAL_R4_TASK_STORAGE_BACKUP)
    static uint8_t bytes[maximumRecord] = {};
#else
    uint8_t bytes[maximumRecord] = {};
#endif
    for (size_t index = 0U; index < keyCount && success; ++index) {
        uint16_t size = 0U; bool present = false;
        success = readSlot(preferences, index, bytes, size, present) &&
                  present == before[index].present && size == before[index].size &&
                  memcmp(bytes, before[index].bytes, size) == 0;
        if (!success) break;
        const Slot& next = wanted[index];
        if (present != next.present || size != next.size || memcmp(bytes, next.bytes, size) != 0) {
            success = next.present
                ? preferences.putBytes(keys[index], next.bytes, next.size) == next.size
                : preferences.remove(keys[index]);
        }
        if (success) success = readSlot(preferences, index, bytes, size, present) &&
                              present == next.present && size == next.size &&
                              memcmp(bytes, next.bytes, size) == 0;
        wipe(bytes, sizeof(bytes));
    }
    wipe(bytes, sizeof(bytes)); preferences.end();
    return success;
}

} // namespace port
} // namespace storage_gate
#endif
