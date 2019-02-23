
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
#include <Wire.h>

// LED

// Params for width and height
const uint8_t kMatrixWidth = 2;
const uint8_t kMatrixHeight = 56;

const uint8_t vPixelDensity = 16;
const uint16_t vMatrixHeight = kMatrixHeight * vPixelDensity;
const uint8_t noiseHeight = kMatrixHeight / 2;

// Param for different pixel layoutsd
const bool    kMatrixSerpentineLayout = true;

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 255

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// GENERAL VARIABLES

const uint8_t numSides = 2;

//Animation
volatile uint8_t baseHue = 150;
volatile uint8_t baseSat = 255;

const uint8_t climaxThreshold = 70;
const int timeThreshold = 100;
const uint8_t reactionThreshold = 2;
const uint8_t numConcurrentReactions = 2;
const uint8_t deactiveThreshold[] = {160, 160};

static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;
//static int noiseScale = 200;
static int noiseScale = 1000;
uint8_t noise[2][kMatrixWidth][noiseHeight];
uint8_t paletteRatio = 3;

//DEFINE_GRADIENT_PALETTE(NoisePalette_p) { //Bølgen
//  0, 255, 149, 2,
//  80, 252, 47, 214,
//  170, 81, 50, 255,
//  255, 255, 149, 2
//};

DEFINE_GRADIENT_PALETTE(NoisePalette_p) { //Turkis, magenta, blå
  0, 49, 247, 247,
  80, 50, 47, 249,
  170, 239, 45, 178,
  255, 49, 247, 247
};
//
//DEFINE_GRADIENT_PALETTE(NoisePalette_p) { //Grøn blå
//  0, 48, 244, 215,
//  180, 50, 47, 239,
//  255, 48, 244, 215,
//};

CRGBPalette16 noisePalette( NoisePalette_p );

//Input
const int numReadings = 3;

SimpleTimer timer;

// UNIQUE VARIABLES

CRGB leds[2][NUM_LEDS];

#define DOPPLER0 A0
#define DOPPLER1 A1
#define LED0 6
#define LED1 7

const uint8_t ledPin[] = {LED0, LED1};
const uint8_t dopplerPin[] = {DOPPLER0, DOPPLER1};

int currentActivity[2];
int activityDelta[2];
int lastActivity[2];
long lastInteractionTime[2];

unsigned long startInteractionTime = 0;
unsigned long totalInteractionTime = 0;
volatile uint8_t mappedInteractionTime = 0;

volatile boolean isActive[] = {false, false};
boolean wasActive[] = {false, false};
volatile boolean isClimaxing[] = {false, false};
boolean isReadyToReact[][numConcurrentReactions] = {{true, true}, {true, true}};
volatile boolean isReacting[][numConcurrentReactions] = {{false, false}, {false, false}};
boolean isFadingIn[] = {false, false};

uint8_t buildUp[] = {0, 0};
uint8_t reactionHeight[2][2];
uint8_t animationHeight[] = {20, 20};

uint16_t dotPositionY[2][2]; //Side, dot number
uint16_t dotTarget[2][2];
uint16_t dotOld[2][2];
uint8_t dotT[2][2];
uint8_t hurryT[2][2];
uint8_t relaxT[2][2];
uint8_t beatT[2][2];
boolean isDotMoving[2][2];
boolean isHurrying[2][2];
boolean isRelaxing[2][2];
uint16_t hurryStartY[2][2];
boolean reactionLocked[] = {false, false};

uint16_t activeReactionLedsY[2][2][numConcurrentReactions * 2];
uint8_t activeReactionLedsT[2][2][numConcurrentReactions * 2];

uint8_t fadeInT[] = {0, 0};

uint8_t climaxT[2];

int dopplerVal[2]; // Mellem 500-1024, skal smoothes

int lastSensorAverage[2]; // det sidste stykke tids læsninger
int lastSensorTotal[2];
int lastSensorReadings[2][numReadings];
int readIndex[2];

// These will be used to determine lower and higher bounds
int lowestReading[] = {520, 520};
int highestReading[] = {100, 100};

// I2C Variables
#define SLAVE_ADDRESS 0x08

uint8_t counter = 0;
const uint8_t sendBufferSize = 6;
const uint8_t receiveBufferSize = 9;
uint8_t receiveBuffer[receiveBufferSize];
uint8_t sendBuffer[sendBufferSize];
volatile bool i2cClimax = 0;

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, LED0, COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<CHIPSET, LED1, COLOR_ORDER>(leds[1], NUM_LEDS).setCorrection(TypicalSMD5050);

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

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
//  Serial.print("A0 ");
  Serial.println(currentActivity[0]);
