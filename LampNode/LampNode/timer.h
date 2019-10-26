// timer.h

#ifndef _TIMER_h
#define _TIMER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class TimerClass
{
 protected:


 public:
	 void set(unsigned long* startTime);
	 bool isExpired(unsigned long startTime, unsigned long expiryTime);
	 unsigned long elapsed(unsigned long startTime);
};

extern TimerClass Timer;

#endif

