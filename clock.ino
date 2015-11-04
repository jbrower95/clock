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
// These are military values for the time stored.
volatile int hour = 23;
volatile int minute = 59;
volatile int second = 55;

SoftwareSerial screen(2,4); // pin 4 = TX to screen

volatile boolean refresh = false;

/* Used to communicate over serial with the computer. */
const char __SEPARATOR = ':';
char dateInput[1024];
size_t dateCounter;
bool transmissionComplete = false; // used to communicate over serial

const char *locale_AM = "AM";
const char *locale_PM = "PM";

void setup() {
    Serial.begin(9600); // set up debug
    screen.begin(9600); // set up screen @ 9600 baud
    
    delay(500); // wait for display to boot up
    clearScreen();
    
    screen.write("Waiting for time...");
    
    while (true) {
      serialEvent();
      if (transmissionComplete) {
        clearScreen();
        dateInput[dateCounter] = 0;
        if (parseDate(dateInput, dateCounter)) {
          drawTime();
          initializeTimers();
          break;
        } else {
          clearScreen();
          screen.write("Whoops! Incorrect format.");
        }
        transmissionComplete = false;
      }
    }
}

void loop() {
  if (refresh) {
   drawTime();
   refresh = false;
  }
}

/* Reads from the Serial connection.
 *
 * Stores the resulting string in dateInput[].
 */
void serialEvent() {
  while (Serial.available() && dateCounter < 1024) {
    char inChar = (char)Serial.read();
    dateInput[dateCounter++] += inChar;
    if (inChar == '\n') {
      transmissionComplete = true;
    }
  }
}




/* Parses an inputted date string.
 * 
 * input: The string to parse. 
 *    e.g "12:15:15"
 * length: The size of the inputted string.
 *    e.g: 9
 */
bool parseDate(char *input, size_t length) {
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
    if (input[i] == __SEPARATOR || input[i] == '\n') {
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

              
