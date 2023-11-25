/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
// 3. M5CoreS3/examples/Basics/RTC/RTC_Date_Time/RTC_Date_Time.ino at main · m5stack/M5CoreS3
https://github.com/m5stack/M5CoreS3/blob/main/examples/Basics/RTC/RTC_Date_Time/RTC_Date_Time.ino
// 4. Examples · m5stack/M5Unit-RTC – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Unit-RTC/examples/Unit_RTC_M5Core/Unit_RTC_M5Core.ino

*/

#include <Arduino.h>
#include <M5Stack.h>
#include <Unit_RTC.h>

Unit_RTC RTC;

rtc_time_type RTCtime;
rtc_date_type RTCdate;

char str_buffer[64];

void flushTime() {
    RTC.getTime(&RTCtime);  // Gets the time in the real-time clock.
    RTC.getDate(&RTCdate);
    sprintf(str_buffer, "%d/%02d/%02d %02d:%02d:%02d", RTCdate.Year,
            RTCdate.Month, RTCdate.Date, RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
    // Stores real-time time and date data
    // to timeStrbuff.
    M5.Lcd.setCursor(40, 100);
    // Move the cursor position to (x,y).
    M5.Lcd.println(str_buffer);
    // Output the contents of.
}

void setupTime() {
    RTCtime.Hours   = 0;  // Set the time.
    RTCtime.Minutes = 40;
    RTCtime.Seconds = 20;
    if (!RTC.setTime(&RTCtime)) Serial.println("wrong time set!");
    // and writes the set time to the real time clock.
    RTCdate.Year  = 2023;  // Set the date.
    RTCdate.Month = 8;
    RTCdate.Date  = 28;
    if (!RTC.setDate(&RTCdate)) Serial.println("wrong date set!");
}

void setup() {
  M5.begin();
  M5.Power.begin();

  delay(1000);
  setupTime();
  M5.Lcd.setTextSize(2);  // Set the text size.
}

void loop() {
  flushTime();
  delay(1000);
}
