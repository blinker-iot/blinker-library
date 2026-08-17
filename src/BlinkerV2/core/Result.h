#ifndef BLINKER_CORE_RESULT_H
#define BLINKER_CORE_RESULT_H

#include <stdint.h>

namespace blinker {

enum class ErrorCode : uint8_t {
    Ok = 0,
    InvalidArgument,
    BufferTooSmall,
    InvalidMagic,
    UnsupportedVersion,
    UnsupportedFeature,
    InvalidHeader,
    InvalidEncoding,
    NonCanonicalEncoding,
    InvalidUtf8,
    NestingTooDeep,
    TruncatedInput,
    TrailingData,
    CapacityExceeded,
    DuplicateField,
    ValueOutOfRange,
    AlreadyExists,
    SequenceConflict,
    StateConflict,
    NotFound,
    NotConfigured,
    NotConnected,
    WouldBlock,
    AuthenticationRequired,
    ProtocolError,
    InternalError,
    ManifestConflict
};

class Result {
public:
    Result() : code_(ErrorCode::Ok) {}
    explicit Result(ErrorCode code) : code_(code) {}

    static Result success() { return Result(ErrorCode::Ok); }
    static Result failure(ErrorCode code) { return Result(code); }

    bool ok() const { return code_ == ErrorCode::Ok; }
    ErrorCode code() const { return code_; }
    explicit operator bool() const { return ok(); }

private:
    ErrorCode code_;
};

} // namespace blinker

#endif
