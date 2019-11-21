// 
// 
// 

#include "led.h"
#include "timer.h"
#include "userconfig.h"
#include <FastLED.h>
#include "config.h"

#define LED_COUNT 60


lamp_mode_t Mode = UNKNOWN;

unsigned long ledTimer = 0;
unsigned long brightnessTimer = 0;
unsigned long twinkleTimer = 0;
unsigned long rainbowTimer = 0;
unsigned long cycleTimer = 0;
int cnt = 0;

unsigned int target_colour[3] = { 0,0,0 }; // rgb value that LEDs are currently set to
unsigned int current_colour[3] = { 0,0,0 };  // rgb value which we aim to set the LEDs to
unsigned int transition[50][3];
unsigned int pulse[30][3];

bool target_met = false;
bool pulse_animation = false;
int pulse_addr = 0;
int brightness = 155;
bool standby = false;

// Define the array of leds
CRGB leds[LED_COUNT];

uint8_t plane_map[8][8] = 
{
	{0, 1, 2, 3, 4, 5, 6, 7},
	{8, 9, 10, 11, 12, 13, 14, 15},
	{16, 17, 18, 19, 20, 21, 22, 23},
	{24, 25, 26, 27, 28, 29, 30, 31},
	{32, 33, 34, 35, 36, 37, 38, 39},
	{40, 41, 42, 43, 44, 45, 46, 47},
	{48, 49, 50, 51, 52, 53, 54, 55},
	{56, 57, 58, 59, 60, 61, 62, 63}
};


void LedClass::init()
{
	/* Set LED state */
	FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, LED_COUNT);

	/* init timers */
	Timer.set(&ledTimer);
	Timer.set(&brightnessTimer);
	Timer.set(&twinkleTimer);
	Timer.set(&rainbowTimer);
	Timer.set(&cycleTimer);

	for (int i = 0; i < LED_COUNT; i++)
		leds[i] = CRGB(0, 0, 0);
	FastLED.show();
}


void LedClass::process()
{
	/* fetch config */
	config_data_t conf = Config.getReference()->configuration;

	brightness = conf.brightness;

	/* if the colour has changed, run the process*/
	if ((target_colour[0] != conf.colour.red) || (target_colour[1] != conf.colour.green) || (target_colour[2] != conf.colour.blue))
	{
		target_colour[0] = conf.colour.red;
		target_colour[1] = conf.colour.green;
		target_colour[2] = conf.colour.blue;
		startColourTransition();
	}

	if (Mode != conf.mode)
	{
		/* do something clever on a change of state like fade to black */
		Mode = (lamp_mode_t)conf.mode;
		if (Mode == COLOUR)
			startColourTransition();
	}

	if (!standby && conf.standby)
	{
		if (Mode == COLOUR)
			startColourTransition();
	}

	standby = conf.standby;

	//Config.save();

	if (!standby)
	{
		/* Periodically update the brightness */
		if (Timer.isExpired(brightnessTimer, BRIGHTNESS_UPDATE_TIMEOUT))
		{
			Timer.set(&brightnessTimer); // reset timer
			set_brightness();
		}

		switch (Mode)
		{
			case COLOUR:
				/* Periodically update the LEDs */
				if (Timer.isExpired(ledTimer, LED_UPDATE_TIMEOUT))
				{
					Timer.set(&ledTimer); // reset timer
					fadeToColourTarget();
				}

				break;

			case TWINKLE:
				if (Timer.isExpired(twinkleTimer, TWINKLE_UPDATE_TIMEOUT))
				{
					Timer.set(&twinkleTimer); // reset timer
					twinkle();
				}
				break;

			case RAINBOW:
				if (Timer.isExpired(rainbowTimer, RAINBOW_UPDATE_TIMEOUT))
				{
					Timer.set(&rainbowTimer); // reset timer
					rainbow();
				}
				break;

			case CYCLE:
				if (Timer.isExpired(cycleTimer, CYCLE_UPDATE_TIMEOUT))
				{
					Timer.set(&cycleTimer); // reset timer
					int r, g, b;
					Wheel(cnt++, &r, &g, &b);
					setColour(r, g, b);
					if (cnt >= 256)
						cnt = 0;
				}
				break;

			default:
				if (Timer.isExpired(twinkleTimer, TWINKLE_UPDATE_TIMEOUT))
				{
					Timer.set(&twinkleTimer); // reset timer
					stripe();
				}
				break;
		}
	}
	else
	{
		// do nothing if off
		applyColour(0,0,0);
	}
}


