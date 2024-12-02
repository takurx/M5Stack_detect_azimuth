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

// RTC
#include <RTClib.h>

// MPU6886+BMM150
/*
#define M5STACK_MPU6886
#include "BMM150class.h"
#include <utility/quaternionFilters.h>
*/
//#define DISPLAY_AHRS
//#define DISPLAY_RAW

// BNO055
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Ticker.h>
Ticker bno055ticker; //タイマー割り込み用のインスタンス
#define BNO055interval 10 //何ms間隔でデータを取得するか
//Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire); //ICSの名前, デフォルトアドレス, 謎
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// Other
#include "CSV_Parser.h"

#define LCD



/****************************************
 * *globala variable
*****************************************/ 

// RTC
RTC_PCF8563 rtc;

// MPU6886+BMM150
/*
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

float temp = 0.0F;

float pitch = 0.0F;
float roll = 0.0F;
float yaw = 0.0F;

BMM150class bmm150;
*/

// BNO055
uint8_t STATUS_SYSTEM, STATUS_GYRO, STATUS_ACCEL, STATUS_MAG = 0;
float YAW = 0.0F;
float PITCH = 0.0F;
float ROLL = 0.0F;
uint8_t TEMPERATURE = 0;

enum MODE {
  MANUAL,
  AUTO
};
uint8_t current_mode = MANUAL;

// Other
uint8_t PIN_PWM_OUT = 5;
uint8_t pwm_channel = 0;
uint8_t pwm_duty = 16; // 50% = 128/256 = 128/2^8, pwm_resolution = 8


// MPU6886+BMM150, YAW,  0.00 is North, 90.00 is East, 180.00 is South, 270.00 is West
// BNO055, Euler angle, YAW, 270.00 is North, 00.00 is East, 90.00 is South, 180.00 is West
// -> 0.00 is North, 90.00 is East, 180.00 is South, 270.00 is West
float current_azimuth;
float target_azimuth;
float last_target_azimuth;

char timeStrbuff[64];

const int periodWorkMinute = 5;
bool isTurn;

char csv_str[] = "000 2023-00-00 00:00:00 -00.00000000000000 000.000000000000000\n"; 
DateTime ct;
DateTime dataTime;

File myFile;

uint32_t currentTime = 0;
uint32_t lastUpdate = 0, firstUpdate = 0;
uint32_t lastAzimuthUpdate = 0;
float deltat = 0.0f;

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

// state_calibration_azimuth: azimuth調整状態フラグ、モーター稼動状態
bool state_calibration_azimuth = false;



/****************************************
 * *Function prototype
*****************************************/ 

// MPU6886+BMM150
/*
void initGyro();
*/
// BNO055
void get_bno055_data(void);

void seek_sd_card ();
void work_0p75Degree ();
void work_azimuth_run ();
void work_azimuth_stop ();



/****************************************
 * *Setup
*****************************************/ 

