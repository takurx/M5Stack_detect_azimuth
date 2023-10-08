#include <Arduino.h>
#include <M5Stack.h>

uint8_t pin_on_off = 5;

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World");

  pinMode(pin_on_off, OUTPUT);
  digitalWrite(pin_on_off, LOW);
}

void loop() {
  M5.update();
  M5.Lcd.setCursor(0, 0);
  if (M5.BtnA.isPressed()) {
    M5.Lcd.print("Button A is pressed, pin 5 is HIGH");
    digitalWrite(pin_on_off, HIGH);
  }
  if (M5.BtnB.isPressed()) {
    M5.Lcd.print("Button B is pressed, pin 5 is LOW");
    digitalWrite(pin_on_off, LOW);
  }
  delay(1);
}
