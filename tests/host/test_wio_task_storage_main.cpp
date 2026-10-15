#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.inc>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalTaskStorage.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)
using namespace blinker;
namespace {
uint32_t tick = 0U;
uint32_t now() { return tick; }
class Flash final : public IWioTerminalQspiFlash {
public:
    std::vector<uint8_t> bytes;
    unsigned starts = 0U, ends = 0U, reads = 0U, erases = 0U, programs = 0U;
    unsigned failProgram = 0U;
    unsigned failReadAfterPrograms = 0U;
    bool failErase = false, failRead = false, corruptReadback = false;
    uint32_t programDelay = 0U;
    Flash() : bytes(WioTerminalQspiOwner::expectedCapacity, uint8_t(0xffU)) {}
    bool begin(uint32_t& capacity) override { ++starts; capacity = static_cast<uint32_t>(bytes.size()); return true; }
    void end() override { ++ends; }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        ++reads; std::memcpy(output, bytes.data() + address, size);
        if (corruptReadback && programs != 0U) output[0] ^= 1U;
        return !failRead && (failReadAfterPrograms == 0U || programs < failReadAfterPrograms);
    }
    bool erase(uint32_t address) override {
        ++erases;
        std::fill(bytes.begin() + address, bytes.begin() + address + 4096U, uint8_t(0xffU));
        return !failErase;
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        ++programs; tick += programDelay;
        if (address >= WioTerminalQspiOwner::expectedCapacity - 16384U &&
            address < WioTerminalQspiOwner::expectedCapacity - 8192U)
            CHECK(size <= 256U && address / 256U == (address + size - 1U) / 256U);
        for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i];
        return programs != failProgram;
    }
};
typedef WioTerminalTaskSnapshotBlobStore<4U> Tasks;
const uint32_t taskAddress = WioTerminalQspiOwner::expectedCapacity - 16384U;
const uint32_t identityAddress = WioTerminalQspiOwner::expectedCapacity - 8192U;
std::vector<uint8_t> identity(const Flash& f) {
    return std::vector<uint8_t>(f.bytes.begin() + identityAddress, f.bytes.end());
}
void expect(IAtomicBlobStore& store, const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> output(bytes.size()); size_t written = 0U;
    CHECK(store.load(MutableByteSpan(output.data(), output.size()), written));
    CHECK(written == bytes.size() && output == bytes);
}
std::vector<uint8_t> fixture(bool enabledValue) {
    TaskSnapshotHeader header;
    std::memset(header.authority.deviceInstanceId, 1, 16U);
    std::memset(header.authority.taskScope, 2, 16U);
    std::memset(header.authority.rootDigest, 3, 32U);
    header.tableRevision = 1U; header.nextTaskId = 5U;
    header.requestId[0] = 1U; header.requestDigest[0] = 1U;
    TaskSnapshotRecord records[4];
    for (size_t i = 0U; i < 4U; ++i) {
        records[i].taskId = static_cast<uint32_t>(i + 1U);
        records[i].definitionRevision = 1U;
        records[i].timing.durationMillis = 1000U;
        records[i].key[0] = 'p'; records[i].key[1] = static_cast<uint8_t>('1' + i);
        records[i].keyLength = 2U; records[i].valueLength = 1U;
        records[i].value[0] = enabledValue ? 0xf5U : 0xf4U;
        // Disabled and not Started: codec tests, never a hardware authority.
    }
    std::vector<uint8_t> result(Tasks::maximumSnapshotSize); size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(header, records, 4U, MutableByteSpan(result.data(), result.size()), written));
    CHECK(written == 644U); return result;
}
void noAdmission() {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    Tasks tasks(bank); CHECK(tasks.begin().code() == ErrorCode::NotConfigured);
    CHECK(f.starts == 0U && f.reads == 0U && f.erases == 0U && f.programs == 0U);
    Tasks invalid(bank, true, 0U); CHECK(invalid.begin().code() == ErrorCode::InvalidArgument);
    CHECK(f.starts == 0U);
}
void coexistenceAndTombstone() {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    CHECK(bank.begin());
    const std::vector<uint8_t> instance(DeviceInstanceIdStore::serializedSize, uint8_t(0x31U));
    CHECK(bank.deviceInstanceBlob().replace(ByteView(instance.data(), instance.size())));
    const std::vector<uint8_t> before = identity(f);
    Tasks tasks(bank, true); CHECK(tasks.begin()); CHECK(f.starts == 1U);
    uint8_t output[644] = {}; size_t written = 99U;
    CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::NotFound && written == 0U);
    const std::vector<uint8_t> first = fixture(false), second = fixture(true);
    CHECK(tasks.replace(ByteView(first.data(), first.size()))); expect(tasks, first);
    CHECK(tasks.replace(ByteView(second.data(), second.size()))); expect(tasks, second);
    CHECK(tasks.load(MutableByteSpan(output, 1U), written).code() == ErrorCode::BufferTooSmall && written == 644U);
    expect(tasks, second); expect(bank.deviceInstanceBlob(), instance); CHECK(identity(f) == before);
    // Task lease still works after network/credential owner stop.
    bank.end(); CHECK(f.ends == 0U); expect(tasks, second);
    CHECK(tasks.clear()); CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::NotFound);
    const unsigned erased = f.erases; CHECK(tasks.clear()); CHECK(f.erases == erased);
    CHECK(identity(f) == before); tasks.end(); CHECK(f.ends == 1U);
    WioTerminalFlashBlobBank another(owner); Tasks reopened(another, true);
    CHECK(reopened.begin()); CHECK(reopened.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::NotFound);
    CHECK(f.starts == 2U); reopened.end(); CHECK(f.ends == 2U);
}
void foreignRegion() {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    CHECK(bank.begin()); f.bytes[taskAddress + 4095U] = 0x44U;
    Tasks tasks(bank, true); CHECK(tasks.begin().code() == ErrorCode::StateConflict);
    CHECK(f.erases == 0U && f.programs == 0U && f.ends == 0U && !owner.quarantined());
    const uint8_t a = 5U; CHECK(bank.wifiSelectorBlob().replace(ByteView(&a, 1U)));
    bank.end(); CHECK(f.ends == 1U);
}
void unknownProgram(bool finalPage) {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    CHECK(bank.begin()); Tasks tasks(bank, true); CHECK(tasks.begin());
    const auto first = fixture(false), second = fixture(true);
    CHECK(tasks.replace(ByteView(first.data(), first.size())));
    const auto original = identity(f);
    const unsigned beforeRead = f.reads, beforeErase = f.erases, beforeProgram = f.programs;
    f.failProgram = beforeProgram + (finalPage ? 3U : 1U);
    CHECK(tasks.replace(ByteView(second.data(), second.size())).code() == ErrorCode::WouldBlock);
    CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    CHECK(f.reads == beforeRead + 2U); CHECK(f.erases == beforeErase + 1U);
    CHECK(f.programs == beforeProgram + (finalPage ? 3U : 1U));
    const unsigned count = f.reads + f.erases + f.programs;
    uint8_t output[644] = {}; size_t written = 0U;
    CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::StateConflict);
    CHECK(!tasks.clear()); CHECK(f.reads + f.erases + f.programs == count);
    tasks.end(); bank.end(); CHECK(f.ends == 0U); CHECK(!bank.begin());
    // A new owner models a real new lifecycle, NOT a production unlock API.
    WioTerminalQspiOwner fresh(f, now); WioTerminalFlashBlobBank rebooted(fresh);
    Tasks restored(rebooted, true); CHECK(restored.begin());
    expect(restored, finalPage ? second : first); CHECK(identity(f) == original);
    CHECK(f.erases == beforeErase + 1U); CHECK(f.programs == beforeProgram + (finalPage ? 3U : 1U));
}
void unknownEraseAndLatePage() {
    for (unsigned mode = 0U; mode < 2U; ++mode) {
        tick = 0U; Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
        CHECK(bank.begin()); Tasks tasks(bank, true, 10U); CHECK(tasks.begin());
        const auto first = fixture(false), second = fixture(true);
        CHECK(tasks.replace(ByteView(first.data(), first.size())));
        const unsigned reads = f.reads, programs = f.programs;
        if (mode == 0U) f.failErase = true; else f.programDelay = 10U;
        CHECK(tasks.replace(ByteView(second.data(), second.size())).code() == ErrorCode::WouldBlock);
        CHECK(owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
        CHECK(f.reads == reads + 2U && f.programs == programs + (mode == 0U ? 0U : 1U));
    }
}
void unknownRead(bool afterWrite) {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    Tasks tasks(bank, true); CHECK(tasks.begin()); const auto first = fixture(false), second = fixture(true);
    CHECK(tasks.replace(ByteView(first.data(), first.size())));
    if (afterWrite) {
        f.failReadAfterPrograms = f.programs + 3U;
        CHECK(tasks.replace(ByteView(second.data(), second.size())).code() == ErrorCode::WouldBlock);
    } else {
        f.failRead = true; uint8_t output[644] = {}; size_t written = 9U;
        CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::WouldBlock);
        CHECK(written == 0U);
    }
    CHECK(owner.quarantined() && owner.outcome() ==
        (afterWrite ? WioTerminalQspiOwner::WriteUncertain : WioTerminalQspiOwner::ReadUncertain));
    const unsigned calls = f.reads + f.erases + f.programs;
    const unsigned writes = f.erases + f.programs;
    CHECK(!tasks.clear()); CHECK(!tasks.replace(ByteView(first.data(), first.size())));
    CHECK(f.reads + f.erases + f.programs == calls); tasks.end(); CHECK(f.ends == 0U);
    f.failRead = false; f.failReadAfterPrograms = 0U;
    WioTerminalQspiOwner fresh(f, now); WioTerminalFlashBlobBank anchor(fresh); Tasks restored(anchor, true);
    CHECK(restored.begin()); expect(restored, afterWrite ? second : first);
    CHECK(f.erases + f.programs == writes);
}
void verificationAndCorruption() {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    Tasks tasks(bank, true); CHECK(tasks.begin()); const auto data = fixture(false);
    f.corruptReadback = true;
    CHECK(tasks.replace(ByteView(data.data(), data.size())).code() == ErrorCode::WouldBlock);
    CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    tasks.end(); f.corruptReadback = false;
    f.bytes[taskAddress + 660U] ^= 1U; // Corrupt the only complete outer CRC.
    WioTerminalQspiOwner fresh(f, now); WioTerminalFlashBlobBank anchor(fresh); Tasks bad(anchor, true);
    CHECK(bad.begin()); uint8_t output[644] = {}; size_t written = 0U;
    CHECK(bad.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::InvalidEncoding);
    const unsigned writes = f.erases + f.programs;
    CHECK(!bad.replace(ByteView(data.data(), data.size()))); CHECK(f.erases + f.programs == writes);
}
void equalGenerationAndCapacity() {
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner); Tasks tasks(bank, true);
    CHECK(tasks.begin()); const auto data = fixture(false); CHECK(tasks.replace(ByteView(data.data(), data.size())));
    std::memcpy(f.bytes.data() + taskAddress + 4096U, f.bytes.data() + taskAddress, 664U);
    uint8_t output[644] = {}; size_t written = 0U;
    CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::SequenceConflict);
    const unsigned writes = f.erases + f.programs;
    std::vector<uint8_t> tooLarge(645U, uint8_t(1U));
    CHECK(tasks.replace(ByteView(tooLarge.data(), tooLarge.size())).code() == ErrorCode::CapacityExceeded);
    CHECK(f.erases + f.programs == writes);
}
void unsupportedPeer() {
    for (unsigned mode = 0U; mode < 2U; ++mode) {
        Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
        Tasks tasks(bank, true); CHECK(tasks.begin()); const auto data = fixture(false);
        CHECK(tasks.replace(ByteView(data.data(), data.size())));
        uint8_t* peer = f.bytes.data() + taskAddress + 4096U;
        std::memcpy(peer, f.bytes.data() + taskAddress, 664U);
        peer[11] = 2U;
        size_t payload = data.size();
        if (mode == 0U) peer[4] = 2U;
        else {
            ++payload; peer[12] = static_cast<uint8_t>(payload >> 8U);
            peer[13] = static_cast<uint8_t>(payload); peer[16U + data.size()] = 0U;
        }
        const uint32_t crc = computeCrc32(ByteView(peer, 16U + payload));
        for (size_t i = 0U; i < 4U; ++i)
            peer[16U + payload + i] = static_cast<uint8_t>(crc >> ((3U - i) * 8U));
        uint8_t output[644] = {}; size_t written = 0U;
        CHECK(tasks.load(MutableByteSpan(output, sizeof(output)), written).code() ==
            (mode == 0U ? ErrorCode::UnsupportedVersion : ErrorCode::CapacityExceeded));
        const unsigned writes = f.erases + f.programs;
        CHECK(!tasks.replace(ByteView(data.data(), data.size()))); CHECK(!tasks.clear());
        CHECK(f.erases + f.programs == writes);
    }
}
void corruptIdentityIsNotMissing() {
    class CountingRandom final : public IRandom {
    public:
        unsigned calls = 0U;
        Result fill(MutableByteSpan output) override {
            ++calls; std::memset(output.data, 0x31, output.size); return Result::success();
        }
    } random;
    Flash f; WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner);
    CHECK(bank.begin()); DeviceInstanceIdStore instances(bank.deviceInstanceBlob());
    DeviceInstanceId id;
    CHECK(instances.loadOrCreate(random, id)); CHECK(random.calls == 1U);
    const auto expected = identity(f);
    std::memcpy(f.bytes.data() + identityAddress + 4096U, f.bytes.data() + identityAddress, 572U);
    f.bytes[identityAddress + 4096U + 12U] ^= 1U;
    // An intact peer must still recover the original, without regeneration.
    DeviceInstanceId recovered; CHECK(instances.loadOrCreate(random, recovered));
    CHECK(sameDeviceInstanceId(id, recovered) && random.calls == 1U);
    f.bytes[identityAddress + 12U] ^= 1U;
    const auto before = identity(f); const unsigned writes = f.erases + f.programs;
    CHECK(instances.loadOrCreate(random, recovered).code() == ErrorCode::InvalidEncoding);
    CHECK(random.calls == 1U && f.erases + f.programs == writes && identity(f) == before);
    const uint8_t value = 1U;
    CHECK(bank.wifiSelectorBlob().replace(ByteView(&value, 1U)).code() == ErrorCode::InvalidEncoding);
    CHECK(bank.deviceInstanceBlob().clear().code() == ErrorCode::InvalidEncoding);
    CHECK(f.erases + f.programs == writes);
    // The earlier empty bank was allowed to create exactly one identity.
    CHECK(expected != before);
}
void legacyIdentity(size_t accessSize) {
    Flash f;
    const size_t size = 16U + 4U * 5U + DeviceInstanceIdStore::serializedSize +
        WifiCredentialStore::serializedSelectorSize + WifiCredentialStore::serializedProfileSize * 2U + accessSize;
    std::vector<uint8_t> raw(size, uint8_t(0U));
    std::memcpy(raw.data(), "BWB2", 4U); raw[4] = 2U; raw[11] = 1U;
    const size_t offset = size - accessSize - 4U;
    raw[offset] = 1U; raw[offset + 2U] = static_cast<uint8_t>(accessSize >> 8U);
    raw[offset + 3U] = static_cast<uint8_t>(accessSize);
    std::fill(raw.begin() + offset + 4U, raw.end(), uint8_t(0x6aU));
    const uint32_t crc = computeCrc32(ByteView(raw.data(), raw.size()));
    for (size_t i = 0U; i < 4U; ++i) raw[12U + i] = static_cast<uint8_t>(crc >> ((3U - i) * 8U));
    std::memcpy(f.bytes.data() + identityAddress, raw.data(), raw.size());
    const auto before = identity(f);
    WioTerminalQspiOwner owner(f, now); WioTerminalFlashBlobBank bank(owner); CHECK(bank.begin());
    const std::vector<uint8_t> expected(accessSize, uint8_t(0x6aU)); expect(bank.deviceAccessBlob(), expected);
    Tasks tasks(bank, true); CHECK(tasks.begin()); const auto data = fixture(false);
    CHECK(tasks.replace(ByteView(data.data(), data.size()))); expect(tasks, data);
    expect(bank.deviceAccessBlob(), expected); CHECK(identity(f) == before);
}
}
int main() {
    noAdmission(); coexistenceAndTombstone(); foreignRegion();
    unknownProgram(false); unknownProgram(true); unknownEraseAndLatePage();
    unknownRead(false); unknownRead(true);
    verificationAndCorruption(); equalGenerationAndCapacity(); unsupportedPeer();
    corruptIdentityIsNotMissing();
    legacyIdentity(DeviceAccessStore::serializedSize);
    legacyIdentity(DeviceAccessStore::previousSerializedSize);
    legacyIdentity(DeviceAccessStore::legacySerializedSize);
    std::cout << "Wio task storage media-model tests passed; no hardware proof\n";
}
