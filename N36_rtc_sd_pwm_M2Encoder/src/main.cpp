// N36: N34 RTC/SD/PWM/BNO055 application adapted to the published M2 reader.
// BNO055 is display-only; absolute encoder feedback alone controls tracking.
#include <Arduino.h>
#include <M5Stack.h>
#include <RTClib.h>
#include <Adafruit_BNO055.h>
#include <M2Encoder.h>
#include "EncoderInput.h"
#include "ScanControl.h"
#include "SunTable.h"

#ifndef N36_ENABLE_MOTOR
#define N36_ENABLE_MOTOR 0
#endif
#ifndef N36_ALIGNMENT_CONFIRMED
#define N36_ALIGNMENT_CONFIRMED 0
#endif
#ifndef N36_ENCODER_OFFSET_DEG
#define N36_ENCODER_OFFSET_DEG 0.0f
#endif
// Scan profile: all zero (default) disables host-side configure/start. Values must be
// qualified for the installation (settling, blanking, stable reads, rotation speed).
#ifndef N36_SCAN_PERIOD_US
#define N36_SCAN_PERIOD_US 0
#endif
#ifndef N36_SCAN_SETTLE_US
#define N36_SCAN_SETTLE_US 0
#endif
#ifndef N36_SCAN_BLANK_US
#define N36_SCAN_BLANK_US 0
#endif
#ifndef N36_SCAN_STABLE_READS
#define N36_SCAN_STABLE_READS 0
#endif
#ifndef N36_SCAN_INVERTED
#define N36_SCAN_INVERTED 0
#endif
#ifndef N36_RESOLVE_PULSES
#define N36_RESOLVE_PULSES 0
#endif
static constexpr uint8_t PWM_PIN = 5, PWM_CHANNEL = 0, PWM_DUTY = 16;
static constexpr bool motor_enabled = N36_ENABLE_MOTOR && N36_ALIGNMENT_CONFIRMED;
RTC_PCF8563 rtc;
Adafruit_BNO055 bno(55, 0x28);
m2enc::M2Encoder encoder;
n36::Policy policy_with_resolve() { n36::Policy p; p.resolve_pulses = N36_RESOLVE_PULSES; return p; }
n36::Tracking tracker(policy_with_resolve());
static n36::ScanProfile scanProfile() {
    n36::ScanProfile p; p.period_us = N36_SCAN_PERIOD_US; p.settle_us = N36_SCAN_SETTLE_US; p.blank_us = N36_SCAN_BLANK_US;
    p.stable_reads = N36_SCAN_STABLE_READS; p.inverted = N36_SCAN_INVERTED != 0; return p;
}
n36::ScanStarter scan_starter(scanProfile());
n36::SunTable sun_table;
n36::Observation observation;
n36::SunTarget target;
File sun_file;
bool rtc_ok = false, bno_ok = false;
uint32_t utc = 0, clock_at = 0, encoder_at = 0, display_at = 0, bno_at = 0, diag_at = 0;
bool diag_ok = false, quality_ok = false; uint8_t diag_reset_cause = 0; uint16_t diag_fault_count = 0, dead_sensors = 0, suspect_sensors = 0;
char line[160]; size_t line_used = 0;
float imu_heading = NAN;
uint8_t imu_sys = 0, imu_gyro = 0, imu_accel = 0, imu_mag = 0;

static void output() { ledcWrite(PWM_CHANNEL, motor_enabled && tracker.motorOn() ? PWM_DUTY : 0); }
static uint16_t popcount10(uint16_t m) { uint16_t n = 0; for (uint8_t i = 0; i < 10; ++i) n += (m >> i) & 1; return n; }
// Read-only diagnostics while the encoder is not usable or reports a fault; never during PWM.
static void serviceDiagnostics(uint32_t now) {
    if (now - diag_at < 1000 || (observation.valid && !observation.degraded)) return;
    diag_at = now;
    m2enc::BoardDiagnostic d; diag_ok = encoder.readBoardDiagnostic(d) == m2enc::Ok && d.valid;
    if (diag_ok) { diag_reset_cause = d.reset_cause; diag_fault_count = d.fault_count; }
    m2enc::Quality q; quality_ok = encoder.readQuality(q) == m2enc::Ok;
    if (quality_ok) { dead_sensors = popcount10(q.dead_mask); suspect_sensors = popcount10(q.suspect_mask); }
}
static void stop() { tracker.stop(millis()); output(); }

// All I2C operations run in loop(), not concurrent Ticker callbacks. Bound
// Wire waits; stop PWM before slower RTC/SD/BNO/display work. This remains a
// foreground software stop, not a hardware emergency-stop guarantee.
static void serviceSun() {
    if (!rtc_ok || !sun_file) { target = n36::SunTarget(); return; }
    size_t budget = 1024;
    while (budget-- && sun_table.needsRow(utc)) {
        int c = sun_file.read();
        if (c < 0) {
            if (line_used) { sun_table.fail(); line_used = 0; }
            break;
        }
        if (c == '\n') {
            line[line_used] = 0; n36::SunRow row;
            if (!n36::parseSunRow(line, row) || !sun_table.push(row, utc)) sun_table.fail();
            line_used = 0;
        } else if (line_used + 1 < sizeof line) line[line_used++] = char(c);
        else { sun_table.fail(); break; }
    }
    target = sun_table.target(utc);
}

