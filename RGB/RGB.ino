// NeoPixelBrightness
// This example will cycle brightness from high to low of
// three pixels colored Red, Green, Blue.  
// This demonstrates the use of the NeoPixelBrightnessBus 
// with integrated brightness support
//
// There is serial output of the current state so you can 
// confirm and follow along
//

#include <NeoPixelBrightnessBus.h> // instead of NeoPixelBus.h
#define WIDTH   10
#define HEIGHT  6

const uint16_t PixelCount = 60; // this example assumes 3 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 12;  // make sure to set this to the correct pin, ignored for Esp8266

#define colorSaturation 255 // saturation of color constants
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor off(0,0,0);
RgbColor white(colorSaturation, colorSaturation, colorSaturation);

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;       // will store last time LED was updated
unsigned long modeMillis = 0;       // will store last time LED was updated
const long interval = 50;               // interval at which to update LEDs

enum {SWEEP1, SWEEP2, SWEEP3 } MODE;

uint8_t LEDindex[HEIGHT][WIDTH] =  
{
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
  {10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
  {30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
  {40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
  {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}
};

uint8_t x = 0;
uint8_t y = 0;

// Make sure to provide the correct color order feature
// for your NeoPixels
NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

// you loose the original color the lower the dim value used
// here due to quantization
const uint8_t c_MinBrightness = 8; 
const uint8_t c_MaxBrightness = 255;

int8_t direction; // current direction of dimming

void setup()
{
    Serial.begin(9600);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();

    direction = -1; // default to dim first
    
    Serial.println();
    Serial.println("Running...");

    MODE = SWEEP1;
}


void loop()
{
  /*
    uint8_t brightness = strip.GetBrightness();
    Serial.println(brightness);
    
    delay(10);

    // swap diection of dim when limits are reached
    //
    if (direction < 0 && brightness <= c_MinBrightness)
    {
      direction = 1;
    }
    else if (direction > 0 && brightness >= c_MaxBrightness)
    {
      direction = -1;
    }
    // apply dimming
    brightness += direction;
    strip.SetBrightness(brightness);
*/

  currentMillis = millis();

  switch (MODE)
  {
    case SWEEP1:
      setPosition();
      break;
      
    case SWEEP2:
      setPosition2();
      break;
      
    case SWEEP3:
      setPosition3();
      break;
      
    default:
      //do nothing
      break;
  }


  if (currentMillis - modeMillis >= 10000)
  {
    if (MODE == SWEEP3)
      MODE = SWEEP1;
    else if (MODE == SWEEP1)
      MODE = SWEEP2;
    else if (MODE == SWEEP2)
      MODE = SWEEP3;

    allOff();
    modeMillis = currentMillis;
  }

  

}


void setPosition(void)
{
    // every 50 ms refresh the led state
  if (currentMillis - previousMillis >= interval) 
  {
    //Serial.print(y);
    Serial.println(" ON");
    // set our three original colors
    for (uint8_t y=0; y<HEIGHT; y++)
    {
      strip.SetPixelColor(LEDindex[y][x], blue);
      strip.SetPixelColor(LEDindex[y][x-1], off);
    }
    strip.Show();

    if (x < WIDTH)
     x++;
    else
     x = 0;
     
    previousMillis = currentMillis;
  }
}

void setPosition2(void)
{
    // every 50 ms refresh the led state
  if (currentMillis - previousMillis >= interval) 
  {
    Serial.print(y);
    Serial.println(" ON");
    Serial.print(y-1);
    Serial.println(" OFF");
    // set our three original colors
    for (uint8_t x=0; x<WIDTH; x++)
    {
      strip.SetPixelColor(LEDindex[y][x], blue);
      strip.SetPixelColor(LEDindex[y-direction][x], off);
    }
    strip.Show();
     
    if (y >= HEIGHT-1)
    {
      direction = -1;
    }
    else if (y <= 0)
    {
      direction = 1;
    }

    y += direction;

    previousMillis = currentMillis;
  }
}

void setPosition3(void)
{
  static uint8_t z = 0;
  static bool state = true;
    // every 50 ms refresh the led state
  if (currentMillis - previousMillis >= interval) 
  {
    if (state)
      strip.SetPixelColor(z, blue);
    else
      strip.SetPixelColor(z, off);
    
    strip.Show();

    if (z>PixelCount)
    {
      state = !state;
      z=0;
    }
     else
      z++;
    
    previousMillis = currentMillis;
  }
}

void allOff(void)
{
  for (uint8_t i=0; i<PixelCount; i++)
  {
    strip.SetPixelColor(i, off);
    strip.Show();
  }
}

