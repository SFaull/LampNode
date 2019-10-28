// network.h

#ifndef _NETWORK_h
#define _NETWORK_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class NetworkClass
{
 protected:


 public:
	void init();
	void process();
};

extern NetworkClass Network;

#endif

