/* 
 * Author: Sam Faull
 * Details: WiFi enabled lamp
 *          ESP8266 WiFi module & WS281B RGB LEDs 
 * 
 * Pin allocations: 
 * NA
 */

#include <LEAmDNS_Priv.h>
#include <LEAmDNS_lwIPdefs.h>
#include <LEAmDNS.h>
#include <ESP8266mDNS_Legacy.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ESP8266WebServerSecureBearSSL.h>
#include <ESP8266WebServerSecureAxTLS.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiServerSecureAxTLS.h>
#include <WiFiServerSecure.h>
#include <WiFiServer.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClientSecureAxTLS.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <CertStoreBearSSL.h>
#include <BearSSLHelpers.h>
#include "config.h"
#include "network.h"
#include "button.h"
#include "led.h"
#include <EEPROM.h>
#include "config.h"
#include "credentials.h"
#include "timer.h"


bool active = false;
bool lastActive = false;
 

void setup() 
{

	/* Setup I/O */
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
	digitalWrite(LED_BUILTIN, HIGH);

	/* Setup serial */
	Serial.begin(115200);
	//Serial.flush();
	Serial.println("Starting up...");
	/* swap serial port and then back again - seems to fix pixel update issue */
	//Serial.swap();
	//delay(200);
	//Serial.swap();

	int err = Config.init();
	if (err < 0)
		Serial.printf("Config returned error %d\n", err);

	Button.init();
	Led.init();	
	Network.init();
}

void loop() 
{
	Network.process();
	Led.process();
	Button.process();
}

/*
void setStandby(bool state)
{
  if (state)
  {
    Led.applyColour(0,0,0);
    //writeEEPROM(MEM_STANDBY, 1);
  }
  else
  {
    Led.setColourTarget(target_colour[0],target_colour[1],target_colour[2]); // todo fixme
    //writeEEPROM(MEM_STANDBY, 0);
  }
    
  standby = state;
}
*/

