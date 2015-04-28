#include "U8glib.h"
#include <Time.h>
#include <Timezone.h>

#define BEDROOM
//#define BATHROOM

#ifdef BEDROOM
#include <Process.h>

#define TIME_PROVIDER get_time
#endif

#ifdef BATHROOM
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#define TIME_PROVIDER RTC.get
#endif

#define BLUE_PIN  10
#define WHITE_PIN 11
#define BACK_PIN 9

#define PI 3.141592653589
#define RAD_PER_DAY (24 / (2 * PI))

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);

TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST",  First, Sun, Nov, 2, -480};    //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

void printDigits(int);
void printDigits(int, char, unsigned int);
void digitalClockDisplay(time_t);
void setWhite(unsigned int);
void setBlue(unsigned int);
void fade(unsigned long);

#ifdef BEDROOM
time_t get_time() {
  Process date;

  if (!date.running()) {
    date.begin(F("date"));
    date.addParameter(F("+%s"));
    date.run();
  }

  while (date.running()) {
    delay(50);
  }

  if (date.available() > 0) {
    String timeString = date.readString();
    date.flush();
    return timeString.toInt();
  }

  return 0;
}
#endif

unsigned int prevBlue = 0;
unsigned int prevWhite = 0;

unsigned long fade_after = 0;

void setup() {
  setWhite(255);
  setBlue(255);
  fade((unsigned long)(120000));  // fade after two minutes of full brightness
  
  u8g.begin();
#ifdef BEDROOM
  Bridge.begin();        // initialize Bridge
#endif

  setSyncProvider(TIME_PROVIDER);
}

void loop() {
  utc = now();
  local = myTZ.toLocal(utc, &tcr);

  float x = hour(local) + ((minute(local) + (second(local) / 60.0)) / 60.0);
  float curve = sin ((x - 9.0) / RAD_PER_DAY);

  int white = (curve +  .5 ) / 1.5  * 255;
  int blue  = (curve + 1.25) / 2.25 * 255;

  if (white < 0)
    white = 0;

  if (blue < 0)
    blue = 0;
  
  if (fade_after == 0) {
    setWhite(white);
    setBlue(blue);    
  } else if (fade_after <= millis()) {
    bool needs_fade = false;
    if (prevBlue < blue) {
      blue = prevBlue + 1;
      needs_fade = true;
    } else if (prevBlue > blue) {
      blue = prevBlue - 1;
      needs_fade = true;
    }
    
    if (prevWhite < white) {
      white = prevWhite + 1;
      needs_fade = true;
    } else if (prevWhite > white) {
      white = prevWhite - 1;
      needs_fade = true;
    }
    
    if (needs_fade) {
      fade(100); 
      setWhite(white);
      setBlue(blue);
    } else {
      fade_after = 0;
    }
  }

  render();
  delay(50);
}

void setWhite(unsigned int value) {
  if (prevWhite != value) {
    analogWrite(WHITE_PIN, value);
    analogWrite(BACK_PIN, value);
    prevWhite = value;
  }
}

void setBlue(unsigned int value) {
  if (prevBlue != value) {
    analogWrite(BLUE_PIN, value);
    prevBlue = value;
  } 
}

void fade(unsigned long ms) {
  fade_after = millis() + ms; 
}

void render()
{
  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );
}

void draw()
{
//  u8g.setFont(u8g_font_unifont);
//  u8g.setFont(u8g_font_4x6);
  u8g.setFont(u8g_font_helvR08r);

  u8g.setPrintPos(1, 25);
  u8g.print(F("Blue: "));
  u8g.print(prevBlue);

  u8g.setPrintPos(1, 35);
  u8g.print(F("White: "));
  u8g.print(prevWhite);

  u8g.setPrintPos(1, 45);
  u8g.print(F("Fade After: "));
  u8g.println(fade_after);

  u8g.setPrintPos(1, 55);
  u8g.print(F("Millis: "));
  u8g.println(millis());

  digitalClockDisplay(local);
}

void digitalClockDisplay(time_t t)
{
  u8g.setPrintPos(1, 15);
  printDigits(hour(t));
  u8g.print(':');
  printDigits(minute(t));
  u8g.print(':');
  printDigits(second(t));
}

void printDigits(int number, char leader, unsigned int lead)
{
  int digits = log(number) / log(10);
  for (int i = digits + 1; i < lead; i++) {
    u8g.print(leader);
  }

  u8g.print(number);
}

void printDigits(int digits)
{
  printDigits(digits, '0', 2);
}
