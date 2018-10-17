#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 25

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 3

// Define the array of leds
CRGB leds[NUM_LEDS];

Animation currentAnimation;
Animation incomingAnimation;

//Predefined animations
Animation aniDark;
Animation aniIdleHigh;
Animation aniAppearHigh;
Animation aniMirror;
Animation aniIdleLow;
Animation aniClimax;
Animation aniDisappearFade;
Animation aniDisappearHide;
Animation aniAppearLow;
Animation aniRandomPause;
Animation aniStimulation;

boolean animationEnded;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Serial.begin(9600);

  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB( 0, 0, 0);
  }

  initAnimations();
}

void loop() {

  float y = animate(currentAnimation);

  for (int i = 0; i < 4; i++) {
    leds[i] = CHSV( 224, 187, y * 225);
  }

  FastLED.show();
  delay(10);

  Serial.print(y);
  Serial.print("\t");
  Serial.println(t);
}

float animate(Animation c) {
  float y = 0;

  if (currentAnimation.hasEnded()) {
    if (!currentAnimation.isCycling()) {
      currentAnimation = currentAnimation.getNext();
    }
  }

  y = currentAnimation.animate();
  
  return y;
}

//float appear() {
//  float p[] = {0, 0.85, 1.23, 0.8, 5.0}; //y1, y1c, y2, y2c
//  float y = 0;
//
//  y = (1 - t) * (1 - t) * p[0] + 2 * (1 - t) * t * p[1] + t * t * p[3];
//
//  return y;
//}
//
//float idleHigh() {
//  float p[] = {0.8, 0.8, 1.23, 0.8, 7.0}; //y1, y1c, y2, y2c
//  float y = 0;
//
//  y = (1 - t) * (1 - t) * (1 - t) * p[0] + 3 * (1 - t) * (1 - t) * t * p[1] + 3 * (1 - t) * t * t * p[2] + t * t * t * p[3]; //Cubic Bezier
//
//  return y;
//}

void initAnimations() {

  aniAppear.Set(
  { 0, 0.85, 1.23, 0.8, 5.0 }
  );
  aniIdleHigh.Set(
  {0.8, 0.8, 1.23, 0.8, 7.0 }
  );

}

