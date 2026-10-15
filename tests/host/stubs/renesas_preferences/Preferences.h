#ifndef TEST_RENESAS_PREFERENCES_H
#define TEST_RENESAS_PREFERENCES_H

#include <stddef.h>
#include <stdint.h>

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemOwner.h>

typedef enum {
    PT_I8, PT_U8, PT_I16, PT_U16, PT_I32, PT_U32, PT_I64, PT_U64,
    PT_STR, PT_BLOB, PT_INVALID
} PreferenceType;

#define _PREF_TYPE "+PREFTYPE"
#define _PREF_LEN "+PREFLEN"
#define _PREF_BEGIN "+PREFBEGIN"
#define _PREF_END "+PREFEND"
#define _PREF_GET "+PREFGET"
#define _PREF_PUT "+PREFPUT"
#define PROMPT(x) x ":"
#define CMD_WRITE(x) "AT" x "="
#define CMD(x) "AT" x "\r\n"
#define MODEM_TIMEOUT 10000U

namespace fake_preferences {

inline void copyLabel(char* output, size_t capacity, const char* source) {
    if (capacity == 0U) return;
    size_t length = source == nullptr ? 0U : std::strlen(source);
    if (length >= capacity) length = capacity - 1U;
    if (length != 0U) std::memcpy(output, source, length);
    output[length] = '\0';
}

enum PutMode {
    PutNormally = 0,
    RejectNextPut,
    CorruptNextPut,
    ApplyButReportFailure,
    ApplyButFailReadback,
    ApplyButFailMetadata
};

enum MetadataOperation { MetadataNone, MetadataType, MetadataLength };

struct Entry {
    char key[16];
    char namespaceName[16];
    // Credential records stay small; optional four-slot task snapshots need
    // 644 payload + 20 journal bytes. Host-only media capacity, not MCU RAM.
    uint8_t value[700];
    size_t size;
    bool present;
    PreferenceType type;
};

struct State {
    Entry entries[20];
    PutMode nextPutMode;
    bool beginAllowed;
    bool modemBeginAllowed;
    unsigned beginCalls;
    unsigned endCalls;
    unsigned endRequests;
    unsigned putCalls;
    bool namespaceOpen;
    char namespaceName[16];
    MetadataOperation nextMetadataFailure;
    bool malformedMetadata;
    char metadataFailureKey[16];
    char readFailureKey[16];
    uint32_t nowMillis;
    uint32_t tickMillis;
    uint32_t timeoutMillis;
    uint32_t rpcMillis;
    uint32_t nextRpcMillis;
    uint32_t passthroughMillis;
    uint32_t writeHeaderMillis;
    uint32_t timeoutAtPassthrough;
    unsigned rpcCalls;
    unsigned metadataCalls;
    unsigned getCalls;
    unsigned nowaitCalls;
    unsigned passthroughCalls;
    unsigned getCallsAtPut;
    unsigned metadataCallsAtPut;
    unsigned modemBeginCalls;
    uint32_t timeoutHistory[128];
    unsigned timeoutCalls;
    char pendingKey[16];
    size_t pendingSize;
    bool untrimmed;
    void (*rpcHook)();

    State() { reset(); }

