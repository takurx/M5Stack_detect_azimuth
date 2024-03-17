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

unsigned int periodWorkMinute = 5;
bool isTurn;

char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
DateTime ct;
DateTime dataTime;

File myFile;

void seek_sd_card () {
  //char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
  int i = 0;
  //DateTime ct = rtc.now();
  ct = rtc.now();
  Serial.print(ct.unixtime());
  //DateTime dataTime;
  while(1) {
    while (myFile.available()) {       
      //delay(100); 
      int readData = myFile.read();
      csv_str[i] = readData;
      i++;
      //Serial.print(readData);
      if (readData == '\n') {  // Read 1 line
        //Serial.print("line next");
        CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

        int32_t *number_seek =          (int32_t*)cp[0];
        char    **current_day_seek =    (char**)cp[1];
        char    **current_time_seek =   (char**)cp[2];
        float   *sun_elevation_seek =   (float*)cp[3];
        float   *sun_azimuth_seek =     (float*)cp[4];

        //Serial.print(current_day_seek[0]);
        //Serial.print(", ");

        strcpy(csv_str, current_day_seek[0]);
        strcat(csv_str, "\n");
        CSV_Parser cp2(csv_str, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
        
        //cp2.print();
        uint16_t *dt_year_seek = (uint16_t*)cp2[0];
        uint8_t *dt_month_seek = (uint8_t*)cp2[1];
        uint8_t *dt_day_seek = (uint8_t*)cp2[2];

        strcpy(csv_str, current_time_seek[0]);
        strcat(csv_str, "\n");
        CSV_Parser cp3(csv_str, /*format*/ "ucucuc", /*has_header*/ false, /*delimiter*/ ':');
        
        //cp3.print();
        uint8_t *dt_hour_seek = (uint8_t*)cp3[0];
        uint8_t *dt_minute_seek = (uint8_t*)cp3[1];
        uint8_t *dt_second_seek = (uint8_t*)cp3[2];
        
        /*
        uint8_t *dt_hour_seek = 0;
        uint8_t *dt_minute_seek = 0;
        uint8_t *dt_second_seek = 0;
        */

        dataTime = DateTime(dt_year_seek[0], dt_month_seek[0], dt_day_seek[0], dt_hour_seek[0], dt_minute_seek[0], dt_second_seek[0]);

        //M5.Lcd.printf("%d\n", dataTime.unixtime());
        Serial.print(ct.unixtime());
        Serial.print(',');
        Serial.println(dataTime.unixtime());
        //delay(100);

        i = 0;
        break;
      }
    }
    if(ct.unixtime() < dataTime.unixtime()) {
      //delay(3000);
      Serial.println("ok, seek");
      break;
    }
  }
}

void setup () {
  M5.begin();
  M5.Power.begin();

  //delay(1000);
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

  isTurn = false;

  // read SD card
  // delay(3000);
  if (!SD.begin()) {  // Initialize the SD card.
    M5.Lcd.println("Card failed, or not present");  // Print a message if the SD card initialization fails or if the SD card does not exist
    while (1)
      ;
  }
  M5.Lcd.println("TF card initialized.");
  if (SD.exists("/info_sun_angle.csv")) {  // Check if the "/info_sun_angle.csv" file exists.
    M5.Lcd.println("info_sun_angle.csv exists.");
  } 
  else {
    M5.Lcd.println("info_sun_angle.csv doesn't exist.");
  }

  // print line from SD card
  myFile = SD.open("/info_sun_angle.csv", FILE_READ);  // Open the file "/info_sun_angle.csv" in read mode.

  delay(1000);
  seek_sd_card();
  Serial.println("finish seek");
  delay(1000);
}

void loop () {
    DateTime now = rtc.now();
    DateTime dt;

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", now.year(),
            now.month(), now.day(), now.hour(), now.minute(), now.second());
    M5.Lcd.print(timeStrbuff);

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

    if (now.minute()%periodWorkMinute == 0 && isTurn == false)
    {
        isTurn = true;
        Serial.println("5 minutes turn");

        int i = 0;
        while (myFile.available()) {        
          int readData = myFile.read();
          csv_str[i] = readData;
          i++;
          //M5.Lcd.write(readData);
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

            //DateTime dt = DateTime(2023, 11, 25, 21, 35, 00);
            //DateTime dt;
                
            //char current_day_test[] = "2023-11-25";
            //char current_day_test[] = "2023-11-25\n";
            //CSV_Parser cp2(current_day_test, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
            //stcurrent_day[0] << "\n";
            //strcat(current_day[0], "\n");

            strcpy(csv_str, current_day[0]);
            strcat(csv_str, "\n");
            //CSV_Parser cp2(current_day[0], /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
            CSV_Parser cp2(csv_str, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
            
            //cp2.print();
            uint16_t *dt_year = (uint16_t*)cp2[0];
            uint8_t *dt_month = (uint8_t*)cp2[1];
            uint8_t *dt_day = (uint8_t*)cp2[2];
            /*
            Serial.println(dt_year[0]);
            Serial.println(dt_month[0]);
            Serial.println(dt_day[0]);
            */

            strcpy(csv_str, current_time[0]);
            strcat(csv_str, "\n");
            //CSV_Parser cp3(current_time[0], /*format*/ "ucucuc", /*has_header*/ false, /*delimiter*/ ':');
            CSV_Parser cp3(csv_str, /*format*/ "ucucuc", /*has_header*/ false, /*delimiter*/ ':');
            
            //cp3.print();
            uint8_t *dt_hour = (uint8_t*)cp3[0];
            uint8_t *dt_minute = (uint8_t*)cp3[1];
            uint8_t *dt_second = (uint8_t*)cp3[2];
            /*
            Serial.println(dt_hour[0]);
            Serial.println(dt_minute[0]);
            Serial.println(dt_second[0]);
            */

            dt = DateTime(dt_year[0], dt_month[0], dt_day[0], dt_hour[0], dt_minute[0], dt_second[0]);

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

            /*
            M5.Lcd.printf("%d\n", dt.unixtime());
            M5.Lcd.println(number[0], DEC);
            M5.Lcd.println(current_day[0]);
            M5.Lcd.println(current_time[0]);
            M5.Lcd.println(sun_elevation[0]);
            M5.Lcd.println(sun_azimuth[0]);
            */

            M5.Lcd.printf("%d\n", dt.unixtime());
            M5.Lcd.print(current_day[0]);
            M5.Lcd.print(" ");
            M5.Lcd.println(current_time[0]);
            M5.Lcd.print("Target: ");
            M5.Lcd.println(sun_azimuth[0]);
            //target_azimuth = sun_azimuth[0];
            //M5.Lcd.println(target_azimuth);

            i = 0;
            break;
          }
        }
    }
    else if (now.minute()%periodWorkMinute != 0)
    {
        isTurn = false;
    }
    else
    {
        // do nothing
    }

    delay(3000);
}


