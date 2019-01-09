/*
   ––––––––––––––––
  |C I R C         |
  |U I T ~ ~ ~ ~ ~ |
  |                |
  |                |
  |                |
  |~ ~ ~ ~ C I R C |
  |            U S |
  ––––––––––––––––
  SEMEION ARDUINO
  A CIRCUIT CIRCUS PROJECT
*/

// INCLUDES
#include <FastLED.h>


// LED
#define NUM_LEDS 60
#define MIN_LED 0
#define LED_PIN 3
CRGB leds[NUM_LEDS];

// Potentiometer
#define POT_PIN A3

// Doppler sensors
#define DOPPLER_PIN1 A0
#define DOPPLER_PIN2 A2

uint8_t currProximity = 0;
uint8_t lastProximity = 0;
int proximityReadingInterval = 100;
unsigned long lastProximityReading = 0;

// These will be used to determine lower and higher bounds
int lowestReading = 1024;
int highestReading = 0;

int acceleration;

uint8_t deactiveThreshold = 15;
uint8_t activeThreshold = 50;
uint8_t accelerationThreshold = 30;

// Timer for first contact
int firstContactInterval = 3000;
unsigned long firstContactTimer;

// Timer for feeling unpleasant when active
int feltUnpleasantInterval = 5000;
unsigned long feltUnpleasantTimer;

// -1 = IDLE, 0 = DEACTIVE, 1 = ACTIVE
int activityState = -1;
// 0 = UNPLEASANT, 1 = PLEASANT
int pleasureState = 0;

/**
 * NOISE
 */
// 1D Noise is adapted from FastLEDs Noise example, which uses 2D
// The 32bit version of our coordinates
static uint16_t x;
static uint16_t y;

// We're using the x dimension to map to the pixel.  We'll
// use the y-axis for "time". 
// Speed determines how fast time moves forward.
// Try 1 for a very slow moving effect, or 60 for something that ends up looking like water.
// 
const float minSpeed = 1.0f;
const float maxSpeed = 5.0f;
float speed = minSpeed;

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
// 
// uint16_t scale = 1; // mostly just solid colors
uint16_t scale = 311; // a nice start
// uint16_t scale = 4011; // very zoomed out and shimmery

// This is where we keep our noise values
uint8_t noise[NUM_LEDS];

/**
 * LED VALUES
 */
CRGBPalette16 currentPalette;
TBlendType currentBlending;
uint8_t minBrightness;
uint8_t maxBrightness;
uint8_t minColor;
uint8_t maxColor;

/**
 * ANIMATION
 */
#define MAX_ANI 2 //Maximum number of curves per animation
bool readyToChangeAnimation;
float t;
float duration;
int iterator = 1;
float currentAnimation[10];
uint8_t maxLed = NUM_LEDS;
uint8_t fadeScales[NUM_LEDS];
const uint8_t fadeAmount = 2;

/**
 * STATES
 */
 DEFINE_GRADIENT_PALETTE(MainPalette_p) {
   0, 250, 19, 31,
   255, 0, 178, 252
 };


float idleSpeed = 0.1f;

uint8_t minIdleBri = 10;
uint8_t maxIdleBri = 20;

uint8_t minIdleCol = 40;
uint8_t maxIdleCol = 60;
float aniIdle[] = {0.1, 0.5, 0.5, 0.1, 600.0, -99, 0, 0, 0, 0};

float deUnSpeed = 0.2f;

uint8_t minDeUnBri = 40;
uint8_t maxDeUnBri = 70;

uint8_t minDeUnCol = 60;
uint8_t maxDeUnCol = 80;
float aniDeUn[] = {0.66, 1.15, 1.035, 0.665, 300.0, -99, 0, 0, 0, 0};

float dePlSpeed = 0.3f;

uint8_t minDePlBri = 40;
uint8_t maxDePlBri = 70;

uint8_t minDePlCol = 0;
uint8_t maxDePlCol = 20;
float aniDePl[] = {0.3, 1.1, 1.1, 0.3, 400.0, -99, 0, 0, 0, 0};

float acUnSpeed = 0.8f;

uint8_t minAcUnBri = 65;
uint8_t maxAcUnBri = 100;

uint8_t minAcUnCol = 80;
uint8_t maxAcUnCol = 100;
float aniAcUn[] = {0.585, 1.255, 0.945, 0.6, 150.0, -99, 0, 0, 0, 0};

float acPlSpeed = 0.7f;

uint8_t minAcPlBri = 80;
uint8_t maxAcPlBri = 100;

