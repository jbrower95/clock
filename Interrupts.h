#include <avr/pgmspace.h>
#ifndef INTERRUPT_H
	volatile int playCounter;
	void setDingDongPlaying();
	void setCuckooPlaying();
	void initializeTimers();
	#define INTERRUPT_H
#endif
