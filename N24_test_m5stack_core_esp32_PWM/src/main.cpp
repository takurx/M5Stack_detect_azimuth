/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
// 3. M5StackGray デューティー比固定周波数可変PWM
https://coskxlabsite.stars.ne.jp/html/for_students/M5Stack/PWMTest/PWMTest2.html
*/

#include <Arduino.h>
#include <M5Stack.h>

uint8_t PIN_PWM_OUT = 5;
uint8_t pwm_channel = 0;
uint8_t pwm_duty = 5; // 50% = 128/256 = 128/2^8, pwm_resolution = 8
  
void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World");

  pinMode(PIN_PWM_OUT, OUTPUT);
  //digitalWrite(pin_pwm_out, LOW);
  //uint8_t pwm_channel = 0;
  uint32_t pwm_frequency = 100;
  uint8_t pwm_resolution = 8;
  ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_PWM_OUT, pwm_channel);
  ledcWrite(pwm_channel, pwm_duty);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    M5.Lcd.print("Button A was pressed");
    pwm_duty = pwm_duty + 1; // + 32; // +12.5%
    M5.Lcd.printf("PWM Duty: %d\n", pwm_duty);
    //ledcWrite(pwm_channel, pwm_duty);
  }

  if (M5.BtnB.wasPressed()) {
    M5.Lcd.print("Button B was pressed");
    pwm_duty = pwm_duty - 1; // - 32; // -12.5%
    M5.Lcd.printf("PWM Duty: %d\n", pwm_duty);
    //ledcWrite(pwm_channel, pwm_duty);
  }

  if (M5.BtnC.wasPressed()) {
    M5.Lcd.print("Button C was pressed");
    M5.Lcd.printf("PWM Duty: %d\n", pwm_duty);
    ledcWrite(pwm_channel, pwm_duty);
    //sleep(1); //100 Hz -> 100 pulse
    //delay(500); //50 pulse, OK
    //delay(100); //10 pulse, not work
    delay(200); //20 pulse
    ledcWrite(pwm_channel, 0);
  }
}
