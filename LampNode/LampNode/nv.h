// nv.h

#ifndef _NV_h
#define _NV_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class NvClass
{
 protected:


 public:
	void init();
	void writeEEPROM(int address, int val);
	int readEEPROM(int address);
};

extern NvClass Nv;

#endif

