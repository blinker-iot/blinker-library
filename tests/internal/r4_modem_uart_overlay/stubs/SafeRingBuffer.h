#ifndef BLINKER_TEST_R4_UART_RING_H
#define BLINKER_TEST_R4_UART_RING_H
#include <deque>
namespace arduino {
template <size_t Size> class SafeRingBufferN {
    std::deque<int> values_;
public:
    bool availableForStore() const { return values_.size() < Size; }
    void store_char(int value) { values_.push_back(value); }
    int available() const { return static_cast<int>(values_.size()); }
    int peek() const { return values_.empty() ? -1 : values_.front(); }
    int read_char() { if (values_.empty()) return -1; int v=values_.front(); values_.pop_front(); return v; }
    void clear() { values_.clear(); }
};
}
#endif
