#include <stdio.h>
#include <stdlib.h>
#include <limits>
#include "EncoderInput.h"
#include "SunTable.h"
TwoWire Wire;
static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static void registers(TwoWire& bus, float angle) {
    bus.registers.fill(0); bus.nack = bus.short_read = false;
    unsigned cell = (unsigned)floorf(n36::wrap(angle) * 5.0f);
    bus.registers[0] = cell; bus.registers[1] = cell >> 8;
    bus.registers[4] = m2enc::ST_ABSOLUTE;
    bus.registers[5] = 1; bus.registers[7] = 7; bus.registers[14] = 1;
}
static n36::Observation obs(float angle, uint32_t now) {
    n36::Observation o; o.valid = true; o.degrees = angle; o.at_ms = now;
    return o;
}
static n36::SunTarget sun(float angle, float altitude = 30) {
    n36::SunTarget s; s.azimuth = angle; s.elevation = altitude; s.valid = true; return s;
}
static void adapter_tests() {
    TwoWire bus; m2enc::M2Encoder encoder; registers(bus, 120);
    CHECK(encoder.begin(bus));
    auto o = n36::readEncoder(encoder, 50, 2); CHECK(o.valid && fabs(o.degrees - 122) < .001);
    bus.nack = true; o = n36::readEncoder(encoder, 60, 0); CHECK(!o.valid && !isfinite(o.degrees));
    registers(bus, 120); bus.short_read = true; CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    for (uint8_t version : {0, 6, 14, 255}) {
        registers(bus, 120); bus.registers[7] = version;
        CHECK(n36::readEncoder(encoder, 0, 0).error == n36::Reason::Firmware);
    }
    for (uint8_t flag : {m2enc::ST_PROBATION, m2enc::ST_CFG_ERROR, m2enc::ST_NEED_MOTION}) {
        registers(bus, 120); bus.registers[4] |= flag; CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    }
    registers(bus, 120); bus.registers[14] = 2; CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    registers(bus, 120); bus.registers[20] = 1; CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    registers(bus, 120); bus.registers[0] = 0xff; bus.registers[1] = 0xff;
    CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    registers(bus, 120); bus.registers[0] = 8; bus.registers[1] = 7; // 1800, not a valid angle
    CHECK(!n36::readEncoder(encoder, 0, 0).valid);
    registers(bus, 120); bus.registers[4] |= m2enc::ST_DEGRADED; bus.registers[16] = 3;
    o = n36::readEncoder(encoder, 0, 0); CHECK(o.valid && o.degraded);
    CHECK(!n36::readEncoder(encoder, 0, NAN).valid);
    registers(bus, 359.8f); o = n36::readEncoder(encoder, 0, 1); CHECK(o.valid && o.degrees < 1.1f);
}
static void controller_tests() {
    n36::Tracking t; t.observe(obs(359, 0)); t.tick(0, sun(1)); CHECK(!t.motorOn());
    t.arm(0); t.tick(0, sun(1)); CHECK(t.motorOn());
    t.observe(obs(359, 100)); t.tick(199, sun(1)); CHECK(t.motorOn());
    t.arm(199); // Repeated arm must not restart the 200 ms pulse.
    t.tick(200, sun(1)); CHECK(!t.motorOn());
    t.observe(obs(359, 999)); t.tick(999, sun(1)); CHECK(!t.motorOn());
    t.tick(1000, sun(1)); CHECK(t.motorOn());
    t.stop(1001); t.tick(1002, sun(1)); CHECK(!t.motorOn() && !t.armed());
    t.arm(1002); t.observe(obs(2, 1002)); t.tick(1002, sun(1));
    CHECK(!t.motorOn() && t.reason() == n36::Reason::Alignment);
    t.arm(0); t.observe(obs(10, 0)); t.tick(149, sun(11)); CHECK(t.motorOn());
    t.tick(150, sun(11)); CHECK(!t.motorOn() && t.reason() == n36::Reason::Stale);
    t.observe(obs(10, 151)); t.tick(151, sun(11)); CHECK(!t.armed()); // No auto-restart.
    for (int mode = 0; mode < 4; ++mode) {
        t.arm(0); t.observe(obs(10, 0)); auto s = sun(11);
        if (mode == 0) s.valid = false;
        if (mode == 1) s.azimuth = NAN;
        if (mode == 2) s.elevation = 0;
        if (mode == 3) s.azimuth = 360;
        t.tick(0, s); CHECK(!t.motorOn() && !t.armed());
    }
    t.arm(0); t.observe(obs(10, 0)); t.tick(0, sun(10.2)); CHECK(!t.motorOn());
    t.stop(0);
    t.arm(UINT32_MAX - 99); t.observe(obs(10, UINT32_MAX - 99));
    t.tick(UINT32_MAX - 99, sun(11)); CHECK(t.motorOn());
    t.observe(obs(10, 0)); t.tick(100, sun(11)); CHECK(!t.motorOn()); // 200 ms across wrap.
    t.stop(100); t.arm(0);
    for (uint32_t ms = 0; ms <= 12000; ms += 10) {
        if (ms % 100 == 0 && t.armed()) t.arm(ms);
        t.observe(obs(10, ms)); t.tick(ms, sun(12));
    }
    CHECK(!t.motorOn() && t.reason() == n36::Reason::Stall);
    t.arm(0); t.observe(obs(10, 0)); t.tick(0, sun(12)); CHECK(t.motorOn());
    t.observe(n36::Observation()); t.tick(1, sun(12)); CHECK(!t.motorOn() && !t.armed());
}
static void table_tests() {
    n36::SunRow a, b;
    CHECK(n36::parseSunRow("0 2026-01-01 00:00:00 30.0 359.8", a));
    CHECK(a.utc == 1767225600UL);
    CHECK(n36::parseSunRow("1 2026-01-01 00:05:00 31.0 0.2\r", b));
    CHECK(b.utc - a.utc == 300);
    for (const char* bad : {"", "0 2026-02-29 00:00:00 30 1", "0 2026-01-01 25:00:00 30 1",
            "0 2026-01-01 00:00:00 nan 1", "0 2026-01-01 00:00:00 30 360",
            "0 2026-01-01 00:00:00 30 1 garbage", "0 2026-01-01 00:00:00 30"}) {
        n36::SunRow invalid; CHECK(!n36::parseSunRow(bad, invalid));
    }
    n36::SunTable table; CHECK(!table.target(a.utc).valid);
    CHECK(table.push(a, a.utc)); CHECK(!table.target(a.utc).valid); // EOF is not an endless target.
    CHECK(table.push(b, a.utc)); CHECK(table.target(a.utc + 299).valid);
    CHECK(!table.target(a.utc + 300).valid); // Must load a following row.
    n36::SunTable future; CHECK(future.push(b, a.utc)); CHECK(!future.target(a.utc).valid);
    n36::SunTable reversed; CHECK(reversed.push(a, a.utc)); CHECK(!reversed.push(a, a.utc)); CHECK(reversed.failed());
    n36::SunTable jump; CHECK(jump.push(a, a.utc)); CHECK(jump.push(b, a.utc));
    CHECK(jump.target(a.utc + 20).valid); CHECK(!jump.target(a.utc + 10).valid); // RTC backwards.
    n36::SunTable gap; b.utc = a.utc + 301; CHECK(gap.push(a, a.utc)); CHECK(gap.push(b, a.utc));
    CHECK(!gap.target(a.utc).valid);
}
// Independent physical state: only PWM drives angle. Sun/IMU do not assign
// encoder position. A quantized register-file model supplies the REAL library.
struct Plant {
    double angle = 120, velocity = 0, rate = .8, tau = .05, slack = .1;
    bool jammed = false;
    void step(bool pwm) {
        double wanted = pwm && !jammed ? rate : 0;
        velocity += (wanted - velocity) * (.001 / tau);
        double movement = velocity * .001;
        if (slack > 0) { double used = fmin(slack, movement); slack -= used; movement -= used; }
        angle += movement;
    }
};
static void world_tests() {
    unsigned scenarios = 0; double max_error = 0; unsigned max_pulse = 0;
    for (double rate : {.4, .8, 1.2}) for (double inertia : {.02, .08}) {
        Plant plant; plant.rate = rate; plant.tau = inertia;
        TwoWire bus; registers(bus, plant.angle); m2enc::M2Encoder enc; CHECK(enc.begin(bus));
        n36::Tracking tracking; tracking.arm(0); unsigned pulse = 0;
        for (uint32_t ms = 0; ms < 120000; ++ms) {
            plant.step(tracking.motorOn());
            // Analytic sensitivity fixture, NOT astronomical ephemeris accuracy.
            auto target = sun(float(121 + .003 * ms / 1000.0));
            if (ms % 50 == 0) { registers(bus, plant.angle); tracking.observe(n36::readEncoder(enc, ms, 0)); }
            tracking.tick(ms, target);
            if (tracking.motorOn()) { ++pulse; if (pulse > max_pulse) max_pulse = pulse; } else pulse = 0;
            CHECK(tracking.armed()); CHECK(pulse <= 200);
            if (ms > 30000) { double error = n36::distance(plant.angle, target.azimuth); if (error > max_error) max_error = error; CHECK(error < .65); }
        }
        ++scenarios;
    }
    // Bus break while driving stops immediately when the next read fails.
    for (bool short_read : {false, true}) {
        TwoWire bus; registers(bus, 120); m2enc::M2Encoder enc; CHECK(enc.begin(bus));
        n36::Tracking tracking; tracking.arm(0);
        tracking.observe(n36::readEncoder(enc, 0, 0)); tracking.tick(0, sun(121)); CHECK(tracking.motorOn());
        bus.nack = !short_read; bus.short_read = short_read;
        tracking.observe(n36::readEncoder(enc, 50, 0)); tracking.tick(50, sun(121)); CHECK(!tracking.motorOn());
        registers(bus, 120); tracking.observe(n36::readEncoder(enc, 100, 0)); tracking.tick(100, sun(121));
        CHECK(!tracking.armed()); ++scenarios;
    }
    // Jam / frozen register model must trip the cumulative no-motion policy.
    for (bool frozen : {false, true}) {
        Plant plant; plant.jammed = !frozen; TwoWire bus; registers(bus, 120);
        m2enc::M2Encoder enc; CHECK(enc.begin(bus)); n36::Tracking tracking; tracking.arm(0);
        for (uint32_t ms = 0; ms < 12000; ++ms) {
            plant.step(tracking.motorOn());
            if (ms % 50 == 0) { registers(bus, frozen ? 120 : plant.angle); tracking.observe(n36::readEncoder(enc, ms, 0)); }
            tracking.tick(ms, sun(122));
        }
        CHECK(!tracking.motorOn() && tracking.reason() == n36::Reason::Stall); ++scenarios;
    }
    printf("{\"world_scenarios\":%u,\"max_tracking_error_deg\":%.6f,\"max_pulse_ms\":%u,\"checks\":%u,\"physical_proven\":false}\n",
           scenarios, max_error, max_pulse, checks);
}
int main() { adapter_tests(); controller_tests(); table_tests(); world_tests(); return 0; }
