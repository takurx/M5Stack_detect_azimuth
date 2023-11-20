/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
// 3. michalmonday/CSV-Parser-for-Arduino: It turns CSV string into an associative array (like dict in python)
https://github.com/michalmonday/CSV-Parser-for-Arduino
*/

#include <Arduino.h>
#include <M5Stack.h>

#include "CSV_Parser.h"

void setup() {
  M5.begin();
  M5.Power.begin();

  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello World on Serial");
  M5.Lcd.print("Hello World on LCD");

/*
  char * csv_str = "my_strings,my_longs,my_ints,my_chars,my_floats,my_hex,my_to_be_ignored\n"
         "hello,70000,140,10,3.33,FF0000,this_value_wont_be_stored\n"
         "world,80000,150,20,7.77,0000FF,this_value_wont_be_stored\n"
         "noice,90000,160,30,9.99,FFFFFF,this_value_wont_be_stored\n";
*/
  //CSV_Parser cp(csv_str, /*format*/ "sLdcfx-");

  char * csv_str = "0 2023-09-30 15:00:00 -56.541758301711155 12.815009799070014\n"
          "1 2023-09-30 15:05:00 -56.29869177488241 15.029561866680686\n"
          "2 2023-09-30 15:10:00 -56.017956331817025 17.214479540554834\n"
          "3 2023-09-30 15:15:00 -55.70049406658381 19.366397684034716\n";

   CSV_Parser cp(csv_str, /*format*/ "Lssff", /*has_header*/ false, /*delimiter*/ ' ');

  cp.print();

  // retrieving parsed values (and casting them to correct types,
  // corresponding to format string provided in constructor above)
  /*
  char    **strings =         (char**)cp["my_strings"];
  int32_t *longs =          (int32_t*)cp["my_longs"];
  int16_t *ints =           (int16_t*)cp["my_ints"];
  char    *chars =             (char*)cp["my_chars"];
  float   *floats =           (float*)cp["my_floats"];
  int32_t *longs_from_hex = (int32_t*)cp["my_hex"];    // CSV_Parser stores hex as longs (casting to int* would point to wrong address when ints[ind] is used)
  */

  int32_t *number =          (int32_t*)cp[0];
  char    **current_day =         (char**)cp[1];
  char    **current_time =         (char**)cp[2];
  float   *sun_elevation =           (float*)cp[3];
  float   *sun_azimuth =           (float*)cp[4];
  
  Serial.println("CSV_Parser example");

  for (int i = 0; i < cp.getRowsCount(); i++) {
      /*
      Serial.print(strings[i]);             Serial.print(" - ");
      Serial.print(longs[i], DEC);          Serial.print(" - ");
      Serial.print(ints[i], DEC);           Serial.print(" - ");
      Serial.print(chars[i], DEC);          Serial.print(" - ");
      Serial.print(floats[i]);              Serial.print(" - ");
      Serial.print(longs_from_hex[i], HEX); Serial.println();
      */

      Serial.print(number[i], DEC);          Serial.print(" - ");
      Serial.print(current_day[i]);             Serial.print(" - ");
      Serial.print(current_time[i]);             Serial.print(" - ");
      Serial.print(sun_elevation[i]);              Serial.print(" - ");
      Serial.print(sun_azimuth[i]);              Serial.println();
  }
}

void loop() {

}

