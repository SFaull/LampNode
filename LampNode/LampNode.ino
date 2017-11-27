/* 
 * Author: Sam Faull
 * Details: WiFi enabled lamp
 *          ESP8266 WiFi module & WS281B RGB LEDs 
 * 
 * Pin allocations: 
 * NA
 */
 
/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" whenever the button is pressed
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <config.h> // this stores the private variables such as wifi ssid and password etc.
#include <NeoPixelBrightnessBus.h> // instead of NeoPixelBus.h
#include <stdio.h>
#include <string.h>


/* EEPROM memory map */
#define MEM_RED         0
#define MEM_BLUE        1
#define MEM_GREE        2
#define MEM_MODE        3
#define MEM_TRANSITION  4



#define BUTTON D3               //button on pin D3
#define INPUT_READ_TIMEOUT 50   //check for button pressed every 50ms
#define LED_UPDATE_TIMEOUT 20   // update led every 20ms

#define colorSaturation 255 // saturation of color constants

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long runTime,
              ledTimer, 
              readInputTimer = 0;

long buttonTime = 0;
long lastPushed = 0; // stores the time when button was last depressed
long lastCheck = 0;  // stores the time when last checked for a button press
char msg[50];        // message buffer

// Flags
bool button_pressed = false; // true if a button press has been registered
bool button_released = false; // true if a button release has been registered
bool target_met = false;
bool charging_animation=false;
bool group_animation=false;

const uint16_t PixelCount = 60; // this example assumes 3 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 14;  // make sure to set this to the correct pin, ignored for Esp8266

unsigned int target_colour[3] = {0,0,0}; // rgb value that LEDs are currently set to
unsigned int current_colour[3] = {0,0,0};  // rgb value which we aim to set the LEDs to
unsigned int transition[50][3];
 
enum {OFF, NORMAL, PARTY} Mode;   // various modes of operation
enum {FADE, INSTANT} Transition;  // The way in which the lamp animates between colours

NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void setup() 
{
  /* Setup I/O */
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON, INPUT_PULLUP);  // Enables the internal pull-up resistor
  
  /* Setup serial */
  Serial.begin(115200);
  Serial.flush();
  /* swap serial port and then back again - seems to fix pixel update issue */
  Serial.swap();
  delay(200);
  Serial.swap();
  
  /* Setup WiFi and MQTT */ 
  setup_wifi();
  client.setServer(MQTTserver, MQTTport);
  client.setCallback(callback);
  
  /* Start timers */
  setTimer(&readInputTimer);
  setTimer(&ledTimer);

  /* Initialise EEPROM */
  EEPROM.begin(512);
  getColourFromMemory();
  setColourTarget(target_colour[0],target_colour[1],target_colour[2]);
  Mode = readEEPROM(MEM_MODE);
  Transition = readEEPROM(MEM_TRANSITION);

  /* Set LED state */
  strip.Begin();
  strip.Show();
}

void loop() 
{
  /* Check WiFi Connection */
  if (!client.connected()) 
    reconnect();
  client.loop();
  
  unsigned long now = millis();  // get elapsed time


  switch (Mode)
  {
    case OFF:
      
    break;

    case NORMAL:
      /* Periodically update the LEDs */
      if(timerExpired(ledTimer, LED_UPDATE_TIMEOUT))
      {
        setTimer(&ledTimer); // reset timer
        /*
        if(charging_animation)
        {
          chargingAnimation();
        }
        if(group_animation)
        {
          groupAnimation();
        }
        */
        fadeToColourTarget();
      }

    break;

    case PARTY:
      /* Periodically update the LEDs */
      if(timerExpired(ledTimer, LED_UPDATE_TIMEOUT))
      {
        setTimer(&ledTimer); // reset timer
        fadeToColourTarget();
        //music2Brightness();
      }

    break;

    default:
      // unknown state - do nothing
    break;
  }
  


  /* Periodically read the inputs */
  if (timerExpired(readInputTimer, INPUT_READ_TIMEOUT)) // check for button press periodically
  {
    setTimer(&readInputTimer);  // reset timer
    
    readInputs();
    
    if (button_pressed)
    {
      //start conting
      lastPushed = now; // start the timer 
      Serial.print("Button pushed... ");
      button_pressed = false;
      charging_animation = true;
    }
    
    if (button_released)
    {
      Serial.println("Button released.");

      //get the time that button was held in
      buttonTime = now - lastPushed;

      snprintf (msg, 75, "hello world #%ld", buttonTime);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("/test/outTopic", msg);
      button_released = false;
      charging_animation=false;
      group_animation=true;
    }
  }
}

