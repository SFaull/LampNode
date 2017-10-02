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

unsigned long previousMillis = 0;       // will store last time LED was updated
const long interval = 50;               // interval at which to update LEDs

bool MODE = false;

int index[HEIGHT][WIDTH] =  
{
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
  {10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
  {30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
  {40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
  {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}
};

int x = 0;
int y = 0;

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

  unsigned long currentMillis = millis();
  
  // every 50 ms refresh the led state
  if (currentMillis - previousMillis >= interval) 
  {
    MODE ? setPosition() : setPosition2();
    previousMillis = currentMillis;
  }
}


void setPosition(void)
{
  //Serial.print(y);
  Serial.println(" ON");
    // set our three original colors
    for (int y=0; y<HEIGHT; y++)
    {
      strip.SetPixelColor(index[y][x], blue);
      strip.SetPixelColor(index[y][x-1], off);
    }
    strip.Show();

    if (x < WIDTH)
     x++;
    else
     x = 0;
}

void setPosition2(void)
{
  Serial.print(y);
  Serial.println(" ON");
  Serial.print(y-1);
  Serial.println(" OFF");
    // set our three original colors
    for (int x=0; x<WIDTH; x++)
    {
      strip.SetPixelColor(index[y][x], blue);
      strip.SetPixelColor(index[y-direction][x], off);
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
}
