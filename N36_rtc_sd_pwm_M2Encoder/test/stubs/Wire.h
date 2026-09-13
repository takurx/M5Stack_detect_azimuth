#pragma once
#include <stdint.h>
#include <stddef.h>
#include <array>
class TwoWire {
    uint8_t selected_ = 0, cursor_ = 0;
public:
    void begin(int, int) {}
    void setClock(uint32_t) {}
    void setTimeOut(uint16_t) {}
    std::array<uint8_t, 23> registers{};
    bool nack = false, short_read = false;
    void beginTransmission(uint8_t) {}
    size_t write(uint8_t selector) { selected_ = selector; return 1; }
    uint8_t endTransmission(bool = true) { return nack ? 2 : 0; }
    uint8_t requestFrom(uint8_t, uint8_t n) { cursor_ = selected_; return short_read ? n - 1 : n; }
    int read() { return cursor_ < registers.size() ? registers[cursor_++] : 0xee; }
};
extern TwoWire Wire;
