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
volatile bool redraw = true;
volatile int current_value = 0;
volatile unsigned long current_value_at = 0;

const byte interruptPinI = 2;

void setup() {
  // setup 1 second clock interrupt
  cli();
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // set entire TCCR1B register to 0
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register to 1Hz
  OCR1A = 15624; // = (16*10^6) / (1*1024) - 1 (must be <65536)
  TCCR1B |= (1 << WGM12); // turn on CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10); // Set CS12 and CS10 bits for 1024 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

  Serial.begin(9600);

  // display could use either 0x3C or 0x3D
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, 1)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  pinMode(interruptPinI, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinI), increaseI, FALLING);
}

// timer1 interrupt
ISR(TIMER1_COMPA_vect) {
  current_time += 1;
  redraw = true;
}

// This is the 1/I button, which is used to increase the value by 1
void increaseI() {
  current_value += 1;
  if (current_value > 33) {
    current_value = 0;
  }
  current_value_at = millis();
  redraw = true;
}

void loop() {
  // automatically store the value if high enough
  if (current_value > 0 && (millis() - current_value_at) >= 1000) {
    if (current_value > 2) {
      // todo records storage
    }
    current_value = 0;
    redraw = true;
  }

  if (redraw) {
    redraw = false;
    display.clearDisplay();

    // draw current time
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(current_time);

    // draw value
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(100, 0);
    display.println(current_value);

    display.display();
  }
  delay(100);
}
