#ifndef BLINKER_TEST_WIO_STORAGE_GATE_H
#define BLINKER_TEST_WIO_STORAGE_GATE_H

#include "StorageGateCommon.h"
#include "WioRawQspi.h"

// 0: unchanged identity-only 8 KiB USB contract.
// 1: fixed 16 KiB capture only; foreign task bytes are protected, never claimed.
// 2: restore ONLY task sectors to the compiled original raw backup digest.
#ifndef BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE
#define BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE 0
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE < 0 || BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE > 2
#error "Invalid Wio storage gate mode"
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0
#ifndef BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID
#error "Wio 16 KiB maintenance requires the frozen source SHA256"
#endif
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2
#if !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio raw task restore requires explicit exclusive region reservation"
#endif
#ifndef BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX
#error "Wio raw task restore requires the original fixed 16 KiB SHA256"
#endif
#include <BlinkerV2/core/Sha256.h>
#endif

namespace storage_gate {
namespace port {

enum : size_t {
    maximumSnapshot = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0 ? 8192U : 16384U,
    identityOffset = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0 ? 0U : 8192U,
    sectorSize = 4096U, currentImageSize = 572U
};
static const char* const name = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0
    ? "wio-terminal-owned-qspi-tail" : BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 1
    ? "wio-terminal-fixed-16k-capture" : "wio-terminal-fixed-16k-task-restore";
static const uint32_t baseAddress = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0
    ? storage_gate_qspi::OwnedQspiAccess::identityAddress : storage_gate_qspi::OwnedQspiAccess::taskAddress;
static const uint16_t entryOffsets[5] = {16U, 48U, 68U, 188U, 308U};
static const uint16_t entryLimits[5] = {28U, 16U, 116U, 116U, 260U};

constexpr int hexDigit(char value) {
    return value >= '0' && value <= '9' ? value - '0' :
        value >= 'a' && value <= 'f' ? value - 'a' + 10 :
        value >= 'A' && value <= 'F' ? value - 'A' + 10 : -1;
}
constexpr bool validHexDigest(const char* text, size_t offset = 0U) {
    return offset == 64U ? text[offset] == '\0' :
        hexDigit(text[offset]) >= 0 && validHexDigest(text, offset + 1U);
}
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0
inline bool buildPublicInfo(uint8_t* data, size_t& size, size_t capacity, const uint32_t* uid) {
    size = 0U;
    static constexpr char sourceHex[] = BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID;
    static_assert(sizeof(sourceHex) == 65U, "Wio maintenance source SHA256 must have 64 hex characters");
    static_assert(validHexDigest(sourceHex), "Wio maintenance source SHA256 must be hexadecimal");
    if (data == nullptr || uid == nullptr || capacity < 63U) return false;
    data[0] = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE;
    putLittle32(data + 1U, storage_gate_qspi::OwnedQspiAccess::capacityBytes);
    putLittle32(data + 5U, storage_gate_qspi::OwnedQspiAccess::taskAddress);
    putLittle32(data + 9U, storage_gate_qspi::OwnedQspiAccess::identityAddress);
    putLittle16(data + 13U, 8192U);
    for (size_t word = 0U; word < 4U; ++word)
        for (size_t byte = 0U; byte < 4U; ++byte)
            data[15U + word * 4U + byte] = static_cast<uint8_t>(uid[word] >> ((3U - byte) * 8U));
    for (size_t i = 0U; i < 32U; ++i) {
        const int high = hexDigit(sourceHex[i * 2U]), low = hexDigit(sourceHex[i * 2U + 1U]);
        if (high < 0 || low < 0) { wipe(data, 63U); return false; }
        data[31U + i] = static_cast<uint8_t>((high << 4U) | low);
    }
    size = 63U;
    return true;
}
#endif
// Public fixed-layout metadata, not storage contents or a physical ownership
// claim. Reading the SAMD UID matches the Seeed 1.8.5 USB serial word order.
inline bool publicInfo(uint8_t* data, size_t& size, size_t capacity) {
    size = 0U;
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0 && defined(__SAMD51__)
    const uint32_t uid[4U] = {
        *reinterpret_cast<volatile const uint32_t*>(0x008061fcUL),
        *reinterpret_cast<volatile const uint32_t*>(0x00806010UL),
        *reinterpret_cast<volatile const uint32_t*>(0x00806014UL),
        *reinterpret_cast<volatile const uint32_t*>(0x00806018UL)
    };
    return buildPublicInfo(data, size, capacity, uid);
#else
    (void)data; (void)capacity;
    return false;
#endif
}

struct Bank {
    uint32_t generation;
    uint16_t imageSize;
    bool blank;
};

// CRC of an existing image with the on-media CRC field virtually zeroed.
// Never normalize old images or their padding in a captured raw snapshot.
inline uint32_t imageCrc(const uint8_t* bytes, size_t size) {
    uint32_t result = 0xffffffffU;
    for (size_t i = 0U; i < size; ++i) {
        result ^= i >= 12U && i < 16U ? 0U : bytes[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
            result = (result >> 1U) ^ ((0U - (result & 1U)) & 0xedb88320U);
    }
    return ~result;
}
inline bool validBank(const uint8_t* bytes, Bank& bank) {
    bank = Bank{0U, 0U, true};
    for (size_t i = 0U; i < sectorSize; ++i) {
        if (bytes[i] != 0xffU) { bank.blank = false; break; }
    }
    if (bank.blank) return true;
    bank.generation = big32(bytes + 8U);
    if (memcmp(bytes, "BWB2", 4U) != 0 || bytes[4] != 2U || bytes[5] != 0U ||
        bytes[6] != 0U || bytes[7] != 0U || bank.generation == 0U) return false;
    const uint32_t expected = big32(bytes + 12U);
    if (expected == imageCrc(bytes, currentImageSize)) bank.imageSize = currentImageSize;
    else if (expected == imageCrc(bytes, 508U)) bank.imageSize = 508U;
    else if (expected == imageCrc(bytes, 488U)) bank.imageSize = 488U;
    else return false;
    for (size_t i = 0U; i < 5U; ++i) {
        const uint8_t* item = bytes + entryOffsets[i];
        const uint16_t size = big16(item + 2U);
        const uint16_t limit = i == 4U
            ? static_cast<uint16_t>(bank.imageSize - entryOffsets[i] - 4U)
            : entryLimits[i];
        if (item[1] != 0U || item[0] > 2U || size > limit ||
            (item[0] == 1U ? size == 0U : size != 0U)) return false;
    }
    return true;
}
inline bool equalLogicalImages(const uint8_t* left, const Bank& leftBank,
                               const uint8_t* right, const Bank& rightBank) {
    for (size_t i = 0U; i < currentImageSize; ++i) {
        const uint8_t l = i < leftBank.imageSize ? left[i] : 0U;
        const uint8_t r = i < rightBank.imageSize ? right[i] : 0U;
        if (l != r) return false;
    }
    return true;
}
inline bool decode(const uint8_t* data, size_t size, uint8_t& active, uint8_t* identity) {
    if (data == nullptr || identity == nullptr || size != maximumSnapshot) return false;
    // The first 8 KiB of a 16 KiB capture is uninterpreted task raw data. Its
    // being blank, damaged, or recognizable NEVER constitutes region ownership.
    data += identityOffset;
    Bank banks[2];
    if (!validBank(data, banks[0]) || !validBank(data + sectorSize, banks[1]) ||
        (banks[0].blank && banks[1].blank)) return false;
    if (!banks[0].blank && !banks[1].blank && banks[0].generation == banks[1].generation &&
        !equalLogicalImages(data, banks[0], data + sectorSize, banks[1])) return false;
    active = banks[0].blank || banks[1].generation > banks[0].generation ? 1U : 0U;
    const uint8_t* item = data + active * sectorSize + entryOffsets[0];
    return item[0] == 1U && instanceAnchor(item + 4U, big16(item + 2U), identity);
}
inline bool snapshot(uint8_t* output, size_t& size) {
    size = 0U;
    uint8_t identity[16U] = {}, active = 0U;
    storage_gate_qspi::Transaction transaction(BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0);
    const bool result = transaction.ready() &&
        storage_gate_qspi::access().read(baseAddress, output, maximumSnapshot) &&
        decode(output, maximumSnapshot, active, identity);
    wipe(identity, sizeof(identity));
    if (result) size = maximumSnapshot;
    return result;
}
inline bool matchesSnapshot(const uint8_t* expected, size_t size) {
    if (size != maximumSnapshot) return false;
    storage_gate_qspi::Transaction transaction(BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0);
    return transaction.ready() && storage_gate_qspi::access().matches(baseAddress, expected, size);
}
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2
inline bool originalBackup(const uint8_t* desired, size_t size) {
    static constexpr char digestHex[] = BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX;
    static_assert(sizeof(digestHex) == 65U, "Original Wio raw SHA256 must have 64 hex characters");
    static_assert(validHexDigest(digestHex), "Original Wio raw SHA256 must be hexadecimal");
    uint8_t actual[blinker::kSha256Size] = {}, expected[blinker::kSha256Size] = {};
    bool valid = size == maximumSnapshot;
    for (size_t i = 0U; valid && i < sizeof(expected); ++i) {
        const int high = hexDigit(digestHex[i * 2U]), low = hexDigit(digestHex[i * 2U + 1U]);
        valid = high >= 0 && low >= 0;
        if (valid) expected[i] = static_cast<uint8_t>((high << 4U) | low);
    }
    const bool result = valid && blinker::sha256(blinker::ByteView(desired, size),
        blinker::MutableByteSpan(actual, sizeof(actual))) && memcmp(actual, expected, sizeof(actual)) == 0;
    wipe(actual, sizeof(actual)); wipe(expected, sizeof(expected));
    return result;
}
#endif
inline bool validateTarget(const uint8_t* desired, size_t desiredSize,
                           const uint8_t* current, size_t currentSize) {
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 1
    (void)desired; (void)desiredSize; (void)current; (void)currentSize;
    return false; // Capture-only firmware has no raw mutation permission.
#else
    uint8_t desiredIdentity[16U] = {}, currentIdentity[16U] = {}, active = 0U;
    bool result = decode(desired, desiredSize, active, desiredIdentity) &&
                        decode(current, currentSize, active, currentIdentity) &&
                        memcmp(desiredIdentity, currentIdentity, 16U) == 0;
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2
    result = result && memcmp(desired + identityOffset, current + identityOffset, 8192U) == 0 &&
             originalBackup(desired, desiredSize);
#endif
    wipe(desiredIdentity, sizeof(desiredIdentity)); wipe(currentIdentity, sizeof(currentIdentity));
    return result;
#endif
}
inline bool restore(const uint8_t* desired, size_t desiredSize,
                    const uint8_t* current, size_t currentSize) {
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 1
    (void)desired; (void)desiredSize; (void)current; (void)currentSize;
    return false;
#else
    uint8_t identity[16U] = {}, active = 0U;
    const bool valid = validateTarget(desired, desiredSize, current, currentSize) &&
                       decode(desired, desiredSize, active, identity);
    wipe(identity, sizeof(identity));
    if (!valid) return false;
    storage_gate_qspi::Transaction transaction(BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0,
        BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2);
    if (!transaction.ready()) return false;
    // Restore the ORIGINAL snapshot's active bank first, then the other bank.
    // No product is running. A failure leaves this maintenance firmware loaded;
    // do not boot a product until the host has reconciled a complete snapshot.
    for (uint8_t ordinal = 0U; ordinal < 2U; ++ordinal) {
        const uint8_t slot = BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2 ? ordinal
            : ordinal == 0U ? active : static_cast<uint8_t>(1U - active);
        const uint32_t address = baseAddress + slot * sectorSize;
        const uint8_t* before = current + slot * sectorSize;
        const uint8_t* next = desired + slot * sectorSize;
        if (!storage_gate_qspi::access().restoreSector(address, before, next)) return false;
    }
    return true;
#endif
}

} // namespace port
} // namespace storage_gate
#endif
