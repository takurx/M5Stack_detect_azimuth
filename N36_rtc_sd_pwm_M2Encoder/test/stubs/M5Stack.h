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
extern std::string fake_sd;
extern bool fake_sd_present;
class File {
    size_t cursor_ = 0;
    bool open_ = false;
public:
    File() = default;
    explicit File(bool open) : open_(open) {}
    explicit operator bool() const { return open_; }
    int read() { return open_ && cursor_ < fake_sd.size() ? (unsigned char)fake_sd[cursor_++] : -1; }
};
constexpr int SPI = 0, FILE_READ = 0;
struct FakeSD {
    bool begin(int, int, int) { return fake_sd_present; }
    File open(const char*, int) { return File(fake_sd_present); }
};
extern FakeSD SD;
