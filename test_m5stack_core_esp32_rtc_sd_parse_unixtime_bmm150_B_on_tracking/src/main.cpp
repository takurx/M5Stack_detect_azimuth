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
// 6. bmm150 ahrs
*/

#include <Arduino.h>
#include <M5Stack.h>

#include <RTClib.h>

#include "CSV_Parser.h"

#define M5STACK_MPU6886
#define LCD
#include "BMM150class.h"
#include <utility/quaternionFilters.h>

//#define DISPLAY_AHRS

uint8_t pin_on_off = 5;
bool pin_on_off_state = false;

RTC_PCF8563 rtc;

char timeStrbuff[64];

File myFile;

//#define DISPLAY_RAW
//#define DISPLAY_AHRS
float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;

int AVERAGENUM_GY = 256;
float init_gyroX = 0.0F;
float init_gyroY = 0.0F;
float init_gyroZ = 0.0F;

float magnetX = 0.0F;
float magnetY = 0.0F;
float magnetZ = 0.0F;

//hard
float magoffsetX = 0.0F;
float magoffsetY = 0.0F;
float magoffsetZ = 0.0F;

//soft
float magscaleX = 0.0F;
float magscaleY = 0.0F;
float magscaleZ = 0.0F;

float pitch = 0.0F;
float roll = 0.0F;
float yaw = 0.0F;

float temp = 0.0F;

BMM150class bmm150;

uint32_t currentTime = 0;
uint32_t lastUpdate = 0, firstUpdate = 0;
uint32_t lastAzimuthUpdate = 0;
float deltat = 0.0f, sum = 0.0f;

void initGyro() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("begin gyro calibration");

  for (int i = 0;i < AVERAGENUM_GY;i++) {
    M5.IMU.getGyroData(&gyroX,&gyroY,&gyroZ);
    init_gyroX += gyroX;
    init_gyroY += gyroY;
    init_gyroZ += gyroZ;
    delay(5);
  }
  init_gyroX /= AVERAGENUM_GY;
  init_gyroY /= AVERAGENUM_GY;
  init_gyroZ /= AVERAGENUM_GY;
}

void setup () {
  M5.begin();
  M5.Power.begin();

  pinMode(pin_on_off, OUTPUT);
  digitalWrite(pin_on_off, LOW);
  pin_on_off_state = false;

  M5.IMU.Init();
  initGyro();
  bmm150.Init();

  bmm150.getMagnetOffset(&magoffsetX, &magoffsetY, &magoffsetZ);
  bmm150.getMagnetScale(&magscaleX, &magscaleY, &magscaleZ);

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
  } 
  else {
    M5.Lcd.println("info_sun_angle.csv doesn't exist.");
  }

  // print line from SD card
  myFile = SD.open("/info_sun_angle.csv", FILE_READ);  // Open the file "/info_sun_angle.csv" in read mode.
}

