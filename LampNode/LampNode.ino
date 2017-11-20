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

EEPROM LUT:

Address | Contents
-----------------
000     | R
001     | G
002     | B
003     | Brightness

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


#define BUTTON D3               //button on pin D3
#define INPUT_READ_TIMEOUT 50   //check for button pressed every 50ms
#define LED_UPDATE_TIMEOUT 5   // update led every 20ms

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
bool updateRequired;

bool colourFade = true;

const uint16_t PixelCount = 60; // this example assumes 3 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 14;  // make sure to set this to the correct pin, ignored for Esp8266

unsigned int rgbTarget[3] = {'0','0','0'}; // rgb value that LEDs are currently set to
unsigned int rgbValue[3] = {'0','0','0'};  // rgb value which we aim to set the LEDs to
 



NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void setup() 
{
  /* Setup I/O */
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON, INPUT_PULLUP);  // Enables the internal pull-up resistor
  
  /* Setup serial */
  Serial.begin(115200);
  Serial.flush();
  /* swap serial port and then vak again - seems to fix pixel update issue */
  Serial.swap();
  delay(10);
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
  getRGB();

  /* Set LED state */
  strip.Begin();
  strip.Show();

  
}

int switcheroo = 0;

void loop() 
{
  if (!client.connected()) 
    reconnect();
  client.loop();
  
  unsigned long now = millis();  // get elapsed time

  if(timerExpired(ledTimer, LED_UPDATE_TIMEOUT) && colourFade == true)
  {
    setTimer(&ledTimer); // reset timer
    
    updateRequired = false; // assume no update required
    
    if (rgbValue[switcheroo] < rgbTarget[switcheroo])
    {
      updateRequired = true; // need to update
      rgbValue[switcheroo]++;
      
    }
    else if (rgbValue[switcheroo] > rgbTarget[switcheroo])
    {
      updateRequired = true; // need to update
      rgbValue[switcheroo]--;
    }

    if(updateRequired == true)
      applyColour(rgbValue[0],rgbValue[1],rgbValue[2]); // only do this if we need to
    else
    {
      if (switcheroo < 2)
        switcheroo++;
      else
        switcheroo = 0;
    }
  }
  
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

      colourFade = !colourFade; // temporary way of turning colourfade animation on and off
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
  char input[length];
  /* --------------- Print incoming message to serial ------------------ */
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
    input[i] = (char)payload[i];  // store payload as char array
  }
  Serial.println();

  /* ----- Split message by separator character and store rgb values ---- */
  char * command;
  //int rgb[3] = {'0','0','0'};
  int index = 0;
  Serial.print("rgb(");
  command = strtok (input," (,)");  // this is the first part of the string (rgb) - ignore this
  while (index<3)
  {
    command = strtok (NULL, " (,)");
    rgbTarget[index] = atoi(command);
    Serial.print(rgbTarget[index]);
    Serial.print(", ");
    index++;
  }
  Serial.println(")");
  setRGB();

  if (colourFade == false)
  {
    for (int i=0; i<3; i++)
      rgbValue[i] = rgbTarget[i];
    applyColour(rgbValue[0],rgbValue[1],rgbValue[2]);
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
      client.publish("/test/outTopic", "LampNode01 connected");  // potentially not necessary
      // ... and resubscribe
      client.subscribe("/test/inTopic");
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
  
  button_state = !digitalRead(BUTTON); // read button state (active low)
  
  if (button_state && !last_button_state) // on a rising edge we register a button press
    button_pressed = true;
    
  if (!button_state && last_button_state) // on a falling edge we register a button press
    button_released = true;

  last_button_state = button_state;
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
    delay(1);
    strip.Show();
    delay(1);
  }
  else
    Serial.println("Invalid RGB value, colour not set");
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

/* gets the last saved RGB value from the eeprom and stores it in rgbTarget */
void getRGB(void)
{
  for (int addr = 0; addr < 3; addr++)
  {
    rgbTarget[addr] = readEEPROM(addr);

    Serial.print("EEPROM read: ");
    Serial.print("[");
    Serial.print(addr);
    Serial.print("] ");
    Serial.println(rgbTarget[addr]);
  }
}

/* stores the last RGB value from rgbTarget in the eeprom */
void setRGB(void)
{
  Serial.println("Saving RGB value");
  for (int addr = 0; addr < 3; addr++)
  {
    writeEEPROM(addr, rgbTarget[addr]);
    
    Serial.print("EEPROM write: ");
    Serial.print("[");
    Serial.print(addr);
    Serial.print("] ");
    Serial.println(rgbTarget[addr]);
  }
}

