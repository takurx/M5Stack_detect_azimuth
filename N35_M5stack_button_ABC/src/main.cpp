#include <Arduino.h>
#include <M5Stack.h>

int Button_A_pin = 2;
int Button_B_pin = 5;
int Button_C_pin = 13;

bool Button_A_state;
bool Button_B_state;
bool Button_C_state;

String print_bool (bool value) {
  if (value) {
    return "HIGH";
  }
  else {
    return "LOW";
  }
}

void setup() {
  M5.begin();
  M5.Power.begin();

  Serial.begin(115200);
  Serial.println("Hello World!");
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Hello World!");

  pinMode(Button_A_pin, OUTPUT);
  pinMode(Button_B_pin, OUTPUT);
  pinMode(Button_C_pin, OUTPUT);

  digitalWrite(Button_A_pin, LOW);
  digitalWrite(Button_B_pin, LOW);
  digitalWrite(Button_C_pin, LOW);

  Button_A_state = false;
  Button_B_state = false;
  Button_C_state = false;

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Button A: " + print_bool(Button_A_state));
  M5.Lcd.println("Button B: " + print_bool(Button_B_state));
  M5.Lcd.println("Button C: " + print_bool(Button_C_state));
}

void loop() {
  if (M5.BtnA.wasPressed()) {
    Button_A_state = !Button_A_state;
    if (Button_A_state) {
      digitalWrite(Button_A_pin, HIGH);
    }
    else {
      digitalWrite(Button_A_pin, LOW);
    }
    Serial.println("Button A: " + print_bool(Button_A_state));
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Button A: " + print_bool(Button_A_state));
    M5.Lcd.println("Button B: " + print_bool(Button_B_state));
    M5.Lcd.println("Button C: " + print_bool(Button_C_state));
  }
  
  if (M5.BtnB.wasPressed()) {
    Button_B_state = !Button_B_state;
    if (Button_B_state) {
      digitalWrite(Button_B_pin, HIGH);
    }
    else {
      digitalWrite(Button_B_pin, LOW);
    }
    Serial.println("Button B: " + print_bool(Button_B_state));
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Button A: " + print_bool(Button_A_state));
    M5.Lcd.println("Button B: " + print_bool(Button_B_state));
    M5.Lcd.println("Button C: " + print_bool(Button_C_state));
  }
  
  if (M5.BtnC.wasPressed()) {
    Button_C_state = !Button_C_state;
    if (Button_C_state) {
      digitalWrite(Button_C_pin, HIGH);
    }
    else {
      digitalWrite(Button_C_pin, LOW);
    }
    Serial.println("Button C: " + print_bool(Button_C_state));
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Button A: " + print_bool(Button_A_state));
    M5.Lcd.println("Button B: " + print_bool(Button_B_state));
    M5.Lcd.println("Button C: " + print_bool(Button_C_state));
  }

}