void setup () {
  M5.begin();
  M5.Power.begin();

  pinMode(PIN_PWM_OUT, OUTPUT);
  //digitalWrite(pin_pwm_out, LOW);
  //uint8_t pwm_channel = 0;
  uint32_t pwm_frequency = 100;
  uint8_t pwm_resolution = 8;
  ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_PWM_OUT, pwm_channel);
  //ledcWrite(pwm_channel, pwm_duty);

  // MPU6886+BMM150
  /*
  M5.IMU.Init();
  initGyro();
  bmm150.Init();
  bmm150.getMagnetOffset(&magoffsetX, &magoffsetY, &magoffsetZ);
  bmm150.getMagnetScale(&magscaleX, &magscaleY, &magscaleZ);
  */

  // BNO055
  pinMode(21, INPUT_PULLUP); //SDA 21番ピンのプルアップ(念のため)
  pinMode(22, INPUT_PULLUP); //SDA 22番ピンのプルアップ(念のため)

  Serial.begin(115200);
  Serial.println("Orientation Sensor Raw Data Test"); Serial.println("");

  if (!bno.begin()) // センサの初期化
  {
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

  delay(1000);

  /* Display the current temperature */
  TEMPERATURE = bno.getTemp();
  Serial.print("Current Temperature: ");
  Serial.print(TEMPERATURE);
  Serial.println(" C");
  Serial.println("");

  bno.setExtCrystalUse(false);

  Serial.println("Calibration status values: 0=uncalibrated, 3=fully calibrated");
  bno055ticker.attach_ms(BNO055interval, get_bno055_data);

  M5.Lcd.setTextSize(2);  // Set the text size.
  Serial.begin(115200);

  // RTC
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
  //rtc.adjust(DateTime(2024, 4, 19, 2, 0, 0));

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
  //current_azimuth = sun_azimuth;

  /*
  while(1)
  {
    if (current_azimuth > sun_azimuth[0]) {
      break;
    }
    else {
      Serial.print(sun_azimuth[0]);
      Serial.print(", ");
      Serial.println(current_azimuth);

      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("initializing azimuth......");
      M5.Lcd.print(sun_azimuth[0]);
      M5.Lcd.print(", ");
      M5.Lcd.println(current_azimuth);

      work_0p75Degree();
      current_azimuth = current_azimuth + 0.90;
      delay(800); 
      //200+800=1000ms, move 0.90 degree -> 5 minutes = 300000ms, 
      //300000/1000 = 300 times, 0.90 deg * 300 times = 225.0 deg
    }
  }
  */

  //delay(1000);
  //seek_sd_card();
  //Serial.println("finish seek");
  //delay(1000);
  //float target_azimuth = *sun_azimuth;
  target_azimuth = *sun_azimuth;
  current_azimuth = target_azimuth;
  //int init_azimuth_count = int(target_azimuth) / 100; // 0-360.00 / 100 = 0-3
  int init_azimuth_count = 0;
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



/****************************************
 * *Loop
*****************************************/ 

void loop () {
  // MPU6886+BMM150
  /*
  float magnetX1, magnetY1, magnetZ1;
  */
  // RTC
  DateTime now = rtc.now();
  DateTime dt;
  // Other
  int i;
  int readData_loop;

  // put your main code here, to run repeatedly:
  M5.update();

  // MPU6886+BMM150
  /*
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
  */

  currentTime = micros();
  deltat = ((currentTime - lastUpdate) / 1000000.0f);
  lastUpdate = currentTime;

  /*
  MadgwickQuaternionUpdate(accX, accY, accZ, gyroX * DEG_TO_RAD, gyroY * DEG_TO_RAD, gyroZ * DEG_TO_RAD, -magnetX1, magnetY1, -magnetZ1, deltat);
  */

/*
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
*/

  /*
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
  */

/*
#ifdef DISPLAY_AHRS
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  //M5.Lcd.setCursor(0, 100);
  M5.Lcd.printf("  yaw: % 5.2f    pitch: % 5.2f    roll: % 5.2f   \r\n", (yaw), (pitch), (roll));
#endif
*/

  if (M5.BtnA.wasPressed()) {
    M5.Lcd.println("Button A was pressed");
    // Function: Reset current azimuth, 太陽に合わせたと仮定して、現在角度をtarget_azimuthにリセットする
    // Function: set Manual mode, 手動モードに設定する
    current_azimuth = target_azimuth;
    M5.Lcd.println("Reset current azimuth to target azimuth");
    current_mode = MANUAL;
    M5.Lcd.println("Set Manual mode");
  }
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.println("Button B was pressed");
    // Function: Reset current azimuth, IMUセンサが正しいと仮定して、現在角度をIMUセンサのazimuth(YAW)にリセットする
    // Function: set Auto mode, 自動モードに設定する
    //current_azimuth = 180.00;
    //M5.Lcd.println("Reset azimuth to 180.00");
    current_azimuth = YAW;
    M5.Lcd.println("Reset current azimuth to sensor azimuth");
    current_mode = AUTO;
    M5.Lcd.println("Set Auto mode");

    /*
    // target_azimuthに向けて駆動する
    if (target_azimuth > current_azimuth)
    {
      //current_azimuth = YAW;
      //None
    }
    else  // target_azimuth < current_azimuth
    {
      current_azimuth = YAW - 360.0;
    }

    while(1)
    {
      if (current_azimuth > target_azimuth) {
        break;
      }
      else {
        Serial.print(target_azimuth);
        Serial.print(", ");
        Serial.println(current_azimuth);

        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("sensor reset azimuth......");
        M5.Lcd.print(target_azimuth);
        M5.Lcd.print(", ");
        M5.Lcd.println(current_azimuth);

        work_0p75Degree();
        current_azimuth = current_azimuth + 0.90;
        delay(800); 
        //200+800=1000ms, move 0.90 degree -> 5 minutes = 300000ms, 
        //300000/1000 = 300 times, 0.90 deg * 300 times = 225.0 deg
      }
    }
    */
  }
  if (M5.BtnC.wasPressed()) {
    M5.Lcd.println("Button C was pressed");
    if (state_calibration_azimuth == false) {
      work_azimuth_run();
    }
    else { // state_calibration_azimuth == true
      work_azimuth_stop();
    }
  }

  if (state_calibration_azimuth == false) 
  {
    deltat = ((currentTime - lastAzimuthUpdate) / 1000000.0f);
    // deltatで3秒ごとに実行する
    if (deltat > 3.0)
    {
      lastAzimuthUpdate = currentTime;

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

      if (current_mode == AUTO)
      {
        if (STATUS_SYSTEM == 3 && STATUS_MAG > 1)
        {
          current_azimuth = YAW;
        }
      }

      // checkPeriodで5分ごと、かつ、isTurnがfalseの場合のみ
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
            
            last_target_azimuth = target_azimuth;
            target_azimuth = sun_azimuth[0];
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
      //M5.Lcd.println(sun_azimuth[0]);
      M5.Lcd.println(target_azimuth);
      M5.Lcd.print("Current_Azimuth: ");
      M5.Lcd.println(current_azimuth);
      M5.Lcd.print("Sensor_Status_sys (=3): ");
      M5.Lcd.println(STATUS_SYSTEM);
      M5.Lcd.print("Sensor_Status_Mag (<1): ");
      M5.Lcd.println(STATUS_MAG);
      M5.Lcd.print("Sensor_Azimuth: ");
      M5.Lcd.println(YAW);
      M5.Lcd.print("Current_Mode: ");
      if (current_mode == MANUAL) {
        M5.Lcd.println("Manual");
      }
      else {
        M5.Lcd.println("Auto");
      }
      
      // 現在角度が目標角度よりも小さい場合、目標角度に向けて駆動する
      //if (current_azimuth < sun_azimuth[0]) {
      if (current_azimuth < target_azimuth) {
        work_0p75Degree();
        current_azimuth = current_azimuth + 0.90;
        //delay(2800);
        //200+2800=3000ms, move 0.1 degree -> 5 minutes = 300000ms, 
        //300000/3000 = 100 times, 0.1 deg * 100 times = 10.0 deg
        // 現在角度が360.00を超えた場合、360.00を引く
        /*
        if (current_azimuth > 360.00) {
          current_azimuth = current_azimuth - 360.00;
        }
        // target 359.9
        // current 359.5
        // のときうかつに0.9足して引くと危ない
        */
      }
      // 現在角度が目標角度よりも大きい場合
      else {   // current_azimuth > target_azimuth
        // 360-0.0の切り替わり、境界のケースの処理
        // 現在角度が目標角度よりも大きい、かつ、前回の目標角度が目標角度よりも大きい、かつ、現在の角度が目標角度+360.0よりも小さい場合、駆動する
        if (last_target_azimuth > target_azimuth)
        {
          if (current_azimuth < target_azimuth + 360.0)
          {
            // example
            // current_azimuth = 350.00
            // target_azimuth = 1.97
            // last_target_azimuth = 357.59

            work_0p75Degree();
            current_azimuth = current_azimuth + 0.90;
          }
          // else, current_azimuth > target_azimuth && last_target_azimuth > target_azimuth && current_azimuth > target_azimuth + 360.0)
          // 何もしない
        }
        // 現在角度が目標角度よりも大きい、かつ、前回の目標角度が目標角度よりも小さい場合
        else //last_target_azimuth < target_azimuth
        {
          if (current_azimuth > 360.00) {
            // 現在角度が360.00を超えた場合、360.00を引く
            current_azimuth = current_azimuth - 360.00;
          } 
          //else, current_azimuth > target_azimuth && last_target_azimuth < target_azimuth && current_azimuth < 360.0
          // 何もしない 
        }
      }
    }
  }
}



