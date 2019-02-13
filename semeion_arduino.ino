
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

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 255

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// GENERAL VARIABLES

const uint8_t numSides = 1;

//Animation

int baseHue = 150;
int baseSat = 255;

const uint8_t climaxThreshold = 20;
const uint8_t deactiveThreshold = 150;
const int timeThreshold = 5000;
const uint8_t reactionThreshold = 5;
//const uint8_t maxActiveConfettiLeds = 80;
const uint8_t maxActiveReactionLeds = 10;

//Curves. Leaving these here for easy reference
//float ease[] = {0, 1.5, 1.07, 0, 50}; // ease-in-out
//float hard[] = {1, 1, 1, 0, 20}; // Hard flash ease out

static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;
static uint16_t hueNoiseX;
static uint16_t hueNoiseY;
static uint16_t hueNoiseZ;
static int noiseScale = 5;
static int noiseOctaves = 2;
uint16_t noiseSpeed = 2;

//Input
const int numReadings = 5;

SimpleTimer timer;

// UNIQUE VARIABLES

CRGB leds[2][NUM_LEDS];

const uint8_t ledPin[] = {3, 2};
const uint8_t dopplerPin[] = {A1, A0};

int currentActivity[2];
int lastActivity[2];
long lastInteractionTime[2];

boolean isActive[] = {false, false};
boolean wasActive[] = {false, false};
boolean isClimaxing[] = {false, false};
boolean isReadyToReact[] = {true, true};
boolean isReacting[] = {false, false};

uint8_t buildUp[] = {0, 0};
uint8_t confettiHeight[2];
uint8_t reactionHeight[] = {0, 0};
uint8_t animationHeight[] = {20, 20};

//uint8_t numActiveConfettiLeds[] = {71, 71};
//uint8_t activeConfettiLeds[2][maxActiveConfettiLeds];
//uint8_t activeConfettiLedsT[2][maxActiveConfettiLeds];

uint8_t dotPositionY[2][2]; //Side, dot number
uint8_t dotTarget[2][2];
uint8_t dotT[2][2];
//uint8_t waveT[2][2];
boolean isDotMoving[2][2];

uint8_t numActiveReactionLeds[] = {0, 0};
uint8_t activeReactionLeds[2][maxActiveReactionLeds];
uint8_t activeReactionLedsT[2][maxActiveReactionLeds];

uint8_t rollUpT[] = {0, 0};
boolean isRollingUp[] = {false, false};
boolean isRollingDown[] = {false, false};

uint8_t climaxT[2];
long lastClimaxTime[2];
int climaxLength[] = {1000, 1000};

int dopplerVal[2]; // Mellem 500-1024, skal smoothes

int lastSensorAverage[2]; // det sidste stykke tids læsninger
int lastSensorTotal[2];
int lastSensorReadings[2][numReadings];
int readIndex[2];

// These will be used to determine lower and higher bounds
int lowestReading[] = {520, 520};
int highestReading[] = {100, 100};

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, 3, COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<CHIPSET, 2, COLOR_ORDER>(leds[1], NUM_LEDS).setCorrection(TypicalSMD5050);

  FastLED.setBrightness( BRIGHTNESS );

  timer.setInterval(50, readCalculate);

  for (int s = 0; s < numSides; s++) {
    pinMode(dopplerPin[s], INPUT);

    fill_solid(leds[s], kMatrixWidth * kMatrixHeight, CRGB::Black);
    //    for (int i = 0; i < maxActiveConfettiLeds; i++) {
    //      activeConfettiLeds[s][i] = random8(kMatrixWidth * kMatrixHeight - 1);
    //      activeConfettiLedsT[s][i] = random8(1, 255);
    //    }
  }
}