//  Serial.print(", A1 ");
//  Serial.print(currentActivity[1]);
//  Serial.print(", DT ");
//  Serial.print(deactiveThreshold[0]);
//  Serial.print(", DT ");
//  Serial.println(deactiveThreshold[1]);
  //  Serial.print("L ");
  //  Serial.print(lastActivity[0]);
  //  Serial.print("H ");
  //  Serial.println(reactionHeight[0]);
  //  //  //  Serial.print(", RH ");
  //  //  //  //  Serial.print(reactionHeight);
  //   Serial.print(", oY ");
  //   Serial.print(dotOld[0][0] / vPixelDensity);
  //   Serial.print(", cY ");
  //   Serial.print(dotPositionY[0][0] / vPixelDensity);
  //   Serial.print(", tY ");
  //   Serial.println(dotTarget[0][0] / vPixelDensity);
  //  Serial.print(", mH");
  //  Serial.println(vMatrixHeight / vPixelDensity);
  //    Serial.print(", AH ");
  //    Serial.println(animationHeight[0]);
  //  Serial.print(", R1 ");
  //  Serial.print(isReacting[0][0]);
  //  Serial.print(", RT1 ");
  //  Serial.print(isReadyToReact[0][0]);
  //  Serial.print(", R2 ");
  //  Serial.print(isReacting[0][1]);
  //  Serial.print(", RT2 ");
  //  Serial.println(isReadyToReact[0][1]);
  //    Serial.print(", A ");
  //    Serial.print(isActive[0]);
  //    Serial.print(", WA ");
  //    Serial.print(wasActive[0]);
  //    Serial.print(", DM ");
  //    Serial.print(isDotMoving[0][0]);
  //    Serial.print(", H ");
  //    Serial.print(isHurrying[0][0]);
  //    Serial.print(", Ht ");
  //    Serial.println(hurryT[0][0]);
  //      Serial.print(", R ");
  //    Serial.println(isRelaxing[0][0]);
  //   Serial.print("BU ");
  //   Serial.print(buildUp[0]);
  //  Serial.print(", HR ");
  //  Serial.print(highestReading);
  //     Serial.print(", C ");
  //     Serial.println(isClimaxing[0]);
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

      if (currentActivity[s] > deactiveThreshold[s]) {
        //Be active
        isActive[s] = true;
        lastInteractionTime[s] = millis();
      } else if (currentActivity[s] < deactiveThreshold[s] && millis() - lastInteractionTime[s]  > timeThreshold) {
        //Become idle
        totalInteractionTime += millis() - startInteractionTime;
        isActive[s] = false;
        buildUp[s] = constrain(buildUp[s] - 1, 0, climaxThreshold);
      }

      boolean isReady = false;
      for (int i = 0; i < numConcurrentReactions; i++) {
        if (isReadyToReact[s][i]) {
          isReady = true;
        }
      }

      if (isActive[s] && isReady && currentActivity[s] - lastActivity[s] > reactionThreshold && !reactionLocked[s]) {
        triggerReaction(s);
        reactionLocked[s] = true;
        buildUp[s]++;
        // Serial.println(buildUp[s]);
        buildUp[s] = constrain(buildUp[s], 0, 255);
      }
      if (buildUp[s] >= climaxThreshold) {
        //Climax
        isClimaxing[s] = true;
        i2cClimax = 1;
        isActive[s] = false;
        for (int i = 0; i < numConcurrentReactions; i++) {
          isReacting[s][i] = false;
        };
      }
      //if (lastActivity[s] - currentActivity[s] > 5){
      //reactionLocked[s] = false;
      //}
      if (currentActivity[s] < lastActivity[s]) {
        reactionLocked[s] = false;
      }
    }
  }
}

