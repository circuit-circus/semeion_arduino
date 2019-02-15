
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
const uint8_t vMatrixHeight = kMatrixHeight * 2;

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

const uint8_t climaxThreshold = 5;
const uint8_t deactiveThreshold = 150;
const int timeThreshold = 200;
const uint8_t reactionThreshold = 2;
const uint8_t numActiveReactionLeds = 2;

//#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;
//static uint16_t hueNoiseX;
//static uint16_t hueNoiseY;
//static uint16_t hueNoiseZ;
static int noiseScale = 200;
static int noiseOctaves = 2;
uint16_t noiseSpeed = 1;
uint8_t noise[2][kMatrixWidth][kMatrixHeight];
boolean colorLoop = true;

//DEFINE_GRADIENT_PALETTE(NoisePalette_p) {
//  0, 250, 19, 31,
//  255, 0, 178, 252
//};
DEFINE_GRADIENT_PALETTE(NoisePalette_p) {
  0, 255, 149, 2,
  80, 252, 47, 214,
  170, 81, 50, 255,
  255, 255, 149, 2
};
CRGBPalette16 noisePalette( NoisePalette_p );

//Input
const int numReadings = 3;

SimpleTimer timer;

// UNIQUE VARIABLES

CRGB leds[2][NUM_LEDS];
CRGB vLeds[2][NUM_LEDS*2];

const uint8_t ledPin[] = {3, 2};
const uint8_t dopplerPin[] = {A1, A0};

int currentActivity[2];
int activityDelta[2];
int lastActivity[2];
long lastInteractionTime[2];

boolean isActive[] = {false, false};
boolean wasActive[] = {false, false};
boolean isClimaxing[] = {false, false};
boolean isReadyToReact[] = {true, true};
boolean isReacting[] = {false, false};
boolean isFadingIn[] = {false, false};

uint8_t buildUp[] = {0, 0};
uint8_t reactionHeight[] = {0, 0};
uint8_t animationHeight[] = {20, 20};

//uint8_t numActiveConfettiLeds[] = {71, 71};
//uint8_t activeConfettiLeds[2][maxActiveConfettiLeds];
//uint8_t activeConfettiLedsT[2][maxActiveConfettiLeds];

uint8_t dotPositionY[2][2]; //Side, dot number
uint8_t dotTarget[2][2];
uint8_t dotT[2][2];
uint8_t hurryT[2][2];
boolean isDotMoving[2][2];
boolean isHurrying[2][2];
boolean isRelaxing[2][2];

//uint8_t numActiveReactionLedsY[] = {2, 0};
uint8_t activeReactionLedsY[2][2][numActiveReactionLeds];
uint8_t activeReactionLedsT[2][2][numActiveReactionLeds];

uint8_t fadeInT[] = {0, 0};

uint8_t climaxT[2];
//long lastClimaxTime[2];
//int climaxLength[] = {1000, 1000};

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
//  Serial.print(", A ");
//  Serial.print(currentActivity[0]);
//  Serial.print(", DT ");
//  Serial.println(deactiveThreshold);
  //  //  //  Serial.print(", RH ");
  //  //  //  Serial.print(reactionHeight);
  //  Serial.print(", Y ");
  //  Serial.print(dotPositionY[0][0]);
  //          Serial.print(", T ");
  //          Serial.println(dotTarget[0][0]);
  //    Serial.print(", AH ");
  //    Serial.println(animationHeight[0]);
  //  Serial.print(", R ");
  //  Serial.print(isReacting[0]);
  //  Serial.print(", RTR ");
  //  Serial.print(isReadyToReact[0]);
  //  Serial.print(", A ");
  //  Serial.print(isActive[0]);
  //  Serial.print(", WA ");
  //  Serial.print(wasActive[0]);
  //  Serial.print(", DM ");
  //  Serial.print(isDotMoving[0][0]);
  //  Serial.print(", H ");
  //  Serial.println(isHurrying[0][0]);
  //  Serial.print(", BU ");
  //  Serial.println(buildUp[0]);
  //  Serial.print(", HR ");
  //  Serial.print(highestReading);
    Serial.print(", C ");
    Serial.print(isClimaxing[0]);
      Serial.print(", F ");
    Serial.println(isFadingIn[0]);
  //  Serial.print(", AL ");
  //  Serial.println(numActiveConfettiLeds);
  //  Serial.print("N1 ");
  //  Serial.print(noise[0][0][0]);
  //  Serial.print(", N2 ");
  //  Serial.println(noise[0][1][30]);
}

