// led.h

#ifndef _LED_h
#define _LED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum Modes { COLOUR, TWINKLE, RAINBOW, CYCLE };   // various modes of operation

class LedClass
{
 protected:
	 bool coinFlip(void);
	 byte rgb2wheel(int R, int G, int B);

 public:
	void init();
	void process();
	void fadeToColourTarget(void);
	void applyColour(uint8_t r, uint8_t g, uint8_t b);
	void music2Brightness(void);
	void Wheel(byte WheelPos, int* r, int* g, int* b);
	void rainbow(void);
	void twinkle(void);
	void setTheMode(Modes temp);
	void set_brightness(void);
	void getColourFromMemory(void);
	void saveColourToMemory(void);
	void setColour(int r, int g, int b);
	void setColourTarget(int r, int g, int b);
	void setColourTransition(void);
	void generatePulse(void);
	void connectingAnimation(void);
};

extern LedClass Led;

#endif

