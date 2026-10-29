#ifndef BLINKER_TEST_SERIAL_OBSERVATION_H
#define BLINKER_TEST_SERIAL_OBSERVATION_H

#include <BlinkerV2/location/LocationFix.h>
#include <string.h>

namespace blinker { namespace cloud_location_candidate {

enum class ObservationStatus : uint8_t { Accepted, Invalid, OutOfOrder, LimitReached, ApplyRejected };
enum class ObservationKind : uint8_t { None, Fix, Lost };

struct ObservationReply {
    uint32_t sequence;
    ObservationKind kind;
    ObservationStatus status;
    ErrorCode error;
    ObservationReply() : sequence(0U), kind(ObservationKind::None),
        status(ObservationStatus::Invalid), error(ErrorCode::InvalidArgument) {}
    static const char* source() { return "InjectedNonSatellite"; }
};

// Internal, explicitly authorized test input only; no Arduino, driver, heap,
// timer, transport or default coordinates. The caller feeds at most 32 bytes
// per poll and prints each returned reply before accepting further commands.
class SerialObservation {
public:
    typedef Result (*FixSink)(const LocationFix&, uint32_t sampledAtMillis);
    typedef void (*LostSink)();
    static const size_t maximumBytesPerPoll = 32U;
    static const uint8_t maximumAttempts = 16U;

    SerialObservation(FixSink fix, LostSink lost)
        : fix_(fix), lost_(lost), line_(), lastSequence_(0U), used_(0U), attempts_(0U), rejectedLine_(false) {}

    // One byte has constant bounded work. A terminal line produces one reply.
    // Sequence is consumed before calling the sink: uncertain callback results
    // or a lost serial reply must never cause this boot to replay an observation.
    bool push(uint8_t byte, uint32_t nowMillis, ObservationReply& reply) {
        if (byte != '\n') {
            if (rejectedLine_) return false;
            if (used_ >= sizeof(line_) - 1U || (byte < 0x20U && byte != '\r') || byte > 0x7eU) {
                rejectedLine_ = true;
            } else line_[used_++] = static_cast<char>(byte);
            return false;
        }
        reply = ObservationReply();
        if (!rejectedLine_) {
            if (used_ != 0U && line_[used_ - 1U] == '\r') --used_;
            line_[used_] = '\0';
            parse(nowMillis, reply);
        }
        used_ = 0U;
        rejectedLine_ = false;
        return true;
    }

    uint32_t lastSequence() const { return lastSequence_; }
    uint8_t attempts() const { return attempts_; }

private:
    static bool number(const char*& cursor, uint32_t limit, uint32_t& result) {
        if (*cursor < '0' || *cursor > '9') return false;
        if (*cursor == '0' && cursor[1] >= '0' && cursor[1] <= '9') return false;
        uint32_t value = 0U;
        do {
            const uint32_t digit = static_cast<uint32_t>(*cursor++ - '0');
            if (value > (limit - digit) / 10U) return false;
            value = value * 10U + digit;
        } while (*cursor >= '0' && *cursor <= '9');
        result = value;
        return true;
    }

    static bool coordinate(const char*& cursor, uint32_t limit, int32_t& result) {
        const bool negative = *cursor == '-';
        if (negative) ++cursor;
        uint32_t value = 0U;
        if (!number(cursor, limit, value) || (negative && value == 0U)) return false;
        result = negative ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);
        return true;
    }

    void parse(uint32_t nowMillis, ObservationReply& reply) {
        const bool isFix = used_ >= 4U && memcmp(line_, "FIX ", 4U) == 0;
        const bool isLost = used_ >= 5U && memcmp(line_, "LOST ", 5U) == 0;
        if (!isFix && !isLost) return;
        const char* cursor = line_ + (isFix ? 4U : 5U);
        uint32_t sequence = 0U;
        if (!number(cursor, UINT32_MAX, sequence) || sequence == 0U) return;
        LocationFix observation;
        if (isFix) {
            if (*cursor++ != ' ' || !coordinate(cursor, 900000000U, observation.latitudeE7) ||
                *cursor++ != ' ' || !coordinate(cursor, 1800000000U, observation.longitudeE7)) return;
            observation.valid = true;
        }
        if (*cursor != '\0') return;
        reply.sequence = sequence;
        reply.kind = isFix ? ObservationKind::Fix : ObservationKind::Lost;
        if (sequence <= lastSequence_) {
            reply.status = ObservationStatus::OutOfOrder; reply.error = ErrorCode::SequenceConflict; return;
        }
        if (attempts_ >= maximumAttempts) {
            reply.status = ObservationStatus::LimitReached; reply.error = ErrorCode::CapacityExceeded; return;
        }
        lastSequence_ = sequence;
        ++attempts_;
        if ((isFix && fix_ == nullptr) || (isLost && lost_ == nullptr)) {
            reply.status = ObservationStatus::ApplyRejected;
            reply.error = ErrorCode::NotConfigured;
            return;
        }
        Result result;
        if (isFix) result = fix_(observation, nowMillis);
        else lost_();
        reply.status = result ? ObservationStatus::Accepted : ObservationStatus::ApplyRejected;
        reply.error = result.code();
    }

    FixSink fix_;
    LostSink lost_;
    char line_[96];
    uint32_t lastSequence_;
    uint8_t used_;
    uint8_t attempts_;
    bool rejectedLine_;
    SerialObservation(const SerialObservation&);
    SerialObservation& operator=(const SerialObservation&);
};

} } // namespace blinker::cloud_location_candidate

#endif
