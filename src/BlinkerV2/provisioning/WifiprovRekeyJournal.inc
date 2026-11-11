#include "WifiprovRekeyJournal.h"

#include <string.h>

#include "../core/Crc32.h"

namespace blinker {
namespace {

const uint8_t kRekeyJournalMagic[] = {'B', 'W', 'R', 2U};
enum : size_t {
    kRekeyJournalPhaseOffset = sizeof(kRekeyJournalMagic),
    kRekeyJournalBindingOffset = kRekeyJournalPhaseOffset + 1U,
    kRekeyJournalAdminOffset = kRekeyJournalBindingOffset + security::kWifiprovRekeyBindingSize,
    kRekeyJournalAdminVersionOffset = kRekeyJournalAdminOffset + kControllerIdSize,
    kRekeyJournalNetworkOffset = kRekeyJournalAdminVersionOffset + 4U,
    kRekeyJournalChecksumOffset = kRekeyJournalNetworkOffset + 4U
};
static_assert(kRekeyJournalChecksumOffset + 4U == WifiprovRekeyJournal::serializedSize, "Rekey journal size");

void rekeyJournalWriteU32(uint8_t* bytes, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) bytes[i] = static_cast<uint8_t>(value >> (24U - 8U * i));
}
uint32_t rekeyJournalReadU32(const uint8_t* bytes) {
    uint32_t value = 0U;
    for (size_t i = 0U; i < 4U; ++i) value = (value << 8U) | bytes[i];
    return value;
}

bool validRekeyJournalAdmin(const WifiprovRekeyAdmin& admin) {
    uint8_t nonzero = 0U;
    for (auto value : admin.controllerId) nonzero = static_cast<uint8_t>(nonzero | value);
    return admin.version != 0U && nonzero != 0U;
}

Result encodeRekeyJournal(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin,
    WifiprovRekeyJournalPhase phase, uint8_t* bytes, uint32_t networkGeneration = 0U) {
    if (!validRekeyJournalAdmin(admin)) return Result::failure(ErrorCode::InvalidArgument);
    const Result result = security::encodeWifiprovRekeyBinding(binding,
        MutableByteSpan(bytes + kRekeyJournalBindingOffset, security::kWifiprovRekeyBindingSize));
    if (!result) return result;
    memcpy(bytes, kRekeyJournalMagic, sizeof(kRekeyJournalMagic));
    bytes[kRekeyJournalPhaseOffset] = static_cast<uint8_t>(phase);
    memcpy(bytes + kRekeyJournalAdminOffset, admin.controllerId, sizeof(admin.controllerId));
    rekeyJournalWriteU32(bytes + kRekeyJournalAdminVersionOffset, admin.version);
    rekeyJournalWriteU32(bytes + kRekeyJournalNetworkOffset, networkGeneration);
    rekeyJournalWriteU32(bytes + kRekeyJournalChecksumOffset, computeCrc32(ByteView(bytes, kRekeyJournalChecksumOffset)));
    return Result::success();
}

bool validRekeyJournal(ByteView bytes, WifiprovRekeyJournalPhase& phase) {
    if (bytes.size != WifiprovRekeyJournal::serializedSize ||
        memcmp(bytes.data, kRekeyJournalMagic, sizeof(kRekeyJournalMagic)) != 0 ||
        rekeyJournalReadU32(bytes.data + kRekeyJournalChecksumOffset) != computeCrc32(bytes.subspan(0U, kRekeyJournalChecksumOffset))) return false;
    phase = static_cast<WifiprovRekeyJournalPhase>(bytes.data[kRekeyJournalPhaseOffset]);
    const uint32_t network = rekeyJournalReadU32(bytes.data + kRekeyJournalNetworkOffset);
    return phase == WifiprovRekeyJournalPhase::Complete ? network != 0U : network == 0U &&
        (phase == WifiprovRekeyJournalPhase::AttemptReserved || phase == WifiprovRekeyJournalPhase::ApplyConsumed);
}

} // namespace

