// 
// 
// 

#include "button.h"

unsigned long runTime = 0;
unsigned long readInputTimer = 0;
long buttonTime = 0;
long lastPushed = 0; // stores the time when button was last depressed
long lastCheck = 0;  // stores the time when last checked for a button press
// Flags
bool button_pressed = false; // true if a button press has been registered
bool button_released = false; // true if a button release has been registered
bool button_short_press = false;

void ButtonClass::init()
{
	/* Start timers */
	Timer.set(&readInputTimer);
	pinMode(BUTTON, INPUT_PULLUP);  // Enables the internal pull-up resistor
}

void ButtonClass::process()
{
	unsigned long now = millis();  // get elapsed time

/* Periodically read the inputs */
	if (Timer.isExpired(readInputTimer, INPUT_READ_TIMEOUT)) // check for button press periodically
	{
		Timer.set(&readInputTimer);  // reset timer

		readInputs();

		if (button_pressed)
		{
			//start conting
			lastPushed = now; // start the timer 
			Serial.println("Button pushed... ");
			button_pressed = false;
		}

		if (((now - lastPushed) > 1000) && button_short_press) //check the hold time
		{
			Serial.println("Button held...");
			if (!standby)
				client.publish(MQTT_COMMS, "Press");
			button_short_press = false;
		}

		if (button_released)
		{
			//get the time that button was held in
			//buttonTime = now - lastPushed;

			if (button_short_press)  // for a short press we turn the device on/off
			{
				Serial.println("Button released (short press).");
				setStandby(!standby);  // change the standby state
			}
			else  // for a long press we do the animation thing
			{
				Serial.println("Button released (long press).");
				if (!standby)  // lets only do the pulsey animation if the lamp is on in the first place
					client.publish(MQTT_COMMS, "Release");
			}
			button_released = false;
			button_short_press = false;
		}
	}
}

void readInputs(void)
{
	static bool button_state, last_button_state = false; // Remembers the current and previous button states

	button_state = digitalRead(BUTTON); // read button state (active high)

	if (button_state && !last_button_state) // on a rising edge we register a button press
	{
		button_pressed = true;
		button_short_press = true;  // initially assume its gonna be a short press
	}

	if (!button_state && last_button_state) // on a falling edge we register a button press
		button_released = true;

	last_button_state = button_state;
}


ButtonClass Button;