void loop() {
  determineStates();
  setAnimation();
  FastLED.show();
  timer.run();

  //  Serial.print("H ");
  //  Serial.print(confettiHeight);
  //    Serial.print("DOWN ");
  //    Serial.print(isRollingDown[0]);
  //    Serial.print(" UP ");
  //  //    Serial.print(isRollingUp[0]);
  //        Serial.print(", A ");
  //        Serial.print(currentActivity[0]);
  //  //  //  Serial.print(", RH ");
  //  //  //  Serial.print(reactionHeight);
//          Serial.print(", Y ");
//          Serial.print(dotPositionY[0][0]);
//          Serial.print(", T ");
//          Serial.println(dotTarget[0][0]);
  //    Serial.print(", AH ");
  //    Serial.println(animationHeight[0]);
  //  Serial.print(", R ");
  //  Serial.print(isReacting);
  //  Serial.print(", RTR ");
  //  Serial.print(isReadyToReact);
  //  Serial.print(", BU ");
  //  Serial.println(buildUp);
  //  Serial.print(", HR ");
  //  Serial.print(highestReading);
  //  Serial.print(", C ");
  //  Serial.println(isClimaxing);
  //  Serial.print(", AL ");
  //  Serial.println(numActiveConfettiLeds);
}

void determineStates() {

  for (int s = 0; s < numSides; s++) {

    if (!isClimaxing[s]) {

      if (currentActivity[s] > deactiveThreshold) {
        //Be active
        if (!isActive[s]) {
          isRollingUp[s] = true;
        }
        isActive[s] = true;
        lastInteractionTime[s] = millis();
      } else if (currentActivity[s] < deactiveThreshold && millis() - lastInteractionTime[s]  > timeThreshold) {
        //Become idle
        if (isActive[s]) {
          isRollingDown[s] = true;
        }
        isActive[s] = false;
        buildUp[s] = 0;
      }

      if (isActive[s] && isReadyToReact[s] && currentActivity[s] - lastActivity[s] > reactionThreshold) {
        isReacting[s] = true;
        buildUp[s]++;
        buildUp[s] = constrain(buildUp[s], 0, 255);
        reactionHeight[s] = map(currentActivity[s], reactionThreshold, 255, 0, animationHeight[s]);
      }
      if (isActive[s] && isReadyToReact[s] && currentActivity[s] > 235) {
        //Reaction trail
      }

      if (buildUp[s] > climaxThreshold) {
        //Climax
        isClimaxing[s] = true;
        lastClimaxTime[s] = millis();
        isRollingDown[s] = true;
        isActive[s] = false;
        isReacting[s] = false;
      }

    } else if (millis() - lastClimaxTime[s] > climaxLength[s] && isClimaxing[s] && climaxT[s] >= 254) {
      isClimaxing[s] = false;
      buildUp[s] = 0;
      climaxT[s] = 0;
    }
    if (rollUpT[s] >= 254) {
      isRollingUp[s] = false;
      isRollingDown[s] = false;
      rollUpT[s] = 0;
    }
  }
}

void setAnimation() {
  for (int s = 0; s < numSides; s++) {
    if (!isClimaxing[s]) {
      //buildUpAnimation(s);
      //confettiAnimation(s);
      exploreAnimation(s);
      if (isReacting[s]) {
        reactionAnimation(s);
      }
    } else {
      climaxAnimation(s);
    }
    //    if (isRollingUp[s]) {
    //      rollUpAnimation(s, false);
    //    } else if (isRollingDown[s]) {
    //      rollUpAnimation(s, true);
    //    }
  }
}

void readCalculate() {

  for (int s = 0; s < numSides; s++) {
    dopplerVal[s] = analogRead(dopplerPin[s]);

    if (dopplerVal[s] < lowestReading[s]) {
      lowestReading[s] = dopplerVal[s];
    }

    if (dopplerVal[s] > highestReading[s]) {
      highestReading[s] = dopplerVal[s];
    }

    dopplerVal[s] = map(dopplerVal[s], lowestReading[s], highestReading[s], 0, 255);

    // Calculate last average
    lastSensorTotal[s] = lastSensorTotal[s] - lastSensorReadings[s][readIndex[s]];
    lastSensorReadings[s][readIndex[s]] = dopplerVal[s];
    lastSensorTotal[s] = lastSensorTotal[s] + lastSensorReadings[s][readIndex[s]];
    lastSensorAverage[s] = lastSensorTotal[s] / numReadings;
    readIndex[s] += 1;
    if (readIndex[s] >= numReadings) {
      readIndex[s] = 0;
    }

    lastActivity[s] = currentActivity[s];
    currentActivity[s] = lastSensorAverage[s];
  }
}