Result WifiprovRekeyJournal::inspect(const security::WifiprovRekeyBinding& binding,
    const WifiprovRekeyAdmin& admin, WifiprovRekeyJournalPhase& phase) {
    if (uncertain_) return Result::failure(ErrorCode::StateConflict);
    uint8_t expected[serializedSize];
    Result result = encodeRekeyJournal(binding, admin, WifiprovRekeyJournalPhase::AttemptReserved, expected);
    if (!result) return result;
    uint8_t bytes[serializedSize];
    size_t written = 0U;
    result = storage_.load(MutableByteSpan(bytes, sizeof(bytes)), written);
    if (!result) return result;
    WifiprovRekeyJournalPhase found;
    if (!validRekeyJournal(ByteView(bytes, written), found)) return Result::failure(ErrorCode::InvalidEncoding);
    if (memcmp(bytes + kRekeyJournalBindingOffset, expected + kRekeyJournalBindingOffset,
        kRekeyJournalNetworkOffset - kRekeyJournalBindingOffset) != 0) return Result::failure(ErrorCode::StateConflict);
    phase = found;
    return Result::success();
}

Result WifiprovRekeyJournal::read(WifiprovRekeyJournalRecord& record) {
    if (uncertain_) return Result::failure(ErrorCode::StateConflict);
    uint8_t bytes[serializedSize];
    size_t written = 0U;
    Result result = storage_.load(MutableByteSpan(bytes, sizeof(bytes)), written);
    if (!result) return result;
    WifiprovRekeyJournalRecord current;
    const ByteView encoded(bytes, written);
    if (!validRekeyJournal(encoded, current.phase) ||
        !security::decodeWifiprovRekeyBinding(encoded.subspan(kRekeyJournalBindingOffset, security::kWifiprovRekeyBindingSize), current.binding))
        return Result::failure(ErrorCode::InvalidEncoding);
    memcpy(current.admin.controllerId, bytes + kRekeyJournalAdminOffset, sizeof(current.admin.controllerId));
    current.admin.version = rekeyJournalReadU32(bytes + kRekeyJournalAdminVersionOffset);
    current.networkGeneration = rekeyJournalReadU32(bytes + kRekeyJournalNetworkOffset);
    if (!validRekeyJournalAdmin(current.admin)) return Result::failure(ErrorCode::InvalidEncoding);
    record = current;
    return Result::success();
}

Result WifiprovRekeyJournal::write(ByteView bytes) {
    Result result = storage_.replace(bytes);
    if (result) {
        uint8_t actual[serializedSize];
        size_t written = 0U;
        result = storage_.load(MutableByteSpan(actual, sizeof(actual)), written);
        if (result && (written != bytes.size || memcmp(bytes.data, actual, bytes.size) != 0))
            result = Result::failure(ErrorCode::InternalError);
    }
    if (!result) uncertain_ = true;
    return result;
}

Result WifiprovRekeyJournal::reserveAttempt(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin) {
    WifiprovRekeyJournalPhase existing;
    Result result = inspect(binding, admin, existing);
    if (result) return Result::failure(ErrorCode::StateConflict);
    if (result.code() != ErrorCode::NotFound) return result;
    uint8_t bytes[serializedSize];
    result = encodeRekeyJournal(binding, admin, WifiprovRekeyJournalPhase::AttemptReserved, bytes);
    return result ? write(ByteView(bytes, sizeof(bytes))) : result;
}

Result WifiprovRekeyJournal::consumeApply(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin) {
    WifiprovRekeyJournalPhase existing;
    Result result = inspect(binding, admin, existing);
    if (!result) return result;
    if (existing != WifiprovRekeyJournalPhase::AttemptReserved) return Result::failure(ErrorCode::StateConflict);
    uint8_t bytes[serializedSize];
    result = encodeRekeyJournal(binding, admin, WifiprovRekeyJournalPhase::ApplyConsumed, bytes);
    return result ? write(ByteView(bytes, sizeof(bytes))) : result;
}

Result WifiprovRekeyJournal::complete(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin,
    uint32_t networkGeneration) {
    if (networkGeneration == 0U) return Result::failure(ErrorCode::InvalidArgument);
    WifiprovRekeyJournalPhase existing;
    Result result = inspect(binding, admin, existing);
    if (!result) return result;
    if (existing != WifiprovRekeyJournalPhase::ApplyConsumed) return Result::failure(ErrorCode::StateConflict);
    uint8_t bytes[serializedSize];
    result = encodeRekeyJournal(binding, admin, WifiprovRekeyJournalPhase::Complete, bytes, networkGeneration);
    return result ? write(ByteView(bytes, sizeof(bytes))) : result;
}

} // namespace blinker
