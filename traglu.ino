// This project uses a 128x64 pixel monochrome OLED display

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// from https://github.com/jerabaul29/Compile_time_Cpp_UNIX_timestamp/tree/master/CompilationTime_PureMacro/src
#include "lib/CompilationTime_original.h"
#include "lib/CompilationTime.h"

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// TODO: this should be replaced with a Real-time clocko
// NOTE: The generated value is corrected for the local timezone (CEST+0200)
volatile unsigned long current_time = __TIME_UNIX__ - 2*60*60;

void setup() {
  Serial.begin(9600);

  // display could use either 0x3C or 0x3D
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, 1)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
}

void loop() {
  display.clearDisplay();

  // draw current time
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(current_time);

  display.display();
  delay(100);
}
