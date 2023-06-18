/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
*/

#include <Arduino.h>
#include <M5Stack.h>

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World");
}

void loop() {

}
