#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <sstream>
#include <string>
#define HEX 16
#define OUTPUT 1
#define LOW 0
extern uint32_t fake_ms;
extern uint32_t fake_pwm;
inline uint32_t millis() { return fake_ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int, int) {}
inline void ledcWrite(int, uint32_t duty) { fake_pwm = duty; }
struct FakeSerial { void begin(int) {} void println(const char*) {} };
extern FakeSerial Serial;
// Only the String surface used by the unmodified public host library.
class String {
    std::string value_;
public:
    String() = default;
    String(uint16_t v, int base) { std::ostringstream s; if (base == HEX) s << std::hex; s << v; value_ = s.str(); }
    template<class T> String& operator+=(T v) { std::ostringstream s; s << v; value_ += s.str(); return *this; }
    String& operator+=(const String& s) { value_ += s.value_; return *this; }
};
