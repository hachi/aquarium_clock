#include <Time.h>
#include <Timezone.h>

#define BEDROOM
//#define BATHROOM

#ifdef BEDROOM
#define HAS_LCD
#include <LiquidCrystal.h>
#include <Process.h>

#define BLUE_PIN  3
#define WHITE_PIN 11

#define TIME_PROVIDER get_time
#endif

#ifdef BATHROOM
#define HAS_LCD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#define BLUE_PIN  10
#define WHITE_PIN 11
#define BACK_PIN 9

#define TIME_PROVIDER RTC.get
#endif

#define PI 3.141592653589
#define RAD_PER_DAY (24 / (2 * PI))

#ifdef BEDROOM
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // 10 is the bloody backlight
#endif

#ifdef BATHROOM
LiquidCrystal_I2C lcd(0x27, 20, 4);
#endif

TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST",  First, Sun, Nov, 2, -480};    //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

void printDigits(int);
void printDigits(int, char, unsigned int);
void digitalClockDisplay(time_t);

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

void setup() {
#ifdef BEDROOM
  Bridge.begin();        // initialize Bridge
  lcd.begin(16, 2);
#endif
#ifdef BATHROOM
  lcd.init();
  lcd.backlight();
#endif
  lcd.setCursor(0, 0);
  lcd.print(F("Clock syncing"));

  setSyncProvider(TIME_PROVIDER);
  lcd.clear();
  lcd.setCursor(0, 0);
  if (timeStatus() != timeSet) {
    lcd.print(F("Clock sync fail"));
  }
  else {
    lcd.print(F("Clock synced"));
  }
  
  delay(2000);
  lcd.clear();

#ifdef BACK_PIN  
  analogWrite(BACK_PIN, 50);
#endif
}

void loop() {
  static int prevBlue = -1;
  static int prevWhite = -1;
  
  utc = now();
  local = myTZ.toLocal(utc, &tcr);

  digitalClockDisplay(local);

  float x = hour(local) + ((minute(local) + (second(local) / 60.0)) / 60.0);
  float curve = sin ((x - 9.0) / RAD_PER_DAY);
  
  int white = (curve +  .5 ) / 1.5  * 255;
  int blue  = (curve + 1.25) / 2.25 * 255;
  
  if (white < 0)
    white = 0;
    
  if (blue < 0)
    blue = 0;
    
  if (prevWhite != white) {
    lcd.setCursor(0, 1);
    lcd.print(F("W:"));
    printDigits(white, ' ', 3);
    analogWrite(WHITE_PIN, white);
    prevWhite = white;
  }
  
  if (prevBlue != blue) {
    lcd.setCursor(10, 1);
    lcd.print(F("B:"));
    printDigits(blue, ' ', 3);
    analogWrite(BLUE_PIN, blue);
    prevBlue = blue;
  }
  
  delay(50);
}

void digitalClockDisplay(time_t t)
{
  static int prevsec = 62;
  int seconds = second(t);
  if (seconds == prevsec)
    return;
  prevsec = seconds;
  lcd.setCursor(0, 0);
  printDigits(hour(t));
  lcd.print(F(":"));
  printDigits(minute(t));
  lcd.print(F(":"));
  printDigits(seconds);
}

void printDigits(int number, char leader, unsigned int lead)
{
  int digits = log(number)/log(10);
  for (int i = digits + 1; i < lead; i++) {
    lcd.print(leader);
  }
    
  lcd.print(number);
}

void printDigits(int digits)
{
  printDigits(digits, '0', 2);
}

