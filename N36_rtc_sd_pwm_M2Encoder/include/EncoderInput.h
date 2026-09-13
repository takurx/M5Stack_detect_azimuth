#pragma once
#include <M2Encoder.h>
#include "Tracking.h"

namespace n36 {
constexpr uint8_t kTooFastFlag = 0x80;
// Use the current CRC-checked host API and charge both reads against their TTL.
// Device origin and an explicit host installation offset are separate.
inline Observation readEncoder(m2enc::M2Encoder& encoder, uint32_t started_ms, float offset_deg) {
    Observation o; o.at_ms = started_ms;
    m2enc::Reading r{};
    const m2enc::Result result = encoder.poll(r);
    if (result == m2enc::UnsupportedProtocol) { o.error = Reason::Firmware; return o; }
    if (result == m2enc::DeviceRestarted) { o.error = Reason::Restarted; return o; }
    if (result != m2enc::Ok) return o;
    // Name the sensor's own state before treating the sample as merely non-absolute.
    o.degraded = (r.core_status & (m2enc::Degraded | m2enc::BitFault)) != 0;
    if (!r.configured || r.state != 2) { o.error = Reason::NotScanning; return o; }
    if (r.core_status & m2enc::ConfigurationError) { o.error = Reason::SensorConfig; return o; }
    if (r.core_status & m2enc::Probation) { o.error = Reason::Probation; return o; }
    // Core 0x0E reports 0x80 when a sensor edge outran the scan (M2Encoder 0.2.1 names it TooFast);
    // the pinned 0.2.0 header has no symbol for it, so the raw bit is checked here.
    if (r.core_status & kTooFastFlag) { o.error = Reason::TooFast; return o; }
    if (r.core_status & m2enc::NeedMotion) { o.error = Reason::NeedMotion; return o; }
    m2enc::Position p;
    if (!r.usable(micros()) || encoder.readPosition(p) != m2enc::Ok) return o;
    const uint32_t now_us = micros();
    float degrees;
    if (!r.usable(now_us) || !p.angleDegrees(now_us, degrees) || !isfinite(offset_deg)) {
        o.error = Reason::NotAbsolute; return o;
    }
    const uint32_t status_left = r.valid_for_us - uint32_t(now_us - r.received_since_us);
    const uint32_t position_left = p.valid_for_us - uint32_t(now_us - p.received_since_us);
    const uint32_t remaining = status_left < position_left ? status_left : position_left;
    // Round down and reserve one millisecond for micros/millis phase alignment.
    if (remaining < 2000) { o.error = Reason::Stale; return o; }
    o.at_ms = millis();
    o.valid_for_ms = remaining / 1000 - 1;
    o.degrees = wrap(degrees + offset_deg);
    o.degraded = (r.core_status & (m2enc::Degraded | m2enc::BitFault)) != 0;
    o.valid = true; o.error = Reason::Ok; return o;
}
} // namespace n36
