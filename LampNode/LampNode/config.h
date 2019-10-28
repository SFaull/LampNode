// config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <FastLED.h>

typedef struct
{
	bool standby;
	uint8_t brightness;
	CRGB colour;
	mode_t mode;
}config_t;

class ConfigClass
{
 protected:
	 config_t config;

 public:
	void init();
	/* add get/set functions */
};

extern ConfigClass Config;

#endif

