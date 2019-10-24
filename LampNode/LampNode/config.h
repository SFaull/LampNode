/* EEPROM memory map */
#define MEM_RED         0
#define MEM_GREEN       1
#define MEM_BLUE        2
#define MEM_MODE        3
#define MEM_STANDBY     4
#define MEM_BRIGHTNESS  5

#define MAX_BRIGHTNESS 153 // ~60%

/* Physical connections */
#define BUTTON        D1               //button on pin D1
#define AIN           A0
#define WS2812_PIN    14  // make sure to set this to the correct pin, ignored for Esp8266

/* Timers */
#define INPUT_READ_TIMEOUT     50   //check for button pressed every 50ms
#define LED_UPDATE_TIMEOUT     20   // update led every 20ms
#define RAINBOW_UPDATE_TIMEOUT 30
#define CYCLE_UPDATE_TIMEOUT   40
#define TWINKLE_UPDATE_TIMEOUT 50
#define BRIGHTNESS_UPDATE_TIMEOUT 50
#define TEMPERATURE_READ_TIMEOUT 2000

#define DEVICE_NAME           "LampNode01"
#define SERIAL_BAUD_RATE      115200

#define MQTT_TOPIC            DEVICE_NAME "/#"
#define MQTT_MODE             DEVICE_NAME "/Mode"
#define MQTT_POWER            DEVICE_NAME "/Power"
#define MQTT_COLOUR           DEVICE_NAME "/Colour"
#define MQTT_BRIGHTNESS       DEVICE_NAME "/Brightness"
#define MQTT_ANNOUNCEMENTS    DEVICE_NAME "/Announcements"
#define MQTT_COMMS            DEVICE_NAME "/Comms"