void determineStates() {

  for (int s = 0; s < numSides; s++) {

    if (!isClimaxing[s]) {

      if (currentActivity[s] > deactiveThreshold) {
        //Be active
        isActive[s] = true;
        lastInteractionTime[s] = millis();
      } else if (currentActivity[s] < deactiveThreshold && millis() - lastInteractionTime[s]  > timeThreshold) {
        //Become idle
        isActive[s] = false;
        buildUp[s] = 0;
      }

      if (isActive[s] && isReadyToReact[s] && currentActivity[s] - lastActivity[s] > reactionThreshold) {
        isReacting[s] = true;
        buildUp[s]++;
        buildUp[s] = constrain(buildUp[s], 0, 255);
        reactionHeight[s] = map(currentActivity[s], reactionThreshold, 255, 0, 50);
      }
      if (buildUp[s] > climaxThreshold) {
        //Climax
        isClimaxing[s] = true;
        isActive[s] = false;
        isReacting[s] = false;
      }
    }
  }
}

void setAnimation() {
  for (int s = 0; s < numSides; s++) {
    if (!isClimaxing[s]) {
      noiseAnimation(s);
      dotAnimation(s);
      if(isFadingIn[s]){
        fadeInAnimation(s);  
      }
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

void noiseAnimation(uint8_t s) {
  fillnoise8(s);
  mapNoiseToLEDsUsingPalette(s);
}

void dotAnimation(uint8_t s) {
  float curve[] = {0.0, 1.59, 0.03, 1.0, 20};

  for (int x = 0; x < kMatrixWidth; x++) {

    if (!wasActive[s] && isActive[s]) {
      //If it is changing to become active, hurry the dots to the middle of the display
      isHurrying[s][x] = true;
    } else if (wasActive[s] && !isActive[s]) {
      //isRelaxing[s][x] = true;
    }

    if (isActive[s] && wasActive[s]) {
      //If Semeion is active and is not changing. Make the two dots oscillate with the currentActivity level
      if (isHurrying[s][x]) {
        dotTarget[s][x] = kMatrixHeight / 2 + (sin8(dotT[s][x]) / 255.0 - 0.5) * (constrain(currentActivity[s] - deactiveThreshold, 0, 255 - deactiveThreshold) / 4);
        hurryAnimation(s, x);
      } else if (isRelaxing[s][x]) {
        dotTarget[s][x] = kMatrixHeight / 2 + (sin8(dotT[s][x]) / 255.0 - 0.5) * (constrain(currentActivity[s] - deactiveThreshold, 0, 255 - deactiveThreshold) / 4);
        //relaxAnimation(s, x);
      } else {
        dotPositionY[s][x] = kMatrixHeight / 2 + (sin8(dotT[s][x]) / 255.0 - 0.5) * (constrain(currentActivity[s] - deactiveThreshold, 0, 255 - deactiveThreshold) / 4);
      }
      leds[s][XY(x, dotPositionY[s][x])] = CRGB::Blue;
      dotT[s][x] += 10 + round(buildUp[s] / 10);
      isDotMoving[s][x] = true;
    } else {
      //First check if an animation is running
      if (!isDotMoving[s][x]) {
        dotT[s][x] = 0;
        dotPositionY[s][x] = dotTarget[s][x];
        if (!isActive[s]) {
          //Pick a random target to move to
          dotTarget[s][x] = (uint8_t)random8(kMatrixHeight);
        }
        isDotMoving[s][x] = true;
      }

      //Animate
      int8_t delta = dotTarget[s][x] - dotPositionY[s][x];
      float deltaY = delta * animate(curve, dotT[s][x]);
      uint8_t adjustedTime = delta * curve[4];
      uint8_t y = round(dotPositionY[s][x] + deltaY);
      //      if ( s == 0 && x == 0) {
      //        Serial.print(" aY ");
      //        Serial.print(y);
      //
      //      }

      leds[s][XY(x, y)] = CRGB::Blue;
      dotT[s][x] = animateTime(adjustedTime, dotT[s][x]);
      //Check if the animation has finished

    }
    if (dotT[s][x] > 253) {
      isDotMoving[s][x] = false;
      dotT[s][x] = 0;
    }
  }

  if (isActive[s]) {
    wasActive[s] = true;
  } else if (!isActive[s]) {
    wasActive[s] = false;
  }
}

void hurryAnimation(uint8_t s, uint8_t x) {
  float curve[] = {0.0, 0.98, 1.0, 1.0, 20};

  int8_t delta =  dotTarget[s][x] - dotPositionY[s][x];
  float deltaY = delta * animate(curve, hurryT[s][x]);
  dotPositionY[s][x] = round(dotPositionY[s][x] + deltaY);
  hurryT[s][x] = animateTime(curve[4], hurryT[s][x]);

  //Check if the animation has finished
  if (hurryT[s][x] > 254 ) {
    isHurrying[s][x] = false;
    hurryT[s][x] = 0;
  }
}

void reactionAnimation(uint8_t s) {
  float curve[] = {1, 1, 1, 0, 20}; // Hard flash ease out
  float movement[] = {0, 1, 1, 0, 20}; // Fast start, slow down

  boolean stillReacting = false;

  if (isReadyToReact[s]) {
    for (int x = 0; x < kMatrixWidth; x++) {
      for (int i = 0; i < numActiveReactionLeds; i++) {
        if (activeReactionLedsT[s][x][i] >= 255) {
          activeReactionLedsY[s][x][i] = dotPositionY[s][x];
          activeReactionLedsT[s][x][i] = 0;
        }
      }
    }
  }

  isReadyToReact[s] = false;
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int i = 0; i < numActiveReactionLeds; i++) {
      if (activeReactionLedsT[s][x][i] < 254) {
        float deltaB = animate(curve, activeReactionLedsT[s][x][i]);
        float deltaP = animate(movement, activeReactionLedsT[s][x][i]);
        float y;
        if ( i % 2) {
          y = activeReactionLedsY[s][x][i] + (deltaP * reactionHeight[s]);
        }
        else {
          y = activeReactionLedsY[s][x][i] - (deltaP * reactionHeight[s]);
        }
        y = round(constrain(y, 0, kMatrixHeight));
        leds[s][XY(x, y)] = CHSV(baseHue + i - int(numActiveReactionLeds / 2) , 0,  deltaB * 255);
        //leds[activeReactionLedsY[s][i]] = CHSV(0, 255,  deltaB * 255);
        activeReactionLedsT[s][x][i] = animateTime(curve[4], activeReactionLedsT[s][x][i]);
        stillReacting = true;

      }
    }
  }

  if (!stillReacting && currentActivity[s] < 240) {
    isReadyToReact[s] = true;
    isReacting[s] = false;
  } else if (!stillReacting && currentActivity[s] > 240) {
    isReadyToReact[s] = true;
  }
}

void climaxAnimation(uint8_t s) {
  float curve[] = {1, 1, 1, 0, 100};

  float y = animate(curve, climaxT[s]);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[s][XY(i, j)] = CHSV(200 + i - int(numActiveReactionLeds / 2) , 0, 255 * y);
    }
  }

  climaxT[s] = animateTime(curve[4], climaxT[s]);

  if (climaxT[s] > 254) {
      isClimaxing[s] = false;
      buildUp[s] = 0;
      climaxT[s] = 0;
      isFadingIn[s] = true;  
  }
}

