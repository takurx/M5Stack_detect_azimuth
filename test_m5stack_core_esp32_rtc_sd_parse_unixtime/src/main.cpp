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
// 5. Installation · adafruit/RTClib – PlatformIO Registry
https://registry.platformio.org/libraries/adafruit/RTClib/installation
*/

#include <Arduino.h>
#include <M5Stack.h>

#include <RTClib.h>

#include "CSV_Parser.h"

RTC_PCF8563 rtc;

char timeStrbuff[64];

File myFile;

void setup () {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.setTextSize(2);  // Set the text size.

  Serial.begin(115200);

#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  // read SD card
  if (!SD.begin()) {  // Initialize the SD card.
      M5.Lcd.println("Card failed, or not present");  // Print a message if the SD card initialization fails or if the SD card does not exist
      while (1)
          ;
  }
  M5.Lcd.println("TF card initialized.");
  if (SD.exists("/info_sun_angle.csv")) {  // Check if the "/info_sun_angle.csv" file exists.
      M5.Lcd.println("info_sun_angle.csv exists.");
  } else {
      M5.Lcd.println("info_sun_angle.csv doesn't exist.");
  }

  // print line from SD card
  myFile = SD.open("/info_sun_angle.csv",
                    FILE_READ);  // Open the file "/info_sun_angle.csv" in read mode.

}

void loop () {
    DateTime now = rtc.now();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Hello World\n");
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", now.year(),
            now.month(), now.day(), now.hour(), now.minute(), now.second());
    M5.Lcd.print(timeStrbuff);
    M5.Lcd.print("\n");

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    //Serial.print(" (");
    //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    //Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");

    // calculate a date which is 7 days, 12 hours and 30 seconds into the future
    DateTime future (now + TimeSpan(7,12,30,6));

    Serial.print(" now + 7d + 12h + 30m + 6s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();

    //Serial.println();

    //char csv_str[] = "3 2023-09-30 15:15:00 -55.70049406658381 19.366397684034716\n"; 
    char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
    int i = 0;

    while (myFile.available()) {        
        int readData = myFile.read();
        csv_str[i] = readData;
        i++;
        M5.Lcd.write(readData);
        if (readData == '\n') {  // Read 1 line
            CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

            //cp.print();
            //M5.Lcd.clear();
            //M5.Lcd.setCursor(0, 0);

            int32_t *number =          (int32_t*)cp[0];
            char    **current_day =    (char**)cp[1];
            char    **current_time =   (char**)cp[2];
            float   *sun_elevation =   (float*)cp[3];
            float   *sun_azimuth =     (float*)cp[4];
            
            Serial.print(number[0], DEC);       Serial.print(" - ");
            Serial.print(current_day[0]);       Serial.print(" - ");
            Serial.print(current_time[0]);      Serial.print(" - ");
            Serial.print(sun_elevation[0]);     Serial.print(" - ");
            Serial.print(sun_azimuth[0]);       Serial.println();

            DateTime dt = DateTime(2023, 11, 25, 21, 35, 00);

            Serial.print(dt.unixtime());
            Serial.print(',');
            Serial.print(dt.year(), DEC);
            Serial.print('/');
            Serial.print(dt.month(), DEC);
            Serial.print('/');
            Serial.print(dt.day(), DEC);
            Serial.print(' ');
            Serial.print(dt.hour(), DEC);
            Serial.print(':');
            Serial.print(dt.minute(), DEC);
            Serial.print(':');
            Serial.print(dt.second(), DEC);
            Serial.println();
            //DateTime dt = DateTime(current_day[0], crrent_time[0]);

            M5.Lcd.println(number[0], DEC);
            M5.Lcd.println(current_day[0]);
            M5.Lcd.println(current_time[0]);
            M5.Lcd.println(sun_elevation[0]);
            M5.Lcd.println(sun_azimuth[0]);

            i = 0;
            break;
        }
    }

    delay(1000);
}


