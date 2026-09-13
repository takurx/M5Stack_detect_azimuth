#pragma once
#include <math.h>
#include <stdint.h>

namespace n36 {
inline float wrap(float a) { a = fmodf(a, 360.0f); return a < 0 ? a + 360.0f : a; }
inline float distance(float a, float b) { return fabsf(wrap(a - b + 180.0f) - 180.0f); }
enum class Reason { Disarmed, BusError, Firmware, NotAbsolute, Stale, NoSun,
                    Night, Alignment, AtTarget, Tracking, Cooldown, Stall };
inline const char* name(Reason r) {
    switch (r) {
    case Reason::Disarmed: return "DISARMED"; case Reason::BusError: return "BUS ERROR";
    case Reason::Firmware: return "FW UNSUPPORTED"; case Reason::NotAbsolute: return "NO ABSOLUTE";
    case Reason::Stale: return "STALE"; case Reason::NoSun: return "NO SUN DATA";
    case Reason::Night: return "NIGHT"; case Reason::Alignment: return "ALIGN MANUALLY";
    case Reason::AtTarget: return "AT TARGET"; case Reason::Tracking: return "TRACKING";
    case Reason::Cooldown: return "PULSE GAP"; case Reason::Stall: return "NO MOTION";
    }
    return "UNKNOWN";
}
struct Observation {
    float degrees = NAN;
    uint32_t at_ms = 0;
    uint32_t valid_for_ms = 150;
    Reason error = Reason::BusError;
    bool valid = false, degraded = false;
};
struct SunTarget { float azimuth = NAN, elevation = NAN; bool valid = false; };
struct Policy {
    uint32_t sample_ttl_ms = 150, pulse_ms = 200, gap_ms = 800, stall_on_ms = 2000;
    float deadband_deg = 0.4f, max_forward_deg = 5.0f, motion_deg = 0.3f;
};
// One-way PWM, as in N34. No automatic homing, reverse, IMU fallback or restart.
class Tracking {
    Policy policy_;
    Observation observation_;
    bool armed_ = false, on_ = false, gap_ = false, observed_ = false;
    uint32_t pulse_at_ = 0, stopped_at_ = 0, last_tick_ = 0, on_ms_ = 0;
    float anchor_ = NAN;
    Reason reason_ = Reason::Disarmed;
    void halt(Reason r, uint32_t now) {
        if (on_) stopped_at_ = now;
        on_ = false; armed_ = false; reason_ = r;
    }
public:
    explicit Tracking(Policy policy = Policy()) : policy_(policy) {}
    void observe(const Observation& o) { observation_ = o; observed_ = true; }
    void stop(uint32_t now) { halt(Reason::Disarmed, now); }
    void arm(uint32_t now) {
        if (armed_) return; // Repeated B presses must not extend PWM or clear the stall budget.
        // The next tick must validate every input before producing output.
        armed_ = true; on_ = false; gap_ = false; on_ms_ = 0;
        anchor_ = NAN; last_tick_ = now; reason_ = Reason::Disarmed;
    }
    void tick(uint32_t now, const SunTarget& sun) {
        uint32_t elapsed = now - last_tick_; last_tick_ = now;
        if (on_) on_ms_ += elapsed;
        if (!armed_) { on_ = false; return; }
        if (!observed_ || !observation_.valid) { halt(observation_.error, now); return; }
        if (now - observation_.at_ms >= policy_.sample_ttl_ms) { halt(Reason::Stale, now); return; }
        if (now - observation_.at_ms >= observation_.valid_for_ms) { halt(Reason::Stale, now); return; }
        if (!sun.valid || !isfinite(sun.azimuth) || !isfinite(sun.elevation) ||
            sun.azimuth < 0 || sun.azimuth >= 360 || sun.elevation < -90 || sun.elevation > 90) {
            halt(Reason::NoSun, now); return;
        }
        if (sun.elevation <= 0) { halt(Reason::Night, now); return; }
        if (!isfinite(anchor_) || distance(anchor_, observation_.degrees) >= policy_.motion_deg) {
            anchor_ = observation_.degrees; on_ms_ = 0;
        }
        if (on_ms_ >= policy_.stall_on_ms) { halt(Reason::Stall, now); return; }
        float ahead = wrap(sun.azimuth - observation_.degrees);
        if (distance(sun.azimuth, observation_.degrees) <= policy_.deadband_deg) {
            if (on_) { stopped_at_ = now; gap_ = true; }
            on_ = false; reason_ = Reason::AtTarget; return;
        }
        if (ahead > policy_.max_forward_deg) { halt(Reason::Alignment, now); return; }
        if (on_ && now - pulse_at_ >= policy_.pulse_ms) {
            on_ = false; stopped_at_ = now; gap_ = true;
        }
        if (!on_ && gap_ && now - stopped_at_ < policy_.gap_ms) { reason_ = Reason::Cooldown; return; }
        if (!on_) { on_ = true; pulse_at_ = now; }
        reason_ = Reason::Tracking;
    }
    bool motorOn() const { return on_; }
    bool armed() const { return armed_; }
    Reason reason() const { return reason_; }
};
} // namespace n36
