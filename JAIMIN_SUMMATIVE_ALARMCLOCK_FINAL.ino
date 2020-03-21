/*
   Alarm Clock using the RTC DS3232 and LEDs.
   @Jaimin Jaffer
   Last updated January 23, 2020
*/

#include <NewTone.h> //https://bitbucket.org/teckel12/arduino-new-tone
#include <IRremote.h> //https://github.com/z3t0/Arduino-IRremote
#include <DS3232RTC.h> //https://github.com/JChristensen/DS3232RTC
#include <LiquidCrystal.h>

//initialize the library by associating any needed LCD interface pin with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//pin used for turning the LCD backlight on and off.
const int backLight = 7;

//declare and initialize the IR receiver pin and set it up
int RECV_PIN = 10;
IRrecv irrecv(RECV_PIN);
decode_results results;

//pin for the standard white LED
const int ledPin = 13;

//boolean that communicates whether the alarm is on or off
boolean alarmOn = false;

//set up alarm minutes and hours as integers
int alarmHour = 0;
int alarmMinute = 0;

//custom alarm symbol character
byte alarmSymbol[] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100,
  B00000
};

//custom all black character
byte allBlack[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

//define buzzer pin
int melodyPin = 8; //Using digital pin 8
#define NOTE_C6  1047 //define the only note being used

//pins for the RGB light
int red_light_pin = 6;
int green_light_pin = 9;
int blue_light_pin = A0;

void setup() {

  //setup Serial Monitor
  Serial.begin(9600);

  //setTime(23, 59, 55, 19, 1, 2020);   //set the system time to 23h31m30s on 13Feb2009
  //RTC.set(now());                     //set the RTC from the system time

  //get the time from the RTC
  setSyncProvider(RTC.get);

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.

  //setup customer characters
  lcd.createChar(0, alarmSymbol);
  lcd.createChar(1, allBlack);

  //turn off any alarms
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);

  //enable the IR receiver
  irrecv.enableIRIn();

  //setup the rgb pins as outputs and set the rgb to off.
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  RGB_color(0, 0, 0);
}


