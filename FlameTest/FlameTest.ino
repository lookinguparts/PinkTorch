#include <Adafruit_NeoPixel.h>
#define PIN 6
#define NUMPIXELS 144

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500 // pause between pixels

void setup() {
  pixels.begin();
  Serial.begin(57600);
}
void loop() {

  pixels.clear(); // Set all pixel colors to 'off'
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 100, 180))//Pink
    ;
    pixels.setBrightness(100);
    pixels.show();
  Serial.print("loop");
  Serial.println(i);
  //Serial.println("string");
    delay(DELAYVAL);
  }
}
