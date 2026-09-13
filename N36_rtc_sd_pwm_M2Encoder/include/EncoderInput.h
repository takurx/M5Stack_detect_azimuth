#pragma once
#include <M2Encoder.h>
#include "Tracking.h"

namespace n36 {
// Adapter uses the published library, without sensor firmware or code tables.
// Its 0x07 register file has no CRC/age token: successful transport is NOT
// proof of sensor freshness or physical correctness. Reject other protocols.
inline Observation readEncoder(m2enc::M2Encoder& encoder, uint32_t started_ms, float offset_deg) {
    Observation o; o.at_ms = started_ms;
    m2enc::Reading r{};
    if (!encoder.read(r)) return o; // Never reuse a previous successful reading.
    if (r.fw_version != 0x07) { o.error = Reason::Firmware; return o; }
    const uint8_t forbidden = m2enc::ST_CFG_ERROR | m2enc::ST_PROBATION | m2enc::ST_NEED_MOTION;
    if (!r.valid || r.cells >= 1800 || r.n_cand != 1 || r.suspect ||
        (r.status & forbidden) || !isfinite(r.deg) || !isfinite(offset_deg)) {
        o.error = Reason::NotAbsolute; return o;
    }
    o.degrees = wrap(r.deg + offset_deg); o.degraded = r.degraded || r.dead;
    o.valid = true; o.error = Reason::AtTarget; return o;
}
} // namespace n36
