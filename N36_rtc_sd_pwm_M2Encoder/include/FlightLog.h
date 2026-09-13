#pragma once
#include <stdint.h>
#include <stdio.h>
#include "Tracking.h"

namespace n36 {
// Field log: encoder samples and controller state, buffered in RAM and written to SD only
// while no pulse is requested. From the angle column during pulses the loaded shaft speed
// of the actual installation can be read; nothing here is a measured specification.
struct LogRow {
    uint32_t ms = 0, utc = 0; float angle = NAN, target = NAN, rate = NAN;
    uint8_t reason = 0, valid = 0, pwm = 0, resolves = 0, reset_cause = 0, dead = 0; uint16_t fault = 0, pulse_ms = 0;
};
template<class FileT, unsigned N = 48> class FlightLog {
    LogRow rows_[N]; unsigned head_ = 0, count_ = 0; uint32_t dropped_ = 0, written_ = 0;
public:
    void record(const LogRow& r) {
        if (count_ == N) { ++dropped_; head_ = (head_ + 1) % N; --count_; } // oldest row yields
        rows_[(head_ + count_) % N] = r; ++count_;
    }
    unsigned pending() const { return count_; }
    uint32_t dropped() const { return dropped_; }
    uint32_t written() const { return written_; }
    static void header(FileT& f) { f.print("ms,utc,angle,valid,reason,target,pwm,pulse_ms,rate_deg_per_ms,resolves,reset_cause,fault,dead\n"); }
    // Writes at most `limit` rows per call; the caller guarantees the motor is off.
    void flush(FileT& f, unsigned limit) {
        char line[160];
        while (count_ && limit--) {
            const LogRow& r = rows_[head_];
            snprintf(line, sizeof line, "%lu,%lu,%.2f,%u,%s,%.2f,%u,%u,%.5f,%u,%u,%u,%u\n",
                     (unsigned long)r.ms, (unsigned long)r.utc, (double)r.angle, r.valid, name(Reason(r.reason)), (double)r.target,
                     r.pwm, r.pulse_ms, (double)r.rate, r.resolves, r.reset_cause, r.fault, r.dead);
            f.print(line); ++written_;
            head_ = (head_ + 1) % N; --count_;
        }
        f.flush();
    }
};
} // namespace n36
