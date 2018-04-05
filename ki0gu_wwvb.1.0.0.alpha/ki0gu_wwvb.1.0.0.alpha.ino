/*  KI0GU 4/3/2018
 *   
 *   WWVB Testing w/RTC v1.0.0.alpha
 *   The First working version
 *   A lot of code clean-up needed here
 *   
 *   Arduino board: UNO
 *   LCD: LCD keypad shield integrated
 *   RTC: DS1307
 *   WWVB Receiver: Universal-Solder WWVB Receiver Module 
 *   https://universal-solder.com/product/60khz-wwvb-atomic-radio-clock-receiver-replaces-c-max-cmmr-6p-60/
 *   
 *   When the board is first powered on, it tries to aquire a signal from WWVB.
 *   If this signal is weak or has errors, it reads the internal RTC and displays that to the LCD.
 *   It then attempts to aquire the signal again.  Once it is succesful, it will display GMT on the display
 *   and show the aquired time/date.
 *   
 *   Some things to be done:
 *   
 *   1. Allow manual setting of time/date with buttons on LCD shield
 *   2. Turn on/off receiver and just read time from RTC
 *   3. Show signal strength on LCD
 *   4. Alarm clock option with external speaker
 *   
 */

#include <stdio.h>
#include <Wire.h>                 // i2c bus Library
#include <LiquidCrystal.h>        // LCD Display Library
#define DS1307_I2C_ADDRESS 0x68   // Address for RTC


// constants for WWVB Code
unsigned short data = 2;
unsigned short count = 0; 
unsigned long duration = 0;
int hours, minutes, seconds = 0;
char time[6] = "00:00";
int last, curr = 0;
int frame[60];
int dayOfYear, month, day, year = 0;
boolean leapYear = false;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // Pins for LCD

// Begin RTC Code 

// Convert decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val / 10 * 16) + (val % 10) );
}

// Convert binary coded decimal to decimal numbers
byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}

// Day of Year to month translation (thanks to Capt.Tagon)
// End of Month - to calculate Month and Day from Day of Year 
int eomYear[14][2] = {
  {0,0},      // Begin
  {31,31},    // Jan
  {59,60},    // Feb
  {90,91},    // Mar
  {120,121},  // Apr
  {151,152},  // May
  {181,182},  // Jun
  {212,213},  // Jul
  {243,244},  // Aug
  {273,274},  // Sep
  {304,305},  // Oct
  {334,335},  // Nov
  {365,366},  // Dec
  {366,367}   // overflow
};

// Set the RTC time
void setDS1307time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS1307
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);                    // set next input to start at the seconds register
  Wire.write(decToBcd(second));     // set seconds
  Wire.write(decToBcd(minute));     // set minutes
  Wire.write(decToBcd(hour));       // set hours
  Wire.write(decToBcd(dayOfWeek));  // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month));      // set month
  Wire.write(decToBcd(year));       // set year (0 to 99)
  Wire.endTransmission();
}

void readDS1307time(byte *second, byte *minute, byte *hour, byte *dayOfWeek,byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);                    // set DS1307 register pointer to 00h
  Wire.endTransmission();

  // request seven bytes of data from DS1307 starting from register 00h
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}


void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  
  // retrieve data from DS1307
  readDS1307time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
                 
  // send Time to the serial
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch (dayOfWeek) {
    case 1:
      Serial.println("Sunday");
      break;
    case 2:
      Serial.println("Monday");
      break;
    case 3:
      Serial.println("Tuesday");
      break;
    case 4:
      Serial.println("Wednesday");
      break;
    case 5:
      Serial.println("Thursday");
      break;
    case 6:
      Serial.println("Friday");
      break;
    case 7:
      Serial.println("Saturday");
      break;
    default:
      Serial.println(dayOfWeek);
  }

  // send Time to the lcd display
  lcd.setCursor(0, 1);
  lcd.print(hour, DEC);
  lcd.print(":");
  if (minute < 10)
    {
      lcd.print("0");
    }
  lcd.print(minute, DEC);
  lcd.print(" ");
  lcd.print(month, DEC);
  lcd.print("/");
  lcd.print(dayOfMonth, DEC);
  lcd.print("/");
  lcd.print(year, DEC);

}
// End RTC Code

