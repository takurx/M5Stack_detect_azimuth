#pragma once
#include <M2Encoder.h>
#include "Tracking.h"

namespace n36 {
// Explicit, receipt-checked scan configuration and start. Disabled unless the build
// supplies a qualified period (N36_SCAN_PERIOD_US > 0); never runs on its own.
struct ScanProfile { uint32_t period_us = 0, settle_us = 0, blank_us = 0; uint8_t stable_reads = 0; bool inverted = false;
                     bool enabled() const { return period_us > 0 && stable_reads > 0; } };
class ScanStarter {
public:
    enum State { Idle, Configuring, Starting, Done, Failed };
private:
    ScanProfile profile_;
    State state_ = Idle;
    uint16_t pending_ = 0; uint32_t deadline_ = 0; bool retried_ = false;
    uint8_t last_result_ = 0; uint32_t timeout_ms_;
    void submitted(m2enc::M2Encoder& e, m2enc::Result r, State next, uint32_t now) {
        if (r != m2enc::Submitted) { state_ = Failed; return; }
        state_ = next; pending_ = e.pendingId(); deadline_ = now + timeout_ms_; retried_ = false;
    }
public:
    explicit ScanStarter(ScanProfile p = ScanProfile(), uint32_t timeout_ms = 500) : profile_(p), timeout_ms_(timeout_ms) {}
    State state() const { return state_; }
    bool active() const { return state_ == Configuring || state_ == Starting; }
    uint8_t lastResult() const { return last_result_; }
    // Called on an explicit operator action only.
    bool request(m2enc::M2Encoder& e, uint32_t now) {
        if (!profile_.enabled() || active()) return false;
        m2enc::Timing t = { profile_.settle_us, profile_.blank_us, profile_.stable_reads, profile_.inverted };
        submitted(e, e.configureFixed(profile_.period_us, t), Configuring, now);
        return state_ == Configuring;
    }
    // One poll per call; returns the reading so the caller can keep observing status.
    m2enc::Result service(m2enc::M2Encoder& e, m2enc::Reading& r, uint32_t now) {
        m2enc::Result res = e.poll(r);
        if (!active()) return res;
        if (res == m2enc::Ok && r.command_id == pending_) {
            last_result_ = r.command_result;
            if (r.command_result == m2enc::M2_OK) {
                if (state_ == Configuring) submitted(e, e.start(), Starting, now);
                else state_ = Done;
            } else state_ = Failed;
            return res;
        }
        if (int32_t(now - deadline_) >= 0) {
            if (!retried_ && e.retry() == m2enc::Submitted) { retried_ = true; deadline_ = now + timeout_ms_; }
            else state_ = Failed;
        }
        return res;
    }
    const char* name() const {
        switch (state_) {
        case Idle: return "SCAN IDLE"; case Configuring: return "SCAN CONFIG"; case Starting: return "SCAN START";
        case Done: return "SCAN STARTED"; case Failed: return "SCAN FAILED";
        }
        return "SCAN ?";
    }
};
} // namespace n36
