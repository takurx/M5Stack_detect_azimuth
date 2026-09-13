#pragma once
#include <stdint.h>
extern uint32_t fake_utc;
extern bool fake_rtc_valid;
class DateTime {
    uint32_t utc_;
public:
    explicit DateTime(uint32_t utc) : utc_(utc) {}
    bool isValid() const { return fake_rtc_valid; }
    uint32_t unixtime() const { return utc_; }
};
class RTC_PCF8563 {
public:
    bool begin() { return true; }
    bool lostPower() { return !fake_rtc_valid; }
    bool isrunning() { return fake_rtc_valid; }
    DateTime now() { return DateTime(fake_utc); }
};