void rollUpAnimation(uint8_t s, boolean inverse) {
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

  float y = constrain(animate(curve, rollUpT[s]), 0, 1);
  animationHeight[s] = kMatrixHeight * y;
  rollUpT[s] = animateTime(curve[4], rollUpT[s]);
}

void buildUpAnimation(uint8_t s) {
  uint8_t brightness = round(255 * (buildUp[s] / 255.0));
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[s][XY(i, j)] = CHSV(200, 0, brightness);
    }
  }
}

//void confettiAnimation(uint8_t s) {
//  float curve[] = {0, 1.5, 1.07, 0, 100}; // ease-in-out
//  confettiHeight[s] = animationHeight[s];
//
//  numActiveConfettiLeds[s] = constrain(int((confettiHeight[s] * kMatrixWidth) * 0.7), 0, maxActiveConfettiLeds);
//
//  for (int i = 0; i < numActiveConfettiLeds[s]; i++) {
//    if (activeConfettiLedsT[s][i] >= 255) {
//      activeConfettiLeds[s][i] = pick(random(kMatrixWidth), random8(kMatrixHeight - confettiHeight[s], kMatrixHeight), activeConfettiLeds[s], numActiveConfettiLeds[s], confettiHeight[s]);
//      activeConfettiLedsT[s][i] = 0;
//    }
//  }
//
//  for (int i = 0; i < maxActiveConfettiLeds; i++) {
//
//    if (activeConfettiLedsT[s][i] < 254) {
//      float y = animate(curve, activeConfettiLedsT[s][i]);
//      leds[s][activeConfettiLeds[s][i]] = CHSV(baseHue + i - int(maxActiveConfettiLeds / 2) , 255,  y * 255);
//
//      //      if (currentActivity > 220) {
//      //        activeConfettiLedsT[i] = animateTime(curve[4], activeConfettiLedsT[i], 2.0);
//      //      } else {
//      activeConfettiLedsT[s][i] = animateTime(curve[4], activeConfettiLedsT[s][i]);
//      //      }
//    }
//  }
//}

