/* 
 * Author: Sam Faull
 * Details: WiFi enabled lamp
 *          ESP8266 WiFi module & WS281B RGB LEDs 
 * 
 * Pin allocations: 
 * NA
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <config.h> // this stores the private variables such as wifi ssid and password etc.
#include <NeoPixelBrightnessBus.h> // instead of NeoPixelBus.h
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "credentials.h"
#include "timer.h"

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long runTime         = 0;
unsigned long ledTimer        = 0;
unsigned long brightnessTimer = 0;
unsigned long twinkleTimer    = 0;
unsigned long rainbowTimer    = 0;
unsigned long cycleTimer      = 0;
unsigned long readTempTimer   = 0;
unsigned long readInputTimer  = 0;

long buttonTime = 0;
long lastPushed = 0; // stores the time when button was last depressed
long lastCheck = 0;  // stores the time when last checked for a button press
char msg[50];        // message buffer

// Flags
bool button_pressed = false; // true if a button press has been registered
bool button_released = false; // true if a button release has been registered
bool button_short_press = false;
bool target_met = false;
bool pulse_animation = false;
int pulse_addr = 0;
int brightness = 155;

bool active = false;
bool lastActive = false;
bool overheating = false;

const uint16_t PixelCount = 60; // this example assumes 3 pixels, making it smaller will cause a failure


unsigned int target_colour[3] = {0,0,0}; // rgb value that LEDs are currently set to
unsigned int current_colour[3] = {0,0,0};  // rgb value which we aim to set the LEDs to
unsigned int transition[50][3];
unsigned int pulse[30][3];
 
enum Modes {COLOUR, TWINKLE, RAINBOW, CYCLE};   // various modes of operation
bool standby = false;

enum Modes Mode;

NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, WS2812_PIN);

RgbColor genericColour(0,255,0);

void setTheMode(Modes temp);

void timer_init(void)
{
	/* Start timers */
	Timer.set(&readInputTimer);
	Timer.set(&readTempTimer);
	Timer.set(&ledTimer);
	Timer.set(&brightnessTimer);
	Timer.set(&twinkleTimer);
	Timer.set(&rainbowTimer);
	Timer.set(&cycleTimer);
}

void io_init(void)
{
    /* Setup I/O */
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON, INPUT_PULLUP);  // Enables the internal pull-up resistor
  digitalWrite(LED_BUILTIN, HIGH); 
}

void serial_init(void)
{
    /* Setup serial */
  Serial.begin(115200);
  Serial.flush();
  /* swap serial port and then back again - seems to fix pixel update issue */
  //Serial.swap();
  //delay(200);
  //Serial.swap();
}

void led_init(void)
{
    /* Set LED state */
  strip.Begin();
  strip.Show();
}

void wifi_init(void)
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

void eeprom_init(void)
{
    /* Initialise EEPROM */
  EEPROM.begin(512);
  getColourFromMemory();
  setColourTarget(target_colour[0],target_colour[1],target_colour[2]);
  setTheMode((Modes)readEEPROM(MEM_MODE));
  
  if(readEEPROM(MEM_STANDBY)==0)
  {
    setStandby(false);
    Serial.println("STANDBY FALSE");
  }
  else 
  {
    setStandby(true);
    Serial.println("STANDBY TRUE");
  }
    
  brightness = readEEPROM(MEM_BRIGHTNESS);
}

void setup() 
{
  io_init();
  serial_init();
  timer_init();
  led_init();
  wifi_init();
  eeprom_init();
}

void led_process()
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

void button_process()
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

int cnt = 0;
void loop() 
{
  /* Check WiFi Connection */
  if (!client.connected()) 
    reconnect();
  client.loop();
  ArduinoOTA.handle();
  
  led_process();
  button_process();
}



