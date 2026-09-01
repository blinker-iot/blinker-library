#include "DirectRecord.h"

#include "bbp2/Frame.h"
#include "../security/DirectSecureSession.h"

namespace blinker {
namespace direct {

Result decodeRecordSize(ByteView prefix, size_t& recordSize) {
    recordSize = 0U;
    if (prefix.data == nullptr || prefix.empty()) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (prefix.data[0] == security::kDirectSecureMagic) {
        return security::decodeDirectSecureRecordSize(prefix, recordSize);
    }
    bbp2::FrameHeader header;
    Result result = bbp2::decodeHeader(prefix, header);
    if (result) recordSize = bbp2::encodedFrameSize(header);
    return result;
}

Result validateRecord(ByteView record) {
    if (record.data == nullptr || record.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (record.data[0] == security::kDirectSecureMagic) {
        security::DirectSecureRecordView parsed;
        return security::parseDirectSecureRecord(record, parsed);
    }
    bbp2::FrameView parsed;
    return bbp2::parseFrame(record, parsed);
}

} // namespace direct
} // namespace blinker
