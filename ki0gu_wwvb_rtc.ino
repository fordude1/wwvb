// KI0GU WWVB Testing w/RTC

#include <Wire.h>
#include "RTClib.h"
#include <stdio.h>
#include <LiquidCrystal.h>

#define DS1307_ADDRESS 0x68
#define SECONDS_PER_DAY 86400L
#define SECONDS_FROM_1970_TO_2000 946684800

unsigned short data = 2;
unsigned short count = 0; 
unsigned long duration = 0;
int hours, minutes, seconds = 0;
char time[6] = "00:00";
int last, curr = 0;
int frame[60];
int dayOfYear, month, day, year = 0;
boolean leapYear = false;

//RTC_Millis rtc;
RTC_DS1307 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup () {
    Serial.begin(57600);
    pinMode(2, INPUT); // Pin for WWBV input
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //rtc.begin(DateTime(__DATE__, __TIME__));
    lcd.begin(16, 2);
}

void loop () {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("KI0GU WWVB Clock");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    //Serial.print(now.month(), DEC);
    lcd.print(now.month(), DEC);
    //Serial.print('/');
    lcd.print('/');
    //Serial.print(now.day(), DEC);
    lcd.print(now.day(), DEC);
    //Serial.print('/');
    //Serial.print(now.year(), DEC);
    //Serial.print(' ');
    lcd.print(' ');
    //Serial.print(now.hour(), DEC);
    lcd.print(now.hour(), DEC);
    //Serial.print(':');
    lcd.print(':');
    //Serial.print(now.minute(), DEC);
    lcd.print(now.minute(), DEC);
    //Serial.print(':');
    //lcd.print(':');
    //Serial.print(now.second(), DEC);
    //lcd.print(now.second(), DEC);
    //Serial.println();
    //delay(1000);
    // End RTC

    while ( count <= 60 ) {
    duration = pulseIn(data, LOW, 10000000);
    //Serial.print(duration );
    if (duration >= 700000) { //  marker (2)
      if (last == 2) {
        count = 61;
        continue;
      }
      last = 2;
      Serial.print("*");
      frame[count] = last;
      count++;

    } else if (duration >= 400000) { //  1
      last = 1;
      Serial.print("1");
      frame[count] = last;
      count++;

    } else if (duration >= 100000) { //  0
      last = 0;
      Serial.print("0");
      frame[count] = last;
      count++;

    } else {
      last = 3;
      Serial.print("_");    //  unknown (3)
      frame[count] = last;
      count++;
    }
  }
  Serial.print("|\t");
  parseTime();
  parseDate();
  Serial.println();
  count = 1;
  frame[0] = 2;
}

void parseTime() {
  for (int foo = 0; foo < 60; foo++) {
    if (frame[foo] == 3) {
      Serial.print("BAD FRAME"); //throw out the whole frame, it's not reliable...
      return;
    }
  }
  //  FOR GMT:
  minutes = (frame[1] * 40) + (frame[2] * 20) + (frame[3] * 10) + (frame[5] * 8) + (frame[6] * 4) + (frame[7] * 2) + (frame[8] * 1);
  hours = (frame[12] * 20) + (frame[13] * 10) + (frame[15] * 8) + (frame[16] * 4) + (frame[17] * 2) + (frame[18] * 1);

  //  TIME ZONE WORK:
  hours += (frame[57] * 1); //DST check
  hours -= 6; //CST TIME ZONE
  if (hours < 0) {
    hours += 24;
  }

  minutes += 1; //hack so clock shows current time, rather than actual time 1 minute ago
  if (minutes == 60) {
    minutes = 0;
    hours += 1;
  }

  sprintf(time, "%02d:%02d", hours, minutes);
  Serial.print(time);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GMT Time: ");
  lcd.print(time);
  return;
}

void parseDate() {
  for (int foo = 0; foo < 60; foo++) {
    if (frame[foo] == 3) {
      return; //throw out the whole frame, it's not reliable...
    }
  }

  //parse DAY (since Jan 1)
  dayOfYear = (frame[22] * 200) + (frame[23] * 100) + (frame[25] * 80) + (frame[26] * 40) + (frame[27] * 20) + (frame[28] * 10)
              + (frame[30] * 8) + (frame[31] * 4) + (frame[32] * 2) + (frame[33] * 1);

  //parse Leap Year boolean
  leapYear = frame[55];

  //parse YEAR (20--)
  year = (frame[45] * 80) + (frame[46] * 40) + (frame[47] * 20) + (frame[48] * 10)
         + (frame[50] * 8) + (frame[51] * 4) + (frame[52] * 2) + (frame[53] * 1);


  if (dayOfYear <= 31) { //  Jan
    month = 1;
    day = dayOfYear;
  } else if (dayOfYear <= 59) { //  Feb (no leap)
    month = 2;
    day = dayOfYear - 31;
  } else if (dayOfYear <= 90) { //  Mar
    month = 3;
    day = dayOfYear - 59;
  } else if (dayOfYear <= 120) { //  Apr
    month = 4;
    day = dayOfYear - 90;
  } else if (dayOfYear <= 151) { //  May
    month = 5;
    day = dayOfYear - 120;
  } else if (dayOfYear <= 181) { //  June
    month = 6;
    day = dayOfYear - 151;
  } else if (dayOfYear <= 212) { //  July
    month = 7;
    day = dayOfYear - 181;
  } else if (dayOfYear <= 243) { //  Aug
    month = 8;
    day = dayOfYear - 212;
  } else if (dayOfYear <= 273) { //  Sept
    month = 9;
    day = dayOfYear - 243;
  } else if (dayOfYear <= 304) { //  Oct
    month = 10;
    day = dayOfYear - 273;
  } else if (dayOfYear <= 334) { //  Nov
    month = 11;
    day = dayOfYear - 304;
  } else if (dayOfYear <= 365) { // Dec
    month = 12;
    day = dayOfYear - 334;
  } else {
    // something got messed up...
    // ERROR?
  }

  Serial.print("\t");
  //  int weekday = parseDayOfWeek(); // good intentions...
  //  Serial.print( week[weekday] );
  Serial.print(" ");
  Serial.print(month);
  Serial.print("/");
  Serial.print(day);
  Serial.print("/");
  Serial.print(year + 2000);
  // Print to lcd
  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(month);
  lcd.print("/");
  lcd.print(day);
  lcd.print("/");
  lcd.print(year + 2000);
  
}
