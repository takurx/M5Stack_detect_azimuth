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

    // print line from SD card
    File myFile = SD.open("/info_sun_angle.csv",
                     FILE_READ);  // Open the file "/info_sun_angle.csv" in read mode.
    if (myFile) {
        M5.Lcd.println("/info_sun_angle.csv Content:");
        // Read the data from the file and print it until the reading is complete.
        while (myFile.available()) {
            M5.Lcd.write(myFile.read());  // Read 1 character every 1 second.
            sleep(1);
        }
        myFile.close();
    } else {
        M5.Lcd.println("error opening /info_sun_angle.csv");  // If the file is not open.
    }
}

void loop() {

}
