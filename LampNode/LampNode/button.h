// button.h

#ifndef _BUTTON_h
#define _BUTTON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class ButtonClass
{
 protected:


 public:
	void init();
	void process();
	void readInputs(void);
};

extern ButtonClass Button;

#endif