void setAnimation() {
  for (int s = 0; s < numSides; s++) {
    if (!isClimaxing[s]) {
      noiseAnimation(s);
      dotAnimation(s);
      if (isFadingIn[s]) {
        fadeInAnimation(s);
      }
      for (int i = 0; i < numConcurrentReactions; i++) {
        if (isReacting[s][i]) {
          reactionAnimation(s, i);
        }
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
  //  fill_solid(leds[s], kMatrixWidth * kMatrixHeight, CRGB::Black);
  fillnoise8(s);
  mapNoiseToLEDsUsingPalette(s);
}

void dotAnimation(uint8_t s) {
  float curve[] = {0.0, 1.59, 0.03, 1.0, 50};

  for (int x = 0; x < kMatrixWidth; x++) {

    uint8_t bri = quadwave8(beatT[s][x]) / 2 + 127 ;
    bri = constrain(bri, 0, 255);
    beatT[s][x] = animateTime(40 - buildUp[s], beatT[s][x]);
    if (beatT[s][x] > 255) {
      beatT[s][x] = 0;
    }

    if (!wasActive[s] && isActive[s]) {
      //If it is changing to become active, hurry the dots to the middle of the display
      isHurrying[s][x] = true;
      dotOld[s][x] = dotPositionY[s][x];
      dotT[s][x] = 0;
      isRelaxing[s][x] = false;
      relaxT[s][x] = 0;
      startInteractionTime = millis();
    } else if (wasActive[s] && !isActive[s]) {
      //If it is changing to idle. Make the animation start slowly
      isRelaxing[s][x] = true;
      dotOld[s][x] = dotPositionY[s][x];
      dotT[s][x] = 0;
      isHurrying[s][x] = false;
      hurryT[s][x] = 0;
    }

    if (isActive[s]) { //Oscillate
      //If Semeion is active and is not changing. Make the two dots oscillate with the currentActivity level´
      float shift = (float)(climaxThreshold - buildUp[s]) / climaxThreshold;

      uint16_t y = vMatrixHeight / 2 + ((sin8(dotT[s][x] + (x * (127 * shift))) / 255.0 - 0.5) * (max(0, currentActivity[s] - deactiveThreshold[s]) * (vPixelDensity / 5)));

      if (isHurrying[s][x]) {
        dotPositionY[s][x] = hurryAnimation(s, x, y);
      } else {
        dotPositionY[s][x] = y;
        dotOld[s][x] = dotPositionY[s][x];
      }
      dotT[s][x] = animateTime(40, dotT[s][x]);
      isDotMoving[s][x] = true;

    } else { //Explore

      //First check if an animation is running
      if (!isDotMoving[s][x]) {

        //Pick a random target to move to
        dotOld[s][x] = dotPositionY[s][x];
        dotTarget[s][x] = random16(vMatrixHeight);
        isDotMoving[s][x] = true;
      }

      //Animate
      int16_t delta = dotTarget[s][x] - dotOld[s][x];
      int16_t deltaY = round(delta * animate(curve, dotT[s][x]));
      uint8_t adjustedTime = constrain(abs(delta) * curve[4], 30, 255);
      uint16_t y = constrain(dotOld[s][x] + deltaY, 0, vMatrixHeight);

      if (isRelaxing[s][x]) {
        dotPositionY[s][x] = relaxAnimation(s, x, y);
      } else {
        dotPositionY[s][x] = y;
      }

      dotT[s][x] = animateTime(adjustedTime, dotT[s][x]);
    }
    if (dotT[s][x] > 254) {
      isDotMoving[s][x] = false;
      dotT[s][x] = 0;
    }
    CRGB color = ColorFromPalette(noisePalette, baseHue + 127, bri);
    downsampleDots(s, x, dotPositionY[s][x], color);
  }

  if (isActive[s]) {
    wasActive[s] = true;
  } else if (!isActive[s]) {
    wasActive[s] = false;
  }
}

uint16_t hurryAnimation(uint8_t s, uint8_t x, uint16_t y) {
  float curve[] = {0.0, 0.98, 1.0, 1.0, 40.0};

  int16_t delta = y - dotOld[s][x];
  uint16_t easedY = (uint16_t) dotOld[s][x] + (delta * animate(curve, hurryT[s][x]));

  hurryT[s][x] = animateTime(curve[4], hurryT[s][x]);

  //Check if the animation has finished
  if (hurryT[s][x] > 254 ) {
    isHurrying[s][x] = false;
    hurryT[s][x] = 0;
  }
  return constrain(easedY, 0, vMatrixHeight);
}

uint16_t relaxAnimation(uint8_t s, uint8_t x, uint16_t y) {
  float curve[] = {0.0, 0.0, 1.0, 1.0, 80.0};

  int16_t delta = y - dotOld[s][x] ;
  uint16_t easedY = (uint16_t)round(dotOld[s][x] + (delta * animate(curve, relaxT[s][x])));

  relaxT[s][x] = animateTime(curve[4], relaxT[s][x]);

  //Check if the animation has finished
  if (relaxT[s][x] > 254 ) {
    isRelaxing[s][x] = false;
    relaxT[s][x] = 0;
  }
  return constrain(easedY, 0, vMatrixHeight);
}

void triggerReaction(uint8_t s) {
  boolean startedReaction = false;

  for (int i = 0; i < numConcurrentReactions; i++) {
    if (isReacting[s][i] == false && !startedReaction) {
      isReacting[s][i] = true;
      startedReaction = true;
      //Serial.print(startedReaction);Serial.println("s");
      setReactionHeight(s, i);
    }
  }
}

void reactionAnimation(uint8_t s, uint8_t j) {
  float curve[] = {1, 1, 1, 0, 40}; // Hard flash ease out
  float react = (float) baseSat / 255;
  float movement[] = {0, 1, react, react, 40}; // Fast start, slow down

  boolean stillReacting[] = {false, false};


  if (isReadyToReact[s][j]) {
    for (int x = 0; x < kMatrixWidth; x++) {
      for (int i = 0; i < 2; i++) {
        if (activeReactionLedsT[s][x][i + (j * 2)] >= 255) {
          activeReactionLedsY[s][x][i + (j * 2)] = dotPositionY[s][x];
          activeReactionLedsT[s][x][i + (j * 2)] = 0;
        }
      }
    }
  }

  isReadyToReact[s][j] = false;
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int i = 0; i < numConcurrentReactions; i++) {
      if (activeReactionLedsT[s][x][i + (j * 2)] < 254) {
        float deltaB = animate(curve, activeReactionLedsT[s][x][i + (j * 2)]);
        float deltaP = animate(movement, activeReactionLedsT[s][x][i + (j * 2)]);
        float y;
        if ( i % 2) {
          y = activeReactionLedsY[s][x][i + (j * 2)] + (deltaP * reactionHeight[s][j]);
        }
        else {
          y = activeReactionLedsY[s][x][i + (j * 2)] - (deltaP * reactionHeight[s][j]);
        }
        y = round(constrain(y, 0, vMatrixHeight));
        uint8_t bri = (uint8_t) map(reactionHeight[s][j], 0, vMatrixHeight / 2, 150, 255);
        //leds[s][XY(x, y)] += CHSV(0, 0, deltaB * bri);
        CRGB c = CHSV(0, 0, deltaB * bri);
        downsampleDots(s, x, y, c);
        //leds[activeReactionLedsY[s][i]] = CHSV(0, 255,  deltaB * 255);
        activeReactionLedsT[s][x][i + (j * 2)] = animateTime(curve[4], activeReactionLedsT[s][x][i + (j * 2)]);
        stillReacting[j] = true;
      }
    }
  }

  if (!stillReacting[j] && currentActivity[s] < 240) {
    isReadyToReact[s][j] = true;
    isReacting[s][j] = false;
  } else if (!stillReacting[j] && currentActivity[s] > 240) {
    isReadyToReact[s][j] = true;
    //buildUp[s]++;
    setReactionHeight(s, j);
  } else if (stillReacting[j]) {
    //triggerReaction(s);
  }

}

void setReactionHeight(uint8_t s, uint8_t i) {
  uint8_t heightRaw = map(currentActivity[s], 240, 255, 160, vMatrixHeight / 2);
  reactionHeight[s][i] = constrain(heightRaw, 0, vMatrixHeight / 2);
}

void climaxAnimation(uint8_t s) {
  float curve[] = {1, 1, 1, 0, 100};

  float y = animate(curve, climaxT[s]);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      //leds[s][XY(i, j)] = CHSV(baseHue, 150, 255 * y);
      leds[s][XY(i, j)] = ColorFromPalette( noisePalette, baseHue + 127, 255 * y);
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

      leds[s][XY(x, y)].r = constrain(leds[s][XY(x, y)].r - round(255 * delta), 0, 255);
      leds[s][XY(x, y)].g = constrain(leds[s][XY(x, y)].g - round(255 * delta), 0, 255);
      leds[s][XY(x, y)].b = constrain(leds[s][XY(x, y)].b - round(255 * delta), 0, 255);

    }
  }

  fadeInT[s] = animateTime(curve[4], fadeInT[s]);
  if (fadeInT[s] > 254) {
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
  uint8_t noiseSpeed = (buildUp[s] + 10) / 5;
  if ( noiseSpeed < 50) {
    dataSmoothing = 200 - (noiseSpeed * 4);
  }

  for (int i = 0; i < kMatrixWidth; i++) {
    int ioffset = noiseScale * i;
    for (int j = 0; j < noiseHeight; j++) {
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
  //  noiseX += noiseSpeed[s] / 8;
  //  noiseY -= noiseSpeed[s] / 16;
}

void mapNoiseToLEDsUsingPalette(uint8_t s) {
  //static uint8_t ihue = 0;
  uint8_t dimFactor = 10;

  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < noiseHeight; y++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = map(noise[s][kMatrixWidth - x][noiseHeight - y], 0, 255, baseHue - (255 / paletteRatio), baseHue + (255 / paletteRatio));
      uint8_t bri =   round(noise[s][x][y] / dimFactor);

      // if this palette is a 'loop', add a slowly-changing base value
      //      if ( colorLoop) {
      //        index += ihue;
      //      }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      //            if ( bri > 127 ) {
      //              bri = 255;
      //            } else {
      //              bri = dim8_raw( bri * 2);
      //            }

      uint8_t scale = kMatrixHeight / noiseHeight;

      CRGB color = ColorFromPalette( noisePalette, index, bri);

//      uint8_t glitterLimit = (255 / dimFactor);
//      if ( bri > glitterLimit - buildUp[s]) {
//        float factor = (float) (bri - (glitterLimit - buildUp[s])) / glitterLimit;
//        uint8_t highest;
//
//        highest = color.r ;
//        if ( color.g > highest) {
//          highest = color.g;
//        }
//        if (color.b > highest) {
//          highest = color.b;
//        }
//
//        color += CRGB(highest - color.r * factor , highest - color.g * factor, highest - color.b * factor);
//      }

      //color += CRGB(buildUp[s], buildUp[s], buildUp[s]);

      for (int i = 0; i < scale; i++) {
        leds[s][XY(x, constrain(y * scale + i, 0, kMatrixHeight))] = color;
      }
    }
  }

  //  ihue += 1;
}

