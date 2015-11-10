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
int maxCount = 7680;


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
    // Set speaker pin (OCR2A) to do PWM on the current sample
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

/*
 * Initializes Timer2 to do PWM on speaker pin and
 * Timer1 to CTC mode to sample at 8kHz.
 * Datasheet: http://www.atmel.com/images/atmel-8271-8-bit-avr-microcontroller-atmega48a-48pa-88a-88pa-168a-168pa-328-328p_datasheet_complete.pdf
 */
void initializeTimers() {
  pinMode(speakerPin, OUTPUT);

  // KEY: _BV(bit) = (1 << bit)
  
  // Set up Timer 2 to do PWM on the speaker pin to actually play the notes in the melody

  // Setting up to use internal clock (need to clear (0) the AS2 and EXCLK bits on ASSR)
  // --Datasheet pg 158-159
  ASSR &= ~(_BV(EXCLK) | _BV(AS2)); // This clears both AS2 and EXCLK
  // ASSR &= ~(_BV(AS2)); // This clears AS2 (may not be necessary to clear EXCLK)
  
  // Fast PWM when TCCR2A's WGM21 and WGM20 bits are 1
  // and when TCCR2B's WGM22 bit is 0
  // -- Datasheet pg 155
  TCCR2A |= _BV(WGM21) ;
  TCCR2A |= _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);
  
  // For non-inverting PWM on OC2A (the UNO's digital pin 11), 
  // set TCCR2A's COM2A1 bit (1) and clear TCCR2A's COM2A0 bit (0).
  // Clear OC2B (digital pin 3) of PWM for normal port operation by
  // clearing TCCR2A's COM2B0 and COM2B1 bits (0).
  // -- Datasheet pg 153-154 for setting non-inverting mode on OC2A and clearing OC2B
  TCCR2A |= _BV(COM2A1);
  TCCR2A &= ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0)); // May not be necessary
  
  // Set TCCR2B to have no prescaler by clearing its CS22 and CS21 bits (0)
  // and setting its CS20 bit (1)
  // --Datasheet pg 156
  TCCR2B &= ~(_BV(CS22) | _BV(CS21));
  TCCR2B |= _BV(CS20);
  
  // Set up Timer 1 to interrupt at a constant rate, the lowest common denominator
  // of the sample rate and a realistic tick rate.
  // Since OCR1A is 16-bit, we need to stop itnerrupts (Datasheet pg 113)
  cli();
  
  // TCCR1A's WGM11 and WM10 bits and TCCR1B's WGM13 and WGM12 bits
  // control the counting sequence of the counter, the source for max counter value, 
  // and what type of waveform generation to use. 
  // We want Clear Timer on Compare Match (CTC) mode on OCR1A.
  // TCCR1A: clear WGM10 and WGM11
  // TCCR1B: clear WGM13, set WGM12
  // --Datasheet pg 132
  TCCR1A &= ~(_BV(WGM10) | _BV(WGM11));
  TCCR1B &= ~(_BV(WGM13));
  TCCR1B |= _BV(WGM12);

  // No prescaler for TCCR1B
  // Clear CS11 and CS12, set CS10
  // --Datasheet pg 134
  TCCR1B |= _BV(CS10);
  TCCR1B &= ~(_BV(CS11) | _BV(CS12));
  
  // Set OCR1A (compare register) to the lowest common denominator frequency,
  // based on SAMPLE_RATE (8000)
  OCR1A = F_CPU / SAMPLE_RATE;    // 16e6 / 8000 = 2000
  
  // When the timer counter (TCNT1) == OCR1A, the OCF1A flag in TIFR1A is set (1).
  // In TIMSK1, set OCIE1A so that the corresponding interrupt (TIMER1 COMPA) is executed
  // when the OCF1A flag is set.
  // --Datasheet pg 136
  // --Datasheet pg 57 for corresponding interrupt
  TIMSK1 |= _BV(OCIE1A);

  // Set initial sample to 0
  sample = 0;

  // Re-enable interrupts
  sei();
}

/*
 * Resets the sample index and decrements the number of times
 * the melody needs to be played.
 */
inline void decrementPlayback() {
  sample = -1;
  playCounter--;
}
