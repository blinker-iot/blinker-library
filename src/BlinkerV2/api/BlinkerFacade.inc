#include "BlinkerFacade.h"

namespace {

blinker::Diagnostics facadeDiagnostics;
uint16_t lastProductStatus = 0U;
bool hasProductStatus = false;

uint16_t packStatus(const blinker::ProductStatus& status) {
    uint16_t packed = static_cast<uint16_t>(status.state) & 0x07U;
    if (status.started) packed |= 1U << 3U;
    if (status.networkReady) packed |= 1U << 4U;
    if (status.localReady) packed |= 1U << 5U;
    if (status.cloudReady) packed |= 1U << 6U;
    packed |= static_cast<uint16_t>(status.lastError) << 8U;
    return packed;
}

const char* errorText(blinker::ErrorCode error) {
    using blinker::ErrorCode;
    switch (error) {
    case ErrorCode::Ok: return "ok";
    case ErrorCode::InvalidArgument: return "invalid argument";
    case ErrorCode::BufferTooSmall: return "buffer too small";
    case ErrorCode::InvalidMagic: return "invalid magic";
    case ErrorCode::UnsupportedVersion: return "unsupported version";
    case ErrorCode::UnsupportedFeature: return "unsupported feature";
    case ErrorCode::InvalidHeader: return "invalid header";
    case ErrorCode::InvalidEncoding: return "invalid encoding";
    case ErrorCode::NonCanonicalEncoding: return "non-canonical encoding";
    case ErrorCode::InvalidUtf8: return "invalid UTF-8";
    case ErrorCode::NestingTooDeep: return "nesting too deep";
    case ErrorCode::TruncatedInput: return "truncated input";
    case ErrorCode::TrailingData: return "trailing data";
    case ErrorCode::CapacityExceeded: return "capacity exceeded";
    case ErrorCode::DuplicateField: return "duplicate field";
    case ErrorCode::ValueOutOfRange: return "value out of range";
    case ErrorCode::AlreadyExists: return "already started or configured";
    case ErrorCode::SequenceConflict: return "sequence conflict";
    case ErrorCode::StateConflict: return "state conflict";
    case ErrorCode::NotFound: return "not found";
    case ErrorCode::NotConfigured: return "not configured";
    case ErrorCode::NotConnected: return "not connected";
    case ErrorCode::WouldBlock: return "would block";
    case ErrorCode::AuthenticationRequired: return "authentication required";
    case ErrorCode::ProtocolError: return "protocol error";
    case ErrorCode::InternalError: return "internal error";
    case ErrorCode::ManifestConflict: return "manifest conflict";
    }
    return "unknown error";
}

} // namespace

blinker::Diagnostics& blinker::facade_detail::diagnostics() {
    return facadeDiagnostics;
}

void blinker::facade_detail::observeProductStatus(
    const ProductStatus& status) {
    const uint16_t current = packStatus(status);
    const uint16_t previous = hasProductStatus
                                  ? lastProductStatus
                                  : packStatus(ProductStatus());
    hasProductStatus = true;
    lastProductStatus = current;
    facadeDiagnostics.recordProductStatus(previous, current);
}

const char* blinker::BlinkerClass::lastErrorText() const {
    return errorText(lastError_);
}

blinker::BlinkerClass Blinker;
