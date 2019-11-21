// led.h

#ifndef _LED_h
#define _LED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

typedef enum  { 
	COLOUR = 0, 
	TWINKLE, 
	RAINBOW, 
	CYCLE, 
	UNKNOWN 
} lamp_mode_t;   // various modes of operation

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
	void setTheMode(lamp_mode_t temp);
	void set_brightness(void);
	void getColourFromMemory(void);
	//void saveColourToMemory(void);
	void setColour(int r, int g, int b);
	void setColourTarget(int r, int g, int b);
	void startColourTransition(void);
	void generatePulse(void);
	void connectingAnimation(void);
	void stripe(void);
};

extern LedClass Led;

#endif