uint8_t minAcPlCol = 20;
uint8_t maxAcPlCol = 40;
float aniAcPl[] = {0.5, 1.0, 1.0, 0.5, 400.0, -99, 0, 0, 0, 0};


void setup() {

  Serial.begin(9600);
  
  // Setup FastLED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(0, 0, 0);
    fadeScales[i] = 255;
    FastLED.show();
  }

  currentPalette = MainPalette_p;
  minBrightness = minIdleBri;
  maxBrightness = maxIdleBri;
  minColor = minIdleCol;
  maxColor = maxIdleCol;
  memcpy(currentAnimation, aniIdle, 5 * MAX_ANI *sizeof(float));

  // Setup pot
  pinMode(POT_PIN, INPUT);

  // Setup dopplers
  pinMode(DOPPLER_PIN1, INPUT);
  pinMode(DOPPLER_PIN2, INPUT);
}

void loop() {
  readSensor();
  determineStates();
  setVariablesFromStates();
  fillNoise8();
  showLeds();
}

void readSensor() {
  if(millis() - lastProximityReading > proximityReadingInterval) {
    lastProximityReading = millis();
    lastProximity = currProximity;

    int dopplerVal1 = analogRead(DOPPLER_PIN1);
    int dopplerVal2 = analogRead(DOPPLER_PIN2);

//    Serial.print("Doppler 1: ");
//    Serial.print(dopplerVal1);
//    Serial.print(". Dopper 2: ");
//    Serial.println(dopplerVal2);

    // Test if we have new lower/upper bounds
    if(dopplerVal1 < lowestReading) {
      lowestReading = dopplerVal1;
    }
    if(dopplerVal2 < lowestReading) {
      lowestReading = dopplerVal2;
    }
    if(dopplerVal1 > highestReading) {
      highestReading = dopplerVal1;
    }
    if(dopplerVal2 > highestReading) {
      highestReading = dopplerVal2;
    }

//    Serial.print("Lowest: ");
//    Serial.print(lowestReading);
//    Serial.print(". Highest: ");
//    Serial.println(highestReading);
    
    int currentValue = dopplerVal1 + dopplerVal2;
    
    currProximity = map(currentValue, lowestReading*2, highestReading*2, 0, 100);
    //acceleration = abs(currProximity - lastProximity);
    Serial.println(currProximity);
  }
}

void determineStates() {
  // Determine states 
  if(currProximity < deactiveThreshold) {
    // Idle
    activityState = -1;
    pleasureState = 0;
  }
  else if(currProximity < activeThreshold) {
    // Deactive
    
    // Did we come from IDLE?
    if(activityState == -1) {
      pleasureState = 0;
      firstContactTimer = millis();
    }

    activityState = 0;

    if((millis() - firstContactTimer > firstContactInterval) && (pleasureState == 0)) {
      pleasureState = 1;
    }
  }
  else {
    // Active
    activityState = 1;

    if(acceleration > accelerationThreshold) {
      pleasureState = 0;

      feltUnpleasantTimer = millis();
    }
    else if(millis() - feltUnpleasantTimer > feltUnpleasantInterval) {
      pleasureState = 1;
    }
  }
}

void setVariablesFromStates() {
  if(activityState == -1 && minColor != minIdleCol) {
    // currentPalette = MainPalette_p;
    minColor = minIdleCol;
    maxColor = maxIdleCol;
    speed = lerpFloat(minSpeed, maxSpeed, idleSpeed);
    minBrightness = minIdleBri;
    maxBrightness = maxIdleBri;
    t = 0;
    memcpy(currentAnimation, aniIdle, 5 * MAX_ANI *sizeof(float));
    //Serial.println("My state is IDLE");
    Serial.println("IDLE");
  }
  else if(activityState == 0) {
    if(pleasureState == 0 && minColor != minDeUnCol) {
      // currentPalette = DeUnPalette_p;
      minColor = minDeUnCol;
      maxColor = maxDeUnCol;
      speed = lerpFloat(minSpeed, maxSpeed, deUnSpeed);
      minBrightness = minDeUnBri;
      maxBrightness = maxDeUnBri;
      t = 0;
      memcpy(currentAnimation, aniDeUn, 5 * MAX_ANI *sizeof(float));
      //Serial.println("My state is DEACTIVE UNPLEASANT");
      Serial.println("DU");
    }
    else if(pleasureState == 1 && minColor != minDePlCol) {
      // currentPalette = DePlPalette_p;
      minColor = minDePlCol;
      maxColor = maxDePlCol;
      speed = lerpFloat(minSpeed, maxSpeed, dePlSpeed);
      minBrightness = minDePlBri;
      maxBrightness = maxDePlBri;
      t = 0;
      memcpy(currentAnimation, aniDePl, 5 * MAX_ANI *sizeof(float));
      //Serial.println("My state is DEACTIVE PLEASANT");
      Serial.println("DP");
    }
  }
  else if(activityState == 1) {
    if(pleasureState == 0 && minColor != minAcUnCol) {
      // currentPalette = AcUnPalette_p;
      minColor = minAcUnCol;
      maxColor = maxAcUnCol;
      speed = lerpFloat(minSpeed, maxSpeed, acUnSpeed);
      minBrightness = minAcUnBri;
      maxBrightness = maxAcUnBri;
      t = 0;
      memcpy(currentAnimation, aniAcUn, 5 * MAX_ANI *sizeof(float));
      //Serial.println("My state is ACTIVE UNPLEASANT");
      Serial.println("AU");
    }
    else if(pleasureState == 1 && minColor != minAcPlCol) {
      // currentPalette = AcPlPalette_p;
      minColor = minAcPlCol;
      maxColor = maxAcPlCol;
      speed = lerpFloat(minSpeed, maxSpeed, acPlSpeed);
      minBrightness = minAcPlBri;
      maxBrightness = maxAcPlBri;
      t = 0;
      memcpy(currentAnimation, aniAcPl, 5 * MAX_ANI *sizeof(float));
      //Serial.println("My state is ACTIVE PLEASANT");
      Serial.println("AP");
    }
  }
}