void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  /* --------------- Print incoming message to serial ------------------ */
  char input[length+1];
  for (int i = 0; i < length; i++) 
    input[i] = (char)payload[i];  // store payload as char array
  input[length] = '\0'; // dont forget to add a termination character
  
  Serial.println(input);
  
  if (strcmp(topic, MQTT_COLOUR)==0)
  {  
    /* ----- Split message by separator character and store rgb values ---- */
    char * command;
    int index = 0;
    int temp[3];
    Serial.print("rgb(");

    if (input[0] == '#')  // we have received a hex code of format #FFFFFF
    {
      memmove(input, input+1, strlen(input)); // chop the first character off of the string (removes the #)
      unsigned long rgb = strtoul (input, NULL, 16);  // convert string to actual hex value 
      
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
      command = strtok (input," (,)");  // this is the first part of the string (rgb) - ignore this
      while (index<3)
      {
        command = strtok (NULL, " (,)");
        temp[index] = atoi(command);
        Serial.print(temp[index]);
        Serial.print(", ");
        index++;
      }
    }
    Serial.println(")");
    setColourTarget(temp[0],temp[1],temp[2]);
  } 
  
  if (strcmp(topic, MQTT_MODE)==0)
  {    
    Serial.print("Mode set to: ");
   
    if(strcmp(input,"Colour")==0)
    {
      setTheMode(COLOUR);
      Serial.println("COLOUR");
    }
    if(strcmp(input,"Twinkle")==0)
    {
      setTheMode(TWINKLE);
      Serial.println("TWINKLE");
    }
    if(strcmp(input,"Rainbow")==0)
    {
      setTheMode(RAINBOW);
      Serial.println("RAINBOW");
    }
    if(strcmp(input,"Cycle")==0)
    {
      setTheMode(CYCLE);
      Serial.println("CYCLE");
    }
  }

  if (strcmp(topic, MQTT_COMMS)==0)
  {               
    if(strcmp(input,"Press")==0)
    {
      // perform whatever fun animation you desire on touch
      pulse_animation = true;
      pulse_addr = 0;
      generatePulse();
      Serial.println("Press");
    }
    if(strcmp(input,"Release")==0)
    {
      // perform whatever fun animation you desire on touch
      pulse_animation = false;
      setColourTarget(target_colour[0],target_colour[1],target_colour[2]);
      Serial.println("Release");
    }
  }
  if (strcmp(topic, MQTT_ANNOUNCEMENTS)==0)
  {
    if(strcmp(input,"Update")==0)
    {
      Serial.println("Broadcasting parameters");
      
      char brightness_str[4];
      itoa(((brightness*100)/(MAX_BRIGHTNESS+1))+1, brightness_str, 10);
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
  if (strcmp(topic, MQTT_POWER)==0)
  {  
    if(strcmp(input,"On")==0)
    {
      setStandby(false);
      Serial.println("ON");
    }
    if(strcmp(input,"Off")==0)
    {
      setStandby(true);
      Serial.println("OFF");
    }
  }
  if (strcmp(topic, MQTT_BRIGHTNESS)==0)
  {  
    int brightness_temp = atoi(input);
    brightness_temp*=MAX_BRIGHTNESS; // multiply by range
    brightness_temp/=100;  // divide by 100
    Serial.print("Brightness: ");
    Serial.print(brightness_temp);
    if (brightness_temp >= 0 || brightness_temp < 256)
      brightness = brightness_temp;
    
    writeEEPROM(MEM_BRIGHTNESS,brightness);
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

void fadeToColourTarget(void)
{
  static int addr = 0;
    
  if(!target_met)
  {
    setColour(transition[addr][0],transition[addr][1],transition[addr][2]);
    addr++;
    
    if (addr>=50)
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
    RgbColor colour(g,r,b);
    for (uint8_t i=0; i<PixelCount; i++)
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
  static int ADCval, lastADCval, brightness = 0 ;
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



void writeEEPROM(int address, int val)
{
  if ((address < 512) && (address >=0)) // make sure we are in range
  {
    EEPROM.write(address, val);
    EEPROM.commit();
  }
  else
  {
    Serial.print("Invalid EEPROM write address: ");
    Serial.println(address);
  }
}

int readEEPROM(int address)
{
  if ((address < 512) && (address >=0)) // make sure we are in range
  {
    int val;
    val = EEPROM.read(address);
    return val;
  }
  else
  {
    Serial.print("Invalid EEPROM read address: ");
    Serial.println(address);
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
  if(!standby)
    applyColour(current_colour[0],current_colour[1],current_colour[2]);
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
  for(int addr=0; addr<50; addr++)  // for each element in the array
  {
    for (int i=0; i<3; i++)  // for each colour in turn
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
  for(int addr=0; addr<30; addr++)  // for each element in the array
  {
    for (int i=0; i<3; i++)  // for each colour in turn
    {
      pulse[addr][i] = map(addr, 0, 29, current_colour[i], current_colour[i]/5); // compute the proportional colour value
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
  RgbColor colour(10,10,10);
  RgbColor off(0,0,0);
  for (int i=0; i<5; i++)
    strip.SetPixelColor(count+i, colour);
  strip.SetPixelColor(count-1, off);
  //strip.SetBrightness(brightness);
  strip.Show();
  if(count>60)
    count = 0;
  else
    count++;
  delay(30);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void Wheel(byte WheelPos, int *r, int *g, int *b) 
{
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) 
  {
   *r = 255 - WheelPos * 3;
   *g = 0;
   *b = WheelPos * 3;
  } 
  else if(WheelPos < 170) 
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
  static int stepVal = 256/PixelCount;  // note the 256 value can be reduced to show less of the colour spectrum at once.
  int red, green, blue;  
  
  for (int i=0; i<PixelCount; i++)
  {
    Wheel(i*stepVal+offset, &red, &green, &blue); // get our colour
    RgbColor colour(green,red,blue);
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
  int val = rgb2wheel(target_colour[0],target_colour[1],target_colour[2]);
  Wheel(val+offset, &red, &green, &blue); // get our colour
  RgbColor colour(green,red,blue);
  RgbColor off(0,0,0);
  
  if(state)
    strip.SetPixelColor(pix, colour);
  else
    strip.SetPixelColor(pix, off);

  strip.Show();
}


void setTheMode(Modes temp)
{
  Serial.print("mode set to: ");
  Serial.println(temp);
  switch(temp)
  {
    case COLOUR:
    if(!standby)
      setColourTarget(target_colour[0],target_colour[1],target_colour[2]); 
    break;
    
    case TWINKLE:
      setColour(0,0,0);
    break;   
     
    case RAINBOW:
      setColour(0,0,0);
    break; 
    
    case CYCLE:
      setColour(0,0,0);
    break;
    
    default:
      // should never get here
    break;
  }
  
  Mode = temp;
  writeEEPROM(MEM_MODE, Mode);
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

// doesnt work very well
byte rgb2wheel(int R, int G, int B)
{
 return  (B & 0xE0) | ((G & 0xE0)>>3) | (R >> 6);
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
    
    if (coefficient>=1.0)
      pulse_direction = 0;
    if (coefficient<=0.5)
      pulse_direction = 1;  
    
    if (pulse_direction)
      coefficient+=0.025;
    else
      coefficient-=0.025;

    if (Mode == COLOUR)
    {
      strip.Show();
    }
  }
  else
  {
    if(coefficient < 1.0)
    {
      coefficient+=0.025;
      brightness_pulse = brightness * coefficient;
      strip.SetBrightness(brightness_pulse);
    }
    else
    {
      if (last_brightness!=brightness)
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

void initOTA(void)
{
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Started");
    setColour(0,0,0);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Complete");
    setColour(0,0,0);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    strip.SetPixelColor((PixelCount*(progress / (total / 100)))/100, genericColour);
    strip.Show();
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