    void reset() {
        std::memset(this, 0, sizeof(*this));
        beginAllowed = true;
        modemBeginAllowed = true;
        timeoutMillis = MODEM_TIMEOUT;
        copyLabel(namespaceName, sizeof(namespaceName), "bl_v2");
    }
};

inline State& state() {
    static State value;
    return value;
}

inline uint32_t now() {
    const uint32_t sampled = state().nowMillis;
    state().nowMillis += state().tickMillis;
    return sampled;
}
inline void setTimeout(uint32_t value) {
    State& current = state();
    current.timeoutMillis = value;
    if (current.timeoutCalls < sizeof(current.timeoutHistory) / sizeof(current.timeoutHistory[0]))
        current.timeoutHistory[current.timeoutCalls] = value;
    ++current.timeoutCalls;
}

// Test-only MCU RAM restart. Production owner deliberately has no unlock API.
inline void restartOwner() {
    blinker::RenesasUnoModemOwner& owner = blinker::renesasUnoModemOwner();
    owner.~RenesasUnoModemOwner();
    new (&owner) blinker::RenesasUnoModemOwner(&now, &setTimeout, MODEM_TIMEOUT);
}

inline void resetState() { state().reset(); restartOwner(); }

inline void rebootState() {
    State& current = state();
    current.namespaceOpen = false;
    current.pendingKey[0] = '\0'; current.pendingSize = 0U;
    current.nextPutMode = PutNormally;
    current.nextMetadataFailure = MetadataNone;
    current.readFailureKey[0] = '\0';
    current.nowMillis = 0U; current.tickMillis = 0U; current.rpcMillis = 0U; current.nextRpcMillis = 0U;
    current.passthroughMillis = 0U; current.writeHeaderMillis = 0U;
    current.modemBeginAllowed = true; current.untrimmed = false; current.rpcHook = nullptr;
    setTimeout(MODEM_TIMEOUT);
    restartOwner();
}

inline Entry* find(const char* key) {
    State& current = state();
    for (size_t index = 0U;
         index < sizeof(current.entries) / sizeof(current.entries[0]);
         ++index) {
        if (current.entries[index].present &&
            std::strcmp(current.entries[index].namespaceName, current.namespaceName) == 0 &&
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
            copyLabel(entry.namespaceName, sizeof(entry.namespaceName), current.namespaceName);
            entry.type = PT_BLOB;
            entry.size = 0U;
            return &entry;
        }
    }
    return nullptr;
}

inline void failNext(PutMode mode) { state().nextPutMode = mode; }

inline void failMetadata(MetadataOperation operation, const char* key, bool malformed = false) {
    state().nextMetadataFailure = operation;
    state().malformedMetadata = malformed;
    copyLabel(state().metadataFailureKey, sizeof(state().metadataFailureKey), key);
}

inline void failRead(const char* key) { copyLabel(state().readFailureKey, sizeof(state().readFailureKey), key); }

class MetadataModem {
public:
    bool beginned = false;
    void begin(int = 115200, int = 3) { beginned = state().modemBeginAllowed; ++state().modemBeginCalls; }
    void end() { beginned = false; }
    void timeout(size_t value) { setTimeout(static_cast<uint32_t>(value)); }
    void avoid_trim_results() { state().untrimmed = true; }
    void read_using_size() {}

    bool write(const std::string& prompt, std::string& response, const char* format, ...) {
        char command[192] = {};
        va_list args; va_start(args, format);
        std::vsnprintf(command, sizeof(command), format, args); va_end(args);
        advanceRpc();
        State& current = state();
        response.clear();
        if (prompt == PROMPT(_PREF_BEGIN)) {
            ++current.beginCalls;
            const char* name = command + std::strlen(CMD_WRITE(_PREF_BEGIN));
            const char* comma = std::strchr(name, ',');
            if (comma == nullptr || static_cast<size_t>(comma - name) >= sizeof(current.namespaceName)) return false;
            current.namespaceOpen = current.beginAllowed;
            if (current.namespaceOpen) {
                std::memcpy(current.namespaceName, name, static_cast<size_t>(comma - name));
                current.namespaceName[comma - name] = '\0';
            }
            response = current.namespaceOpen ? "1" : "0";
            return true;
        }
        if (prompt == PROMPT(_PREF_END)) {
            ++current.endRequests;
            if (current.namespaceOpen) ++current.endCalls;
            current.namespaceOpen = false; response = "1"; return true;
        }
        if (!current.namespaceOpen) return false;
        const char* equals = std::strchr(command, '=');
        if (equals == nullptr) return false;
        char key[16] = {};
        size_t index = 0U;
        for (const char* input = equals + 1; *input != '\0' && *input != ',' && *input != '\r' && *input != '\n'; ++input) {
            if (index >= sizeof(key) - 1U) return false;
            key[index++] = *input;
        }
        if (prompt == PROMPT(_PREF_GET)) {
            ++current.getCalls;
            const bool untrimmed = current.untrimmed; current.untrimmed = false;
            if (!untrimmed) return false;
            if (std::strcmp(current.readFailureKey, key) == 0) {
                current.readFailureKey[0] = '\0'; return false;
            }
            const Entry* entry = find(key);
            if (entry == nullptr) return false;
            response.assign(reinterpret_cast<const char*>(entry->value), entry->size);
            return true;
        }
        const MetadataOperation operation = prompt == PROMPT(_PREF_TYPE) ? MetadataType :
            (prompt == PROMPT(_PREF_LEN) ? MetadataLength : MetadataNone);
        if (operation == MetadataNone) return false;
        ++current.metadataCalls;
        if (current.nextMetadataFailure == operation &&
            std::strcmp(current.metadataFailureKey, key) == 0) {
            current.nextMetadataFailure = MetadataNone;
            if (!current.malformedMetadata) return false;
            response = "not-a-decimal";
            return true;
        }
        const Entry* entry = find(key);
        response = std::to_string(operation == MetadataType ?
            static_cast<size_t>(entry == nullptr ? PT_INVALID : entry->type) :
            (entry == nullptr ? 0U : entry->size));
        return true;
    }

