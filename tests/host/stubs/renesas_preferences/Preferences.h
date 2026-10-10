#ifndef TEST_RENESAS_PREFERENCES_H
#define TEST_RENESAS_PREFERENCES_H

#include <stddef.h>
#include <stdint.h>

#include <cstring>

namespace fake_preferences {

enum PutMode {
    PutNormally = 0,
    RejectNextPut,
    CorruptNextPut,
    ApplyButReportFailure
};

struct Entry {
    char key[16];
    uint8_t value[300];
    size_t size;
    bool present;
};

struct State {
    Entry entries[20];
    PutMode nextPutMode;
    bool beginAllowed;
    unsigned beginCalls;
    unsigned endCalls;

    State() { reset(); }

    void reset() {
        std::memset(this, 0, sizeof(*this));
        beginAllowed = true;
    }
};

inline State& state() {
    static State value;
    return value;
}

inline void reset() { state().reset(); }

inline Entry* find(const char* key) {
    State& current = state();
    for (size_t index = 0U;
         index < sizeof(current.entries) / sizeof(current.entries[0]);
         ++index) {
        if (current.entries[index].present &&
            std::strcmp(current.entries[index].key, key) == 0) {
            return &current.entries[index];
        }
    }
    return nullptr;
}

inline Entry* allocate(const char* key) {
    Entry* existing = find(key);
    if (existing != nullptr) return existing;
    State& current = state();
    for (size_t index = 0U;
         index < sizeof(current.entries) / sizeof(current.entries[0]);
         ++index) {
        Entry& entry = current.entries[index];
        if (!entry.present) {
            size_t length = std::strlen(key);
            if (length >= sizeof(entry.key)) length = sizeof(entry.key) - 1U;
            std::memcpy(entry.key, key, length);
            entry.key[length] = '\0';
            entry.present = true;
            entry.size = 0U;
            return &entry;
        }
    }
    return nullptr;
}

inline void failNext(PutMode mode) { state().nextPutMode = mode; }

inline bool corrupt(const char* key) {
    Entry* entry = find(key);
    if (entry == nullptr || entry->size == 0U) return false;
    entry->value[entry->size - 1U] ^= 0x80U;
    return true;
}

} // namespace fake_preferences

class Preferences {
public:
    Preferences() : opened_(false) {}
    ~Preferences() { end(); }

    bool begin(const char*, bool = false, const char* = nullptr) {
        ++fake_preferences::state().beginCalls;
        opened_ = fake_preferences::state().beginAllowed;
        return opened_;
    }

    void end() {
        if (!opened_) return;
        ++fake_preferences::state().endCalls;
        opened_ = false;
    }

    bool isKey(const char* key) {
        return opened_ && fake_preferences::find(key) != nullptr;
    }

    size_t getBytesLength(const char* key) {
        fake_preferences::Entry* entry =
            opened_ ? fake_preferences::find(key) : nullptr;
        return entry == nullptr ? 0U : entry->size;
    }

    size_t getBytes(const char* key, void* output, size_t capacity) {
        fake_preferences::Entry* entry =
            opened_ ? fake_preferences::find(key) : nullptr;
        if (entry == nullptr || output == nullptr || capacity < entry->size) {
            return 0U;
        }
        std::memcpy(output, entry->value, entry->size);
        return entry->size;
    }

    size_t putBytes(const char* key, const void* value, size_t size) {
        if (!opened_ || key == nullptr || value == nullptr || size == 0U ||
            size > sizeof(fake_preferences::Entry::value)) {
            return 0U;
        }
        fake_preferences::State& current = fake_preferences::state();
        const fake_preferences::PutMode mode = current.nextPutMode;
        current.nextPutMode = fake_preferences::PutNormally;
        if (mode == fake_preferences::RejectNextPut) return 0U;

        fake_preferences::Entry* entry = fake_preferences::allocate(key);
        if (entry == nullptr) return 0U;
        std::memcpy(entry->value, value, size);
        entry->size = size;
        if (mode == fake_preferences::CorruptNextPut) {
            entry->value[size - 1U] ^= 0x01U;
            return size;
        }
        return mode == fake_preferences::ApplyButReportFailure ? 0U : size;
    }

private:
    bool opened_;
};

#endif
