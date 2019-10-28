/* 
 * Author: Sam Faull
 * Details: WiFi enabled lamp
 *          ESP8266 WiFi module & WS281B RGB LEDs 
 * 
 * Pin allocations: 
 * NA
 */

#include "nv.h"
#include "network.h"
#include "button.h"
#include "led.h"

#include <EEPROM.h>

#include <config.h> // this stores the private variables such as wifi ssid and password etc.

#include "config.h"
#include "credentials.h"
#include "timer.h"


bool active = false;
bool lastActive = false;

bool standby = false;
 

void setup() 
{

	/* Setup I/O */
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
	digitalWrite(LED_BUILTIN, HIGH);

	/* Setup serial */
	Serial.begin(115200);
	Serial.flush();
	/* swap serial port and then back again - seems to fix pixel update issue */
	//Serial.swap();
	//delay(200);
	//Serial.swap();

	Nv.init();
	Button.init();
	Led.init();	
}

void loop() 
{
	Network.process();
	Led.process();
	Button.init();
}


void setStandby(bool state)
{
  if (state)
  {
    applyColour(0,0,0);
    writeEEPROM(MEM_STANDBY, 1);
  }
  else
  {
    setColourTarget(target_colour[0],target_colour[1],target_colour[2]); 
    writeEEPROM(MEM_STANDBY, 0);
  }
    
  standby = state;
}