void loop () {
  float magnetX1, magnetY1, magnetZ1;
  DateTime now, dt;
  float target_azimuth;

  // put your main code here, to run repeatedly:
  M5.update();
  M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
  gyroX -= init_gyroX;
  gyroY -= init_gyroY;
  gyroZ -= init_gyroZ;
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  bmm150.getMagnetData(&magnetX, &magnetY, &magnetZ);

  magnetX1 = (magnetX - magoffsetX) * magscaleX;
  magnetY1 = (magnetY - magoffsetY) * magscaleY;
  magnetZ1 = (magnetZ - magoffsetZ) * magscaleZ;

  M5.IMU.getTempData(&temp);
  float head_dir = atan2(magnetX1, magnetY1);
  if(head_dir < 0)
    head_dir += 2*M_PI;
  if(head_dir > 2*M_PI)
    head_dir -= 2*M_PI;
  head_dir *= RAD_TO_DEG;

  currentTime = micros();
  deltat = ((currentTime - lastUpdate) / 1000000.0f);
  lastUpdate = currentTime;

  MadgwickQuaternionUpdate(accX, accY, accZ, gyroX * DEG_TO_RAD, gyroY * DEG_TO_RAD, gyroZ * DEG_TO_RAD, -magnetX1, magnetY1, -magnetZ1, deltat);

#ifdef DISPLAY_RAW
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.printf("%6.2f  %6.2f  %6.2f      ", gyroX, gyroY, gyroZ);
  M5.Lcd.setCursor(220, 42);
  M5.Lcd.print(" o/s");
  M5.Lcd.setCursor(0, 65);
  M5.Lcd.printf(" %5.2f   %5.2f   %5.2f   ", accX, accY, accZ);
  M5.Lcd.setCursor(220, 87);
  M5.Lcd.print(" G");
  M5.Lcd.setCursor(0, 110);
  M5.Lcd.printf(" %5.2f   %5.2f   %5.2f   ", magnetX, magnetY, magnetZ);
  M5.Lcd.setCursor(220, 132);
  M5.Lcd.print(" mT");
  M5.Lcd.setCursor(0, 155);
  M5.Lcd.printf("Temperature : %.2f C", temp);
#endif


  yaw = atan2(2.0f * (*(getQ() + 1) * *(getQ() + 2) + *getQ() *
                                                          *(getQ() + 3)),
              *getQ() * *getQ() + *(getQ() + 1) * *(getQ() + 1) - *(getQ() + 2) * *(getQ() + 2) - *(getQ() + 3) * *(getQ() + 3));
  pitch = -asin(2.0f * (*(getQ() + 1) * *(getQ() + 3) - *getQ() *
                                                            *(getQ() + 2)));
  roll = atan2(2.0f * (*getQ() * *(getQ() + 1) + *(getQ() + 2) *
                                                     *(getQ() + 3)),
               *getQ() * *getQ() - *(getQ() + 1) * *(getQ() + 1) - *(getQ() + 2) * *(getQ() + 2) + *(getQ() + 3) * *(getQ() + 3));
  yaw = -0.5*M_PI-yaw;
  if(yaw < 0)
    yaw += 2*M_PI;
  if(yaw > 2*M_PI)
    yaw -= 2*M_PI;
  pitch *= RAD_TO_DEG;
  yaw *= RAD_TO_DEG;
  roll *= RAD_TO_DEG;
  // Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) Boulder, Co. USA is
  // 	8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
  // - http://www.ngdc.noaa.gov/geomag-web/#declination
  //  yaw -= 8.5;
#ifdef DISPLAY_AHRS
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  //M5.Lcd.setCursor(0, 100);
  M5.Lcd.printf("  yaw: % 5.2f    pitch: % 5.2f    roll: % 5.2f   \r\n", (yaw), (pitch), (roll));
#endif

  //M5.Lcd.clear();
  //M5.Lcd.setCursor(0, 0);
  //M5.Lcd.setCursor(20, 220);
  //M5.Lcd.printf("BTN_A:CAL ");

  if(M5.BtnA.wasPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("begin calibration in 3 seconds");
    delay(3000);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.print("Flip + rotate core calibration");
    bmm150.bmm150_calibrate(15000);
    delay(100);

    bmm150.Init();
    bmm150.getMagnetOffset(&magoffsetX, &magoffsetY, &magoffsetZ);
    bmm150.getMagnetScale(&magscaleX, &magscaleY, &magscaleZ);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setTextSize(2);
  }

  if (M5.BtnB.isPressed()) {
    M5.Lcd.print("Button B is pressed, pin 5 is HIGH");
    digitalWrite(pin_on_off, HIGH);
    pin_on_off_state = true;
  }
  if (M5.BtnC.isPressed()) {
    M5.Lcd.print("Button C is pressed, pin 5 is LOW");
    digitalWrite(pin_on_off, LOW);
    pin_on_off_state = false;
  }

  deltat = ((currentTime - lastAzimuthUpdate) / 1000000.0f);
  //if(deltat > 3.0)
  if(deltat > 1.0) {
    lastAzimuthUpdate = currentTime;
    //DateTime now;
    now = rtc.now();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    //M5.Lcd.print("Hello World\n");
    if (pin_on_off_state) {
      M5.Lcd.print("pin 5 is HIGH\n");
    }
    else {
      M5.Lcd.print("pin 5 is LOW\n");
    }
    
    //M5.Lcd.printf("yaw: % 5.2f, pitch: % 5.2f, roll: % 5.2f\n", (yaw), (pitch), (roll));
    //M5.Lcd.printf("yaw: % 5.2f\n", (yaw));
    //M5.Lcd.printf("pitch: % 5.2f\n", (pitch));
    //M5.Lcd.printf("roll: % 5.2f\n", (roll));
    M5.Lcd.printf("azimuth: % 5.2f\n", (yaw));
    M5.Lcd.printf("\n");
    
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", now.year(),
            now.month(), now.day(), now.hour(), now.minute(), now.second());
    
    M5.Lcd.print(timeStrbuff);
    M5.Lcd.print("\n");
    M5.Lcd.printf("%d\n", now.unixtime());

    /*
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    */
    //Serial.print(" (");
    //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    //Serial.print(") ");
    /*
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
    */

    // calculate a date which is 7 days, 12 hours and 30 seconds into the future
    /*
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
    */

    //Serial.println();

    //char csv_str[] = "3 2023-09-30 15:15:00 -55.70049406658381 19.366397684034716\n"; 
    char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
    int i = 0;

    //DateTime dt;

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
            
        /*
        Serial.print(number[0], DEC);       Serial.print(" - ");
        Serial.print(current_day[0]);       Serial.print(" - ");
        Serial.print(current_time[0]);      Serial.print(" - ");
        Serial.print(sun_elevation[0]);     Serial.print(" - ");
        Serial.print(sun_azimuth[0]);       Serial.println();
        */

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

        /*
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
        */
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
        target_azimuth = sun_azimuth[0];
        //M5.Lcd.println(target_azimuth);

        i = 0;
        break;
      }
    }
  }

  if(now.unixtime() > dt.unixtime()) {
    M5.Lcd.println("Working...");
  //while(1) {
    if(yaw < target_azimuth) {
      M5.Lcd.println("Turn Clowckwise");
      digitalWrite(pin_on_off, HIGH);
      pin_on_off_state = true;
      //break;
    }
    else {
      M5.Lcd.println("Stop turning");
      digitalWrite(pin_on_off, LOW);
      pin_on_off_state = false;
    }
  }
  else {
    M5.Lcd.println("Waiting...");
  }
  //delay(3000);
}


