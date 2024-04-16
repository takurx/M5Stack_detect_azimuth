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

uint8_t PIN_PWM_OUT = 5;
uint8_t pwm_channel = 0;
uint8_t pwm_duty = 16; // 50% = 128/256 = 128/2^8, pwm_resolution = 8

float current_azimuth; // 0.00 is North, 90.00 is East, 180.00 is South, 270.00 is West

RTC_PCF8563 rtc;

char timeStrbuff[64];

//uint8_t periodWorkMinute = 5;
const int periodWorkMinute = 5;
bool isTurn;

char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
DateTime ct;
DateTime dataTime;

File myFile;

int32_t *number_seek;
char    **current_day_seek;
char    **current_time_seek;
float   *sun_elevation_seek;
float   *sun_azimuth_seek;

uint16_t *dt_year_seek;
uint8_t *dt_month_seek;
uint8_t *dt_day_seek;

uint8_t *dt_hour_seek;
uint8_t *dt_minute_seek;
uint8_t *dt_second_seek;

int32_t *number;
char    **current_day;
char    **current_time;
float   *sun_elevation;
float   *sun_azimuth;

uint16_t *dt_year;
uint8_t *dt_month;
uint8_t *dt_day;

uint8_t *dt_hour;
uint8_t *dt_minute;
uint8_t *dt_second;