void loop()
{
  int value = 0; //stores IR receiver signal in int form

  //checking (and stores if found) for an IR signal
  if (irrecv.decode(&results)) {
    value = results.value;
    Serial.println(value);
    irrecv.resume();
  }

  //displays the custom alarm character if the alarm is on
  if (alarmOn == true) {
    lcd.setCursor(15, 1);
    lcd.write(byte(0));
  } else {
    lcd.setCursor(15, 1);
    lcd.print(" ");
  }

  //updates time
  digitalClockDisplayLCD();

  //play-pause button enables and disables the alarm
  if (value == 765) {
    alarmOn = !alarmOn;
    RTC.alarmInterrupt(ALARM_2, alarmOn);
  }

  //checks if the alarm has triggered
  if ( RTC.alarm(ALARM_2) ) {
    alarmOn = false;
    RTC.alarmInterrupt(ALARM_2, false); //turns off the alarm for now. The user can turn it back on after
    digitalWrite(ledPin, HIGH); //turns on the LEDs
    digitalWrite(backLight, HIGH); //turns on the LCD backlight.(in case it was off)
    playAlarm();
  }

  //func/stop button used to set an alarm.
  if (value == -7651) {
    alarmSet();
  }

  //EQ button used to set the date and time
  if (value == -26521) {
    clockSet();
  }

  //0 button controls the LCD backlight(off and on).
  if (value == 26775) {
    if (digitalRead(backLight) == LOW) {
      digitalWrite(backLight, HIGH);
    } else {
      digitalWrite(backLight, LOW);
    }
  }

  //power button turns on the basic white LEDs.
  if (value == -23971) {
    if (digitalRead(ledPin) == LOW) {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
  }

  //buttons 1-9 control the RGB LED
  remoteToRGB(value, 12495, 255, 0, 0); //Red
  remoteToRGB(value, 6375, 0, 255, 00); //Green
  remoteToRGB(value, 31365, 0, 0, 255); //Blue
  remoteToRGB(value, 4335, 75, 0, 130); //Indigo
  remoteToRGB(value, 14535, 0, 255, 255); //Cyan
  remoteToRGB(value, 23205, 255, 0, 255); //Magenta
  remoteToRGB(value, 17085, 255, 255, 0); //Yellow
  remoteToRGB(value, 19125, 255, 255, 255); //White
  remoteToRGB(value, 21165, 0, 0, 0); //Off
  delay(1000);
}

//checks if IR code is the one being looked for and then acts with the proper RGB values.
void remoteToRGB(int value, int IRnumber, int r, int g, int b) {
  if (value == IRnumber) {
    RGB_color(r, g, b);
  }
}

/**
   Displays date, time and temperature. Values are taken using the library.
*/
void digitalClockDisplayLCD()
{
  lcd.setCursor(0, 0);
  if (hour() < 10) {
    lcd.print(String("0") + hour());
  } else {
    lcd.print(hour());
  }
  lcd.print(formatTime(minute()));
  lcd.print(formatTime(second()));

  lcd.setCursor(0, 1);
  lcd.print(monthToString(month()));
  lcd.print(' ');
  lcd.print(day());
  lcd.print(' ');
  lcd.print(year());

  lcd.setCursor(11, 0);
  int t = RTC.temperature();
  float celsius = t / 4.0;
  lcd.print(celsius);
  lcd.setCursor(15, 0);
  lcd.print('C');

}

/**
   Method allowing the user to set a custom date and time
*/
void clockSet() {
  lcd.clear();
  tmElements_t tm; //makes a tmElements_t structure
  tm.Hour = hour(); //sets the new hour to current RTC hour
  tm.Minute = minute(); //sets the new minute to current RTC minute
  tm.Second = second(); //sets the new second to current RTC second
  tm.Day = day(); //sets the new day to current RTC day
  tm.Month = month(); //sets the new month to current RTC month
  tm.Year = year() - 1970; //sets the new year to current RTC year
  //tmElements_t.Year is the offset from 1970

  // digital clock display of the date & time. (and the words "set time")
  lcd.setCursor(0, 0);
  if (hour() < 10) {
    lcd.print(String("0") + hour());
  } else {
    lcd.print(hour());
  }
  lcd.print(formatTime(minute()));
  lcd.print(formatTime(second()));

  lcd.setCursor(0, 1);
  if (day() < 10) {
    lcd.print(String("0") + day());
  } else {
    lcd.print(day());
  }
  lcd.write("/");
  if (month() < 10) {
    lcd.print(String("0") + month());
  } else {
    lcd.print(month());
  }
  lcd.write("/");
  lcd.print(year());

  lcd.setCursor(9, 0);
  lcd.write("setTime");

  int selectedLocation = 0; //the time value being changed

  int value = 0; //int value of ir receiver signal

  //The LCD blinks(like a cursor) to notify the user what they're changing

  int blinking = LOW; //stores the current blink. (on/off)
  unsigned long previousMillis1 = 0; //will store last time the buzzer was updated (the number of milliseconds passed since the last specified action was done.)

  //loops until
  while (value != 765) {

    long OnTime1 = 300;           // milliseconds of on-time for blink on
    long OffTime1 = 1000;        // milliseconds of off-time for blink off

    unsigned long currentMillis = millis(); //number of milliseconds passed

    // check to see if it's time to change the state of the blinking.
    //if it's high, turn it off and refresh the screen with time values.
    if ((blinking == HIGH) && (currentMillis - previousMillis1 >= OnTime1))
    {
      blinking = LOW;
      previousMillis1 = currentMillis;  // Remember the time

      //display date and time
      lcd.setCursor(0, 0);
      if (tm.Hour < 10) {
        lcd.print(String("0") + tm.Hour);
      } else {
        lcd.print(tm.Hour);
      }

      lcd.setCursor(3, 0);
      if (tm.Minute < 10) {
        lcd.print(String("0") + tm.Minute);
      } else {
        lcd.print(tm.Minute);
      }

      lcd.setCursor(6, 0);
      if (tm.Second < 10) {
        lcd.print(String("0") + tm.Second);
      } else {
        lcd.print(tm.Second);
      }

      lcd.setCursor(0, 1);
      if (tm.Day < 10) {
        lcd.print(String("0") + tm.Day);
      } else {
        lcd.print(tm.Day);
      }

      lcd.setCursor(3, 1);
      if (tm.Month < 10) {
        lcd.print(String("0") + tm.Month);
      } else {
        lcd.print(tm.Month);
      }

      lcd.setCursor(6, 1);
      lcd.print(tm.Year + 1970);

    }
    else if ((blinking == LOW) && (currentMillis - previousMillis1 >= OffTime1)) //otherwise if the blinker is off, turn it on and cover the selected value in a black square for 300ms.
    {
      blinking = HIGH;
      previousMillis1 = currentMillis;   // Remember the time

      //blinks on the right location using selectedLocation
      if (selectedLocation == 0) {
        blinkOn(0, 1, 0);
      } else if (selectedLocation == 1) {
        blinkOn(3, 4, 0);
      } else if (selectedLocation == 2) {
        blinkOn(6, 7, 0);
      } else if (selectedLocation == 3) {
        blinkOn(0, 1, 1);
      } else if (selectedLocation == 4) {
        blinkOn(3, 4, 1);
      } else if (selectedLocation == 5) {
        blinkOn(6, 9, 1);
      }
    }

    //checking for an IR signal
    if (irrecv.decode(&results)) {
      value = results.value;
      Serial.println(value);
      irrecv.resume();
    }

    //if FUNC/STOP is selected then exit without setting the time
    if (value == -7651) {
      irrecv.resume();
      lcd.clear();
      return;
    }

    //if the fastforward button is pressed move to the next time value.
    if (value == -15811) {
      selectedLocation = selectedLocation + 1;
      if (selectedLocation > 5) { //Loop back if value of selectedLocation is too high
        selectedLocation = 0;
      }
      value = 0;
    }

    //if the fastforward button is pressed move to the previous time value.
    if (value == 8925) {
      selectedLocation = selectedLocation - 1; //Loop back if value of selectedLocation is too low
      if (selectedLocation < 0) {
        selectedLocation = 5;
      }
      value = 0;
    }

    //if the up button is pressed change the selected value by +1.
    //every variable has it's own max value, and is looped back if it gets too high.
    if (value == -28561) {

      if (selectedLocation == 0) {

        if (tm.Hour > 23) {
          tm.Hour = 0;
        } else {
          tm.Hour++;
        }
        value = 0;

        lcd.setCursor(0, 0);
        if (tm.Hour < 10) {
          lcd.print(String("0") + tm.Hour);
        } else {
          lcd.print(tm.Hour);
        }

      } else if (selectedLocation == 1) {

        if (tm.Minute > 58) {
          tm.Minute = 0;
        } else {
          tm.Minute++;
        }
        value = 0;

        lcd.setCursor(3, 0);
        if (tm.Minute < 10) {
          lcd.print(String("0") + tm.Minute);
        } else {
          lcd.print(tm.Minute);
        }

      } else if (selectedLocation == 2) {

        if (tm.Second > 59) {
          tm.Second = 0;
        } else {
          tm.Second++;
        }
        value = 0;

        lcd.setCursor(6, 0);
        if (tm.Second < 10) {
          lcd.print(String("0") + tm.Second);
        } else {
          lcd.print(tm.Second);
        }

      } else if (selectedLocation == 3) {

        if (tm.Day > 30) {
          tm.Day = 1;
        } else {
          tm.Day++;
        }
        value = 0;

        lcd.setCursor(0, 1);
        if (tm.Day < 10) {
          lcd.print(String("0") + tm.Day);
        } else {
          lcd.print(tm.Day);
        }

      } else if (selectedLocation == 4) {

        if (tm.Month > 11) {
          tm.Month = 1;
        } else {
          tm.Month++;
        }
        value = 0;

        lcd.setCursor(3, 1);
        if (tm.Month < 10) {
          lcd.print(String("0") + tm.Month);
        } else {
          lcd.print(tm.Month);
        }

      } else if (selectedLocation == 5) {

        if (tm.Year > 130) {
          tm.Year = 30;
        } else {
          tm.Year++;
        }
        value = 0;

        lcd.setCursor(6, 1);
        lcd.print(tm.Year + 1970);

      }

    }

    //if the down button is pressed change the selected value by -1.
    //every variable has it's own max value, and is looped back if it gets too low.
    if (value == -8161) {

      if (selectedLocation == 0) {

        if (tm.Hour < 1) {
          tm.Hour = 23;
        } else {
          tm.Hour--;
        }
        value = 0;

        lcd.setCursor(0, 0);
        if (tm.Hour < 10) {
          lcd.print(String("0") + tm.Hour);
        } else {
          lcd.print(tm.Hour);
        }

      } else if (selectedLocation == 1) {

        if (tm.Minute < 1) {
          tm.Minute = 59;
        } else {
          tm.Minute--;
        }
        value = 0;

        lcd.setCursor(3, 0);
        if (tm.Minute < 10) {
          lcd.print(String("0") + tm.Minute);
        } else {
          lcd.print(tm.Minute);
        }

      } else if (selectedLocation == 2) {

        if (tm.Second < 1) {
          tm.Second = 59;
        } else {
          tm.Second--;
        }
        value = 0;

        lcd.setCursor(6, 0);
        if (tm.Second < 10) {
          lcd.print(String("0") + tm.Second);
        } else {
          lcd.print(tm.Second);
        }

      } else if (selectedLocation == 3) {

        if (tm.Day < 2) {
          tm.Day = 31;
        } else {
          tm.Day--;
        }
        value = 0;

        lcd.setCursor(0, 1);
        if (tm.Day < 10) {
          lcd.print(String("0") + tm.Day);
        } else {
          lcd.print(tm.Day);
        }

      } else if (selectedLocation == 4) {

        if (tm.Month < 2) {
          tm.Month = 12;
        } else {
          tm.Month--;
        }
        value = 0;

        lcd.setCursor(3, 1);
        if (tm.Month < 10) {
          lcd.print(String("0") + tm.Month);
        } else {
          lcd.print(tm.Month);
        }

      } else if (selectedLocation == 5) {

        if (tm.Year < 30) {
          tm.Year = 50;
        } else {
          tm.Year--;
        }
        value = 0;

        lcd.setCursor(6, 1);
        lcd.print(tm.Year + 1970);

      }

    }

  }

  //if the play/pause button is pressed, then set the time.
  if (value == 765) {
    lcd.clear();
    RTC.write(tm); //gives the tmelements time to the rtc.
    setSyncProvider(RTC.get); //program takes from rtc.
  }

}

//blinking on method. For loop to do the length of that value (e.g year is 4, but month is 2 max).
void blinkOn(int startIndex, int endIndex, int row) {
  for (int i = startIndex; i <= endIndex; i++) {
    lcd.setCursor(i, row);
    lcd.write(byte(1));
  }
}


/**
   Sets the alarm
*/
void alarmSet() {
  int selectedLocation = 0; //value that needs to be changed.

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Alarm");
  lcd.setCursor(0, 1);

  //display initial/old alarm value(s).
  if (alarmHour < 10) {
    lcd.print(String("0") + alarmHour + ":");
  } else {
    lcd.print(alarmHour + ":");
  }

  lcd.setCursor(3, 1);
  if (alarmMinute < 10) {
    lcd.print(String("0") + alarmMinute);
  } else {
    lcd.print(alarmMinute);
  }

  int value = 0; //stores ir receiver signal

  int blinking = LOW; //stores the current blink. (on/off)
  unsigned long previousMillis1 = 0; //will store last time the buzzer was updated (the number of milliseconds passed since the last specified action was done.)

  // check to see if it's time to change the state of the blinking.
  //if it's high, turn it off and refresh the screen with time values.
  while (value != 765) {

    long OnTime1 = 300;           // milliseconds of on-time for blink on
    long OffTime1 = 1000;        // milliseconds of off-time for blink off

    unsigned long currentMillis = millis(); //number of milliseconds passed

    // check to see if it's time to change the state of the blinker
    if ((blinking == HIGH) && (currentMillis - previousMillis1 >= OnTime1))
    {
      blinking = LOW;
      previousMillis1 = currentMillis;  // Remember the time

      //refresh the screen when blink is off.
      lcd.setCursor(0, 1);
      if (alarmHour < 10) {
        lcd.print(String("0") + alarmHour);
      } else {
        lcd.print(alarmHour + ":");
      }

      lcd.setCursor(3, 1);
      if (alarmMinute < 10) {
        lcd.print(String("0") + alarmMinute);
      } else {
        lcd.print(alarmMinute);
      }

    }
    else if ((blinking == LOW) && (currentMillis - previousMillis1 >= OffTime1))
    {
      blinking = HIGH;
      previousMillis1 = currentMillis;   // Remember the time

      if (selectedLocation == 0) {
        blinkOn(0, 1, 1);
      } else if (selectedLocation == 1) {
        blinkOn(3, 4, 1);
      }
    }

    //check for an ir signal
    if (irrecv.decode(&results)) {
      value = results.value;
      Serial.println(value);
      irrecv.resume();
    }

    //exit method and don't set anything if func/stop is presse
    if (value == -7651) {
      irrecv.resume();
      lcd.clear();
      return;
    }

    //swap between hour and minute(fastforward button)
    if (value == -15811) {
      selectedLocation = 1 - selectedLocation;
      value = 0;
    }

    //swap between hour and minute(rewind button)
    if (value == 8925) {
      selectedLocation = 1 - selectedLocation;
      value = 0;
    }

    //increase by 1 of selected time value if up arrow is pressed.
    //will loop back if value gets out of range.
    if (value == -28561) {

      if (selectedLocation == 0) {

        if (alarmHour > 23) {
          alarmHour = 0;
        } else {
          alarmHour++;
        }
        value = 0;

        lcd.setCursor(0, 1);
        if (alarmHour < 10) {
          lcd.print(String("0") + alarmHour);
        } else {
          lcd.print(alarmHour + ":");
        }

      } else {

        if (alarmMinute > 59) {
          alarmMinute = 0;
        } else {
          alarmMinute++;
        }
        value = 0;

        lcd.setCursor(3, 1);
        if (alarmMinute < 10) {
          lcd.print(String("0") + alarmMinute);
        } else {
          lcd.print(alarmMinute);
        }

      }

    }

    //decrease by 1 of selected time value if up arrow is pressed.
    //will loop back if value gets out of range.
    if (value == -8161) {

      if (selectedLocation == 0) {

        if (alarmHour < 1) {
          alarmHour = 23;
        } else {
          alarmHour--;
        }
        value = 0;

        lcd.setCursor(0, 1);
        if (alarmHour < 10) {
          lcd.print(String("0") + alarmHour);
        } else {
          lcd.setCursor(0, 1);
          lcd.print(alarmHour + ":");
        }

      } else {

        if (alarmMinute < 1) {
          alarmMinute = 59;
        } else {
          alarmMinute--;
        }
        value = 0;

        lcd.setCursor(3, 1);
        if (alarmHour < 10) {
          lcd.print(String("0") + alarmMinute);
        } else {
          lcd.setCursor(3, 1);
          lcd.print(alarmMinute);
        }

      }

    }

  }

  //if play/pause button is pressed then set the alarm and enable it
  if (value == 765) {
    lcd.clear();
    alarmOn = true;
    RTC.setAlarm(ALM2_MATCH_HOURS, 0, alarmMinute, alarmHour, 0);
    RTC.alarmInterrupt(ALARM_2, true);
  }

}

/**
   Plays the alarm using millis()
*/
void playAlarm() {

  int value = 0; //stores int value of irremote signal.

  int thisBuzz = 0; //counter to store how many times it's beeped
  int notestate = LOW; //stores the current state of the buzzer. (on/off)
  unsigned long previousMillis1 = 0; //will store last time the buzzer was updated (the number of milliseconds passed since the last specified action was done.)

  //constantly checks until either the off button is pressed or it's been 10 minutes. It adds 1 to "thisBuzz" each pass(every second)
  while ((value != -23971) || thisBuzz < 600) {

    //Checks for a signal from an IR remote.
    if (irrecv.decode(&results)) {
      value = results.value;
      Serial.println(value);
      irrecv.resume();
    }

    //exits alarm loop if off button is pressed
    if (value == -23971) {
      irrecv.resume();
      return;
    }

    long OnTime1 = 500;  // milliseconds of on-time
    long OffTime1 = 500; // milliseconds of off-time

    unsigned long currentMillis = millis(); //number of milliseconds passed

    // check to see if it's time to change the state of the buzzer
    if ((notestate == HIGH) && (currentMillis - previousMillis1 >= OnTime1))
    {
      thisBuzz++;
      notestate = LOW;
      previousMillis1 = currentMillis;  // Remember the time
      digitalClockDisplayLCD(); //update display with right time
    }
    else if ((notestate == LOW) && (currentMillis - previousMillis1 >= OffTime1))
    {
      notestate = HIGH;
      previousMillis1 = currentMillis;   // Remember the time
      NewTone(melodyPin, NOTE_C6, 500); //play beep for 500ms
    }
  }

}

/**
   Adds a ":" and a 0 beforehand if int below 10.
   Taken from https://github.com/JChristensen/DS3232RTC

   @param number: number to be formatted(minutes or hours)
   @return formatted number
*/
String formatTime(int number)
{
  String format;
  format = ":";
  if (number < 10) {
    format = format + '0';
  }
  return format + number;
}

/**
   Converts an integer (1-12), to corresponding month in String form.

   @param month: month in integer form to be passed in
   @return month in abbreviated string form
*/
String monthToString(int month) {
  switch (month) {
    case 1:
      return "Jan";
      break;
    case 2:
      return "Feb";
      break;
    case 3:
      return "Mar";
      break;
    case 4:
      return "Apr";
      break;
    case 5:
      return "May";
      break;
    case 6:
      return "Jun";
      break;
    case 7:
      return "Jul";
      break;
    case 8:
      return "Aug";
      break;
    case 9:
      return "Sep";
      break;
    case 10:
      return "Oct";
      break;
    case 11:
      return "Nov";
      break;
    case 12:
      return "Dec";
      break;
    default:
      return "Jan";
      break;
  }
}

/**
   Sets RGB colors.
   Not my method, sourced from
   https://create.arduino.cc/projecthub/muhammad-aqib/arduino-rgb-led-tutorial-fc003e
*/
void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}
