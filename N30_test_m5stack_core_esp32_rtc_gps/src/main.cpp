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
#include <HardwareSerial.h>
//#include <SoftwareSerial.h>
#include <DFRobot_GNSS.h>

RTC_PCF8563 rtc;

//DFRobot_GNSS_UART gnss(&Serial2 ,9600);
//DFRobot_GNSS_UART gnss(&Serial2 ,9600, 16, 17);
  // RX io02
  // TX io05

//SoftwareSerial mySerial(2, 5);
uint16_t GPSBaud = 9600;
uint8_t gps_rx = 16;
uint8_t gps_tx = 17;
//DFRobot_GNSS_UART gnss(&mySerial, GPSBaud, gps_rx, gps_tx);
//DFRobot_GNSS_UART gnss(&mySerial, GPSBaud);
//DFRobot_GNSS_UART gnss(&mySerial, uint16_t(9600));
//DFRobot_GNSS_UART gnss(&mySerial, 9600, 2, 5);
//DFRobot_GNSS_UART gnss(&Serial2, 9600, 16, 17);
DFRobot_GNSS_UART gnss(&Serial2, GPSBaud, gps_rx, gps_tx);

char timeStrbuff[64];

void setup () {
    M5.begin();
    M5.Power.begin();

    //delay(1000);
    M5.Lcd.setTextSize(2);  // Set the text size.

    Serial.begin(115200);

    //Serial2.begin(9600, SERIAL_8N1, 16, 17);

    //Serial2.begin(GPSBaud);

    while(!gnss.begin()){
      Serial.println("NO Deivces !");
      delay(1000);
    }

    gnss.enablePower();      // Enable gnss power 

    /** Set GNSS to be used 
     *   eGPS              use gps
     *   eBeiDou           use beidou
     *   eGPS_BeiDou       use gps + beidou
     *   eGLONASS          use glonass
     *   eGPS_GLONASS      use gps + glonass
     *   eBeiDou_GLONASS   use beidou +glonass
     *   eGPS_BeiDou_GLONASS use gps + beidou + glonass
     */
    gnss.setGnss(eGPS_BeiDou_GLONASS);

    // gnss.setRgbOff();
    gnss.setRgbOn();
    // gnss.disablePower();      // Disable GNSS, the data will not be refreshed after disabling 

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
}

void loop () {
    DateTime now = rtc.now();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Hello World\n");
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

    sTim_t utc = gnss.getUTC();
    sTim_t date = gnss.getDate();
    sLonLat_t lat = gnss.getLat();
    sLonLat_t lon = gnss.getLon();
    double high = gnss.getAlt();
    uint8_t starUserd = gnss.getNumSatUsed();
    double sog = gnss.getSog();
    double cog = gnss.getCog();

    Serial.println("");
    Serial.print(date.year);
    Serial.print("/");
    Serial.print(date.month);
    Serial.print("/");
    Serial.print(date.date);
    Serial.print("/");
    Serial.print(utc.hour);
    Serial.print(":");
    Serial.print(utc.minute);
    Serial.print(":");
    Serial.print(utc.second);
    Serial.println();
    Serial.println((char)lat.latDirection);
    Serial.println((char)lon.lonDirection);
    
    // Serial.print("lat DDMM.MMMMM = ");
    // Serial.println(lat.latitude, 5);
    // Serial.print(" lon DDDMM.MMMMM = ");
    // Serial.println(lon.lonitude, 5);
    Serial.print("lat degree = ");
    Serial.println(lat.latitudeDegree,6);
    Serial.print("lon degree = ");
    Serial.println(lon.lonitudeDegree,6);

    Serial.print("star userd = ");
    Serial.println(starUserd);
    Serial.print("alt high = ");
    Serial.println(high);
    Serial.print("sog =  ");
    Serial.println(sog);
    Serial.print("cog = ");
    Serial.println(cog);
    Serial.print("gnss mode =  ");
    Serial.println(gnss.getGnssMode());

    Serial.println();
    delay(3000);
}