void seek_sd_card () {
  //char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
  int i = 0;
  //DateTime ct = rtc.now();
  int readData_seek;

  ct = rtc.now();
  Serial.print(ct.unixtime());
  //DateTime dataTime;
  while(1) {
    while (myFile.available()) {       
      //delay(100); 
      readData_seek = myFile.read();
      csv_str[i] = readData_seek;
      i++;
      //Serial.print(readData_seek);
      if (readData_seek == '\n') {  // Read 1 line
        //Serial.print("line next");
        CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

        number_seek =          (int32_t*)cp[0];
        current_day_seek =    (char**)cp[1];
        current_time_seek =   (char**)cp[2];
        sun_elevation_seek =   (float*)cp[3];
        sun_azimuth_seek =     (float*)cp[4];

        //Serial.print(current_day_seek[0]);
        //Serial.print(", ");

        strcpy(csv_str, current_day_seek[0]);
        strcat(csv_str, "\n");
        CSV_Parser cp2(csv_str, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
        
        //cp2.print();
        dt_year_seek = (uint16_t*)cp2[0];
        dt_month_seek = (uint8_t*)cp2[1];
        dt_day_seek = (uint8_t*)cp2[2];

        strcpy(csv_str, current_time_seek[0]);
        strcat(csv_str, "\n");
        CSV_Parser cp3(csv_str, /*format*/ "ucucuc", /*has_header*/ false, /*delimiter*/ ':');
        
        //cp3.print();
        dt_hour_seek = (uint8_t*)cp3[0];
        dt_minute_seek = (uint8_t*)cp3[1];
        dt_second_seek = (uint8_t*)cp3[2];
        
        /*
        uint8_t *dt_hour_seek = 0;
        uint8_t *dt_minute_seek = 0;
        uint8_t *dt_second_seek = 0;
        */

        dataTime = DateTime(dt_year_seek[0], dt_month_seek[0], dt_day_seek[0], dt_hour_seek[0], dt_minute_seek[0], dt_second_seek[0]);

        //M5.Lcd.printf("%d\n", dataTime.unixtime());
        //Serial.print(ct.unixtime());
        Serial.print(".");
        //Serial.println(dataTime.unixtime());
        //delay(100);
        //delay(2);

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

void work_0p75Degree () {
  ledcWrite(pwm_channel, pwm_duty);
  //sleep(1); //100 Hz -> 100 pulse
  //delay(500); //50 pulse, OK
  //delay(100); //10 pulse, not work
  delay(200); //20 pulse
  ledcWrite(pwm_channel, 0);
}

void setup () {
  M5.begin();
  M5.Power.begin();

  //delay(1000);
  M5.Lcd.setTextSize(2);  // Set the text size.

  pinMode(PIN_PWM_OUT, OUTPUT);
  //digitalWrite(pin_pwm_out, LOW);
  //uint8_t pwm_channel = 0;
  uint32_t pwm_frequency = 100;
  uint8_t pwm_resolution = 8;
  ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_PWM_OUT, pwm_channel);
  //ledcWrite(pwm_channel, pwm_duty);

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
  // set UTC time,readData
  //rtc.adjust(DateTime(2024, 4, 25, 2, 47, 0));

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

  number = number_seek;
  current_day = current_day_seek;
  current_time = current_time_seek;
  sun_elevation = sun_elevation_seek;
  sun_azimuth = sun_azimuth_seek;
  
  dt_year = dt_year_seek;
  dt_month = dt_month_seek;
  dt_day = dt_day_seek;
  
  dt_hour = dt_hour_seek;
  dt_minute = dt_minute_seek;
  dt_second = dt_second_seek;

  //current_azimuth = 235.00;
  current_azimuth = 0.00;

  while(1)
  {
    if (current_azimuth > sun_azimuth[0]) {
      break;
    }
    else {
      Serial.print(sun_azimuth[0]);
      Serial.print(", ");
      Serial.println(current_azimuth);

      work_0p75Degree();
      current_azimuth = current_azimuth + 0.90;
      delay(800); 
      //200+800=1000ms, move 0.90 degree -> 5 minutes = 300000ms, 
      //300000/1000 = 300 times, 0.90 deg * 300 times = 225.0 deg
    }
  }

  //delay(1000);
  //seek_sd_card();
  //Serial.println("finish seek");
  //delay(1000);
  float target_azimuth = *sun_azimuth;
  int init_azimuth_count = int(target_azimuth) / 100; // 0-360.00 / 100 = 0-3
  Serial.print(target_azimuth);
  Serial.print(", ");
  Serial.println(init_azimuth_count);
  
  int i = 0;
  int j = 0;
  int readData_setup;
  while(j < init_azimuth_count) {
    j++;
    while (myFile.available()) {  
      readData_setup = myFile.read();
      csv_str[i] = readData_setup;
      i++;      
      //M5.Lcd.write(readData_setup);
      if (readData_setup == '\n') {  // Read 1 line
        CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

        number_seek =          (int32_t*)cp[0];
        current_day_seek =    (char**)cp[1];
        current_time_seek =   (char**)cp[2];
        sun_elevation_seek =   (float*)cp[3];
        sun_azimuth_seek =     (float*)cp[4];

        strcpy(csv_str, current_day_seek[0]);
        strcat(csv_str, "\n");

        CSV_Parser cp2(csv_str, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
        
        dt_year_seek = (uint16_t*)cp2[0];
        dt_month_seek = (uint8_t*)cp2[1];
        dt_day_seek = (uint8_t*)cp2[2];

        strcpy(csv_str, current_time_seek[0]);
        strcat(csv_str, "\n");
        CSV_Parser cp3(csv_str, /*format*/ "ucucuc", /*has_header*/ false, /*delimiter*/ ':');
        
        dt_hour_seek = (uint8_t*)cp3[0];
        dt_minute_seek = (uint8_t*)cp3[1];
        dt_second_seek = (uint8_t*)cp3[2];

        dataTime = DateTime(dt_year_seek[0], dt_month_seek[0], dt_day_seek[0], dt_hour_seek[0], dt_minute_seek[0], dt_second_seek[0]);

        i = 0;
        break;
      }
    }
  }

  number = number_seek;
  current_day = current_day_seek;
  current_time = current_time_seek;
  sun_elevation = sun_elevation_seek;
  sun_azimuth = sun_azimuth_seek;
  
  dt_year = dt_year_seek;
  dt_month = dt_month_seek;
  dt_day = dt_day_seek;
  
  dt_hour = dt_hour_seek;
  dt_minute = dt_minute_seek;
  dt_second = dt_second_seek;
}



void loop () {
  DateTime now = rtc.now();
  DateTime dt;
  int i;
  int readData_loop;

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", now.year(),
          now.month(), now.day(), now.hour(), now.minute(), now.second());
  M5.Lcd.print(timeStrbuff);
  M5.Lcd.print(" UTC");

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
  Serial.print(", ");
  Serial.print(isTurn);

  Serial.print(", ");
  uint8_t now_minute = now.minute();
  Serial.print(now_minute);

  //uint8_t checkPeriod = now_minute%periodWorkMinute;
  int current_minute = int(now_minute);
  Serial.print(", ");
  Serial.print(current_minute);
  int checkPeriod = 0;
  checkPeriod = current_minute%periodWorkMinute;
  //checkPeriod = current_minute%5;
  Serial.print(", ");
  Serial.print(checkPeriod);

  Serial.println();

  if (M5.BtnA.wasPressed()) {
    M5.Lcd.println("Button A was pressed");
    current_azimuth = 90.00;
    M5.Lcd.println("Reset azimuth to 90.00");
  }
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.println("Button B was pressed");
    current_azimuth = 180.00;
    M5.Lcd.println("Reset azimuth to 180.00");
  }
  if (M5.BtnC.wasPressed()) {
    M5.Lcd.println("Button C was pressed");
    current_azimuth = 0.00;
    M5.Lcd.println("Reset azimuth to 0.00");
  }

  //if (now.minute()%periodWorkMinute == 0 && isTurn == false)
  if (checkPeriod == 0 && isTurn == false) {
    isTurn = true;
    Serial.println("5 minutes turn");

    i = 0;
    while (myFile.available()) {        
      readData_loop = myFile.read();
      csv_str[i] = readData_loop;
      i++;
      //M5.Lcd.write(readData_loop);
      if (readData_loop == '\n') {  // Read 1 line
        CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

        //cp.print();
        //M5.Lcd.clear();
        //M5.Lcd.setCursor(0, 0);

        number =        (int32_t*)cp[0];
        current_day =   (char**)cp[1];
        current_time =  (char**)cp[2];
        sun_elevation = (float*)cp[3];
        sun_azimuth =   (float*)cp[4];
          
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
        //stcurrent_day[0periodWorkMinute] << "\n";
        //strcat(current_day[0], "\n");

        strcpy(csv_str, current_day[0]);
        strcat(csv_str, "\n");
        //CSV_Parser cp2(current_day[0], /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
        CSV_Parser cp2(csv_str, /*format*/ "uducuc", /*has_header*/ false, /*delimiter*/ '-');
        
        //cp2.print();
        dt_year = (uint16_t*)cp2[0];
        dt_month = (uint8_t*)cp2[1];
        dt_day = (uint8_t*)cp2[2];
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
        dt_hour = (uint8_t*)cp3[0];
        dt_minute = (uint8_t*)cp3[1];
        dt_second = (uint8_t*)cp3[2];
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

        /*
        M5.Lcd.printf("%d\n", dt.unixtime());
        M5.Lcd.print(current_day[0]);
        M5.Lcd.print(" ");
        M5.Lcd.println(current_time[0]);
        M5.Lcd.print("Target: ");
        M5.Lcd.println(sun_azimuth[0]);
        */
        
        //target_azimuth = sun_azimuth[0];
        //M5.Lcd.println(target_azimuth);

        i = 0;
        break;
      }
    }
  }

  else if (checkPeriod != 0 && isTurn == true) {
    isTurn = false;
  }
  else {
    // do nothing
  }

  M5.Lcd.printf("\n");
  M5.Lcd.printf("%d\n", dt.unixtime());
  M5.Lcd.print(current_day[0]);
  M5.Lcd.print(" ");
  M5.Lcd.println(current_time[0]);
  M5.Lcd.print("Target: ");
  M5.Lcd.println(sun_azimuth[0]);
  M5.Lcd.print("Current_Azimuth: ");
  M5.Lcd.println(current_azimuth);

  if (current_azimuth > sun_azimuth[0]) {
    delay(3000);
  }
  else {
    work_0p75Degree();
    current_azimuth = current_azimuth + 0.90;
    delay(2800);
    //200+2800=3000ms, move 0.1 degree -> 5 minutes = 300000ms, 
    //300000/3000 = 100 times, 0.1 deg * 100 times = 10.0 deg
  }
}


