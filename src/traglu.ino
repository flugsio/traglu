// This project uses a 128x64 pixel monochrome OLED display

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// from https://github.com/jerabaul29/Compile_time_Cpp_UNIX_timestamp/tree/master/CompilationTime_PureMacro/src
#include "../lib/CompilationTime_original.h"
#include "../lib/CompilationTime.h"

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

typedef struct {
  // minutes since records_start_time
  unsigned int ts;
  byte value;
} record;

// TODO: this should be replaced with a Real-time clocko
// NOTE: The generated value is corrected for the local timezone (CEST+0200)
volatile unsigned long current_time = __TIME_UNIX__ - 2*60*60;
// unsigned int - 65000
volatile bool redraw = true;
volatile byte current_value = 0;
volatile unsigned long current_value_at = 0;

volatile unsigned long records_start_time = current_time - 14l*60*60;
const int records_length = 10;
volatile int records_cursor = 6;
record records[records_length] = {
  // just some sample data
  { 0,     5},
  { 2*60, 10},
  { 4*60,  7},
  { 6*60, 17},
  { 8*60,  4},
  {10*60,  9},
};

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

byte valueToY(byte value) {
  // max/min breaks the line angle, however the library can't handle it
  return min(63, max(0, display.height()-1-(value-4)*4));
}

// TODO: a bit too messy
byte tsToX(unsigned long ts) {
  // 16 hours in seconds
  unsigned long span = 16l*60*60;
  // seconds per pixel, 1 pixel per 8 minutes
  int width = span/120;
  return min(127, max(3, 3 + (ts - (current_time - span))/width));
}

void loop() {
  // automatically store the value if high enough
  if (current_value > 0 && (millis() - current_value_at) >= 1000) {
    if (current_value > 2) {
      records[records_cursor] = {(unsigned int)((current_time - records_start_time) / 60), current_value};
      records_cursor += 1;
    }
    current_value = 0;
    redraw = true;
  }

  if (redraw) {
    redraw = false;
    display.clearDisplay();

    // mostly everything will draw with WHITE to enable the pixels
    display.setTextColor(WHITE);

    // draw current time
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(current_time);

    // draw value
    display.setTextSize(2);
    display.setCursor(100, 0);
    display.println(current_value);

    // draw graph
    display.setTextSize(1);
    for (byte i = 0; i < min(records_length, records_cursor); i++) {
      // calculate display positions for point
      byte x_pos = tsToX(records[i].ts*60 + records_start_time);
      byte y_pos = valueToY(records[i].value);

      // draw label, center and below/above the point
      if (records[i].value > 10) {
        display.setCursor(x_pos-6, y_pos+10);
      } else {
        display.setCursor(x_pos-3, y_pos-14);
      }
      display.println(records[i].value);

      // draw line between this point and the next, if there is one
      if ((i + 1) < records_length && records[i + 1].value != 0) {
        byte x_pos2 = tsToX(records[i + 1].ts*60 + records_start_time);
        byte y_pos2 = valueToY(records[i + 1].value);
        display.drawLine(x_pos, y_pos, x_pos2, y_pos2, WHITE);
      } else {
        break;
      }
    }

    display.display();
  }
  delay(100);
}
