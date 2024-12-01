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

#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Ticker.h>
Ticker bno055ticker; //タイマー割り込み用のインスタンス
#define BNO055interval 10 //何ms間隔でデータを取得するか

//Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire); //ICSの名前, デフォルトアドレス, 謎
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

void get_bno055_data(void);

void setup(void)
{
  M5.begin();
  M5.Power.begin();

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
  int8_t temp = bno.getTemp();
  Serial.print("Current Temperature: ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.println("");

  bno.setExtCrystalUse(false);

  Serial.println("Calibration status values: 0=uncalibrated, 3=fully calibrated");
    
  bno055ticker.attach_ms(BNO055interval, get_bno055_data);

  M5.Lcd.setTextSize(2);
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
  
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  
  // キャリブレーションのステータスの取得と表示
  uint8_t system, gyro, accel, mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);
  Serial.print("CALIB Sys:");
  Serial.print(system, DEC);
  Serial.print(", Gy");
  Serial.print(gyro, DEC);
  Serial.print(", Ac");
  Serial.print(accel, DEC);
  Serial.print(", Mg");
  Serial.print(mag, DEC);
  
  M5.Lcd.print("CALIB Sys:");
  M5.Lcd.print(system, DEC);
  M5.Lcd.print(", Gy");
  M5.Lcd.print(gyro, DEC);
  M5.Lcd.print(", Ac");
  M5.Lcd.print(accel, DEC);
  M5.Lcd.print(", Mg");
  M5.Lcd.println(mag, DEC);
  
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
  Serial.print(" 　DIR_xyz:");
  Serial.print(euler.x());
  Serial.print(", ");
  Serial.print(euler.y());
  Serial.print(", ");
  Serial.print(euler.z());

  M5.Lcd.print("DIR_xyz:");
  M5.Lcd.print(euler.x());
  M5.Lcd.print(", ");
  M5.Lcd.print(euler.y());
  M5.Lcd.print(", ");
  M5.Lcd.println(euler.z());

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

  Serial.println();
}

void loop(void)
{
}
