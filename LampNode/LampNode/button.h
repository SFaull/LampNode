// button.h

#ifndef _BUTTON_h
#define _BUTTON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

typedef enum
{
	kBtnNone,
	kBtnShortPress,
	kBtnHeld,
	kBtnReleased
} button_t;

class ButtonClass
{
 protected:


 public:
	void init();
	void process();
	void readInputs(void);
	void remoteHold(bool held);
	button_t getPendingButton();
};

extern ButtonClass Button;

#endif