void LedClass::fadeToColourTarget(void)
{
	static int addr = 0;

	if (!target_met)
	{
		setColour(transition[addr][0], transition[addr][1], transition[addr][2]);
		addr++;

		if (addr >= 50)
		{
			target_met = true;
			addr = 0;
		}
	}
}

void LedClass::applyColour(uint8_t r, uint8_t g, uint8_t b)
{
	if (r < 256 && g < 256 && b < 256)
	{
		for (uint8_t i = 0; i < LED_COUNT; i++)
		{
			leds[i] = CRGB(r, g, b);;
		}
		FastLED.show();
#if(0)
		Serial.print("Whole strip set to ");
		Serial.print(r);
		Serial.print(",");
		Serial.print(g);
		Serial.print(",");
		Serial.println(b);
#endif
	}
	else
		Serial.println("Invalid RGB value, colour not set");
}

void LedClass::music2Brightness(void)
{
#if(0)
	static int ADCval, lastADCval, brightness = 0;
	ADCval = analogRead(A0);
	if (ADCval != lastADCval)
	{
		Serial.print("ADC: ");
		Serial.println(ADCval);
		brightness = map(ADCval, 0, 1023, 0, 255);
		leds.SetBrightness(brightness);
		lastADCval = ADCval;
	}
#endif
}



// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void LedClass::Wheel(byte WheelPos, int* r, int* g, int* b)
{
	WheelPos = 255 - WheelPos;
	if (WheelPos < 85)
	{
		*r = 255 - WheelPos * 3;
		*g = 0;
		*b = WheelPos * 3;
	}
	else if (WheelPos < 170)
	{
		WheelPos -= 85;
		*r = 0;
		*g = WheelPos * 3;
		*b = 255 - WheelPos * 3;
	}
	else
	{
		WheelPos -= 170;
		*r = WheelPos * 3;
		*g = 255 - WheelPos * 3;
		*b = 0;
	}
}

void LedClass::rainbow(void)
{
	// here we need to cycle through each led, assigning consectuve colours pulled from the Wheel function. Each time this is called all colours should shift one
	static int offset = 0;
	static int stepVal = 256 / LED_COUNT;  // note the 256 value can be reduced to show less of the colour spectrum at once.
	int red, green, blue;

	for (int i = 0; i < LED_COUNT; i++)
	{
		Wheel(i * stepVal + offset, &red, &green, &blue); // get our colour
		leds[i] = CRGB(red, green, blue);
	}
	FastLED.show();

	if (offset >= 255)
		offset = 0;
	else
		offset++;
}

bool LedClass::coinFlip(void)
{
	int coin = random(2) - 1;
	if (coin)
		return true;
	else
		return false;
}

void LedClass::twinkle(void)
{
	// here we need to cycle through each led, assigning consectuve colours pulled from the Wheel function. Each time this is called all colours should shift one
	int red, green, blue;
	int offset = random(30) - 15;
	int pix = random(60);
	int state = coinFlip();
	int val = rgb2wheel(target_colour[0], target_colour[1], target_colour[2]);
	Wheel(val + offset, &red, &green, &blue); // get our colour

	if (state)
		leds[pix] = CRGB(red, green, blue);
	else
		leds[pix] = CRGB(0, 0, 0);

	FastLED.show();
}


void LedClass::stripe(void)
{
	static uint8_t col = 0;

	// turn all off
	for (int i = 0; i < LED_COUNT; i++)
		leds[i] = CRGB(0, 0, 0);


	// here we need to cycle through each led, assigning consectuve colours pulled from the Wheel function. Each time this is called all colours should shift one
	for (int i = 0; i < 7; i++)
	{
		uint8_t index = plane_map[i][col];
			leds[index] = CRGB(50, 50, 50);
	}

	col++;

	if (col >= 7) col = 0;

	FastLED.show();
}

void LedClass::setTheMode(lamp_mode_t temp)
{
	Serial.print("mode set to: ");
	Serial.println(temp);
	switch (temp)
	{
	case COLOUR:
		if (!standby)
			setColourTarget(target_colour[0], target_colour[1], target_colour[2]);
		break;

	case TWINKLE:
		setColour(0, 0, 0);
		break;

	case RAINBOW:
		setColour(0, 0, 0);
		break;

	case CYCLE:
		setColour(0, 0, 0);
		break;

	default:
		// should never get here
		break;
	}

	Mode = temp;
	//writeEEPROM(MEM_MODE, Mode);
}


// doesnt work very well
byte LedClass::rgb2wheel(int R, int G, int B)
{
	return  (B & 0xE0) | ((G & 0xE0) >> 3) | (R >> 6);
}