/****************************************
 * *Function 
*****************************************/ 
/*
void initGyro() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("begin gyro calibration");

  // MPU6886+BMM150
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
*/

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
  state_calibration_azimuth = true;
  //sleep(1); //100 Hz -> 100 pulse
  //delay(500); //50 pulse, OK
  //delay(100); //10 pulse, not work
  delay(200); //20 pulse
  ledcWrite(pwm_channel, 0);
  state_calibration_azimuth = false;
}

void work_azimuth_run () {
  ledcWrite(pwm_channel, pwm_duty);
  state_calibration_azimuth = true;
}

void work_azimuth_stop () {
  ledcWrite(pwm_channel, 0);
  state_calibration_azimuth = false;
}

void get_bno055_data(void)
{
  // Possible vector values can be:
  // - VECTOR_ACCELEROMETER - m/s^2
  // - VECTOR_MAGNETOMETER  - uT
  // - VECTOR_GYROSCOPE     - rad/s
  // - VECTOR_EULER         - degrees
  // - VECTOR_LINEARACCEL   - m/s^2
  // - VECTOR_GRAVITY       - m/s^2
  
  
  // キャリブレーションのステータスの取得と表示
  //uint8_t system, gyro, accel, mag = 0;
  //uint8_t STATUS_SYSTEM, STATUS_GYRO, STATUS_ACCEL, STAUS_MAG = 0;
  //bno.getCalibration(&system, &gyro, &accel, &mag);
  bno.getCalibration(&STATUS_SYSTEM, &STATUS_GYRO, &STATUS_ACCEL, &STATUS_MAG);
  /*
  Serial.print("CALIB Sys:");
  Serial.print(system, DEC);
  Serial.print(", Gy");
  Serial.print(gyro, DEC);
  Serial.print(", Ac");
  Serial.print(accel, DEC);
  Serial.print(", Mg");
  Serial.print(mag, DEC);
  */

  /*
  // ジャイロセンサ値の取得と表示
  imu::Vector<3> gyroscope = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  Serial.print(" 　Gy_xyz:");
  Serial.print(gyroscope.x());
  Serial.print(", ");
  Serial.print(gyroscope.y());
  Serial.print(", ");
  Serial.print(gyroscope.z());
  */
  
  /*
  // 加速度センサ値の取得と表示
  imu::Vector<3> accelermetor = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  Serial.print(" 　Ac_xyz:");
  Serial.print(accelermetor.x());
  Serial.print(", ");
  Serial.print(accelermetor.y());
  Serial.print(", ");
  Serial.print(accelermetor.z());
  */
  
  /*
  // 磁力センサ値の取得と表示
  imu::Vector<3> magnetmetor = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
  Serial.print(" 　Mg_xyz:");
  Serial.print(magnetmetor .x());
  Serial.print(", ");
  Serial.print(magnetmetor .y());
  Serial.print(", ");
  Serial.print(magnetmetor .z());
  */

  // センサフュージョンによる方向推定値の取得と表示
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  YAW = euler.x();
  // MPU6886+BMM150, YAW,  0.00 is North, 90.00 is East, 180.00 is South, 270.00 is West
  // BNO055, Euler angle, YAW, 270.00 is North, 00.00 is East, 90.00 is South, 180.00 is West
  // -> 0.00 is North, 90.00 is East, 180.00 is South, 270.00 is West
  YAW = YAW + 90.0;
  if (YAW > 360.0) {
    YAW = YAW - 360.0;
  }
  PITCH = euler.y();
  ROLL = euler.z();
  /*
  Serial.print(" 　DIR_xyz:");
  Serial.print(euler.x());
  Serial.print(", ");
  Serial.print(euler.y());
  Serial.print(", ");
  Serial.print(euler.z());
  */

  /*
    // センサフュージョンの方向推定値のクオータニオン
    imu::Quaternion quat = bno.getQuat();
    Serial.print("qW: ");
    Serial.print(quat.w(), 4);
    Serial.print(" qX: ");
    Serial.print(quat.x(), 4);
    Serial.print(" qY: ");
    Serial.print(quat.y(), 4);
    Serial.print(" qZ: ");
    Serial.print(quat.z(), 4);
    Serial.print("\t\t");
  */

  //Serial.println();
}
