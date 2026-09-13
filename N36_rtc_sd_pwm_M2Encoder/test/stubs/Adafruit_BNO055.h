#pragma once
#include <stdint.h>
extern float fake_bno_heading;
class Adafruit_BNO055 {
public:
    Adafruit_BNO055(int, int) {}
    enum { VECTOR_EULER };
    bool begin() { return true; }
    void setExtCrystalUse(bool) {}
    void getCalibration(uint8_t* s, uint8_t* g, uint8_t* a, uint8_t* m) { *s=*g=*a=*m=3; }
    struct Vector { float x() { return fake_bno_heading; } };
    Vector getVector(int) { return Vector(); }
};