float animate(float curve[], uint8_t currentTime) {
  float y = 0;
  float t = (float) currentTime / 255.0;

  y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier

  return y;
}

void fadeInAnimation(uint8_t s) {
  float curve[] = {1, 1, 0, 0, 100};

  float delta = animate(curve, fadeInT[s]);

  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {

     leds[s][XY(x,y)].r = constrain(leds[s][XY(x,y)].r - round(255*delta), 0, 255);
     leds[s][XY(x,y)].g = constrain(leds[s][XY(x,y)].g - round(255*delta), 0, 255);
     leds[s][XY(x,y)].b = constrain(leds[s][XY(x,y)].b - round(255*delta), 0, 255);    

    }
  }

  fadeInT[s] = animateTime(curve[4], fadeInT[s]);
  if (fadeInT[s] > 254){
    isFadingIn[s] = false;  
    fadeInT[s] = 0;
  }
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

void fillnoise8(uint8_t s) {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if ( noiseSpeed < 50) {
    dataSmoothing = 200 - (noiseSpeed * 4);
  }

  for (int i = 0; i < kMatrixWidth; i++) {
    int ioffset = noiseScale * i;
    for (int j = 0; j < kMatrixHeight; j++) {
      int joffset = noiseScale * j;

      uint8_t data = inoise8(noiseX + ioffset, noiseY + joffset, noiseZ);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if ( dataSmoothing ) {
        uint8_t olddata = noise[s][i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[s][i][j] = data;
    }
  }

  noiseZ += noiseSpeed;

  // apply slow drift to X and Y, just for visual variation.
  noiseX += noiseSpeed / 8;
  noiseY -= noiseSpeed / 16;
}

void mapNoiseToLEDsUsingPalette(uint8_t s) {
  static uint8_t ihue = 0;

  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[s][kMatrixWidth - x][kMatrixHeight - y];
      uint8_t bri =   round(noise[s][x][y] / 15);

      // if this palette is a 'loop', add a slowly-changing base value
      if ( colorLoop) {
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      //      if ( bri > 127 ) {
      //        bri = 255;
      //      } else {
      //        bri = dim8_raw( bri * 2);
      //      }

      CRGB color = ColorFromPalette( noisePalette, index, bri);
      leds[s][XY(x, y)] = color;
    }
  }

  ihue += 1;
}

void downSampleDots(){

    
}



