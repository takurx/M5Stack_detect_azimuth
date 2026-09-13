#pragma once
#include <math.h>
#include <stdint.h>

namespace n36 {
inline float wrap(float a) { a = fmodf(a, 360.0f); return a < 0 ? a + 360.0f : a; }
inline float distance(float a, float b) { return fabsf(wrap(a - b + 180.0f) - 180.0f); }
enum class Reason { Ok, Disarmed, BusError, Firmware, NotAbsolute, Stale, NoSun,
                    Night, Alignment, AtTarget, Tracking, Cooldown, Stall,
                    NeedMotion, Probation, NotScanning, SensorConfig, Restarted, WaitSun };
inline const char* name(Reason r) {
    switch (r) {
    case Reason::Ok: return "OK"; case Reason::Disarmed: return "DISARMED"; case Reason::BusError: return "BUS ERROR";
    case Reason::Firmware: return "FW UNSUPPORTED"; case Reason::NotAbsolute: return "NO ABSOLUTE";
    case Reason::Stale: return "STALE"; case Reason::NoSun: return "NO SUN DATA";
    case Reason::Night: return "NIGHT"; case Reason::Alignment: return "ALIGN MANUALLY";
    case Reason::AtTarget: return "AT TARGET"; case Reason::Tracking: return "TRACKING";
    case Reason::Cooldown: return "PULSE GAP"; case Reason::Stall: return "NO MOTION";
    case Reason::NeedMotion: return "NEED MOTION"; case Reason::Probation: return "PROBATION";
    case Reason::NotScanning: return "NOT SCANNING"; case Reason::SensorConfig: return "SENSOR CONFIG";
    case Reason::Restarted: return "SENSOR RESTART"; case Reason::WaitSun: return "WAIT SUN";
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
    // Adaptive pulse: start at min_pulse_ms, learn degrees per ms from the encoder after each
    // settled pulse, and size the next pulse to 70 % of the remaining angle, never above pulse_ms.
    // A pulse that produced no measurable motion (< one 0.2 deg cell) doubles the next one.
    bool adaptive = true; uint32_t min_pulse_ms = 20;
    // Overshoot up to wait_deg holds and waits for the sun (one-way drive); beyond it, ALIGN MANUALLY.
    float wait_deg = 2.0f;
    // Blind forward pulses allowed per arm while the sensor reports NEED MOTION and no
    // sensor fault. 0 disables them (default): the shaft never moves without a valid angle.
    uint8_t resolve_pulses = 0; uint32_t resolve_pulse_ms = 100;
    float deadband_deg = 0.4f, max_forward_deg = 5.0f, motion_deg = 0.5f; // motion_deg = 2.5 sensor cells of 0.2 deg
};
// One-way PWM, as in N34. No automatic homing, reverse, IMU fallback or restart.
class Tracking {
    Policy policy_;
    Observation observation_;
    bool armed_ = false, on_ = false, gap_ = false, observed_ = false;
    uint8_t resolves_ = 0;
    uint32_t pulse_at_ = 0, stopped_at_ = 0, last_tick_ = 0, on_ms_ = 0, pulse_len_ = 0;
    bool measure_pending_ = false;
    float anchor_ = NAN, pulse_start_deg_ = NAN, rate_ = NAN; // rate_: learned deg per ms, NAN until measured
    uint32_t clampPulse(float ms) const {
        uint32_t x = ms < float(policy_.min_pulse_ms) ? policy_.min_pulse_ms : uint32_t(ms);
        if (x > policy_.pulse_ms) x = policy_.pulse_ms;
        return x;
    }
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
        armed_ = true; on_ = false; gap_ = false; on_ms_ = 0; resolves_ = 0; measure_pending_ = false;
        pulse_len_ = policy_.adaptive ? policy_.min_pulse_ms : policy_.pulse_ms;
        anchor_ = NAN; last_tick_ = now; reason_ = Reason::Disarmed;
    }
    void tick(uint32_t now, const SunTarget& sun) {
        uint32_t elapsed = now - last_tick_; last_tick_ = now;
        if (on_) on_ms_ += elapsed;
        if (!armed_) { on_ = false; return; }
        bool settling = !on_ && gap_ && now - stopped_at_ < policy_.gap_ms; // motor off, the last pulse's motion not yet settled
        if (!observed_ || !observation_.valid) {
            // Bounded blind resolve: only NEED MOTION, only without a sensor fault, only within budget.
            if (observed_ && observation_.error == Reason::NeedMotion && !observation_.degraded &&
                resolves_ < policy_.resolve_pulses) {
                if (on_ && now - pulse_at_ >= policy_.resolve_pulse_ms) { on_ = false; stopped_at_ = now; gap_ = true; ++resolves_; }
                else if (!on_) {
                    if (gap_ && now - stopped_at_ < policy_.gap_ms) { reason_ = Reason::Cooldown; return; }
                    on_ = true; pulse_at_ = now;
                }
                reason_ = Reason::NeedMotion; return;
            }
            // While coasting after a pulse the sensor may lose lock (NEED MOTION) or hand back a non-absolute sample;
            // that is expected until the gap has elapsed, so keep waiting instead of halting.
            bool motion_only = observed_ && (observation_.error == Reason::NeedMotion || observation_.error == Reason::NotAbsolute);
            if (motion_only && on_) { on_ = false; stopped_at_ = now; gap_ = true; measure_pending_ = true; reason_ = Reason::Cooldown; return; } // lock lost mid-pulse: stop now, settle
            if (settling && observed_ && motion_only) { reason_ = Reason::Cooldown; return; }
            halt(observation_.error, now); return;
        }
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
        if (ahead > 180.0f) { // the shaft is ahead of the sun: one-way drive cannot back up
            float over = 360.0f - ahead;
            if (over <= policy_.wait_deg) {
                if (on_) { stopped_at_ = now; gap_ = true; }
                on_ = false; reason_ = Reason::WaitSun; return;
            }
            halt(Reason::Alignment, now); return;
        }
        if (ahead > policy_.max_forward_deg) { halt(Reason::Alignment, now); return; }
        if (on_ && now - pulse_at_ >= pulse_len_) {
            on_ = false; stopped_at_ = now; gap_ = true; measure_pending_ = true;
        }
        if (!on_ && gap_ && now - stopped_at_ < policy_.gap_ms) { reason_ = Reason::Cooldown; return; }
        if (!on_) {
            if (measure_pending_) { // the gap has elapsed: the pulse's motion has settled
                float moved = wrap(observation_.degrees - pulse_start_deg_);
                if (moved > 180.0f) moved = 0; // backwards is not forward motion
                if (moved >= 0.2f) { float r = moved / float(pulse_len_); rate_ = isfinite(rate_) ? 0.5f * (rate_ + r) : r; }
                else if (policy_.adaptive) pulse_len_ = clampPulse(2.0f * float(pulse_len_));
                measure_pending_ = false;
            }
            if (policy_.adaptive && isfinite(rate_) && rate_ > 0) pulse_len_ = clampPulse(0.7f * ahead / rate_);
            else if (!policy_.adaptive) pulse_len_ = policy_.pulse_ms;
            on_ = true; pulse_at_ = now; pulse_start_deg_ = observation_.degrees;
        }
        reason_ = Reason::Tracking;
    }
    bool motorOn() const { return on_; }
    uint8_t resolvePulsesUsed() const { return resolves_; }
    uint32_t pulseLengthMs() const { return pulse_len_; }
    float rateDegPerMs() const { return rate_; }
    bool armed() const { return armed_; }
    Reason reason() const { return reason_; }
};
} // namespace n36
