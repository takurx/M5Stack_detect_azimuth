#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "Tracking.h"

namespace n36 {
struct SunRow { uint32_t utc = 0; float elevation = NAN, azimuth = NAN; };
inline bool leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
inline bool parseSunRow(const char* line, SunRow& row) {
    unsigned index; int y, mo, d, h, mi, s, used = 0; double el, az;
    row = SunRow();
    if (sscanf(line, "%u %d-%d-%d %d:%d:%d %lf %lf %n", &index, &y, &mo, &d,
               &h, &mi, &s, &el, &az, &used) != 9 || !used || line[used]) return false;
    if (y < 1970 || y > 2099 || mo < 1 || mo > 12 || d < 1 || h < 0 || h > 23 ||
        mi < 0 || mi > 59 || s < 0 || s > 59 || !isfinite(el) || !isfinite(az) ||
        el < -90 || el > 90 || az < 0 || az >= 360) return false;
    const int mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (d > mdays[mo - 1] + (mo == 2 && leap(y))) return false;
    uint32_t days = 0;
    for (int year = 1970; year < y; ++year) days += leap(year) ? 366 : 365;
    for (int month = 1; month < mo; ++month) days += mdays[month - 1] + (month == 2 && leap(y));
    days += d - 1;
    row.utc = days * 86400UL + h * 3600UL + mi * 60UL + s;
    row.elevation = float(el); row.azimuth = float(az); return true;
}
// Bounded forward-only lookup. Stale, future-only, malformed and unordered
// input never turn into a valid target. No interpolation or ephemeris claim.
class SunTable {
    SunRow current_, next_;
    bool have_current_ = false, have_next_ = false, failed_ = false, seen_ = false;
    bool clock_seen_ = false;
    uint32_t last_row_ = 0, last_clock_ = 0;
public:
    void fail() { failed_ = true; }
    bool failed() const { return failed_; }
    bool needsRow(uint32_t now) {
        if (clock_seen_ && now < last_clock_) fail();
        clock_seen_ = true; last_clock_ = now;
        if (have_next_ && next_.utc <= now) { current_ = next_; have_current_ = true; have_next_ = false; }
        return !failed_ && !have_next_;
    }
    bool push(const SunRow& row, uint32_t now) {
        if (!needsRow(now) || (seen_ && row.utc <= last_row_) ||
            !isfinite(row.azimuth) || !isfinite(row.elevation) || row.azimuth < 0 || row.azimuth >= 360 ||
            row.elevation < -90 || row.elevation > 90) { fail(); return false; }
        seen_ = true; last_row_ = row.utc;
        if (row.utc <= now) { current_ = row; have_current_ = true; }
        else { next_ = row; have_next_ = true; }
        return true;
    }
    SunTarget target(uint32_t now) {
        needsRow(now);
        SunTarget t;
        // Require a following sample, even at EOF; never silently hold last day.
        if (failed_ || !have_current_ || !have_next_ || now < current_.utc ||
            now - current_.utc > 300 || next_.utc - current_.utc > 300) return t;
        t.azimuth = current_.azimuth; t.elevation = current_.elevation; t.valid = true; return t;
    }
};
} // namespace n36
