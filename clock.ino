#include <SoftwareSerial.h>
// The actual clock, initialized to 0:00:00
volatile int hour = 20;
volatile int minute = 49;
volatile int second = 0;

SoftwareSerial screen(3,4); // pin 4 = TX to screen

volatile boolean refresh = false;
volatile boolean buttonPressed = false;

char dateInput[1024];
int dateCounter;
boolean transmissionComplete = false;

void setup() {
  
    Serial.begin(9600);
    screen.begin(9600); // set up serial port for 9600 baud
    
    delay(500); // wait for display to boot up
    clearScreen();
    initializeClock();
    
    pinMode(5, INPUT_PULLUP);
    pinMode(7, OUTPUT);
    pinMode(3, OUTPUT);
    
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    
    digitalWrite(3, LOW);
    digitalWrite(7, HIGH);
    
    pinMode(A0, OUTPUT);
}

void initializeClock() {
    cli();          // disable global interrupts
    TCCR1A = 0;     // set entire TCCR1A register to 0
    TCCR1B = 0;     // same for TCCR1B
    // set compare match register to desired timer count:
    OCR1A = 15624;
    // turn on CTC mode:
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler:
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt:
    TIMSK1 |= (1 << OCIE1A);
    // enable global interrupts:
    sei();    
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
  
  serialEvent();
  if (transmissionComplete) {
    
    
    dateCounter = 0;
    transmissionComplete = false;
  }
  
}

ISR(TIMER1_COMPA_vect)
{
    tick();
    refresh = true;
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
  char *locale;
  if (hour >= 12) {
   locale = "PM"; 
  } else {
   locale = "AM"; 
  }
  sprintf(timeBuffer, "%02d:%02d:%02d %s", hour % 12, minute, second, locale);
  screen.write(timeBuffer);
}

/* Ticks the clock. This advances the clock by 1s. */
void tick() {
 second++;
 
 if (second >= 60) {
  minute++;
  second = 0; 
 }
 
 if (minute >= 60) {
  hour++;
  minute = 0; 
 }
 
 if (hour >= 24) {
  hour = 0; 
 }
}

              
// Plays the ding-dong sound every halfhour
void playDingDong() {
  // TODO: play ding dong
}

void playCuckoo() {
  // TODO: play cuckoo
}

