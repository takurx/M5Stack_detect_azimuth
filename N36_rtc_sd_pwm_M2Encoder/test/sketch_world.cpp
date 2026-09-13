// Runs actual setup()/loop(), not a second implementation of the controller.
// Framework peripherals are deterministic stubs, not an ESP32 CPU model.
#include <stdio.h>
#include <stdlib.h>
#include "../src/main.cpp"
#include "SensorFrame.h"
uint32_t fake_ms = 0, fake_pwm = 0, fake_utc = 1767225600UL;
uint32_t fake_bus_us = 0; int fake_reset_reason = 1; unsigned fake_wdt_resets = 0, fake_log_writes_during_pwm = 0;
bool fake_sd_present = true, fake_rtc_valid = true;
float fake_bno_heading = 270;
std::string fake_sd, fake_log;
FakeSerial Serial; FakeM5 M5; FakeSD SD; TwoWire Wire;
static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL sketch line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void sensor(float angle) {
    sensorFrame(Wire,n36::wrap(angle));
}
static void reset() {
    fake_ms=0; fake_pwm=0; fake_bus_us=0; fake_utc=1767225600UL; fake_rtc_valid=true; fake_sd_present=true;
    Wire=TwoWire(); M5=FakeM5(); tracker=n36::Tracking(); sun_table=n36::SunTable();
    observation=n36::Observation(); target=n36::SunTarget(); sun_file=File(); log_file=File(); flight_log=n36::FlightLog<File>(); fake_log.clear(); fake_log_writes_during_pwm=0;
    rtc_ok=bno_ok=false; utc=clock_at=encoder_at=display_at=bno_at=0; line_used=0;
    fake_sd="0 2026-01-01 00:00:00 30 121\n1 2026-01-01 00:05:00 31 122\n";
    sensor(120); setup(); CHECK(fake_pwm==0);
    for(fake_ms=0; fake_ms<300; ++fake_ms)loop();
    CHECK(target.valid && observation.valid);
}
int main() {
    reset(); M5.BtnB.pressed=true; loop();
    CHECK(fake_pwm==(motor_enabled?16u:0u));
    // Deliberately wrong BNO heading cannot replace encoder feedback.
    fake_bno_heading=1; double actual=120; unsigned maximum=0, pulse=0;
    for(fake_ms=301; fake_ms<30000; ++fake_ms) {
        actual += fake_pwm ? .0008 : 0; sensor(actual);
        fake_utc=1767225600UL+fake_ms/1000; loop();
        if(fake_pwm) { ++pulse; if(pulse>maximum)maximum=pulse; } else pulse=0;
        CHECK(pulse<=200);
    }
    if(motor_enabled)CHECK(n36::distance(actual,121)<.65);
    else CHECK(actual==120 && fake_pwm==0);
    // Flight log: header with the reset reason, one row per encoder sample, never written during PWM.
    CHECK(fake_log.rfind("# boot reset_reason=1\n",0)==0 && fake_log.find("ms,utc,angle,valid,reason")!=std::string::npos);
    CHECK(fake_log.find(",TRACKING,")!=std::string::npos || !motor_enabled);
    CHECK(fake_log_writes_during_pwm==0 && flight_log.written()>500 && fake_wdt_resets>0);
    reset(); M5.BtnB.pressed=true; loop(); Wire.nack=true;
    for(fake_ms=301;fake_ms<360;++fake_ms)loop();
    CHECK(fake_pwm==0 && !tracker.armed());
    Wire.nack=false; for(fake_ms=360;fake_ms<500;++fake_ms)loop(); CHECK(!tracker.armed());
    M5.BtnB.pressed=true; loop(); M5.BtnC.pressed=true; ++fake_ms; loop(); CHECK(fake_pwm==0 && !tracker.armed());
    reset(); Wire.status[24]=7; fake_ms=350; loop(); M5.BtnB.pressed=true; ++fake_ms;loop();
    CHECK(fake_pwm==0 && tracker.reason()==n36::Reason::Firmware);
    reset(); fake_rtc_valid=false; fake_ms=1000;loop(); M5.BtnB.pressed=true; ++fake_ms;loop();
    CHECK(fake_pwm==0 && !tracker.armed());
    reset(); M5.BtnA.pressed=true; M5.BtnB.pressed=true; loop(); CHECK(fake_pwm==0);
    ++fake_ms; loop(); CHECK(fake_pwm==0 && !tracker.armed());
    CHECK(Wire.command_count==0); // Attaching never changes scan/persistence settings.
    // Diagnostics are read only while the reading is unusable or degraded, never during PWM.
    reset(); diagnosticFrame(Wire,0x04,3); qualityFrame(Wire,0x003,0x004); Wire.status[7]|=m2enc::Degraded;
    for(fake_ms=300;fake_ms<2500;++fake_ms)loop();
    CHECK(diag_ok && diag_reset_cause==0x04 && diag_fault_count==3 && dead_sensors==2 && suspect_sensors==1);
    CHECK(Wire.command_count==0);
    // A stopped sensor: B configures and starts only when the build carries a profile; arming needs a second B.
    reset(); Wire.status[6]=3; Wire.status[25]&=~2;
    for(fake_ms=300;fake_ms<400;++fake_ms)loop();
    CHECK(!observation.valid && observation.error==n36::Reason::NotScanning);
    M5.BtnB.pressed=true; loop();
    for(fake_ms=401;fake_ms<1000;++fake_ms)loop();
#if N36_SCAN_PERIOD_US > 0
    CHECK(Wire.command_count==2 && scan_starter.state()==n36::ScanStarter::Done && observation.valid && !tracker.armed() && fake_pwm==0);
    M5.BtnB.pressed=true; loop(); CHECK(tracker.armed());
#else
    CHECK(Wire.command_count==0 && !tracker.armed() && fake_pwm==0 && scan_starter.state()==n36::ScanStarter::Idle);
#endif
    printf("{\"sketch_checks\":%u,\"motor_enabled_in_stub\":%s,\"scan_profile_in_build\":%s,\"max_pulse_ms\":%u,\"esp32_emulated\":false}\n",
           checks,motor_enabled?"true":"false",N36_SCAN_PERIOD_US>0?"true":"false",maximum);
    return 0;
}
