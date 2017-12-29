# LampNode
WiFi enabled lamp with individually addressable RGB LEDs
https://redd.it/7mob5w



Overview
WiFi enabled lamp that is controlled by MQTT. The code is written for LampNode01, if additional lamps are to be programmed all instances of LampNode01 should be changed to LampNode02 so that each lamp has a unique name (LampNode03 and so on.) If the touch pad on top is held, the lamp will begin to pulse and so will all other LampNodes. This feature was added because I have family that live far away so it provides a way of communicating with them over long distance via a subtle animation. 



Wifi Connection
On power up, the lamp will try to connect to the last used network automatically. If this is unsuccessful it will create its own access point named LampNode01. Connect to the access point and go to 192.168.4.1 from your browser. From here you can search for access points and enter a password. The lamp will connect to the selected access point and its local LampNode01 access poin will cease to exist.



Modes of operation
The LampNode currently has 4 modes of operation:
  - Colour: sets the entire LED array to a single solid colour
  - Cycle: uses a colour wheel to slowly cycle through all colours
  - Rainbow: uses a colour wheel but distributes the entire colour range across all LEDs creating a rainbow type effect
  - Twinkle: random LEDs are set to the chosen colour (or slightly different to the chosen colour for some variation). There is also a 50% chance that the randomly chosen LED will be turned off instead of set to a colour. This creates a crude twinkle effect.
  
The onboard EEPROM is used to store the current RGB colour, the brightness, the mode of operation and the current standby state. So if power is lost, when the device is powered back up it will remember its last values and load them up.
  
  
  
Hardware
  - 1m strip of 60 WS2812 LEDs (I only used 57 because 3 LEDs were behaving badly so I removed them)
  - Wemos D1 mini
  - LM2596 DC-DC Buck Converter
  - TTP223 capacitive touch sensor module and aluminium tape
  - DC jack
  - PSU (I chose 12V 2.5A but as long as its got enough juice any voltage from 5 - 30V should be fine) 
  - Frosted acrylic tube
  - Length of 32mm PVC pipe
  - A base and top section (I 3D printed mine but if you dont have one you can be creative)
  
  
  
Communication
Download an MQTT app for your mobile (I use MQTT dash) and set up a CloudMQTT account to host the messages. You will have to add a config.h file to your arduino libraries folder containing the following:
  const char* MQTTserver = xxxx;
  const char* MQTTuser = xxxx;
  const char* MQTTpassword = xxxx;
  const int MQTTport = xxxx;
These parameters can be obtained by loging into CloudMQTT using a web browser.

The app needs to be set up to send messages to the following topics:
    Topic                     Expected messages
    ----------------------------------------------------
    LampNode01/Power          'On', 'Off'
    LampNode01/Mode           'Colour', 'Cycle', 'Rainbow', 'Twinkle'
    LampNode01/Colour         Either RGB or Hex, eg. for red send 'rgb(255,0,0)' or #FF0000
    LampNode01/Brightness     Any value between 0 and 100
  - LampNode01/Announcements  'Update' (this will request the lamp to send all of the above parameters hence updateing all tiles in the app)
  
Obviously if you are setting up a second lamp all of the above will be LampNode02.
  
The topic 'LampNode/Comms' is used to publish 'Press' on a touch event and 'Release' when the touch event is over. All lamps (LampNode01, LampNode02 etc.) are subscribed to this topic so will begin the pulse animation on a press and stop the animation on a release.