void setup () {
    Wire.begin();
    Serial.begin(9600);
    lcd.begin(16, 2);
    pinMode(2, INPUT); // Pin for WWBV input
/* set the initial time here: ONLY DO THIS ONCE!
     DS1307 seconds, minutes, hours, day, date, month, year
     Note for day - Sunday is 1, Monday is 2 etc
  */
   //setDS1307time(0,15,21,03,03,04,18);

  // Initialize LCD and display welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("KI0GU WWVB Clock");
  lcd.setCursor(0, 1);
  lcd.print("Aquiring Signal");
}

void loop () {

/* Start WWVB Testing and 
 * decoding below this line
 */
    while ( count <= 60 ) {
    duration = pulseIn(data, LOW, 10000000);
    //Serial.print(duration );
    if (duration >= 700000) {       //  marker (2)
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
      Serial.print("_");            //  unknown (3)
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
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("KI0GU RTC Clock");
      lcd.setCursor(0, 1);
      displayTime();  // display the real-time clock data
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
  lcd.print("GMT*Time: ");
  lcd.print(time);
  return;
}

void parseDate() {
  for (int foo = 0; foo < 60; foo++) {
    if (frame[foo] == 3) {
      return; //throw out the whole frame, it's not reliable.
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


  if (dayOfYear <= 31) {            //  Jan
    month = 1;
    day = dayOfYear;
  } else if (dayOfYear <= 59) {     //  Feb (no leap)
    month = 2;
    day = dayOfYear - 31;
  } else if (dayOfYear <= 90) {     //  Mar
    month = 3;
    day = dayOfYear - 59;
  } else if (dayOfYear <= 120) {    //  Apr
    month = 4;
    day = dayOfYear - 90;
  } else if (dayOfYear <= 151) {    //  May
    month = 5;
    day = dayOfYear - 120;
  } else if (dayOfYear <= 181) {    //  June
    month = 6;
    day = dayOfYear - 151;
  } else if (dayOfYear <= 212) {    //  July
    month = 7;
    day = dayOfYear - 181;
  } else if (dayOfYear <= 243) {    //  Aug
    month = 8;
    day = dayOfYear - 212;
  } else if (dayOfYear <= 273) {    //  Sept
    month = 9;
    day = dayOfYear - 243;
  } else if (dayOfYear <= 304) {    //  Oct
    month = 10;
    day = dayOfYear - 273;
  } else if (dayOfYear <= 334) {    //  Nov
    month = 11;
    day = dayOfYear - 304;
  } else if (dayOfYear <= 365) {    // Dec
    month = 12;
    day = dayOfYear - 334;
  } else {
    // something got messed up...
    // ERROR?
  }
  
  // Print Date to Serial
  Serial.print("\t");
  // int weekday = parseDayOfWeek(); // good intentions...
  // Serial.print( week[weekday] );
  Serial.print(" ");
  Serial.print(month);
  Serial.print("/");
  Serial.print(day);
  Serial.print("/");
  Serial.print(year + 2000);
  
  // Print Date to lcd
  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(month);
  lcd.print("/");
  lcd.print(day);
  lcd.print("/");
  lcd.print(year + 2000);

  // Set RTC from WWVB
  /* set the initial time here: ONLY DO THIS ONCE!
     DS1307 seconds, minutes, hours, day, date, month, year
     Note for day - Sunday is 1, Monday is 2 etc
  */
 //setDS1307time(0,15,21,03,03,04,18); // Still have to figure out the logic here
  // Translate wwvb day of year into a month and a day of month
  // This routine is courtesy of Capt.Tagon
  //int doy = ((byte) wwvbFrame->DayHun * 100) +
  //           ((byte) wwvbFrame->DayTen * 10) +
  //           ((byte) wwvbFrame->DayOne);
             
}
