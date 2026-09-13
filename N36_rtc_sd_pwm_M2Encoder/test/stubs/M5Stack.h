#pragma once
#include "Arduino.h"
#include <string>
#define TFCARD_CS_PIN 4
struct FakeButton {
    bool pressed = false;
    bool wasPressed() { bool p = pressed; pressed = false; return p; }
};
struct FakeLcd {
    void setTextSize(int) {} void clear() {} void setCursor(int, int) {}
    template<class... Args> void printf(const char*, Args...) {}
    void println(const char*) {}
};
struct FakeM5 {
    FakeButton BtnA, BtnB, BtnC;
    FakeLcd Lcd;
    struct PowerType { void begin() {} } Power;
    void begin() {} void update() {}
};
extern FakeM5 M5;
extern std::string fake_sd, fake_log;
extern bool fake_sd_present;
extern unsigned fake_log_writes_during_pwm;
class File {
    size_t cursor_ = 0;
    bool open_ = false, append_ = false;
public:
    File() = default;
    File(bool open, bool append) : open_(open), append_(append) {}
    explicit operator bool() const { return open_; }
    int read() { return open_ && !append_ && cursor_ < fake_sd.size() ? (unsigned char)fake_sd[cursor_++] : -1; }
    size_t print(const char* text) { if (!open_ || !append_) return 0; if (fake_pwm) ++fake_log_writes_during_pwm; fake_log += text; return 1; }
    void flush() {}
};
constexpr int SPI = 0, FILE_READ = 0, FILE_APPEND = 1;
struct FakeSD {
    bool begin(int, int, int) { return fake_sd_present; }
    File open(const char*, int mode) { return File(fake_sd_present, mode == FILE_APPEND); }
};
extern FakeSD SD;