void LedClass::set_brightness(void)
{
	static int last_brightness = 153;
	static int brightness_pulse = 153;
	static float coefficient = 1.0;
	static bool pulse_direction = 0;

	if (pulse_animation)
	{
		brightness_pulse = brightness * coefficient;

		Serial.print("pulse animation: ");
		Serial.println(brightness_pulse);

		FastLED.setBrightness(brightness_pulse);

		if (coefficient >= 1.0)
			pulse_direction = 0;
		if (coefficient <= 0.5)
			pulse_direction = 1;

		if (pulse_direction)
			coefficient += 0.025;
		else
			coefficient -= 0.025;

		if (Mode == COLOUR)
		{
			FastLED.show();
		}
	}
	else
	{
		if (coefficient < 1.0)
		{
			coefficient += 0.025;
			brightness_pulse = brightness * coefficient;
			FastLED.setBrightness(brightness_pulse);
		}
		else
		{
			if (last_brightness != brightness)
			{
				FastLED.setBrightness(brightness);
				if (Mode == COLOUR)
				{
					FastLED.show();
				}
				last_brightness = brightness;
			}
		}
	}
}


/* gets the last saved RGB value from the eeprom and stores it in target_colour */
void LedClass::getColourFromMemory(void)
{
	/*
	for (int addr = MEM_RED; addr <= MEM_BLUE; addr++)
	{
		target_colour[addr] = readEEPROM(addr);

		Serial.print("EEPROM read: ");
		Serial.print("[");
		Serial.print(addr);
		Serial.print("] ");
		Serial.println(target_colour[addr]);
	}
	*/
}

/* stores the last RGB value from target_colour in the eeprom */

/*
void LedClass::saveColourToMemory(void)
{
	Serial.println("Saving RGB value");
	for (int addr = MEM_RED; addr <= MEM_BLUE; addr++)
	{
		writeEEPROM(addr, target_colour[addr]);

		Serial.print("EEPROM write: ");
		Serial.print("[");
		Serial.print(addr);
		Serial.print("] ");
		Serial.println(target_colour[addr]);
	}
}
*/

void LedClass::setColour(int r, int g, int b)
{
	current_colour[0] = r;
	current_colour[1] = g;
	current_colour[2] = b;
	if (!standby)
		applyColour(current_colour[0], current_colour[1], current_colour[2]);
}

void LedClass::setColourTarget(int r, int g, int b)
{
	target_met = false;

	target_colour[0] = r;
	target_colour[1] = g;
	target_colour[2] = b;

	//saveColourToMemory();
	startColourTransition();
}

void LedClass::startColourTransition(void)
{
	target_met = false;

	for (int addr = 0; addr < 50; addr++)  // for each element in the array
	{
		for (int i = 0; i < 3; i++)  // for each colour in turn
		{
			transition[addr][i] = map(addr, 0, 49, current_colour[i], target_colour[i]); // compute the proportional colour value
		}
		/*
		Serial.print(transition[addr][0]);
		Serial.print(",");
		Serial.print(transition[addr][1]);
		Serial.print(",");
		Serial.println(transition[addr][2]);
		*/
	}
}

void LedClass::generatePulse(void)
{
	for (int addr = 0; addr < 30; addr++)  // for each element in the array
	{
		for (int i = 0; i < 3; i++)  // for each colour in turn
		{
			pulse[addr][i] = map(addr, 0, 29, current_colour[i], current_colour[i] / 5); // compute the proportional colour value
		}
		/*
		Serial.print(pulse[addr][0]);
		Serial.print(",");
		Serial.print(pulse[addr][1]);
		Serial.print(",");
		Serial.println(pulse[addr][2]);
		*/
	}
}
/*
void pulseEffect(void)
{
  setColour(pulse[pulse_addr][0],pulse[pulse_addr][1],pulse[pulse_addr][2]);

  if (pulse_addr>=29)
	pulse_direction = 0;
  if (pulse_addr<=0)
	pulse_direction = 1;

  if (pulse_direction)
	pulse_addr++;
  else
	pulse_addr--;
}
*/
void LedClass::connectingAnimation(void)
{
	static int count = -4;

	for (int i = 0; i < 5; i++)
		leds[count + i] = CRGB(10, 10, 10);
	leds[count - 1] = CRGB(0, 0, 0);
	//strip.SetBrightness(brightness);
	FastLED.show();
	if (count > 60)
		count = 0;
	else
		count++;
	delay(30);
}

LedClass Led;