void fillNoise8() {
  for(int i = MIN_LED; i < NUM_LEDS; i++) {
    int ioffset = scale * i;
    noise[i] = inoise8(x + ioffset, y);
  }
  y += (uint16_t) speed;
}

void showLeds() {
  // Figure out which LEDs should be turned on using a bezier curve
  maxLed = (uint8_t) mapFloat(animate(currentAnimation), 0, 1.0f, 0, NUM_LEDS);

  // A higher value makes the fading faster, but more jarring
  for(int i = MIN_LED; i < NUM_LEDS; i++) {
    // Fade out leds that should not be turned on
    if(i > maxLed) {
      if(fadeScales[i] > fadeAmount - 1) {
        fadeScales[i] -= fadeAmount;
      }
    }
    // Fade in leds that should be turned on
    else {
      if(fadeScales[i] < 255 - fadeAmount + 1) {
        fadeScales[i] += fadeAmount;
      }
    }

    // We use the value at the i coordinate in the noise
    // array for our brightness, and the flipped value from NUM_LEDS-i
    // for our pixel's index into the color palette.
    uint8_t brightness = noise[NUM_LEDS - i];
    uint8_t colorIndex = noise[i];
    colorIndex = map(colorIndex, 0, 255, minColor, maxColor);
    colorIndex = (uint8_t) colorIndex * 2.55;

    CRGB color = ColorFromPalette(currentPalette, colorIndex, brightness);
    leds[i] = color;
    leds[i].nscale8(fadeScales[i]);
  }

  LEDS.show();
}

/**
 * Calculates the y value of a bezier curve, which is defined as a float array
 * @param p A float array containing the points of the bezier curve
 * @return The value of point t on the animation that is passed.
 */
float animate(float p[]) {
  float y = 0;

  y = (1 - t) * (1 - t) * (1 - t) * p[iterator + 0] + 3 * (1 - t) * (1 - t) * t * p[iterator + 1] + 3 * (1 - t) * t * t * p[iterator + 2] + t * t * t * p[iterator + 3]; //Cubic Bezier

  duration = p[iterator + 4];

  t += (1 / duration);

  if (t > 1) {
    t = 0;

    iterator += 5;
    if (p[iterator] == -99 || iterator >= MAX_ANI * 5) {
      iterator = 1;
      readyToChangeAnimation = true;
    }

  }
  else {
    readyToChangeAnimation = false;
  }

  return y;
}

/**
 * UTILITY FUNCTIONS
 */

/**
 * Calculates a number between two numbers at a specific increment.
 * @param x The first number
 * @param y The second number
 * @param lerpBy The amount to interpolate between the two values where 0.0 = x, 1.0 = y, and 0.5 is half-way in between. 
 * @return A lerped float
 */
float lerpFloat(float x, float y, float lerpBy) {
  return x - ((x - y) * lerpBy);
}

/**
 * Map a float, since Arduino can only do it with ints out of the box
 * @param x The float to lerp
 * @param in_min The minimum of the current range
 * @param in_max The maximum of the current range
 * @param out_min The minimum of the desired range
 * @param out_max The maximum of the desired range
 * @return A mapped float 
 */
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
