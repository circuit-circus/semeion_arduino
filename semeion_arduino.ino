
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
#include <SimpleTimer.h>

// LED

// Params for width and height
const uint8_t kMatrixWidth = 2;
const uint8_t kMatrixHeight = 58;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 255

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds[NUM_LEDS];

// Doppler sensors
#define DOPPLER_PIN1 A0
#define DOPPLER_PIN2 A1

int currentActivity;
int lastActivity;
const uint8_t deactiveThreshold = 80;
long lastInteractionTime;
const int timeThreshold = 5000;
boolean isActive = false;
boolean isClimaxing = false;
boolean isReadyToReact = true;
const uint8_t climaxThreshold = 20;
uint8_t buildUp = 0;
const uint8_t reactionThreshold = 5;

//Learning values
int aniSpeed;
int chaos;
int baseHue = 150;

int baseSat = 255;

//Animation values
uint8_t confettiHeight;
const uint8_t maxActiveConfettiLeds = 100;
uint8_t numActiveConfettiLeds = 71;
uint8_t activeConfettiLeds[maxActiveConfettiLeds];
uint8_t activeConfettiLedsT[maxActiveConfettiLeds];

boolean isReacting = false;
uint8_t reactionHeight = 0;
const uint8_t maxActiveReactionLeds = 100;
uint8_t numActiveReactionLeds = 0;
uint8_t activeReactionLeds[maxActiveReactionLeds];
uint8_t activeReactionLedsT[maxActiveReactionLeds];

uint8_t animationHeight = 20;
uint8_t rollUpT = 0;
boolean isRollingUp = false;
boolean isRollingDown = false;

uint8_t climaxT;
long lastClimaxTime;
int climaxLength = 1000;

//Curves. Leaving these here for easy reference
//float ease[] = {0, 1.5, 1.07, 0, 50}; // ease-in-out
//float hard[] = {1, 1, 1, 0, 20}; // Hard flash ease out

int valToUse;
int dopplerVal1; // Mellem 500-1024, skal smoothes
const int numReadings = 10;
int lastSensorAverage; // det sidste stykke tids læsninger
int lastSensorTotal;
int lastSensorReadings[numReadings];
int readIndex;

// These will be used to determine lower and higher bounds
int lowestReading = 520;
int highestReading = 100;

boolean goingDown = true;

int iterator = 1;
#define MAX_ANI 1;

SimpleTimer timer;

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );

  pinMode(DOPPLER_PIN1, INPUT);

  timer.setInterval(50, readCalculate);

  fill_solid(leds, kMatrixWidth * kMatrixHeight, CRGB::Black);

  for (int i = 0; i < maxActiveConfettiLeds; i++) {
    activeConfettiLeds[i] = random8(kMatrixWidth * kMatrixHeight - 1);
    activeConfettiLedsT[i] = random8(1, 255);
  }
}

void loop() {
  determineStates();
  setAnimation();
  FastLED.show();
  timer.run();

  //    Serial.print("H ");
  //    Serial.print(confettiHeight);
  Serial.print("DOWN ");
  Serial.print(isRollingDown);
  Serial.print(" UP ");
  Serial.print(isRollingUp);
  Serial.print(", A ");
  Serial.print(currentActivity);
//  Serial.print(", RH ");
//  Serial.print(reactionHeight);
  Serial.print(", AH ");
  Serial.print(animationHeight);
    Serial.print(", R ");
    Serial.print(isReacting);
  //  Serial.print(", RTR ");
  //  Serial.print(isReadyToReact);
  //  Serial.print(", BU ");
  //  Serial.println(buildUp);
  //    Serial.print(", HR ");
  //    Serial.print(highestReading);
  //    Serial.print(", C ");
  //    Serial.println(isClimaxing);
      Serial.print(", AL ");
      Serial.println(numActiveConfettiLeds);
}

