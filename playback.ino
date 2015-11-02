#include "Interrupts.h"
#include "dingdong.h"

void setup()
{
  Serial.begin(9600);
//    screen.begin(9600); // set up serial port for 9600 baud
  pinMode(11, OUTPUT);
  pinMode(5, INPUT_PULLUP);
    pinMode(7, OUTPUT);
    pinMode(3, OUTPUT);
    
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    
    digitalWrite(3, LOW);
    digitalWrite(7, HIGH);
    
    pinMode(A0, OUTPUT);
  startPlayback(sample, sizeof(sample));
}

void loop()
{
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


