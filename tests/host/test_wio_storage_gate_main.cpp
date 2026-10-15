#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>
#ifndef BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE
#define BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE 1
#endif
#ifndef BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID
#define BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 2
#define BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED 1
// SHA256 of the public deterministic fixture below, not a physical backup.
#define BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX "8321119a2b61d78db2797f6ae5efee74801113f4ad19875f400dc120f76d3ce9"
#endif
#include "../internal/arduino/PlatformStorageBackupGate/WioStorageGate.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)
uint32_t diagnosticTestMillis = 0U;
using namespace blinker;
using storage_gate_qspi::OwnedQspiAccess;
namespace {
uint32_t expireAfterNextClock = 0U;
uint32_t clockNow() {
    const uint32_t value = diagnosticTestMillis;
    diagnosticTestMillis += expireAfterNextClock; expireAfterNextClock = 0U;
    return value;
}
class Flash final : public IWioTerminalQspiFlash {
public:
    std::vector<uint8_t> bytes;
    unsigned begins = 0U, ends = 0U, reads = 0U, erases = 0U, programs = 0U;
    unsigned falseRead = 0U, falseProgram = 0U, expireCloseRead = 0U;
    bool falseErase = false, corruptReadback = false;
    uint32_t readCost = 0U, programCost = 0U, endCost = 0U;
    void (*hook)(void*) = nullptr;
    void (*endHook)() = nullptr;
    void* context = nullptr;
    Flash() : bytes(OwnedQspiAccess::capacityBytes, uint8_t(0xffU)) {}
    bool begin(uint32_t& capacity) override { ++begins; capacity = static_cast<uint32_t>(bytes.size()); return true; }
    void end() override {
        ++ends; if (endHook != nullptr) endHook(); diagnosticTestMillis += endCost;
    }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        ++reads; diagnosticTestMillis += readCost;
        CHECK(address <= bytes.size() && size <= bytes.size() - address);
        std::copy(bytes.data() + address, bytes.data() + address + size, output);
        if (corruptReadback && programs != 0U) output[0] ^= 1U;
        if (hook != nullptr) hook(context);
        // The owner's final native-completion check still sees the old time;
        // only the following close observes the elapsed whole-session budget.
        if (reads == expireCloseRead) expireAfterNextClock = 10000U;
        return reads != falseRead;
    }
    bool erase(uint32_t address) override {
        ++erases; std::fill(bytes.begin() + address, bytes.begin() + address + 4096U, uint8_t(0xffU));
        return !falseErase;
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        ++programs; diagnosticTestMillis += programCost;
        CHECK(size == 256U && address % 256U == 0U);
        for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i];
        return programs != falseProgram;
    }
};
Flash portFlash;
WioTerminalQspiOwner portOwner(portFlash, &clockNow);
std::vector<uint8_t> canonical() {
    std::vector<uint8_t> raw(16384U, uint8_t(0xffU));
    uint8_t* image = raw.data() + 8192U;
    std::fill(image, image + 572U, uint8_t(0U));
    std::memcpy(image, "BWB2", 4U); image[4] = 2U; image[11] = 1U;
    image[16] = 1U; image[19] = 28U;
    std::memcpy(image + 20U, "BDN2", 4U); image[24] = 1U;
    std::fill(image + 28U, image + 44U, uint8_t(1U));
    const uint32_t anchor = storage_gate::crc32(image + 20U, 24U);
    for (size_t i = 0U; i < 4U; ++i) image[44U + i] = static_cast<uint8_t>(anchor >> ((3U - i) * 8U));
    const uint32_t crc = storage_gate::crc32(image, 572U);
    for (size_t i = 0U; i < 4U; ++i) image[12U + i] = static_cast<uint8_t>(crc >> ((3U - i) * 8U));
    return raw;
}
void captureAndRelease() {
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    f.bytes[OwnedQspiAccess::taskAddress] = 0x36U; f.bytes.back() = 0x91U;
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true)); CHECK(capacity == 4194304U);
    std::vector<uint8_t> raw(16384U);
    CHECK(access.read(OwnedQspiAccess::taskAddress, raw.data(), raw.size()));
    CHECK(raw.front() == 0x36U && raw.back() == 0x91U && f.reads == 2U);
    CHECK(access.matches(OwnedQspiAccess::taskAddress, raw.data(), raw.size()));
    CHECK(f.erases == 0U && f.programs == 0U);
    CHECK(access.finish()); CHECK(f.ends == 1U && !access.finish());
    int identityToken = 0, taskToken = 0;
    CHECK(owner.acquire(&identityToken, WioTerminalQspiOwner::Region::Identity));
    CHECK(owner.acquire(&taskToken, WioTerminalQspiOwner::Region::Tasks));
    CHECK(f.begins == 2U); CHECK(owner.release(&identityToken)); CHECK(owner.release(&taskToken));
}
void exactTaskRestore() {
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    const std::vector<uint8_t> identity(f.bytes.begin() + OwnedQspiAccess::identityAddress, f.bytes.end());
    std::vector<uint8_t> before(4096U, uint8_t(0x12U)), desired(4096U, uint8_t(0xffU));
    std::copy(before.begin(), before.end(), f.bytes.begin() + OwnedQspiAccess::taskAddress);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true, true));
    CHECK(access.restoreSector(OwnedQspiAccess::taskAddress, before.data(), desired.data()));
    CHECK(f.erases == 1U && f.programs == 16U);
    CHECK(std::equal(identity.begin(), identity.end(), f.bytes.begin() + OwnedQspiAccess::identityAddress));
    CHECK(std::equal(desired.begin(), desired.end(), f.bytes.begin() + OwnedQspiAccess::taskAddress));
    CHECK(access.restoreSector(OwnedQspiAccess::taskAddress, desired.data(), desired.data()));
    CHECK(f.erases == 1U && f.programs == 16U); access.end(); CHECK(f.ends == 1U);
}
void readOnlyAndRangeGuards() {
    for (unsigned test = 0U; test < 6U; ++test) {
        Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
        uint32_t capacity = 0U; CHECK(access.begin(capacity, test != 3U && test != 5U, test == 4U));
        uint8_t raw[4096U] = {};
        if (test == 0U) CHECK(!access.restoreSector(OwnedQspiAccess::taskAddress, raw, raw));
        else if (test == 1U) CHECK(!access.read(OwnedQspiAccess::taskAddress - 1U, raw, 1U));
        else if (test == 2U) CHECK(!access.read(UINT32_MAX, raw, 2U));
        else if (test == 3U) CHECK(!access.read(OwnedQspiAccess::taskAddress, raw, 1U));
        else if (test == 4U) CHECK(!access.restoreSector(OwnedQspiAccess::identityAddress, raw, raw));
        else CHECK(!access.restoreSector(OwnedQspiAccess::taskAddress, raw, raw));
        CHECK(f.reads == 0U && f.erases == 0U && f.programs == 0U);
        access.end(); CHECK(!access.begin(capacity, true));
    }
}
void unknownStops(bool write, bool late) {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true, write, 100U));
    std::vector<uint8_t> before(4096U, uint8_t(0xffU)), desired(4096U, uint8_t(0x44U));
    if (write) {
        if (late) f.programCost = 100U; else f.falseProgram = 1U;
        CHECK(!access.restoreSector(OwnedQspiAccess::taskAddress, before.data(), desired.data()));
        CHECK(f.programs == 1U && f.erases == 1U && f.reads == 64U);
        CHECK(owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    } else {
        if (late) f.readCost = 100U; else f.falseRead = 1U;
        CHECK(!access.read(OwnedQspiAccess::taskAddress, before.data(), before.size()));
        CHECK(f.reads == 1U && owner.outcome() == WioTerminalQspiOwner::ReadUncertain);
    }
    const unsigned calls = f.reads + f.erases + f.programs;
    CHECK(!access.read(OwnedQspiAccess::taskAddress, before.data(), before.size()));
    access.end(); CHECK(f.ends == 0U && owner.quarantined());
    CHECK(!access.begin(capacity, true)); CHECK(f.reads + f.erases + f.programs == calls);
}
void sharedDeadline() {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true, false, 10U));
    f.readCost = 6U; std::vector<uint8_t> raw(16384U);
    CHECK(!access.read(OwnedQspiAccess::taskAddress, raw.data(), raw.size()));
    CHECK(f.reads == 2U && owner.quarantined()); // Second region got 4 ms, not a new 10 ms.
    access.end(); CHECK(f.ends == 0U);
}
void endAfterDeadlineDoesNotCleanup() {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true, false, 10U));
    uint8_t byte = 0U; CHECK(access.read(OwnedQspiAccess::taskAddress, &byte, 1U));
    const unsigned reads = f.reads; diagnosticTestMillis = 10U;
    access.end(); CHECK(owner.quarantined() && access.failed());
    CHECK(f.reads == reads && f.erases == 0U && f.programs == 0U && f.ends == 0U);
    CHECK(!access.begin(capacity, true));
}
void closeAfterLastSuccessfulRead() {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true));
    f.expireCloseRead = 1U; uint8_t byte = 0U;
    CHECK(access.read(OwnedQspiAccess::taskAddress, &byte, 1U));
    CHECK(!owner.quarantined());
    CHECK(!access.finish() && owner.quarantined() && access.failed());
    CHECK(f.reads == 1U && f.erases == 0U && f.programs == 0U && f.ends == 0U);
    CHECK(!access.finish() && !access.begin(capacity, true));
}
void nativeCloseDeadlineLocksOnlyHelper() {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true));
    uint8_t byte = 0U; CHECK(access.read(OwnedQspiAccess::taskAddress, &byte, 1U));
    f.endCost = 10000U;
    CHECK(!access.finish() && access.failed() && !access.busy());
    CHECK(!owner.quarantined() && owner.capacity() == 0U && f.ends == 1U);
    CHECK(!access.begin(capacity, true));
    CHECK(!access.read(OwnedQspiAccess::taskAddress, &byte, 1U)); CHECK(!access.finish());
    CHECK(f.begins == 1U && f.reads == 1U && f.ends == 1U && f.erases == 0U && f.programs == 0U);
}
void verificationStops() {
    diagnosticTestMillis = 0U;
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true, true));
    std::vector<uint8_t> before(4096U, uint8_t(0xffU)), desired(4096U, uint8_t(0x44U));
    f.corruptReadback = true;
    CHECK(!access.restoreSector(OwnedQspiAccess::taskAddress, before.data(), desired.data()));
    CHECK(f.programs == 16U && f.reads == 65U && owner.quarantined());
    CHECK(owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    access.end(); CHECK(f.ends == 0U);
}
struct Reentry { OwnedQspiAccess* access; Flash* flash; unsigned calls; };
void onRead(void* context) {
    Reentry& value = *static_cast<Reentry*>(context); ++value.calls;
    uint32_t capacity = 0U; uint8_t byte = 0U;
    value.access->end(); CHECK(value.flash->ends == 0U);
    CHECK(!value.access->finish());
    CHECK(!value.access->begin(capacity, true));
    CHECK(!value.access->read(OwnedQspiAccess::identityAddress, &byte, 1U));
    CHECK(!value.access->matches(OwnedQspiAccess::identityAddress, &byte, 1U));
}
void reentryDoesNotRelease() {
    Flash f; WioTerminalQspiOwner owner(f, &clockNow); OwnedQspiAccess access(owner, &clockNow);
    uint32_t capacity = 0U; CHECK(access.begin(capacity, true));
    Reentry reentry = {&access, &f, 0U}; f.hook = &onRead; f.context = &reentry;
    std::vector<uint8_t> raw(16384U); CHECK(access.read(OwnedQspiAccess::taskAddress, raw.data(), raw.size()));
    CHECK(reentry.calls == 2U && !access.failed()); access.end(); CHECK(f.ends == 1U);
}
storage_gate_qspi::Transaction* closingTransaction = nullptr;
void onPortClose() {
    const unsigned ends = portFlash.ends;
    CHECK(closingTransaction != nullptr && !closingTransaction->complete(true));
    storage_gate_qspi::Transaction rejected(true);
    CHECK(!rejected.ready() && !rejected.complete(true));
    CHECK(portFlash.ends == ends);
}
void transactionCloseReentry() {
    diagnosticTestMillis = 0U;
    const unsigned ends = portFlash.ends;
    storage_gate_qspi::Transaction transaction(true); CHECK(transaction.ready());
    closingTransaction = &transaction; portFlash.endHook = &onPortClose;
    CHECK(transaction.complete(true));
    portFlash.endHook = nullptr; closingTransaction = nullptr;
    CHECK(portFlash.ends == ends + 1U && !storage_gate_qspi::access().busy());
    CHECK(!transaction.complete(true));
}
void portContract() {
    diagnosticTestMillis = 0U;
    std::vector<uint8_t> original = canonical(), current = original;
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0
    // The old default gate still restores identity, but never the task region.
    current[8192U + 11U] = 2U;
    const uint32_t newerCrc = storage_gate::port::imageCrc(current.data() + 8192U, 572U);
    for (size_t i = 0U; i < 4U; ++i) current[8192U + 12U + i] =
        static_cast<uint8_t>(newerCrc >> ((3U - i) * 8U));
#else
    current[0] = 0x17U; // Foreign task raw data is still capturable, not claimable.
#endif
    std::copy(current.begin(), current.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress);
    const size_t offset = storage_gate::port::identityOffset == 0U ? 8192U : 0U;
    std::vector<uint8_t> raw(storage_gate::port::maximumSnapshot); size_t size = 0U;
    CHECK(storage_gate::port::snapshot(raw.data(), size) && size == raw.size());
    CHECK(std::equal(raw.begin(), raw.end(), current.begin() + offset));
    CHECK(storage_gate::port::matchesSnapshot(raw.data(), raw.size()));
    uint8_t active = 0U, identity[16U] = {};
    CHECK(storage_gate::port::decode(raw.data(), raw.size(), active, identity));
    CHECK(std::all_of(identity, identity + 16U, [](uint8_t value) { return value == 1U; }));
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0
    CHECK(storage_gate::port::validateTarget(original.data() + offset, raw.size(), raw.data(), raw.size()));
    CHECK(storage_gate::port::restore(original.data() + offset, raw.size(), raw.data(), raw.size()));
    CHECK(portFlash.erases == 1U && portFlash.programs == 16U);
    CHECK(std::equal(original.begin(), original.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress));
#elif BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 1
    CHECK(!storage_gate::port::validateTarget(original.data(), original.size(), current.data(), current.size()));
    CHECK(!storage_gate::port::restore(original.data(), original.size(), current.data(), current.size()));
    CHECK(portFlash.erases == 0U && portFlash.programs == 0U);
#else
    CHECK(storage_gate::port::originalBackup(original.data(), original.size()));
    CHECK(storage_gate::port::validateTarget(original.data(), original.size(), current.data(), current.size()));
    current.back() ^= 1U;
    CHECK(!storage_gate::port::validateTarget(original.data(), original.size(), current.data(), current.size()));
    current.back() ^= 1U; original[1] ^= 1U;
    CHECK(!storage_gate::port::validateTarget(original.data(), original.size(), current.data(), current.size()));
    original[1] ^= 1U;
    CHECK(storage_gate::port::restore(original.data(), original.size(), current.data(), current.size()));
    CHECK(portFlash.erases == 1U && portFlash.programs == 16U);
    CHECK(std::equal(original.begin(), original.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress));
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 0
    uint8_t info[63U] = {}; size_t written = 0U;
    const uint32_t uid[4U] = {0x10203040U, 0x50607080U, 0x90a0b0c0U, 0xd0e0f001U};
    CHECK(storage_gate::port::buildPublicInfo(info, written, sizeof(info), uid));
    CHECK(written == 63U && info[0] == BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE);
    CHECK(storage_gate::little32(info + 1U) == 4194304U);
    CHECK(storage_gate::little32(info + 5U) == 0x3fc000U);
    CHECK(storage_gate::little32(info + 9U) == 0x3fe000U);
    CHECK(storage_gate::little16(info + 13U) == 8192U && info[15] == 0x10U && info[30] == 0x01U);
    CHECK(info[31] == 0x01U && info[62] == 0xefU);
    CHECK(!storage_gate::port::buildPublicInfo(info, written, 62U, uid) && written == 0U);
#endif
}
void portRejectsLateClose() {
    diagnosticTestMillis = 0U;
    const std::vector<uint8_t> original = canonical();
    std::copy(original.begin(), original.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress);
    const unsigned ends = portFlash.ends, reads = portFlash.reads;
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 0
    portFlash.expireCloseRead = reads + 1U;
    std::vector<uint8_t> raw(storage_gate::port::maximumSnapshot); size_t size = 99U;
    CHECK(!storage_gate::port::snapshot(raw.data(), size) && size == 0U);
#elif BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE == 1
    // All reads and close admission succeed; the void native cleanup itself
    // takes the remaining budget. The returned success must still be rejected.
    const unsigned finalRead = reads + 256U; portFlash.endCost = 10000U;
    CHECK(!storage_gate::port::matchesSnapshot(original.data(), original.size()));
    CHECK(portFlash.reads == finalRead && !portOwner.quarantined());
    CHECK(storage_gate_qspi::access().failed() && portFlash.ends == ends + 1U);
#else
    std::vector<uint8_t> current = original; current[0U] = 0x17U;
    std::copy(current.begin(), current.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress);
    // 64 before + 64 after reads for changed sector0, then 64 before
    // reads for unchanged sector1. Every native read itself succeeds.
    portFlash.expireCloseRead = reads + 192U;
    CHECK(!storage_gate::port::restore(original.data(), original.size(), current.data(), current.size()));
    CHECK(std::equal(original.begin(), original.end(), portFlash.bytes.begin() + OwnedQspiAccess::taskAddress));
#endif
#if BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE != 1
    CHECK(portFlash.reads == portFlash.expireCloseRead);
    CHECK(portOwner.quarantined() && storage_gate_qspi::access().failed() && portFlash.ends == ends);
#endif
}
} // namespace
namespace blinker { WioTerminalQspiOwner& wioTerminalQspiOwner() { return portOwner; } }
int main() {
    captureAndRelease(); exactTaskRestore(); readOnlyAndRangeGuards();
    unknownStops(false, false); unknownStops(false, true);
    unknownStops(true, false); unknownStops(true, true);
    sharedDeadline(); endAfterDeadlineDoesNotCleanup(); closeAfterLastSuccessfulRead(); nativeCloseDeadlineLocksOnlyHelper();
    verificationStops(); reentryDoesNotRelease(); transactionCloseReentry(); portContract(); portRejectsLateClose();
    std::cout << "Wio storage maintenance model mode " << BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE
              << " passed; no hardware admission\n";
}
