// N36: N34 RTC/SD/PWM/BNO055 application adapted to the published M2 reader.
// BNO055 is display-only; absolute encoder feedback alone controls tracking.
#include <Arduino.h>
#include <M5Stack.h>
#include <RTClib.h>
#include <Adafruit_BNO055.h>
#include <M2Encoder.h>
#include "EncoderInput.h"
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
static constexpr uint8_t PWM_PIN = 5, PWM_CHANNEL = 0, PWM_DUTY = 16;
static constexpr bool motor_enabled = N36_ENABLE_MOTOR && N36_ALIGNMENT_CONFIRMED;
RTC_PCF8563 rtc;
Adafruit_BNO055 bno(55, 0x28);
m2enc::M2Encoder encoder;
n36::Tracking tracker;
n36::SunTable sun_table;
n36::Observation observation;
n36::SunTarget target;
File sun_file;
bool rtc_ok = false, bno_ok = false;
uint32_t utc = 0, clock_at = 0, encoder_at = 0, display_at = 0, bno_at = 0;
char line[160]; size_t line_used = 0;
float imu_heading = NAN;
uint8_t imu_sys = 0, imu_gyro = 0, imu_accel = 0, imu_mag = 0;

static void output() { ledcWrite(PWM_CHANNEL, motor_enabled && tracker.motorOn() ? PWM_DUTY : 0); }
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
    // begin() establishes the bus pointer; each subsequent read is validated.
    encoder.begin(Wire, 0x36);
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
    else if (b) tracker.arm(millis());

    uint32_t now = millis();
    // Invalidate target if foreground could not refresh the clock recently.
    if (now - clock_at > 1500) target.valid = false;
    tracker.tick(now, target); output();
    if (now - encoder_at >= 50) {
        encoder_at = now;
        observation = n36::readEncoder(encoder, now, N36_ENCODER_OFFSET_DEG);
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
        M5.Lcd.printf("Service: %s          \n", observation.degraded ? "yes" : "no");
        M5.Lcd.printf("Sun: %7.2f %s   \n", target.azimuth, target.valid ? "valid" : "INVALID");
        M5.Lcd.printf("BNO: %7.2f (%u/%u)   \n", imu_heading, imu_sys, imu_mag);
        M5.Lcd.println("A/C STOP; B ARM        ");
    }
}
