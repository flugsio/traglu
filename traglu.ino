// This project uses a 128x64 pixel monochrome OLED display

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// from https://github.com/jerabaul29/Compile_time_Cpp_UNIX_timestamp/tree/master/CompilationTime_PureMacro/src
//#include "lib/CompilationTime_original.h"
//#include "lib/CompilationTime.h"
// from http://www.elecrow.com/wiki/index.php?title=File:RTC.zip
/* #include "lib/RTC/RTClib.h" */
/* RTC_DS1307 RTC; */

// from https://github.com/PaulStoffregen/Time
#include "lib/Time/TimeLib.h"

// from https://github.com/PaulStoffregen/DS1307RTC
//#include "lib/DS1307RTC/DS1307RTC.h"
#include "lib/DS1307RTC/DS1307RTC.cpp"

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

typedef struct {
  // todo, either change to offset, or store off-chip
  unsigned long ts;
  // todo: change this to byte
  double value;
} record;

// TODO: this should be replaced with a Real-time clocko
// NOTE: The generated value is corrected for the local timezone (CEST+0200)
//volatile unsigned long current_time = __TIME_UNIX__ - 2*60*60;
volatile unsigned long current_time = 1595977451 - 2*60*60;
volatile bool redraw = true;
volatile int current_value = 0;
volatile unsigned long current_value_at = 0;

const int records_length = 20;
volatile int records_cursor = 6;
record records[records_length] = {
  // just some sample data
  {current_time - 14l*60*60,  5.0},
  {current_time - 12l*60*60, 10.0},
  {current_time - 10l*60*60,  7.0},
  {current_time -  8l*60*60, 17.4},
  {current_time -  4l*60*60,  4.0},
  {current_time -  2l*60*60,  9.0},
};

const byte interruptPinI = 2;

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

  tmElements_t tm;
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

  /* Wire.begin(); */
  if (getDate(__DATE__) && getTime(__TIME__)) {
    RTC.write(tm);
  }
  /* Wire.end(); */
  delay(100);

  while(true) {
    // display could use either 0x3C or 0x3D
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, 1)) {
      //if(!display.begin(SSD1306_EXTERNALVCC, 0x3C, 1)) {
      Serial.println(F("SSD1306 allocation failed"));
      /* for(;;); */
      delay(1000);
    } else {
      Serial.println(F("SSD1306 continuing"));
      break;
    }
  }

  Serial.println(F("Starting"));

  //Wire.begin();
  pinMode(interruptPinI, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinI), increaseI, FALLING);
}

// timer1 interrupt
ISR(TIMER1_COMPA_vect) {
  current_time += 1;
  redraw = true;
}

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
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

byte valueToY(double value) {
  // max/min breaks the line angle, however the library can't handle it
  return min(63, max(0, display.height()-1-(value-4)*4));
}

// TODO: a bit too messy
byte tsToX(double ts) {
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
      records[records_cursor] = {current_time, (double)current_value};
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
      byte x_pos = tsToX(records[i].ts);
      byte y_pos = valueToY(records[i].value);

      // draw label, center and below/above the point
      if (records[i].value > 10) {
        display.setCursor(x_pos-6, y_pos+10);
      } else {
        display.setCursor(x_pos-3, y_pos-14);
      }
      display.println((byte)records[i].value);

      // draw line between this point and the next, if there is one
      if ((i + 1) < records_length && records[i + 1].value != 0) {
        byte x_pos2 = tsToX(records[i + 1].ts);
        byte y_pos2 = valueToY(records[i + 1].value);
        display.drawLine(x_pos, y_pos, x_pos2, y_pos2, WHITE);
      } else {
        break;
      }
    }

  delay(100);
  //tmElements_t tm;

  if (RTC.read(tm)) {
  /*   print2digits(tm.Hour); */
  /*   Serial.write(':'); */
  /*   print2digits(tm.Minute); */
  /*   Serial.write(':'); */
  /*   print2digits(tm.Second); */
  /*   delay(100); */

    display.setCursor(30, 30);
    display.println((byte)tm.Second);
  /*   delay(100); */
  /*   Serial.print(", Date (D/M/Y) = "); */
  /*   Serial.print(tm.Day); */
  /*   Serial.write('/'); */
  /*   Serial.print(tm.Month); */
  /*   Serial.write('/'); */
  /*   Serial.print(tmYearToCalendar(tm.Year)); */
  /*   Serial.println(); */
  /* } else { */
  /*   if (RTC.chipPresent()) { */
  /*     Serial.println("The DS1307 is stopped.  Please run the SetTime"); */
  /*     Serial.println("example to initialize the time and begin running."); */
  /*     Serial.println(); */
  /*   } else { */
  /*     Serial.println("DS1307 read error!  Please check the circuitry."); */
  /*     Serial.println(); */
    }
  /* } */


    display.display();
  }
  delay(100);

}
