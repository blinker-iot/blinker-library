#ifndef BLINKER_TEST_R4_CLEANUP_IPADDRESS_H
#define BLINKER_TEST_R4_CLEANUP_IPADDRESS_H
#include "Arduino.h"
#include <cstdio>
namespace arduino { enum IPType { IPv4, IPv6 }; }
class IPAddress {
public:
    IPAddress() : bytes_{0U, 0U, 0U, 0U}, type_(arduino::IPv4) {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : bytes_{a, b, c, d}, type_(arduino::IPv4) {}
    explicit IPAddress(uint32_t value) : bytes_{}, type_(arduino::IPv4) { std::memcpy(bytes_, &value, sizeof(bytes_)); }
    explicit IPAddress(arduino::IPType type) : bytes_{}, type_(type) {}
    arduino::IPType type() const { ++typeCalls(); return type_; }
    uint8_t operator[](size_t index) const { ++indexCalls(); return bytes_[index]; }
    uint8_t& operator[](size_t index) { ++indexCalls(); return bytes_[index]; }
    static unsigned& typeCalls() { static unsigned value = 0U; return value; }
    static unsigned& indexCalls() { static unsigned value = 0U; return value; }
    static unsigned& toStringCalls() { static unsigned value = 0U; return value; }
    static bool& failIpv4ToString() { static bool value = false; return value; }
    static void resetObservations() { typeCalls() = indexCalls() = toStringCalls() = 0U; }
    bool fromString(const char* text) {
        unsigned a, b, c, d;
        if (text == nullptr || std::sscanf(text, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
            a > 255U || b > 255U || c > 255U || d > 255U) return false;
        bytes_[0] = static_cast<uint8_t>(a); bytes_[1] = static_cast<uint8_t>(b);
        bytes_[2] = static_cast<uint8_t>(c); bytes_[3] = static_cast<uint8_t>(d);
        type_ = arduino::IPv4; return true;
    }
    String toString() const {
        ++toStringCalls();
        // Compatibility seams, not the native Arduino String allocator or
        // IPv6 formatter. A missing IPv4 conversion makes route regressions
        // observable without claiming an actual out-of-memory experiment.
        if (type_ == arduino::IPv6) return "2001:db8::1";
        if (failIpv4ToString()) return String();
        return std::to_string(bytes_[0]) + "." + std::to_string(bytes_[1]) + "." +
            std::to_string(bytes_[2]) + "." + std::to_string(bytes_[3]);
    }
    bool operator==(const IPAddress& other) const { return type_ == other.type_ && std::memcmp(bytes_, other.bytes_, sizeof(bytes_)) == 0; }
    bool operator==(uint32_t value) const { return *this == IPAddress(value); }
private:
    uint8_t bytes_[4];
    arduino::IPType type_;
};
#endif
