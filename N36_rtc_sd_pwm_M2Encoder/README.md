# N36: RTC/SD tracking with an M2 absolute encoder

N36 is derived from `N34_rtc_sd_pwm_BNO055_WIP` at repository commit
`1f214b38a5af322c932c6e5059fc260a6ff26146`. N34 and N35 are unchanged.
N35 is the separate button/GPIO experiment, not the next tracking application:
its initial commit `153f22b` calls it WIP, and its latest code toggles GPIOs.
N34 is the latest BNO055/RTC/SD/PWM tracking branch in this repository, but is
itself named WIP; this copy does not establish that it is deployed or qualified.

This version uses the existing public
[M2Encoder host library](https://github.com/bakemocho/m2-absolute-encoder-i2c-host)
at `ebbeec3962f5dc21c094b94e22e056b7754feac5`. The dependency is pinned, not
vendored. Its sensor firmware **0x07** register interface is the only accepted
interface. Other versions, including 0x0e, stop tracking. This is not a port of
an unpublished sensor protocol, decoder, or firmware.

## Changes from N34

| N34 feature | N36 behavior |
| --- | --- |
| M5Stack Core ESP32, GPIO 5 PWM, 100 Hz, 8-bit duty 16 | Retained, with physical output disabled by default |
| PCF8563 RTC and `/info_sun_angle.csv` on SD | Retained; UTC, ordered/finite rows, bounded parsing and expiration |
| BNO055 at 0x28 | Display-only heading/calibration; never an automatic substitute for encoder feedback |
| Estimated angle increment after a pulse | Replaced by encoder feedback; no assumed degrees per pulse |
| Button A aligns the estimate to the target | A now stops; it does not change angle or origin |
| Button B selects sensor mode | B explicitly arms, subject to valid encoder and sun inputs |
| Button C toggles continuous motion | C now stops; continuous unobserved motion is not retained |
| Blocking 200 ms motor pulse | Foreground pulse state machine, nominal limit 200 ms |
| BNO055 I2C calls from Ticker | Serialized with RTC/encoder reads in `loop()` |
| Large commented-out BMM150/legacy code | Omitted from N36; original remains in N34 |

The BNO heading retains N34's +90 degree display convention. That is a mounting
convention in this sample, not a universal BNO055 correction. It never silently
sets the encoder origin or establishes true north.

## Build and use

```sh
pio run -d N36_rtc_sd_pwm_M2Encoder
```

No upload is part of the tests. The platform is pinned to espressif32 6.9.0;
the build uses Arduino-ESP32 2.0.17 and the N34 M5Stack board target. M5Stack
0.4.3 is pinned by its official Git commit because that version was unavailable
through the PlatformIO package lookup during validation. It does not
claim compatibility with every Core/Core2/CoreS3 model or Arduino-ESP32 3.x.

The monitor configuration was built successfully with PlatformIO 6.1.19 and
Xtensa GCC 8.4.0+2021r2-patch5: RAM 23,332/327,680 bytes and flash
426,485/1,310,720 bytes. No device was flashed. Both output configurations
were also exercised by the host sketch tests below.

Start with the monitor-only configuration. Inspect actual I2C wiring, voltage,
address conflicts, pull-ups, direction, driver behavior and mechanical travel
limits before enabling output. GPIO 5 is inherited, not a new pin assignment.
I2C uses SDA 21/SCL 22, 100 kHz and a 30 ms Wire timeout. This does not qualify
clock stretching or any cable length. No extra pull-ups are installed by code.

To enable a bench motor only after those checks, set both
`N36_ENABLE_MOTOR=1` and `N36_ALIGNMENT_CONFIRMED=1` in `platformio.ini`.
Set `N36_ENCODER_OFFSET_DEG` to the measured offset from encoder zero to the
north/east/south/west convention of the sun data. Offset is host configuration;
N36 does not write the sensor's persistent origin. Default offset zero does not
mean alignment has been established. Then press B to arm. A or C stops; faults
disarm, and recovery never automatically rearms.
Repeated B presses while armed do not restart a pulse or reset the no-motion budget.

When the encoder is not absolute, the library's `Rotate` advice is not an
automatic movement command. Align/reacquire under a separate supervised
procedure. The sample will not energize the motor to discover an unknown
position. Valid degraded readings remain usable and show a service indication;
probation, contradictory status, multiple candidates and invalid ranges stop.

This remains one-way control, matching N34's single PWM output. It will only
move forward toward a target within 5 degrees; it will not take a near-full
revolution to correct a small overshoot. For example 359 to 1 is permitted,
but 2 to 1 requires manual alignment. Reversal/limit-switch/driver interlocks
would need an explicitly specified hardware interface.

## Sun data

The input format is the space-separated output of the repository's
`Document/test_year_sun_az_al-1.py` (despite the `.csv` extension):

```text
0 2026-01-01 00:00:00 30.0 120.0
1 2026-01-01 00:05:00 30.1 121.0
```

These two lines are synthetic format examples, not a solar prediction for a
site. Configure the generating script's date and location separately. Both
RTC and file timestamps must be UTC. No automatic RTC adjustment to compile
time is performed. Invalid/stopped/lost-power RTC disables targets.

The lookup requires an ordered current and following row, no more than 300
seconds apart. It holds the current row until the following one, without
interpolating across north. Future-only data, reversed timestamps, malformed or
overlong lines, out-of-range/non-finite values, gaps, exhaustion and missing SD
do not become valid targets. Reload/restart after repairing the input; there is
no silent seek back to an earlier day. Night (elevation <= 0) disarms.
The scanner consumes at most 1024 bytes per loop and never scans SD while the
policy requests a motor pulse. Searching a large file at startup can take time;
output stays off and a later explicit arm is required.

## Software limits, not machine safety ratings

The sample policy uses 150 ms since the **start of a host read**, 200 ms pulses,
800 ms minimum pulse gaps, 0.4 degree deadband, and a no-motion latch after
2000 ms cumulative requested PWM with less than 0.3 degree progress. The initial
1000 ms no-motion setting falsely stopped the low-speed/backlash model; the
2000 ms policy passes the documented model range. These are configurable
engineering choices, not measured actuator or sensor limits.

The no-motion threshold includes quantization, so it is not a single-pulse
movement requirement. A motionless read at an already-reached target is not
detectable as a stale sensor with the old protocol. The 0x07 interface has no
CRC or acquisition timestamp: corrupt plausible data and frozen measurements
cannot be excluded merely because I2C succeeded. Do not use this sample as a
safety interlock or as evidence of compatibility with a different firmware.

Pulse and timeout enforcement occurs when foreground code runs. I2C/framework
latency or a stuck CPU can extend output beyond the nominal limit. Slow SD,
BNO and LCD work is excluded while requesting PWM, but an independent hardware
stop/enable interlock is still required for hazardous machinery. No hardware
watchdog or emergency-stop circuit is implemented by this sample.

## Reproduce the world-model tests

```sh
git clone https://github.com/bakemocho/m2-absolute-encoder-i2c-host.git /tmp/m2-host
git -C /tmp/m2-host checkout ebbeec3962f5dc21c094b94e22e056b7754feac5
python3 N36_rtc_sd_pwm_M2Encoder/test/run_world.py /tmp/m2-host
```

Use an unused checkout path if `/tmp/m2-host` already exists. The runner refuses
a different or dirty library version and makes temporary test binaries only.
It compiles the real, unmodified library with the N36 production adapter and
policy. It also executes N36's actual `setup()`/`loop()` against mocked M5Stack,
SD, RTC, BNO055, PWM and Wire interfaces, in monitor and enabled configurations.
No motor/device is connected and no network access occurs in the test runner.

The physical model integrates a rotating object's independent angle from PWM,
with speed, first-order inertia, initial backlash and optional jam. The sun is
an analytic moving target, not a measured/astronomically qualified ephemeris.
The synthetic sensor quantizes that angle into the public register format;
it never copies the target into the measurement. The BNO stub deliberately
disagrees with the encoder to check that it cannot override the feedback.

The suite covers six speed/inertia combinations (0.4/0.8/1.2 deg/s and
20/80 ms, initial 0.1 degree backlash), NACK/short-read recovery and jam/frozen
register cases; unit cases add north wrap, stale data, night, STOP, no rearm,
bad firmware/status/range and sun-table boundaries. ASan/UBSan check host runs.
Eight separately compiled gate-removal mutants must fail at runtime, not merely
fail compilation. `test/validation.json` records the observed result and source
hashes. Model error/PWM timings are not hardware specifications, all-input
proof, ESP32 instruction timing or sensor-firmware emulation.

## Remaining validation before use

- Actual sensor 0x07 compatibility and independent angle reference.
- Real M5Stack bus timing, RTC/SD behavior, electrical and motor-off behavior.
- Installation offset, motor speed/direction/limits, inertia/backlash under load,
  outdoor disturbances and shutdown behavior.

Do not infer these from a passing host test or a successful ESP32 build.
