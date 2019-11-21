// 
// 
// 

#include "button.h"
#include "timer.h"
#include "userconfig.h"
#include "config.h"

unsigned long buttonPushed = 0;

// Flags
bool button_pressed = false; // true if a button press has been registered
bool button_released = false; // true if a button release has been registered
bool button_short_press = false;
bool remote_hold = false;

button_t buttonState;

void ButtonClass::init()
{
	/* Start timers */
	pinMode(BUTTON, INPUT_PULLUP);  // Enables the internal pull-up resistor
	buttonState = kBtnNone;
}

void ButtonClass::remoteHold(bool held)
{
	remote_hold = held;
	if (!held)
		buttonState = kBtnReleased;
}

button_t ButtonClass::getPendingButton()
{
	if (remote_hold)
		return kBtnHeld;

	button_t state = buttonState;
	buttonState = kBtnNone;
	return state;
}

void ButtonClass::process()
{
	readInputs();

	// if rising edge
	if (button_pressed)
	{
		Timer.set(&buttonPushed);  // reset timer
		Serial.println("Button pushed... ");
		button_pressed = false;
	}


	if (Timer.isExpired(buttonPushed, 1000) && button_short_press) //check the hold time
	{
		buttonState = kBtnHeld;
		Serial.println("Button held...");
		//if (!standby)
		//	client.publish(MQTT_COMMS, "Press");
		button_short_press = false;
	}

	if (button_released)
	{
		//get the time that button was held in

		if (button_short_press)  // for a short press we turn the device on/off
		{
			buttonState = kBtnShortPress;
			Serial.println("Button released (short press).");
			//setStandby(!standby);  // change the standby state
		}
		else  // for a long press we do the animation thing
		{
			buttonState = kBtnReleased;
			Serial.println("Button released (long press).");
			//if (!standby)  // lets only do the pulsey animation if the lamp is on in the first place
			//	client.publish(MQTT_COMMS, "Release");
		}
		button_released = false;
		button_short_press = false;
	}
}

void ButtonClass::readInputs(void)
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

