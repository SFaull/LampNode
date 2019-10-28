// 
// 
// 

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

#include <stdio.h>
#include <string.h>

#include "network.h"

WiFiClient espClient;
PubSubClient client(espClient);

char msg[50];        // message buffer

void NetworkClass::init()
{
	/* Setup WiFi and MQTT */
	//setup_wifi();
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;
	wifiManager.autoConnect(DEVICE_NAME);

	client.setServer(MQTT_SERVER, MQTT_PORT);
	client.setCallback(callback);

	initOTA();

}

void NetworkClass::process()
{
	/* Check WiFi Connection */
	if (!client.connected())
		reconnect();
	client.loop();
	ArduinoOTA.handle();
}

void initOTA(void)
{
	ArduinoOTA.onStart([]() {
		Serial.println("OTA Update Started");
		//setColour(0, 0, 0);
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nOTA Update Complete");
		//setColour(0, 0, 0);
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		//strip.SetPixelColor((PixelCount * (progress / (total / 100))) / 100, genericColour);
		//strip.Show();
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");

	/* --------------- Print incoming message to serial ------------------ */
	char input[length + 1];
	for (int i = 0; i < length; i++)
		input[i] = (char)payload[i];  // store payload as char array
	input[length] = '\0'; // dont forget to add a termination character

	Serial.println(input);

	if (strcmp(topic, MQTT_COLOUR) == 0)
	{
		/* ----- Split message by separator character and store rgb values ---- */
		char* command;
		int index = 0;
		int temp[3];
		Serial.print("rgb(");

		if (input[0] == '#')  // we have received a hex code of format #FFFFFF
		{
			memmove(input, input + 1, strlen(input)); // chop the first character off of the string (removes the #)
			unsigned long rgb = strtoul(input, NULL, 16);  // convert string to actual hex value 

			temp[0] = rgb >> 16;
			temp[1] = (rgb & 0x00ff00) >> 8;
			temp[2] = (rgb & 0x0000ff);

			Serial.print(temp[0]);
			Serial.print(", ");
			Serial.print(temp[1]);
			Serial.print(", ");
			Serial.print(temp[2]);
		}
		else  // we have received an rgb val of format rgb(255,255,255)
		{
			command = strtok(input, " (,)");  // this is the first part of the string (rgb) - ignore this
			while (index < 3)
			{
				command = strtok(NULL, " (,)");
				temp[index] = atoi(command);
				Serial.print(temp[index]);
				Serial.print(", ");
				index++;
			}
		}
		Serial.println(")");
		setColourTarget(temp[0], temp[1], temp[2]);
	}

	if (strcmp(topic, MQTT_MODE) == 0)
	{
		Serial.print("Mode set to: ");

		if (strcmp(input, "Colour") == 0)
		{
			setTheMode(COLOUR);
			Serial.println("COLOUR");
		}
		if (strcmp(input, "Twinkle") == 0)
		{
			setTheMode(TWINKLE);
			Serial.println("TWINKLE");
		}
		if (strcmp(input, "Rainbow") == 0)
		{
			setTheMode(RAINBOW);
			Serial.println("RAINBOW");
		}
		if (strcmp(input, "Cycle") == 0)
		{
			setTheMode(CYCLE);
			Serial.println("CYCLE");
		}
	}

	if (strcmp(topic, MQTT_COMMS) == 0)
	{
		if (strcmp(input, "Press") == 0)
		{
			// perform whatever fun animation you desire on touch
			pulse_animation = true;
			pulse_addr = 0;
			generatePulse();
			Serial.println("Press");
		}
		if (strcmp(input, "Release") == 0)
		{
			// perform whatever fun animation you desire on touch
			pulse_animation = false;
			setColourTarget(target_colour[0], target_colour[1], target_colour[2]);
			Serial.println("Release");
		}
	}
	if (strcmp(topic, MQTT_ANNOUNCEMENTS) == 0)
	{
		if (strcmp(input, "Update") == 0)
		{
			Serial.println("Broadcasting parameters");

			char brightness_str[4];
			itoa(((brightness * 100) / (MAX_BRIGHTNESS + 1)) + 1, brightness_str, 10);
			client.publish(MQTT_BRIGHTNESS, brightness_str);

			switch (Mode)
			{
			case COLOUR:
				client.publish(MQTT_MODE, "Colour");
				break;

			case TWINKLE:
				client.publish(MQTT_MODE, "Twinkle");
				break;

			case RAINBOW:
				client.publish(MQTT_MODE, "Rainbow");
				break;

			case CYCLE:
				client.publish(MQTT_MODE, "Cycle");
				break;

			default:
				// should never get here
				break;
			}

			char hexR[3], hexG[3], hexB[3], hex[8];

			sprintf(hexR, "%02X", target_colour[0]);
			sprintf(hexG, "%02X", target_colour[1]);
			sprintf(hexB, "%02X", target_colour[2]);
			strcpy(hex, "#");
			strcat(hex, hexR);
			strcat(hex, hexG);
			strcat(hex, hexB);

			client.publish(MQTT_COLOUR, hex);

			if (standby)
				client.publish(MQTT_POWER, "Off");
			else
				client.publish(MQTT_POWER, "On");
		}
	}
	if (strcmp(topic, MQTT_POWER) == 0)
	{
		if (strcmp(input, "On") == 0)
		{
			setStandby(false);
			Serial.println("ON");
		}
		if (strcmp(input, "Off") == 0)
		{
			setStandby(true);
			Serial.println("OFF");
		}
	}
	if (strcmp(topic, MQTT_BRIGHTNESS) == 0)
	{
		int brightness_temp = atoi(input);
		brightness_temp *= MAX_BRIGHTNESS; // multiply by range
		brightness_temp /= 100;  // divide by 100
		Serial.print("Brightness: ");
		Serial.print(brightness_temp);
		if (brightness_temp >= 0 || brightness_temp < 256)
			brightness = brightness_temp;

		writeEEPROM(MEM_BRIGHTNESS, brightness);
		//if(Mode==COLOUR)
		//  applyColour(target_colour[0],target_colour[1],target_colour[2]);
	}
}

void reconnect() {
	// Loop until we're reconnected
	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection... ");
		// Attempt to connect
		if (client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS))
		{
			Serial.println("Connected");
			// Once connected, publish an announcement...
			//client.publish("/LampNode/Announcements", "LampNode01 connected");  // potentially not necessary
			// ... and resubscribe
			client.subscribe(MQTT_TOPIC);
			client.subscribe(MQTT_COMMS);  // listen for touch events (community topic)
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}


NetworkClass Network;

