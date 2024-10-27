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
#include <TinyGPSPlus.h>

void displayInfo();
void m5lcd_displayInfo();

// The TinyGPSPlus object
TinyGPSPlus gps;

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World on LCD");

  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello World on Serial, setup");

  Serial2.begin(9600);
}



void loop() {
  //delay(1000);
  //Serial.println("Hello World on Serial, loop");
  // This sketch displays information every time a new sentence is correctly encoded.
  while (Serial2.available() > 0)
  {  
    if (gps.encode(Serial2.read()))
    {
      displayInfo();
      m5lcd_displayInfo();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
  delay(1000);
}



void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time(UTC): "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}



void m5lcd_displayInfo()
{
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);

  M5.Lcd.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    M5.Lcd.print(gps.location.lat(), 6);
    M5.Lcd.print(F(","));
    M5.Lcd.print(gps.location.lng(), 6);
  }
  else
  {
    M5.Lcd.print(F("INVALID"));
  }
  M5.Lcd.println();

  M5.Lcd.print(F("Date/Time(UTC): "));
  if (gps.date.isValid())
  {
    M5.Lcd.print(gps.date.month());
    M5.Lcd.print(F("/"));
    M5.Lcd.print(gps.date.day());
    M5.Lcd.print(F("/"));
    M5.Lcd.print(gps.date.year());
  }
  else
  {
    M5.Lcd.print(F("INVALID"));
  }

  M5.Lcd.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.hour());
    M5.Lcd.print(F(":"));
    if (gps.time.minute() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.minute());
    M5.Lcd.print(F(":"));
    if (gps.time.second() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.second());
    M5.Lcd.print(F("."));
    if (gps.time.centisecond() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.centisecond());
  }
  else
  {
    M5.Lcd.print(F("INVALID"));
  }

  M5.Lcd.println();
}


