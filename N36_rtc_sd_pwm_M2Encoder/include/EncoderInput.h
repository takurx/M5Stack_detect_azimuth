#pragma once
#include <M2Encoder.h>
#include "Tracking.h"

namespace n36 {
// Use the current CRC-checked host API and charge both reads against their TTL.
// Device origin and an explicit host installation offset are separate.
inline Observation readEncoder(m2enc::M2Encoder& encoder, uint32_t started_ms, float offset_deg) {
    Observation o; o.at_ms = started_ms;
    m2enc::Reading r{};
    const m2enc::Result result = encoder.poll(r);
    if (result == m2enc::UnsupportedProtocol) { o.error = Reason::Firmware; return o; }
    if (result != m2enc::Ok) return o;
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
