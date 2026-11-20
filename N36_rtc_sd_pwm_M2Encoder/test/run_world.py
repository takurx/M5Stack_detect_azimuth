#!/usr/bin/env python3
"""Compile the unchanged public library with the production N36 policy.

Pass a checkout of the pinned m2-absolute-encoder-i2c-host repository; no
automatic network access, hardware access or writes to the checkout.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PIN = "73d6df16cf9bb8336d8cef991799e35eaf54aee7"
parser = argparse.ArgumentParser()
parser.add_argument("library", type=Path)
args = parser.parse_args()
library = args.library.resolve()
assert subprocess.check_output(["git", "-C", str(library), "rev-parse", "HEAD"], text=True).strip() == PIN
assert not subprocess.check_output(["git", "-C", str(library), "status", "--porcelain"], text=True).strip()

def compile_run(include, output, sanitize=False, sketch=False, motor=False, scan=False, hold=False):
    command = ["c++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O1",
               "-I" + str(include), "-I" + str(ROOT / "test/stubs"), "-I" + str(library / "src"),
               str(ROOT / ("test/sketch_world.cpp" if sketch else "test/world_model.cpp")),
               str(library / "src/M2Encoder.cpp"), "-o", str(output)]
    if motor:
        command += ["-DN36_ENABLE_MOTOR=1", "-DN36_ALIGNMENT_CONFIRMED=1"]
    if scan:
        command += ["-DN36_SCAN_PERIOD_US=20000", "-DN36_SCAN_SETTLE_US=2000", "-DN36_SCAN_BLANK_US=500", "-DN36_SCAN_STABLE_READS=2"]
    if hold:
        command += ["-DN36_HOLD_MS=5000", "-DN36_RESOLVE_PULSES=3", "-DN36_RESOLVE_TOTAL=9", "-DN36_RESOLVE_PULSE_MS=80", "-DN36_WAIT_DEG=6.0f"]
    if sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    compiled = subprocess.run(command, capture_output=True, text=True)
    if compiled.returncode:
        raise RuntimeError("Compilation failed:\n" + compiled.stdout + compiled.stderr)
    return subprocess.run([str(output)], capture_output=True, text=True)

mutants = {
    "stale_gate_removed": ("Tracking.h", "now - observation_.at_ms >= policy_.sample_ttl_ms", "false"),
    "night_gate_removed": ("Tracking.h", "sun.elevation <= 0", "false"),
    "pulse_deadline_removed": ("Tracking.h", "now - pulse_at_ >= pulse_len_", "false"),
    "pulse_length_uncapped": ("Tracking.h", "if (x > policy_.pulse_ms) x = policy_.pulse_ms;", ""),
    "overshoot_wait_unbounded": ("Tracking.h", "over <= policy_.wait_deg", "over >= 0"),
    "settling_tolerance_unbounded": ("Tracking.h", "if (settling && observed_ &&", "if ((settling || true) && observed_ &&"),
    "stall_gate_removed": ("Tracking.h", "on_ms_ >= policy_.stall_on_ms", "false"),
    "device_ttl_gate_removed": ("Tracking.h", "now - observation_.at_ms >= observation_.valid_for_ms", "false"),
    "status_expiry_between_reads_ignored": ("EncoderInput.h", "!r.usable(now_us)", "false"),
    "device_origin_ignored": ("EncoderInput.h", "wrap(degrees + offset_deg)", "wrap(r.cell * 0.2f + offset_deg)"),
    "oversized_sun_gap_accepted": ("SunTable.h", "next_.utc - current_.utc > 300", "false"),
    "resolve_budget_unbounded": ("Tracking.h", "resolves_ < policy_.resolve_pulses", "true"),
    "resolve_ignores_sensor_fault": ("Tracking.h", "!observation_.degraded &&", ""),
    "resolve_arm_cap_removed": ("Tracking.h", "&& resolve_sum_ < resolveCap()", ""),
    "resolve_refill_without_hold": ("Tracking.h", "if (policy_.hold_ms) resolves_ = 0;", "resolves_ = 0;"),
    "hold_unbounded": ("Tracking.h", "now - held_since_ < policy_.hold_ms", "true"),
    "hold_ignores_sensor_fault": ("Tracking.h", "policy_.hold_ms && !observation_.degraded)", "policy_.hold_ms)"),
    "hold_covers_every_error": ("Tracking.h", "if (motion_only && policy_.hold_ms", "if (policy_.hold_ms"),
    "hold_off_by_default": ("Tracking.h", "uint32_t hold_ms = 5000;", "uint32_t hold_ms = 0;"),
    "scan_receipt_result_ignored": ("ScanControl.h", "r.command_result == m2enc::M2_OK", "true"),
    "scan_retries_forever": ("ScanControl.h", "!retried_ && e.retry()", "e.retry()"),
}
with tempfile.TemporaryDirectory(prefix="n36-world-") as directory:
    temporary = Path(directory)
    normal = compile_run(ROOT / "include", temporary / "world")
    if normal.returncode:
        raise RuntimeError(normal.stderr + normal.stdout)
    metrics = json.loads(normal.stdout)
    sanitized = compile_run(ROOT / "include", temporary / "world-sanitized", True)
    if sanitized.returncode:
        raise RuntimeError(sanitized.stderr + sanitized.stdout)
    assert json.loads(sanitized.stdout) == metrics
    sketch_results = []
    for motor, scan, hold in ((False, False, False), (True, False, False), (False, True, False), (True, False, True)):
        sketch = compile_run(ROOT / "include", temporary / ("sketch-%d-%d-%d" % (motor, scan, hold)),
                             sanitize=True, sketch=True, motor=motor, scan=scan, hold=hold)
        if sketch.returncode:
            raise RuntimeError(sketch.stderr + sketch.stdout)
        sketch_results.append(json.loads(sketch.stdout))
    for name, (filename, old, new) in mutants.items():
        include = temporary / name
        include.mkdir()
        for source in (ROOT / "include").glob("*.h"):
            text = source.read_text()
            if source.name == filename:
                assert text.count(old) == 1, (name, text.count(old))
                text = text.replace(old, new)
            (include / source.name).write_text(text)
        result = compile_run(include, temporary / (name + "-run"))
        assert result.returncode != 0 and "FAIL line" in result.stderr, (name, result.stdout, result.stderr)
    report = {"result": "PASS", "library_commit": PIN, "world": metrics, "sketch": sketch_results,
              "sanitizers": "address,undefined", "runtime_mutants_rejected": list(mutants),
              "hardware_tested": False, "sensor_firmware_emulated": False,
              "host_compiler": subprocess.check_output(["c++", "--version"], text=True).splitlines()[0],
              "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in sorted(list((ROOT / "include").glob("*.h")) +
                                                list((ROOT / "src").glob("*.cpp")) +
                                                list((ROOT / "test").glob("*.cpp")) +
                                                list((ROOT / "test").glob("*.py")) +
                                                list((ROOT / "test/stubs").glob("*.h")) +
                                                [ROOT / "platformio.ini"])}}
    print(json.dumps(report, indent=2))