void exploreAnimation(uint8_t s) {
  float curious[] = {0.0, 1.59, 0.03, 1.0, 20};
  float hurry[] = {0.0, 0.98, 1.0, 1.0, 20};
  float curve[5]; 
  
  //Make explorative animation
  fill_solid(leds[s], kMatrixHeight * kMatrixWidth, CRGB::Black);
  fill_2dnoise8 (leds[s], kMatrixWidth, kMatrixHeight, true, noiseOctaves, noiseX, noiseScale, noiseY, noiseScale, noiseZ, noiseOctaves, hueNoiseX, noiseScale, hueNoiseY, noiseScale, hueNoiseZ, true);

  //fill_solid(leds[s], animationHeight[s] * kMatrixWidth, CRGB::Green);
  noiseZ += noiseSpeed;
  hueNoiseZ += noiseSpeed;

  for (int x = 0; x < 2; x++) {
    
    if (isActive[s] && wasActive[s]) {
       //If Semeion is active and is not changing. Make the two dots oscillate with the currentActivity level
      dotPositionY[s][x] = kMatrixHeight / 2 + round((sin8(dotT[s][x]) / 25.5));
      leds[s][XY(x, dotPositionY[s][x])] = CRGB::Blue;
      dotT[s][x] += 5;
      if (dotT[s][x] > 255){
       dotT[s][x] = 0;
      }
//      if (s == 0 && x == 0){
//        //Serial.println("Oscillating");
//        Serial.println(dotPositionY[s][x]);
//       }
    } else {
      //First check if and animation is running
      if (!isDotMoving[s][x]) {
        dotT[s][x] = 0;
        dotPositionY[s][x] = dotTarget[s][x];
        if (!isActive[s]) {
          if (wasActive){
            //If the animation in changing back from the active state, make sure to use the right animation
            memcpy(curve, curious, 5 * sizeof(float));
          }
          //Pick a random target to move to
          dotTarget[s][x] = (uint8_t)random8(kMatrixHeight);
                if (s == 0 && x == 0){
        Serial.println("Exploring");
       }

        } else if (!wasActive[s] && isActive[s]) {
          //If it is changing to become active, hurry the dots to the middle of the display
          dotTarget[s][x] = (uint8_t)round(kMatrixHeight / 2);
          memcpy(curve, hurry, 5 * sizeof(float));
                          if (s == 0 && x == 0){
        Serial.println("Hurrying in place");
       }

        }
        isDotMoving[s][x] = true;
      }

      //Animate 
      int8_t delta = dotTarget[s][x] - dotPositionY[s][x];
      float deltaY = delta * animate(curve, dotT[s][x]);
      uint8_t adjustedTime = delta * curve[4];
      uint8_t y = round(dotPositionY[s][x] + deltaY);
      
      leds[s][XY(x, y)] = CRGB::Red;
      dotT[s][x] = animateTime(adjustedTime, dotT[s][x]);
      //Check if the animation has finished
      if (dotT[s][x] > 253) {
        isDotMoving[s][x] = false;
      }
    }
  }
  if (isActive[s]) {
    wasActive[s] = true;
  } else {
    wasActive[s] = false;
  }
}
//Move the eyes to the center, and make them oscillate according to the currentActivity level.

void reactionAnimation(uint8_t s) {
  float curve[] = {1, 1, 1, 0, 20}; // Hard flash ease out

  boolean stillReacting = false;

  numActiveReactionLeds[s] = constrain(int((reactionHeight[s] * kMatrixWidth) * 0.7), 0, maxActiveReactionLeds);

  if (isReadyToReact[s]) {
    for (int i = 0; i < numActiveReactionLeds[s]; i++) {
      if (activeReactionLedsT[s][i] >= 255) {
        activeReactionLeds[s][i] = pick(random(kMatrixWidth), random8(kMatrixHeight - (animationHeight[s] / 2) - (reactionHeight[s] / 2), kMatrixHeight - (animationHeight[s] / 2) + (reactionHeight[s] / 2)), activeReactionLeds[s], numActiveReactionLeds[s], reactionHeight[s]);
        activeReactionLedsT[s][i] = 0;
      }
    }
  }

  isReadyToReact[s] = false;

  for (int i = 0; i < maxActiveReactionLeds; i++) {
    if (activeReactionLedsT[s][i] < 254) {
      float y = animate(curve, activeReactionLedsT[s][i]);
      leds[s][activeReactionLeds[s][i]] = CHSV(baseHue + i - int(maxActiveReactionLeds / 2) , 100,  y * 255);
      //leds[activeReactionLeds[s][i]] = CHSV(0, 255,  y * 255);
      activeReactionLedsT[s][i] = animateTime(curve[4], activeReactionLedsT[s][i]);
      stillReacting = true;
    }
  }

  if (!stillReacting) {
    isReadyToReact[s] = true;
    isReacting[s] = false;
  }
}

void climaxAnimation(uint8_t s) {
  float curve[] = {1, 1, 1, 0, 100};

  float y = animate(curve, climaxT[s]);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[s][XY(i, j)] = CHSV(200 + i - int(maxActiveReactionLeds / 2) , 0, 255 * y);
    }
  }

  climaxT[s] = animateTime(curve[4], climaxT[s]);
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

  for (int i = 0; i < arrayLength; i++) {
    if (locationArray[i] == p) {
      isTaken = true;
    }
    if (isTaken) {
      i = 0;
      isTaken = false;

      y--;
      if (y < 0) {
        y = height;
        x--;
        if (x < 0) {
          x = kMatrixWidth;
        }
      }
      p = XY(x, y);
    }
  }
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