void setup() {
    // Preserve N34 board, PWM, RTC and SD pin assignments. No auto-time reset.
    pinMode(PWM_PIN, OUTPUT); digitalWrite(PWM_PIN, LOW);
    M5.begin(); M5.Power.begin(); Serial.begin(115200);
    ledcSetup(PWM_CHANNEL, 100, 8); ledcAttachPin(PWM_PIN, PWM_CHANNEL); stop();
    Wire.begin(21, 22); Wire.setClock(100000); Wire.setTimeOut(30);
    // Verify protocol/core and attach to command state. No automatic scan start.
    if (encoder.begin() != m2enc::Ok) Serial.println("No compatible M2 device; tracking stays invalid.");
    rtc_ok = rtc.begin() && !rtc.lostPower() && rtc.isrunning();
    if (rtc_ok) { DateTime now = rtc.now(); rtc_ok = now.isValid(); utc = now.unixtime(); }
    bno_ok = bno.begin();
    if (bno_ok) bno.setExtCrystalUse(false);
    if (SD.begin(TFCARD_CS_PIN, SPI, 40000000)) sun_file = SD.open("/info_sun_angle.csv", FILE_READ);
    M5.Lcd.setTextSize(2); M5.Lcd.clear();
    Serial.println("N36: A/C stop; B arm only with valid inputs. Default: monitor only.");
}

void loop() {
    M5.update();
    // STOP wins if several buttons arrive together.
    bool a = M5.BtnA.wasPressed(), b = M5.BtnB.wasPressed(), c = M5.BtnC.wasPressed();
    if (a || c) stop();
    else if (b) {
        // B on a stopped sensor with a qualified scan profile: configure and start (receipt-checked), do not arm.
        if (observation.error == n36::Reason::NotScanning && !tracker.armed() && scan_starter.request(encoder, millis()))
            Serial.println("N36: scan configure/start requested; press B again once the reading is valid.");
        else tracker.arm(millis());
    }

    uint32_t now = millis();
    // Invalidate target if foreground could not refresh the clock recently.
    if (now - clock_at > 1500) target.valid = false;
    tracker.tick(now, target); output();
    if (now - encoder_at >= 50) {
        encoder_at = now;
        if (observation.error == n36::Reason::Restarted) encoder.begin(); // re-attach after a detected sensor reboot
        if (scan_starter.active()) { m2enc::Reading r; scan_starter.service(encoder, r, now); observation = n36::Observation(); observation.error = n36::Reason::NotScanning; }
        else observation = n36::readEncoder(encoder, now, N36_ENCODER_OFFSET_DEG);
        tracker.observe(observation);
        tracker.tick(millis(), target); output();
    }
    if (tracker.motorOn()) return; // Do not hide an SD/LCD/BNO stall inside PWM.

    now = millis();
    if (now - clock_at >= 1000) {
        if (rtc_ok) {
            DateTime t = rtc.now();
            rtc_ok = t.isValid() && !rtc.lostPower() && rtc.isrunning();
            if (rtc_ok) utc = t.unixtime();
        }
        clock_at = now;
    }
    serviceSun();
    serviceDiagnostics(now);
    if (now - bno_at >= 1000) {
        bno_at = now;
        if (bno_ok) {
            bno.getCalibration(&imu_sys, &imu_gyro, &imu_accel, &imu_mag);
            imu_heading = n36::wrap(bno.getVector(Adafruit_BNO055::VECTOR_EULER).x() + 90.0f);
        }
    }
    if (now - display_at >= 250) {
        display_at = now;
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.printf("N36 %s       \n", motor_enabled ? "PWM enabled" : "MONITOR");
        M5.Lcd.printf("%-22s\n", n36::name(tracker.reason()));
        M5.Lcd.printf("M2: %7.2f %s   \n", observation.degrees, observation.valid ? "valid" : "INVALID");
        M5.Lcd.printf("Service: %s %s     \n", observation.degraded ? "yes" : "no", scan_starter.active() || scan_starter.state() == n36::ScanStarter::Failed ? scan_starter.name() : "");
        if (diag_ok || quality_ok) M5.Lcd.printf("Diag rc=%02x f=%u dead=%u sus=%u \n", diag_reset_cause, diag_fault_count, dead_sensors, suspect_sensors);
        else M5.Lcd.println("Diag: -                 ");
        M5.Lcd.printf("Sun: %7.2f %s   \n", target.azimuth, target.valid ? "valid" : "INVALID");
        M5.Lcd.printf("BNO: %7.2f (%u/%u)   \n", imu_heading, imu_sys, imu_mag);
        M5.Lcd.println("A/C STOP; B ARM        ");
    }
}
