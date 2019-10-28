// 
// 
// 

#include "led.h"
#include "timer.h"
#include <NeoPixelBrightnessBus.h> // instead of NeoPixelBus.h

NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, WS2812_PIN);

RgbColor genericColour(0, 255, 0);

enum Modes { COLOUR, TWINKLE, RAINBOW, CYCLE };   // various modes of operation
enum Modes Mode;
void setTheMode(Modes temp);

unsigned long ledTimer = 0;
unsigned long brightnessTimer = 0;
unsigned long twinkleTimer = 0;
unsigned long rainbowTimer = 0;
unsigned long cycleTimer = 0;

const uint16_t PixelCount = 60; // this example assumes 3 pixels, making it smaller will cause a failure


unsigned int target_colour[3] = { 0,0,0 }; // rgb value that LEDs are currently set to
unsigned int current_colour[3] = { 0,0,0 };  // rgb value which we aim to set the LEDs to
unsigned int transition[50][3];
unsigned int pulse[30][3];

bool target_met = false;
bool pulse_animation = false;
int pulse_addr = 0;
int brightness = 155;


void LedClass::init()
{
	/* Set LED state */
	strip.Begin();
	strip.Show();

	/* init timers */
	Timer.set(&ledTimer);
	Timer.set(&brightnessTimer);
	Timer.set(&twinkleTimer);
	Timer.set(&rainbowTimer);
	Timer.set(&cycleTimer);
}


void LedClass::process()
{
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
			// unknown state - do nothing
			break;
		}
	}
	else
	{
		// do nothing if off
		//applyColour(0,0,0);
	}
}

void fadeToColourTarget(void)
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

void applyColour(uint8_t r, uint8_t g, uint8_t b)
{
	if (r < 256 && g < 256 && b < 256)
	{
		RgbColor colour(g, r, b);
		for (uint8_t i = 0; i < PixelCount; i++)
		{
			strip.SetPixelColor(i, colour);
		}
		strip.Show();
		Serial.print("Whole strip set to ");
		Serial.print(r);
		Serial.print(",");
		Serial.print(g);
		Serial.print(",");
		Serial.println(b);
	}
	else
		Serial.println("Invalid RGB value, colour not set");
}

void music2Brightness(void)
{
	static int ADCval, lastADCval, brightness = 0;
	ADCval = analogRead(A0);
	if (ADCval != lastADCval)
	{
		Serial.print("ADC: ");
		Serial.println(ADCval);
		brightness = map(ADCval, 0, 1023, 0, 255);
		strip.SetBrightness(brightness);
		lastADCval = ADCval;
	}
}



// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void Wheel(byte WheelPos, int* r, int* g, int* b)
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

void rainbow(void)
{
	// here we need to cycle through each led, assigning consectuve colours pulled from the Wheel function. Each time this is called all colours should shift one
	static int offset = 0;
	static int stepVal = 256 / PixelCount;  // note the 256 value can be reduced to show less of the colour spectrum at once.
	int red, green, blue;

	for (int i = 0; i < PixelCount; i++)
	{
		Wheel(i * stepVal + offset, &red, &green, &blue); // get our colour
		RgbColor colour(green, red, blue);
		strip.SetPixelColor(i, colour);
	}
	strip.Show();

	if (offset >= 255)
		offset = 0;
	else
		offset++;
}

bool coinFlip(void)
{
	int coin = random(2) - 1;
	if (coin)
		return true;
	else
		return false;
}

void twinkle(void)
{
	// here we need to cycle through each led, assigning consectuve colours pulled from the Wheel function. Each time this is called all colours should shift one
	int red, green, blue;
	int offset = random(30) - 15;
	int pix = random(60);
	int state = coinFlip();
	int val = rgb2wheel(target_colour[0], target_colour[1], target_colour[2]);
	Wheel(val + offset, &red, &green, &blue); // get our colour
	RgbColor colour(green, red, blue);
	RgbColor off(0, 0, 0);

	if (state)
		strip.SetPixelColor(pix, colour);
	else
		strip.SetPixelColor(pix, off);

	strip.Show();
}


void setTheMode(Modes temp)
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
	writeEEPROM(MEM_MODE, Mode);
}


// doesnt work very well
byte rgb2wheel(int R, int G, int B)
{
	return  (B & 0xE0) | ((G & 0xE0) >> 3) | (R >> 6);
}

void set_brightness(void)
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

		strip.SetBrightness(brightness_pulse);

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
			strip.Show();
		}
	}
	else
	{
		if (coefficient < 1.0)
		{
			coefficient += 0.025;
			brightness_pulse = brightness * coefficient;
			strip.SetBrightness(brightness_pulse);
		}
		else
		{
			if (last_brightness != brightness)
			{
				strip.SetBrightness(brightness);
				if (Mode == COLOUR)
				{
					strip.Show();
				}
				last_brightness = brightness;
			}
		}
	}
}


/* gets the last saved RGB value from the eeprom and stores it in target_colour */
void getColourFromMemory(void)
{
	for (int addr = MEM_RED; addr <= MEM_BLUE; addr++)
	{
		target_colour[addr] = readEEPROM(addr);

		Serial.print("EEPROM read: ");
		Serial.print("[");
		Serial.print(addr);
		Serial.print("] ");
		Serial.println(target_colour[addr]);
	}
}

/* stores the last RGB value from target_colour in the eeprom */
void saveColourToMemory(void)
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

void setColour(int r, int g, int b)
{
	current_colour[0] = r;
	current_colour[1] = g;
	current_colour[2] = b;
	if (!standby)
		applyColour(current_colour[0], current_colour[1], current_colour[2]);
}

void setColourTarget(int r, int g, int b)
{
	target_met = false;

	target_colour[0] = r;
	target_colour[1] = g;
	target_colour[2] = b;

	saveColourToMemory();
	setColourTransition();
}

void setColourTransition(void)
{
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

void generatePulse(void)
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
void connectingAnimation(void)
{
	static int count = -4;
	RgbColor colour(10, 10, 10);
	RgbColor off(0, 0, 0);
	for (int i = 0; i < 5; i++)
		strip.SetPixelColor(count + i, colour);
	strip.SetPixelColor(count - 1, off);
	//strip.SetBrightness(brightness);
	strip.Show();
	if (count > 60)
		count = 0;
	else
		count++;
	delay(30);
}

LedClass Led;

