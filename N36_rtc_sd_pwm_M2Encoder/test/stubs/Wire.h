#pragma once
#include <stdint.h>
#include <stddef.h>
#include <array>
#include <assert.h>
extern uint32_t fake_bus_us;
class TwoWire {
    uint8_t selected_ = 0, cursor_ = 0;
    bool command_ = false;
    std::array<uint8_t,32> response_{};
    static uint8_t crc(const uint8_t* p) {
        uint8_t c=0;
        for(unsigned i=0;i<31;++i) {
            c^=p[i];
            for(unsigned b=0;b<8;++b)c=uint8_t((c<<1)^((c&128)?7:0));
        }
        return c;
    }
public:
    void begin(int, int) {}
    void setClock(uint32_t) {}
    void setTimeOut(uint16_t) {}
    std::array<uint8_t,32> status{}, position{}, diagnostic{}, quality{};
    bool nack = false, short_read = false, corrupt = false;
    // Command model: the next STATUS frame carries the receipt unless auto_ack is off.
    bool auto_ack = true; uint8_t ack_result = 0, last_op = 0; uint16_t last_id = 0;
    uint32_t latency_us=0;
    unsigned command_count=0;
    void beginTransmission(uint8_t address) { assert(address==0x36); command_=false; }
    size_t write(uint8_t selector) { selected_=selector; return 1; }
    size_t write(const uint8_t* p,size_t n) {
        command_=true; ++command_count;
        if (n == 25 && p[0] == 0xa0) {
            last_id = uint16_t(p[2] | (p[3] << 8)); last_op = p[4];
            if (auto_ack) {
                status[2] = p[2]; status[3] = p[3]; status[4] = ack_result;
                if (ack_result == 0) {
                    if (last_op == 3 || last_op == 4) { status[5] = last_op; status[25] |= 2; }   // FIXED/AUTO configured
                    if (last_op == 2) status[6] = 2;                                             // START -> running
                    if (last_op == 1) status[6] = 3;                                             // STOP
                }
            }
        }
        return n;
    }
    uint8_t endTransmission(bool stop=true) { assert(stop==command_); return nack ? 2 : 0; }
    uint8_t requestFrom(uint8_t address,uint8_t n,uint8_t stop) {
        assert(address==0x36 && n==32 && stop);
        response_=selected_==0x80 ? status : selected_==0x84 ? diagnostic : selected_==0x82 ? quality : position;
        response_[31]=crc(response_.data());
        if(corrupt)response_[31]^=1;
        cursor_=0; fake_bus_us+=latency_us;
        return short_read ? 31 : 32;
    }
    int available() { return cursor_ < (short_read ? 31 : 32); }
    int read() { return available() ? response_[cursor_++] : -1; }
};
extern TwoWire Wire;