void setup_wifi() 
{
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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
  
  if (strcmp(topic,"/LampNode/Colour")==0)
  {  
    /* ----- Split message by separator character and store rgb values ---- */
    char * command;
    int index = 0;
    int temp[3];
    Serial.print("rgb(");
    command = strtok (input," (,)");  // this is the first part of the string (rgb) - ignore this
    while (index<3)
    {
      command = strtok (NULL, " (,)");
      temp[index] = atoi(command);
      Serial.print(temp[index]);
      Serial.print(", ");
      index++;
    }
    Serial.println(")");
    setColourTarget(temp[0],temp[1],temp[2]);
  } 
  
  if (strcmp(topic,"/LampNode/Mode")==0)
  {    
    Serial.print("Mode set to: ");
    
    if(strcmp(input,"Off")==0)
    {
      Mode = OFF;
      Serial.println("OFF");
      setColour(0,0,0);
    }
    if(strcmp(input,"Normal")==0)
    {
      Mode = NORMAL;
      Serial.println("NORMAL");
      setColourTarget(target_colour[0],target_colour[1],target_colour[2]); 
    }
    if(strcmp(input,"Party")==0)
    {
      Mode = PARTY;
      Serial.println("PARTY");
    }
    // save to eeprom
     writeEEPROM(MEM_MODE, Mode);
  }
  
  if (strcmp(topic,"/LampNode/Transition")==0)
  {        
    if(strcmp(input,"Fade")==0)
    {
      Transition = FADE;
      Serial.println("FADE");
    }
    if(strcmp(input,"Instant")==0)
    {
      Transition = INSTANT;
      Serial.println("INSTANT");
    }
    // save to eeprom
     writeEEPROM(MEM_TRANSITION, Transition);
  }

  if (strcmp(topic,"/LampNode/Comms")==0)
  {               
    if(strcmp(input,"Poke")==0)
    {
      // perform whatever fun animation you desire on touch
      Serial.println("POKE");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection... ");
    // Attempt to connect
    if (client.connect("LampNode01", MQTTuser, MQTTpassword)) 
    {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      client.publish("/LampNode/Comms", "LampNode01 connected");  // potentially not necessary
      // ... and resubscribe
      client.subscribe("/LampNode/Colour");
      client.subscribe("/LampNode/Comms");
      client.subscribe("/LampNode/Mode");
      client.subscribe("/LampNode/Transition");
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
    button_pressed = true;
    
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

int led = 60;

void chargingAnimation(void)
{
  Serial.println("charging");
  
  RgbColor colour(0,0,0);
  strip.SetPixelColor(led, colour);
  strip.Show();
  if (led>0)
    led--;
}

void groupAnimation(void)
{
  static int count = 0;
  RgbColor colour(current_colour[0],current_colour[1],current_colour[2]);
  strip.SetPixelColor(led, colour);
  strip.Show();
  
  count++;
  led++;
  if (led>=60)
  {
    count = 0;
    led = 60;
    group_animation = false;
    setColourTarget(target_colour[0],target_colour[1],target_colour[2]); // go back to original colour
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

/* pass this function a pointer to an unsigned long to store the start time for the timer */
void setTimer(unsigned long *startTime)
{
  runTime = millis();    // get time running in ms
  *startTime = runTime;  // store the current time
}

/* call this function and pass it the variable which stores the timer start time and the desired expiry time 
   returns true fi timer has expired */
bool timerExpired(unsigned long startTime, unsigned long expiryTime)
{
  runTime = millis(); // get time running in ms
  if ( (runTime - startTime) >= expiryTime )
    return true;
  else
    return false;
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
  for (int addr = MEM_RED; addr <= BLUE; addr++)
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

  applyColour(current_colour[0],current_colour[1],current_colour[2]);
}

void setColourTarget(int r, int g, int b)
{
  target_met = false;
  
  target_colour[0] = r;
  target_colour[1] = g;
  target_colour[2] = b;

  if (Transition != FADE) // if not fading, set colour to target immediately
  {
    setColour(r,g,b);
  }

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
    Serial.print(transition[addr][0]);
    Serial.print(",");
    Serial.print(transition[addr][1]);
    Serial.print(",");
    Serial.println(transition[addr][2]);
    
  }
}

