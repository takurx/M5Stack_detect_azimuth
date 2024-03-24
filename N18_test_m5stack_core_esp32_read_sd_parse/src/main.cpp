/* main.c
// m5stack_hello_world
// Reference
// 1. 【M5Stack】基本のスケッチ例「HelloWorld」を読み解く - 趣味と学びの雑記帳
https://nouka-it.hatenablog.com/entry/2022/02/27/133549
// 2. Installation · m5stack/M5Stack – PlatformIO Registry
https://registry.platformio.org/libraries/m5stack/M5Stack/installation
// 3. M5Stack, Example, TF card write and read
https://github.com/m5stack/M5Stack/blob/master/examples/Basics/TFCard/TFCard.ino
*/

#include <Arduino.h>
#include <M5Stack.h>

#include "CSV_Parser.h"

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.print("Hello World");

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

    char csv_str[] = "3 2023-09-30 15:15:00 -55.70049406658381 19.366397684034716\n"; 
    int i = 0;
    // print line from SD card
    File myFile = SD.open("/info_sun_angle.csv",
                     FILE_READ);  // Open the file "/info_sun_angle.csv" in read mode.
    if (myFile) {
        M5.Lcd.println("/info_sun_angle.csv Content:");
        // Read the data from the file and print it until the reading is complete.
        while (myFile.available()) {            
            int readData = myFile.read();
            csv_str[i] = readData;
            i++;
            M5.Lcd.write(readData);
            if (readData == '\n') {  // Read 1 line every 1 second.

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

                M5.Lcd.println(number[0], DEC);
                M5.Lcd.println(current_day[0]);
                M5.Lcd.println(current_time[0]);
                M5.Lcd.println(sun_elevation[0]);
                M5.Lcd.println(sun_azimuth[0]);

                i = 0;
                delay(2000);
                M5.Lcd.clear();
                M5.Lcd.setCursor(0, 0);
            }
        }
        myFile.close();
    } else {
        M5.Lcd.println("error opening /info_sun_angle.csv");  // If the file is not open.
    }
}

void loop() {

}
