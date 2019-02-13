
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

const uint8_t numSides = 2;

//Animation

int baseHue = 150;
int baseSat = 255;

const uint8_t climaxThreshold = 20;
const uint8_t deactiveThreshold = 150;
const int timeThreshold = 5000;
const uint8_t reactionThreshold = 5;
const uint8_t maxActiveReactionLeds = 10;

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
uint8_t reactionHeight[] = {0, 0};
uint8_t animationHeight[] = {20, 20};

//uint8_t numActiveConfettiLeds[] = {71, 71};
//uint8_t activeConfettiLeds[2][maxActiveConfettiLeds];
//uint8_t activeConfettiLedsT[2][maxActiveConfettiLeds];

uint8_t dotPositionY[2][2]; //Side, dot number
uint8_t dotTarget[2][2];
uint8_t dotT[2][2];
boolean isDotMoving[2][2];

uint8_t numactiveReactionLedsY[] = {0, 0};
uint8_t activeReactionLedsY[2][2][maxActiveReactionLeds];
uint8_t activeReactionLedsT[2][2][maxActiveReactionLeds];

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
  Serial.print(", A ");
  Serial.print(currentActivity[0]);
  Serial.print(", DT ");
  Serial.print(deactiveThreshold);
  //  //  //  Serial.print(", RH ");
  //  //  //  Serial.print(reactionHeight);
  Serial.print(", Y ");
  Serial.print(dotPositionY[0][0]);
  //          Serial.print(", T ");
  //          Serial.println(dotTarget[0][0]);
  //    Serial.print(", AH ");
  //    Serial.println(animationHeight[0]);
  //  Serial.print(", R ");
  //  Serial.print(isReacting);
  //  Serial.print(", RTR ");
  //  Serial.print(isReadyToReact);
  Serial.print(", BU ");
  Serial.println(buildUp[0]);
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
      exploreAnimation(s);
      if (isReacting[s]) {
        reactionAnimation(s);
      }
    } else {
      climaxAnimation(s);
    }
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

void exploreAnimation(uint8_t s) {
  float curious[] = {0.0, 1.59, 0.03, 1.0, 20};
  float hurry[] = {0.0, 0.98, 1.0, 1.0, 20};
  float curve[5];

  //Set a background
  fill_solid(leds[s], kMatrixHeight * kMatrixWidth, CRGB::Black);
  fill_2dnoise8 (leds[s], kMatrixWidth, kMatrixHeight, true, noiseOctaves, noiseX, noiseScale, noiseY, noiseScale, noiseZ, noiseOctaves, hueNoiseX, noiseScale, hueNoiseY, noiseScale, hueNoiseZ, true);

  //Update noise
  noiseZ += noiseSpeed;
  hueNoiseZ += noiseSpeed;

  for (int x = 0; x < 2; x++) {

    if (isActive[s] && wasActive[s]) {
      //If Semeion is active and is not changing. Make the two dots oscillate with the currentActivity level
      dotPositionY[s][x] = kMatrixHeight / 2 + (sin8(dotT[s][x]) / 255.0 - 0.5) * (constrain(currentActivity[s] - deactiveThreshold, 0, 255 - deactiveThreshold) / 5);
      leds[s][XY(x, dotPositionY[s][x])] = CRGB::Blue;
      dotT[s][x] += 10 + round(buildUp[s] / 10);
      if (dotT[s][x] > 255) {
        dotT[s][x] = 0;
      }
      if (s == 0 && x == 0) {
        //Serial.println("Oscillating");
        Serial.println(round(sin8(dotT[s][x]) / 255.0 - 0.5));
      }
    } else {
      //First check if an animation is running
      if (!isDotMoving[s][x]) {
        dotT[s][x] = 0;
        dotPositionY[s][x] = dotTarget[s][x];
        if (!isActive[s]) {
          if (wasActive) {
            //If the animation in changing back from the active state, make sure to use the right animation
            memcpy(curve, curious, 5 * sizeof(float));
          }
          //Pick a random target to move to
          dotTarget[s][x] = (uint8_t)random8(kMatrixHeight);
          //          if (s == 0 && x == 0) {
          //            Serial.println("Exploring");
          //          }

        } else if (!wasActive[s] && isActive[s]) {
          //If it is changing to become active, hurry the dots to the middle of the display
          dotTarget[s][x] = (uint8_t)round(kMatrixHeight / 2);
          memcpy(curve, hurry, 5 * sizeof(float));
          //          if (s == 0 && x == 0) {
          //            Serial.println("Hurrying in place");
          //          }

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
  float movement[] = {0, 1, 1, 0, 20}; // Fast start, slow down

  boolean stillReacting = false;

  numactiveReactionLedsY[s] = 4;

  if (isReadyToReact[s]) {
    for (int x = 0; x < kMatrixWidth; x++) {
      for (int i = 0; i < numactiveReactionLedsY[s]; i++) {
        if (activeReactionLedsT[s][x][i] >= 255) {
          activeReactionLedsY[s][x][i] = XY(x, dotPositionY[s][x]);
          activeReactionLedsT[s][x][i] = 0;
        }
      }
    }
  }

  isReadyToReact[s] = false;
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int i = 0; i < maxActiveReactionLeds; i++) {
      if (activeReactionLedsT[s][x][i] < 254) {
        float deltaB = animate(curve, activeReactionLedsT[s][x][i]);
        float deltaP = animate(movement, activeReactionLedsT[s][x][i]);
        float y;
        if ( i & 0x01) {
          y = activeReactionLedsY[s][x][i] + (deltaP) * 30;
        }
        else {
          y = activeReactionLedsY[s][x][i] - (deltaP) * 30;
        }
        y = constrain(y, 0, kMatrixHeight);
        leds[s][XY(x, y)] = CHSV(baseHue + i - int(maxActiveReactionLeds / 2) , 0,  deltaB * 255);
        //leds[activeReactionLedsY[s][i]] = CHSV(0, 255,  deltaB * 255);
        activeReactionLedsT[s][x][i] = animateTime(curve[4], activeReactionLedsT[s][x][i]);
        stillReacting = true;
      }
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
