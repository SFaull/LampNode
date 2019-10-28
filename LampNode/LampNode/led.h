// led.h

#ifndef _LED_h
#define _LED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class LedClass
{
 protected:


 public:
	void init();
	void process();
};

extern LedClass Led;

#endif