void determineStates() {

  if (!isClimaxing) {

    if (currentActivity > deactiveThreshold) {
      //Be active
      if (!isActive) {
        isRollingUp = true;
      }
      isActive = true;
      lastInteractionTime = millis();
    } else if (currentActivity < deactiveThreshold && millis() - lastInteractionTime  > timeThreshold) {
      //Become idle
      if (isActive) {
        isRollingDown = true;
      }
      isActive = false;
      buildUp = 0;
    }

    if (isActive && isReadyToReact && currentActivity - lastActivity > reactionThreshold) {
      isReacting = true;
      buildUp++;
      buildUp = constrain(buildUp, 0, 255);
      reactionHeight = map(currentActivity, reactionThreshold, 255, 0, animationHeight);
    }
    if (isActive && isReadyToReact && currentActivity > 235){
      //Reaction trail  
    }

    if (buildUp > climaxThreshold) {
      //Climax
      isClimaxing = true;
      lastClimaxTime = millis();
      isRollingDown = true;
      isActive = false;
      isReacting = false;
    }

  } else if (millis() - lastClimaxTime > climaxLength && isClimaxing && climaxT >= 254) {
    isClimaxing = false;
    buildUp = 0;
    climaxT = 0;
  }
  if (rollUpT >= 254) {
    isRollingUp = false;
    isRollingDown = false;
    rollUpT = 0;
  }
}

void setAnimation() {

  if (!isClimaxing) {
    //buildUpAnimation();
    confettiAnimation();
    if (isReacting) {
      reactionAnimation();
    }
  } else {
    climaxAnimation();
  }
  if (isRollingUp) {
    rollUpAnimation(false);
  } else if (isRollingDown) {
    rollUpAnimation(true);
  }
}

void readCalculate() {

  dopplerVal1 = analogRead(DOPPLER_PIN1);

  if (dopplerVal1 < lowestReading) {
    lowestReading = dopplerVal1;
  }

  if (dopplerVal1 > highestReading) {
    highestReading = dopplerVal1;
  }

  dopplerVal1 = map(dopplerVal1, lowestReading, highestReading, 0, 255);

  // Calculate last average
  lastSensorTotal = lastSensorTotal - lastSensorReadings[readIndex];
  lastSensorReadings[readIndex] = dopplerVal1;
  lastSensorTotal = lastSensorTotal + lastSensorReadings[readIndex];
  lastSensorAverage = lastSensorTotal / numReadings;
  readIndex += 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  lastActivity = currentActivity;
  currentActivity = lastSensorAverage;
}

void rollUpAnimation(boolean inverse) {
  float curve[5];
  if (!inverse) {
    curve[0] = 0.3;
    curve[1] = 1.3;
    curve[2] = 0.5;
    curve[3] = 1.0;
    curve[4] = 50;
  } else {
    curve[0] = 1.0;
    curve[1] = 0.5;
    curve[2] = 1.3;
    curve[3] = 0.3;
    curve[4] = 50;
  }

  float y = constrain(animate(curve, rollUpT), 0, 1);
  animationHeight = kMatrixHeight * y;
  rollUpT = animateTime(curve[4], rollUpT);
}

void buildUpAnimation() {
  uint8_t brightness = round(255 * (buildUp / 255.0));
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i, j)] = CHSV(200, 0, brightness);
    }
  }
}

void confettiAnimation() {
  float curve[] = {0, 1.5, 1.07, 0, 100}; // ease-in-out
  confettiHeight = animationHeight;

  numActiveConfettiLeds = constrain(int((confettiHeight * kMatrixWidth) * 0.7), 0, maxActiveConfettiLeds);

  for (int i = 0; i < numActiveConfettiLeds; i++) {
    if (activeConfettiLedsT[i] >= 255) {
      activeConfettiLeds[i] = pick(random(kMatrixWidth), random8(kMatrixHeight - confettiHeight, kMatrixHeight), activeConfettiLeds, numActiveConfettiLeds, confettiHeight);
      activeConfettiLedsT[i] = 0;
    }
  }

  for (int i = 0; i < maxActiveConfettiLeds; i++) {

    if (activeConfettiLedsT[i] < 254) {
      float y = animate(curve, activeConfettiLedsT[i]);
      leds[activeConfettiLeds[i]] = CHSV(baseHue + i - int(maxActiveConfettiLeds / 2) , 255,  y * 255);

//      if (currentActivity > 220) {
//        activeConfettiLedsT[i] = animateTime(curve[4], activeConfettiLedsT[i], 2.0);
//      } else {
        activeConfettiLedsT[i] = animateTime(curve[4], activeConfettiLedsT[i]);
//      }
    }
  }
}