void downsampleDots(uint8_t s, uint8_t x, uint16_t y, CRGB c) {

  float pos = (float) y / vPixelDensity ;

  float delta = fmod(pos, 1);

  uint8_t r_c = round(c.r * delta);
  uint8_t g_c = round(c.g * delta);
  uint8_t b_c = round(c.b * delta);

  uint8_t r_f = round(c.r * (1.0f - delta));
  uint8_t g_f = round(c.g * (1.0f - delta));
  uint8_t b_f = round(c.b * (1.0f - delta));

  uint8_t floorPos = floor(pos);
  uint8_t ceilPos = ceil(pos);

  leds[s][XY(x, constrain(ceilPos, 0, kMatrixHeight))] += CRGB(r_c, g_c, b_c);
  leds[s][XY(x, constrain(floorPos, 0, kMatrixHeight))] += CRGB(r_f, g_f, b_f);
}

/**
   I2C Functions
*/

// Read data in to buffer, offset in first element.
void receiveData(int byteCount) {
  counter = 0;
  while (Wire.available()) {
    receiveBuffer[counter] = Wire.read();
    // Serial.print(F("Got data: "));
    // Serial.println(receiveBuffer[counter]);
    counter++;
  }

  if (receiveBuffer[0] == 96) {
    isClimaxing[0] = true;
    isClimaxing[1] = true;
  }
  // A faulty connection would send 255
  // so we're also sending 120 to make sure that the connection is solid
  else if (receiveBuffer[0] == 95 && receiveBuffer[1] == 120) {
    baseHue = (uint8_t) receiveBuffer[2];
    baseSat = (uint8_t) receiveBuffer[3];
  }
}

// Use the offset value to select a function
void sendData() {
  if (receiveBuffer[0] == 99) {
    writeStates();
  }
  else if (receiveBuffer[0] == 98) {
    sendSettings();
  }
}

// Write data
void writeStates() {
  sendBuffer[0] = 120;
  sendBuffer[1] = i2cClimax;
  sendBuffer[2] = (isActive[0] ? 1 : 0);
  sendBuffer[3] = (isActive[1] ? 1 : 0);
  sendBuffer[4] = (isReacting[0] ? 1 : 0);
  sendBuffer[5] = (isReacting[1] ? 1 : 0);

  Wire.write(sendBuffer, sendBufferSize);

  if (i2cClimax == 1) {
    i2cClimax = 0;
  }
}

void sendSettings() {
  // Divide by two because we have two sides
  totalInteractionTime /= 2;
  mappedInteractionTime = (uint8_t) map(totalInteractionTime, 0, 60000, 0, 255);

  sendBuffer[0] = 120;
  sendBuffer[1] = baseHue;
  sendBuffer[2] = baseSat;
  sendBuffer[3] = 0;
  sendBuffer[4] = mappedInteractionTime;

  Wire.write(sendBuffer, sendBufferSize);

  totalInteractionTime = 0;
}
