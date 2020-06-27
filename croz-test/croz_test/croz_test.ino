#include <Adafruit_BNO055.h>
#include <FastLED.h>
#include <Wire.h>

// LED Constants
#define LED_PIN     6
#define COLOR_ORDER GRB
#define CHIPSET     WS2813
#define NUM_LEDS    219
#define FRAMES_PER_SECOND 60

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

// The max brightness at 90 degrees
#define MAX_BRIGHTNESS 255

// The max brightness at the "cliff"
#define MID_BRIGHTNESS 55

// The minimum brightness
#define MIN_BRIGHTNESS 10

// The angle subtracted from 90 where the cliff starts (20 = 70 deg)
#define CLIFF_DEGREES 20.0

// the large the number, the less frequent the flicker happens
// 60 is approximately once per second
// 0 = disabled
#define FLICKER_RANDOMNESS 180

// Do not change
#define MAX_Y 90.0
#define CLIFF (MAX_Y - CLIFF_DEGREES)
// LED Data
bool gReverseDirection = false;
CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;

// Set the delay between BNO055 samples
#define BNO055_SAMPLERATE_DELAY_MS (100)

// Initialize the BNO055
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

void setup() {
  // Initialize I2C library
  Wire.begin();

  // Start up the serial interface
  Serial.begin(9600);
  Serial.println("Starting...");

  // Print out the list of I2C devices that are discovered
  debugI2C();

  // Try to start up the BNO055, resetting it if it does not start as expected
  if (bno.begin()) {
    Serial.println("BNO055 initialized successfully\n");
  } else {
    while (!bno.begin()) {
      Serial.println("BNO055 is fucked - attempting to reset it");
      resetBNO055();
      delay(1000);
    }
  }

  // Initialize the LED stuff
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  gPal = CRGBPalette16( CRGB::Black, CRGB::LightPink, CRGB::Pink,  CRGB::HotPink);
  
  // Adjust this to limit the amount of current flowing through Teensy
  // Up this from 250 when connecting directly to the battery pack
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500);
}

void loop() {
  random16_add_entropy( random());

  sensors_event_t event;
  bno.getEvent(&event);

  Serial.print("X: ");
  Serial.print(event.orientation.x, 4);
  Serial.print("\tY: ");
  Serial.print(event.orientation.y, 4);
  Serial.print("\tZ: ");
  Serial.print(event.orientation.z, 4);
  Serial.println("");
  
  // If the torch is at a negative Y-angle, reverse the flame 
  gReverseDirection = event.orientation.y < 0;

  // The flame goes between full brightness and mid-brightness over a sensitive
  // period up until the "cliff". After that it gradually goes from mid-brightness
  // to min brightness over a larger period.
  int y = abs(event.orientation.y);
  int brightness = 0;
  if (y > CLIFF) {
    // Linear transitoin from cliff to fully upgright
    // Default = 90 to 70
    brightness = MAX_BRIGHTNESS - (MAX_Y - y) * ((MAX_BRIGHTNESS - MID_BRIGHTNESS) / CLIFF_DEGREES);
  } else {
    // Linear transition from cliff to fully flat
    // Default = 70 to 0
    brightness = ((y / (90 - CLIFF_DEGREES)) * (MID_BRIGHTNESS - MIN_BRIGHTNESS)) + MIN_BRIGHTNESS;
  }
  FastLED.setBrightness(brightness);
  
  Fire2012WithPalette();
  FastLED.show();
  delay(1000 / FRAMES_PER_SECOND);
}

// Fire pattern
void Fire2012WithPalette() {
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 3.5 (pink triangle special)
  // Randomly reduce the heat of every pixel by half 
  if (random(0, FLICKER_RANDOMNESS) == 1) {
    for (int i = 0 ; i < NUM_LEDS; i++) {
      heat[i] = heat[i] / 2;
    }
  }
  
  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

// Scan for I2C devices and print the findings. This is useful for determining
// if the BNO055 is running the correct mode or not. If it's in a bad mode then
// the resetBNO055() function can be used as long as it is wired correctly.
// Stolen from i2c_scanner on Ardiuno Playground.
//
// Good: I2C device found at address 0x28
// Bad: I2C device found at address 0x40
void debugI2C() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning for I2C devices");

  nDevices = 0;
  for (address = 1; address < 127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
}

// Reset procedure for the BNO055
// Required wiring:
//   PS0 -> GND
//   PS1 -> GND
//   RST -> Teensy PIN 7
void resetBNO055() {
  bool broken = true;
  pinMode(7, OUTPUT);

  while (broken) {
    Serial.println("Resetting the BNO055");
    digitalWrite(7, HIGH);
    digitalWrite(7, LOW);
    delay(1);
    digitalWrite(7, HIGH);
    delay(1);
    bno = Adafruit_BNO055(55);
    broken = !bno.begin();
    delay(1000);
  }

  Serial.println("BNO055 reset successfully\n");
}