void reactionAnimation() {
  float curve[] = {1, 1, 1, 0, 20}; // Hard flash ease out

  boolean stillReacting = false;

  numActiveReactionLeds = constrain(int((reactionHeight * kMatrixWidth) * 0.7), 0, maxActiveReactionLeds);

  if (isReadyToReact) {
    for (int i = 0; i < numActiveReactionLeds; i++) {
      if (activeReactionLedsT[i] >= 255) {
        activeReactionLeds[i] = pick(random(kMatrixWidth), random8(kMatrixHeight - (animationHeight / 2) - (reactionHeight / 2), kMatrixHeight - (animationHeight / 2) + (reactionHeight / 2)), activeReactionLeds, numActiveReactionLeds, reactionHeight);
        activeReactionLedsT[i] = 0;
      }
    }
  }

  isReadyToReact = false;

  for (int i = 0; i < maxActiveReactionLeds; i++) {
    if (activeReactionLedsT[i] < 254) {
      float y = animate(curve, activeReactionLedsT[i]);
      leds[activeReactionLeds[i]] = CHSV(baseHue + i - int(maxActiveReactionLeds / 2) , 100,  y * 255);
      //leds[activeReactionLeds[i]] = CHSV(0, 255,  y * 255);
      activeReactionLedsT[i] = animateTime(curve[4], activeReactionLedsT[i]);
      stillReacting = true;
    }
  }

  if (!stillReacting) {
    isReadyToReact = true;
    isReacting = false;
  }
}

void climaxAnimation() {
  float curve[] = {1, 1, 1, 0, 100};

  float y = animate(curve, climaxT);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i, j)] = CHSV(200 + i - int(maxActiveReactionLeds / 2) , 0, 255 * y);
    }
  }

  climaxT = animateTime(curve[4], climaxT);
}

float animate(float curve[], uint8_t currentTime) {
  float y = 0;
  float t = (float) currentTime / 255.0;

  y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier

  return y;
}

uint8_t animateTime(float duration, uint8_t currentTime, float speedMultiplier) {

  float t = (float) currentTime / 255.0;

  if (t + (speedMultiplier / duration) >= 1.0) {
    t = 1.0;
  } else {
    t += (speedMultiplier / duration);
  }

  uint8_t newTime = constrain((uint8_t)round(t * 255.0), 0, 255);
  return newTime;
}

uint8_t animateTime(float duration, uint8_t currentTime) {
  return animateTime(duration, currentTime, 1.0);
}

int pick(int x_, int y_, uint8_t locationArray[], uint8_t arrayLength, uint8_t height) {
  int x = x_;
  int y = y_;
  int p = XY(x, y);
  int checkedPick;
  boolean isTaken = false;

  //  for (int i = 0; i < arrayLength; i++) {
  //    if (locationArray[i] == p) {
  //      isTaken = true;
  //    }
  //    if (isTaken) {
  //      i = 0;
  //      isTaken = false;
  //
  //      y--;
  //      if (y < 0) {
  //        y = height;
  //        x--;
  //        if (x < 0) {
  //          x = kMatrixWidth;
  //        }
  //      }
  //      p = XY(x, y);
  //    }
  //  }
  checkedPick = p;

  return checkedPick;
}

uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;

  if ( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if ( kMatrixSerpentineLayout == true) {
    if ( x & 0x01) {
      // Odd rows run backwards
      uint8_t reverseY = (kMatrixHeight - 1) - y;
      i = (x * kMatrixHeight) + reverseY;
    } else {
      // Even rows run forwards
      i = (x * kMatrixHeight) + y;
    }
  }

  return i;
}
