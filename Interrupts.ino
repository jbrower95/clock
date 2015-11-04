/*
 * Uses PWM to play 8-bit (unsigned) PCM audio (DingDong and Cuckoo) while as needed counting seconds.
 * Uses two timers to do this: Timer1 samples/counts to a second at 8Hz, Timer2 sends HIGH to the speaker pin 
 * at the frequency for the sampled note (using PWM).
 */

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "Interrupts.h"
#include "clock.h"

#define SAMPLE_RATE 8000

int speakerPin = 11;
unsigned const char *melody_data=0; // The data of the melody to be played
int melody_data_length=0; // The length of the melody data
volatile uint16_t sample;
byte lastSample;

int counter = 0;
int maxCount = 7999;


/* Counts to a second; if a second has been reached, call tick() to increment the clock.
 * If a sound should be playing, sample the appropriate sound.
 */
ISR(TIMER1_COMPA_vect) {
  // Check for second
  if (counter >= maxCount) {
    tick();
    counter = 0;
  } else {
    counter++;
  }

  // If nothing to be played, return here
  if (!playCounter) {
    return;
  }

  // If no more to sample, decrement playback
  if (sample >= melody_data_length) {
      decrementPlayback();
  }
  else {
    OCR2A = pgm_read_byte(&melody_data[sample]);
  }

  sample++;
}

/*
 * Sets the melody to "cuckoo"
 */
void setCuckooPlaying() {
  melody_data_length = cuckoo_data_length;
  melody_data = cuckoo_data;
}

/*
 * Sets the melody to "ding_dong"
 */
void setDingDongPlaying() {
  melody_data_length = ding_dong_length;
  melody_data = ding_dong_data;
}


void initializeTimers() {
  pinMode(speakerPin, OUTPUT);
  
  // Set up Timer 2 to do pulse width modulation on the speaker
  // pin.
  
  // Use internal clock (datasheet p.160)
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));
  
  // Set fast PWM mode  (p.157)
  TCCR2A |= _BV(WGM21) | _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);
  
  // Do non-inverting PWM on pin OC2A (p.155)
  // On the Arduino this is pin 11.
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
  
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  
  // Set initial pulse width to the first sample.
  OCR2A = pgm_read_byte(&sounddata_data[0]);
  
  // Set up Timer 1 to send a sample every interrupt.
  cli();
  
  // Set CTC mode (Clear Timer on Compare Match) (p.133)
  // Have to set OCR1A *after*, otherwise it gets reset to 0!
  TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
  TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));
  
  // No prescaler (p.134)
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  
  // Set the compare register (OCR1A).
  // OCR1A is a 16-bit register, so we have to do this with
  // interrupts disabled to be safe.
  OCR1A = F_CPU / SAMPLE_RATE;    // 16e6 / 8000 = 2000
  
  // Enable interrupt when TCNT1 == OCR1A (p.136)
  TIMSK1 |= _BV(OCIE1A);
  
  lastSample = pgm_read_byte(&sounddata_data[sounddata_length-1]);
  sample = 0;
  sei();
}

inline void decrementPlayback() {
  sample = -1;
  playCounter--;
}