    void write_nowait(const std::string& prompt, std::string& response, const char* format, ...) {
        char command[192] = {};
        va_list args; va_start(args, format);
        std::vsnprintf(command, sizeof(command), format, args); va_end(args);
        advanceRpc(); response.clear(); ++state().nowaitCalls;
        state().nowMillis += state().writeHeaderMillis;
        state().pendingKey[0] = '\0'; state().pendingSize = 0U;
        if (prompt != PROMPT(_PREF_PUT)) return;
        const char* key = command + std::strlen(CMD_WRITE(_PREF_PUT));
        const char* comma = std::strchr(key, ',');
        if (comma == nullptr || comma == key || static_cast<size_t>(comma - key) >= sizeof(state().pendingKey)) return;
        char* typeEnd = nullptr;
        const unsigned long type = std::strtoul(comma + 1U, &typeEnd, 10);
        if (type != PT_BLOB || typeEnd == comma + 1U || *typeEnd != ',') return;
        char* sizeEnd = nullptr;
        const unsigned long size = std::strtoul(typeEnd + 1U, &sizeEnd, 10);
        if (sizeEnd == typeEnd + 1U || std::strcmp(sizeEnd, "\r\n") != 0 || size > sizeof(Entry::value)) return;
        std::memcpy(state().pendingKey, key, static_cast<size_t>(comma - key));
        state().pendingKey[comma - key] = '\0'; state().pendingSize = size;
    }

    bool passthrough(const uint8_t* value, size_t size) {
        State& current = state();
        ++current.passthroughCalls; ++current.putCalls;
        current.timeoutAtPassthrough = current.timeoutMillis;
        current.getCallsAtPut = current.getCalls;
        current.metadataCallsAtPut = current.metadataCalls;
        current.nowMillis += current.passthroughMillis;
        const PutMode mode = current.nextPutMode;
        current.nextPutMode = PutNormally;
        if (!current.namespaceOpen || value == nullptr || size == 0U || size != current.pendingSize ||
            size > sizeof(Entry::value) || mode == RejectNextPut) return false;
        Entry* entry = allocate(current.pendingKey);
        if (entry == nullptr) return false;
        std::memcpy(entry->value, value, size); entry->size = size; entry->type = PT_BLOB;
        if (mode == CorruptNextPut) entry->value[size - 1U] ^= 0x01U;
        if (mode == ApplyButFailReadback) failRead(current.pendingKey);
        if (mode == ApplyButFailMetadata) failMetadata(MetadataType, current.pendingKey);
        current.pendingKey[0] = '\0'; current.pendingSize = 0U;
        return mode != ApplyButReportFailure;
    }

private:
    static void advanceRpc() {
        State& current = state();
        ++current.rpcCalls;
        current.nowMillis += current.nextRpcMillis == 0U ? current.rpcMillis : current.nextRpcMillis;
        current.nextRpcMillis = 0U;
        void (*hook)() = current.rpcHook; current.rpcHook = nullptr;
        if (hook != nullptr) hook();
    }
};

inline MetadataModem& metadataModem() { static MetadataModem value; return value; }

inline void reset() { resetState(); metadataModem().beginned = false; }
inline void reboot() { rebootState(); metadataModem().beginned = false; }

inline bool corrupt(const char* key) {
    Entry* entry = find(key);
    if (entry == nullptr || entry->size == 0U) return false;
    entry->value[entry->size - 1U] ^= 0x80U;
    return true;
}

} // namespace fake_preferences

#define modem fake_preferences::metadataModem()

inline unsigned long millis() { return fake_preferences::now(); }

#endif
