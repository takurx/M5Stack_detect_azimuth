/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
// 3. M5CoreS3/examples/Basics/RTC/RTC_Date_Time/RTC_Date_Time.ino at main · m5stack/M5CoreS3
https://github.com/m5stack/M5CoreS3/blob/main/examples/Basics/RTC/RTC_Date_Time/RTC_Date_Time.ino
*/

#include <Arduino.h>
#include <M5Stack.h>
#include <M5CoreS3.h>

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char timeStrbuff[64];

void flushTime() {
    M5.Rtc.GetTime(&RTCtime);  // Gets the time in the real-time clock.
    M5.Rtc.GetDate(&RTCDate);
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
            RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
    // Stores real-time time and date data
    // to timeStrbuff.
    M5.Lcd.setCursor(40, 100);
    // Move the cursor position to (x,y).
    M5.Lcd.println(timeStrbuff);
    // Output the contents of.
}

void setupTime() {
    RTCtime.Hours   = 0;  // Set the time.
    RTCtime.Minutes = 40;
    RTCtime.Seconds = 20;
    if (!M5.Rtc.SetTime(&RTCtime)) Serial.println("wrong time set!");
    // and writes the set time to the real time clock.
    RTCDate.Year  = 2023;  // Set the date.
    RTCDate.Month = 8;
    RTCDate.Date  = 28;
    if (!M5.Rtc.SetDate(&RTCDate)) Serial.println("wrong date set!");
}

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World on LCD");

  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello World on Serial, setup");
}

void loop() {
  delay(1000);
  Serial.println("Hello World on Serial, loop");
}
