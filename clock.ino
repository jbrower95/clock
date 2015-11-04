#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "cuckoo.h"
#include "ding_dong.h"
#include "Interrupts.h"
#include <string.h>

#define SAMPLE_RATE 8000


// The actual clock, initialized to 0:00:00
volatile int hour = 23;
volatile int minute = 59;
volatile int second = 55;


SoftwareSerial screen(2,4); // pin 4 = TX to screen

volatile boolean refresh = false;

char dateInput[1024];
int dateCounter;

bool transmissionComplete = false;

const char *locale_AM = "AM";
const char *locale_PM = "PM";

void setup() {
  
    Serial.begin(9600);
    screen.begin(9600); // set up serial port for 9600 baud
    
    delay(500); // wait for display to boot up
    clearScreen();
    
    pinMode(5, INPUT_PULLUP);
    pinMode(7, OUTPUT);
    pinMode(3, OUTPUT);
    
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    
    digitalWrite(3, LOW);
    digitalWrite(7, HIGH);
    
    pinMode(A0, OUTPUT);

    pinMode(11, OUTPUT);
    screen.write("Waiting for time...");
    while (true) {
      serialEvent();
      if (transmissionComplete) {
        clearScreen();
        dateInput[dateCounter] = 0;
        if (parseDate(dateInput, dateCounter)) {
          Serial.print("Hour: ");
          Serial.println(hour);
          Serial.print("Minute: ");
          Serial.println(minute);
          Serial.print("Second: ");
          Serial.println(second);
          startPlayback(ding_dong_data, sizeof(ding_dong_data));
          return;
        } else {
          clearScreen();
          screen.write("Whoops! Incorrect format.");
        }
        transmissionComplete = false;
      }
    }
}

void serialEvent() {
  while (Serial.available() && dateCounter < 1024) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    dateInput[dateCounter++] += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      transmissionComplete = true;
    }
  }
}


void loop() {
  if (refresh) {
   drawTime();
   refresh = false;
  }
}

bool parseDate(char *input, int length) {
  if (input == NULL || length == 0) {
     return false;
  }

  // null terminate the input string, just in case.
  input[length] = 0;

  volatile int *parts[] = {&hour, &minute, &second};

  int accumulator = 0;
  int part = 0;
  int assigned = 0;
  
  for (int i = 0; i < length; i++) { 
    if (input[i] == ':' || input[i] == '\n') {
      // disperse accumulator
      volatile int *current = parts[part++];

      if (part > 3 || (part == 0 && accumulator > 23) || (part != 0 && accumulator > 59)) {
        Serial.println("Returning with FALSE");
        Serial.println(part);
        Serial.print("Accumulator: ");
        Serial.println(accumulator);
        return false;
      }

      *current = accumulator;
      assigned++;
      accumulator = 0;
    } else {
      // check if it's a number
      int num = (int)input[i];
      if (!isdigit(num)) {
        return false;
      }

      accumulator = accumulator * 10;
      accumulator += num - 48;
    }
  }

  return (assigned == 3);
}

/* Clears the attached LCD display. */
void clearScreen() {
  screen.write(254); // move cursor to beginning of first line
  screen.write(128); 
  screen.write("                "); // clear display by writing all spaces
  screen.write("                ");
  screen.write(254); 
  screen.write(128); 
}

/* Draws the time to the attached LCD display. */
void drawTime() {
  clearScreen();
  char timeBuffer[30];
  const char *locale = (hour >= 12) ? locale_PM : locale_AM;
  int hour_display = (hour == 12 ? 12 : hour % 12);
  sprintf(timeBuffer, "%02d:%02d:%02d %s", hour_display, minute, second, locale);
  screen.write(timeBuffer);
}

/* Ticks the clock. This advances the clock by 1s. */
void tick() {
 second++;
 
 minute += second / 60;
 second = second % 60;

 hour += minute / 60;
 minute = minute % 60;

 hour = hour % 24;

 refresh = true;

 if (second == 0 && minute == 0) {
  // play 'cuckoo' hour times
  setCuckooPlaying();
  playCounter = hour; 
 } else if (second == 0 && minute == 30) {
  // ding dong
  setDingDongPlaying();
  playCounter = 1;
 }

}

              
