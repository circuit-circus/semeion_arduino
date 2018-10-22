#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 25

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.
#define DATA_PIN 3

//Maximum number of curves per animation
#define MAX_ANI 2

// Define the array of leds
CRGB leds[NUM_LEDS];

//Each curve consists of 11 parameters: id, curve1-y1, curve1-ycontrol1, curve1-y2, curve1-ycontrol2, curve1-duration, curve2-y1, curve2-ycontrol1 etc.
float currentAnimation[11];
float incomingAnimation[11];

//Predefined animations
float aniDark[] = {1, 0, 0, 0, 0, 20.0, -99, 0, 0, 0, 0}; //-99 is used to terminate the reading of the array if all the curves are not defined.
//float aniIdleHigh[] = {2, 0.8, 0.8, 1.23, 0.8, 100.0, -99, 0, 0, 0, 0};
float aniIdleHigh[] = {2, 0.8, 0.8, 1.23, 0.8, 100.0, 0.8, 0.2, 0.2, 0.8, 60.0};
float aniAppearHigh[] = {3, 0, 0.6, 1, 0.8, 50.0, -99, 0, 0, 0, 0};
float aniFadeHigh[] = {4, 0.8, 0.2, 0, 0, 50.0, -99, 0, 0, 0, 0};
//float aniMirror[];
//float aniIdleLow[];
//float aniClimax[];
//float aniDisappearHide[];
//float aniAppearLow[];
//float aniRandomPause[];
//float aniStimulation[];



bool animationEnded;
float t;
long startTime;
float duration;
int iterator = 1;

float tweenTime; 
float easing = 0.1;
float targetY;
float startY;
bool isTweening;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Serial.begin(9600);

  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB( 0, 0, 0);
  }

  memcpy(currentAnimation, aniDark, 11 * sizeof(float));
  startTime = millis();
}

void loop() {

  //This logic to trigger animations is only for testing 
  if (millis() - startTime > 5000 && currentAnimation[0] == aniDark[0] && animationEnded) {
    memcpy(currentAnimation, aniAppearHigh, (5 * MAX_ANI + 1)*sizeof(float));
  } else if ( currentAnimation[0] == aniAppearHigh[0] && animationEnded ) {
    memcpy(currentAnimation, aniIdleHigh, (5 * MAX_ANI + 1)*sizeof(float));
    startTime = millis();
  } else if ( millis() - startTime > 7000 && currentAnimation[0] == aniIdleHigh[0] && animationEnded) {
    memcpy(currentAnimation, aniFadeHigh, (5 * MAX_ANI + 1)*sizeof(float));
    startTime = millis();
  } else if ( currentAnimation[0] == aniFadeHigh[0] && animationEnded) {
    memcpy(currentAnimation, aniDark, (5 * MAX_ANI + 1)*sizeof(float));
  }

  float b = animate(currentAnimation);

  for (int i = 0; i < 4; i++) {
    leds[i] = CHSV( 224, 187, b * 225);
  }

  FastLED.show();
  delay(10);

  //Debugging
//  Serial.print(currentAnimation[0]);
//  Serial.print("\t");
//  Serial.print(iterator);
//  Serial.print("\t");
//  Serial.print(b);
//  Serial.print("\t");
//  Serial.println(t);
}

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
      animationEnded = true;
    }

  } else {
    animationEnded = false;
  }

  return y;
}

/** NOT TESTED **/
//Tweening function to tween when a new animation is started before the first one is finished. 
float tweenTo(float p[], float y){
  if (!isTweening){
    startY = y;
    isTweening = true; 
  }
  targetY = animate(p); 
  
  float dy = targetY - startY;

  if (startY == targetY){
    isTweening = false;   
  } else {
    isTweening = true;
  }
  return startY += dy * easing;
}




