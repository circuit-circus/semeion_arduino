
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


// --- INCLUDES ---
#include <FastLED.h>
#include <SimpleTimer.h>
#include <Wire.h>

// --- MATRIX SETTINGS ---
const uint8_t kMatrixWidth = 2;
const uint8_t kMatrixHeight = 56;
const uint8_t vPixelDensity = 16;
const uint16_t vMatrixHeight = kMatrixHeight * vPixelDensity;
const uint8_t noiseHeight = kMatrixHeight / 2;
const bool    kMatrixSerpentineLayout = true;
const uint8_t numSides = 2;

// --- HARDWARE ---
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS 255
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define DOPPLER0 A0
#define DOPPLER1 A1
#define LED0 6
#define LED1 7

// Led
CRGB leds[2][NUM_LEDS];
const uint8_t ledPin[] = {LED0, LED1};
const uint8_t dopplerPin[] = {DOPPLER0, DOPPLER1};

// Doppler
SimpleTimer timer;
const int numReadings = 3;

// --- LEARNING VARIABLES ---
volatile uint8_t learnHue = 150;
volatile uint8_t learnAni1 = 255;
volatile uint8_t learnAni2 = 0;
volatile uint8_t learnAni3 = 0;
volatile uint8_t learnAni4 = 0;
volatile uint8_t learnAni5 = 0;

// --- ANIMATION ---

//Thresholds
const uint8_t climaxThreshold = 60;
const int timeThreshold = 1000;
const uint8_t reactionThreshold = 2;
const uint8_t deactiveThreshold[] = {160, 160};

// Noise
static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;
static int noiseScale = 1000;
uint8_t noise[2][kMatrixWidth][noiseHeight];
uint8_t paletteRatio = 5;

// Farver
DEFINE_GRADIENT_PALETTE(NoisePalette_p) { //Turkis, magenta, blå --- BRUGT PÅ SXSW
  0, 49, 247, 247,
  80, 50, 47, 249,
  170, 239, 45, 178,
  255, 49, 247, 247
};

//DEFINE_GRADIENT_PALETTE(NoisePalette_p) { //Turkis, magenta, blå --- BURDE VÆRE BLEVET BRUGT
//  0, 49, 247, 247,
//  60, 50, 47, 249,
//  120, 239, 45, 178,
//  180, 255, 134, 48,
//  255, 49, 247, 247
//};

CRGBPalette16 noisePalette( NoisePalette_p );

// Interaction Logic
int currentActivity[2];
int activityDelta[2];
int lastActivity[2];
long lastInteractionTime[2];
const uint8_t numConcurrentReactions = 2;

unsigned long startInteractionTime = 0;
unsigned long totalInteractionTime = 0;
volatile uint8_t mappedInteractionTime = 0;

volatile boolean isActive[] = {false, false};
boolean wasActive[] = {false, false};
boolean isReadyToReact[][numConcurrentReactions] = {{true, true}, {true, true}};
volatile boolean isReacting[][numConcurrentReactions] = {{false, false}, {false, false}};
boolean isFadingIn[] = {false, false};
volatile boolean isClimaxing[] = {false, false};
uint8_t buildUp[] = {0, 0};

// Animation parameters
uint8_t reactionHeight[2][2];
uint8_t animationHeight[] = {20, 20};
uint16_t dotPositionY[2][2];
uint16_t dotTarget[2][2];
uint16_t dotOld[2][2];
uint16_t hurryStartY[2][2];

uint8_t dotT[2][2];
uint8_t hurryT[2][2];
uint8_t relaxT[2][2];
uint8_t beatT[2][2];
uint8_t climaxT[2];
uint8_t fadeInT[] = {0, 0};

boolean isDotMoving[2][2];
boolean isHurrying[2][2];
boolean isRelaxing[2][2];
boolean reactionLocked[] = {false, false};

uint16_t activeReactionLedsY[2][2][numConcurrentReactions * 2];
uint8_t activeReactionLedsT[2][2][numConcurrentReactions * 2];

// --- DOPPLER ---
int dopplerVal[2]; // Mellem 500-1024, skal smoothes
int lastSensorAverage[2]; // det sidste stykke tids læsninger
int lastSensorTotal[2];
int lastSensorReadings[2][numReadings];
int readIndex[2];

// These will be used to determine lower and higher bounds
int lowestReading[] = {520, 520};
int highestReading[] = {100, 100};

// --- I2C ---
#define SLAVE_ADDRESS 0x08

uint8_t counter = 0;
const uint8_t sendBufferSize = 8;
const uint8_t receiveBufferSize = 9;
uint8_t receiveBuffer[receiveBufferSize];
uint8_t sendBuffer[sendBufferSize];
volatile bool i2cClimax = 0;
volatile uint8_t sensorStates[] = {0, 0};

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, LED0, COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<CHIPSET, LED1, COLOR_ORDER>(leds[1], NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // A timer is used to limit how often the sensor is read, to save time for animations.
  // The sensor is read at the first opportunity when 50ms has passed. The program is not interrupted.
  timer.setInterval(50, readCalculate);

  //All LEDs are set to black on start up
  for (int s = 0; s < numSides; s++) {
    pinMode(dopplerPin[s], INPUT);
    fill_solid(leds[s], kMatrixWidth * kMatrixHeight, CRGB::Black);
  }

}

void loop() {

  //Determine
  determineStates(); // Determine the state of the program based on sensor values
  setAnimation(); // Update all animations according to the state
  FastLED.show(); // Update LED buffer
  timer.run();   // Advance timer
}

void determineStates() {

  for (int s = 0; s < numSides; s++) { //Do everything on both sides of the being

    if (!isClimaxing[s]) { //It is only necessary to calculate the complex states when the climax animations isn't running.


      //Determine if Being is active or deactive
      if (currentActivity[s] > deactiveThreshold[s]) { // If the activity is aboce the threshold the Being is set to active. The lastInteractionTime is recorded to allow for a grace period when reverting to idle.
        isActive[s] = true;
        lastInteractionTime[s] = millis();
      } else if (currentActivity[s] < deactiveThreshold[s] && millis() - lastInteractionTime[s]  > timeThreshold) { // If the activity is below the threshold, and the grace period has passed, the Being is set to deactive.
        totalInteractionTime += millis() - startInteractionTime;
        isActive[s] = false;
        buildUp[s] = constrain(buildUp[s] - 1, 0, climaxThreshold); // When deactive the buildUp value decreases quickly.
      }

      //Determine if the reaction animation should be played
      boolean isReady = false;

      for (int i = 0; i < numConcurrentReactions; i++) { // Check if there is a available space in the reaction array to start a new animation. If the capacity is not used a new reaction can be triggered.
        if (isReadyToReact[s][i]) {
          isReady = true;
        }
      }

      if (isActive[s] && isReady && currentActivity[s] - lastActivity[s] > reactionThreshold && !reactionLocked[s]) {
        triggerReaction(s); // Start a reaction animation.
        reactionLocked[s] = true; // A lock is used to make sure all reactions doesn't happen at once.
        buildUp[s] += 3; // For every reaction the Being builds up towards a climax.
        buildUp[s] = constrain(buildUp[s], 0, 255);
      }

      if (currentActivity[s] < lastActivity[s]) { // A new reaction can only be triggered if the activity has been decreasing.
        reactionLocked[s] = false;
      }

      if (buildUp[s] >= climaxThreshold) { // If the buildUp is high enough a climax is initiated.
        isClimaxing[0] = true;
        isClimaxing[1] = true;
        i2cClimax = 1;
        isActive[s] = false;
        for (int i = 0; i < numConcurrentReactions; i++) {
          isReacting[s][i] = false;
        }
      }
    }
  }
}

void setAnimation() {
  
  for (int s = 0; s < numSides; s++) { //Do everything on both sides 
    
    if (!isClimaxing[s]) { // If we don't have climax run the other animations
      noiseAnimation(s); // First draw the noise background
      dotAnimation(s); // Then draw the dots
      
      for (int i = 0; i < numConcurrentReactions; i++) { // Check if there are any reactions running and run the animations accordingly.
        if (isReacting[s][i]) {
          reactionAnimation(s, i);
        }
      }
      
      if (isFadingIn[s]) { // If we are fading in after a climax, run that animation last as it works by withdrawing brightness from the other animations. 
        fadeInAnimation(s);
      }
      
    } else { // If we have a climax, simply run the climax animation
      climaxAnimation(s);
    }
  }
}

// This function is called by the timer and reads the doppler sensors
void readCalculate() { 

  for (int s = 0; s < numSides; s++) { //Do everything on both sides
    
    dopplerVal[s] = analogRead(dopplerPin[s]);

    if (dopplerVal[s] < lowestReading[s]) {
      lowestReading[s] = dopplerVal[s];
    }

    if (dopplerVal[s] > highestReading[s]) {
      highestReading[s] = dopplerVal[s];
    }
    
    dopplerVal[s] = map(dopplerVal[s], lowestReading[s], highestReading[s], 0, 255); // Map the readings to an 8 bit integer using the upper and lower boundary for maximum contrast. 

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
    currentActivity[s] = lastSensorAverage[s]; // Save the average as the new activity value
    sensorStates[s] = currentActivity[s]; // The sensorStates values is used when sending the activity via i2c

  }
}

/**
 *  ANIMATIONS 
 */

// This is where all the dot animations and their internal logic is calculated. 
void dotAnimation(uint8_t s) { 
  
  float react3 = (float) learnAni3 / 255;
  float react4 = (float) learnAni4 / 255;
  float react5 = (float) learnAni5 / 255;
  
  float curve[] = {0.0, react3, react4, 1.0, 140.0};
  uint16_t height = 17;

  float scale = (float)buildUp[s] / climaxThreshold;

  for (int x = 0; x < kMatrixWidth; x++) {

    float waveVal = (float) quadwave8(beatT[s][x]) / 255 ;
    uint8_t bri = 255 - (50 + (waveVal * 205)) * scale;
    height = waveVal * (vMatrixHeight / 2) * scale;

    height = constrain(height, vPixelDensity + 1, vMatrixHeight);
    bri = constrain(bri, 0, 255);

    //beatT[s][x] = animateTime(max((climaxThreshold / 3) - buildUp[s], 10), beatT[s][x]);
    beatT[s][x] = animateTime(50 - 30 * scale, beatT[s][x]); // <---- USE THIS LINE
    //beatT[s][x] = animateTime(400, beatT[s][x]);  //<--- DELETE THIS
    if (beatT[s][x] >= 255 - (127 * scale)) {
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
      dotT[s][x] = animateTime(40 - (20 * react5), dotT[s][x]); 
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

    CRGB color = ColorFromPalette(noisePalette, learnHue + 127, bri);
    downsampleDots(s, x, dotPositionY[s][x], height, color);

  }

  if (isActive[s]) {
    wasActive[s] = true;
  } else if (!isActive[s]) {
    wasActive[s] = false;
  }
}

// This animation is used to interpolate from deactive to active animations 
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

// This animation is used to interpolate from active to deactive animations
uint16_t relaxAnimation(uint8_t s, uint8_t x, uint16_t y) { 
  float curve[] = {0.0, 0.0, 0.0, 1.0, 140.0};

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

// This function makes sure that a free spot in the reaction array is used and that it is not triggered twice.
void triggerReaction(uint8_t s) { 
  boolean startedReaction = false;

  for (int i = 0; i < numConcurrentReactions; i++) {
    if (isReacting[s][i] == false && !startedReaction) {
      isReacting[s][i] = true;
      startedReaction = true;
      setReactionHeight(s, i);
    }
  }
}

// This function draws the animation of the lights emanating from the dots
void reactionAnimation(uint8_t s, uint8_t j) {  
  float react1 = (float) learnAni1 / 255;
  float curve[] = {1, react1, react1, 0, 40}; // Hard flash ease out

  float react2 = (float) learnAni2 / 255;
  float movement[] = {0, 1, react2, react2, 40}; // Fast start, slow down
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
        downsampleDots(s, x, y, 17, c);
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

void noiseAnimation(uint8_t s) {
  fillnoise8(s);  // Create noise data
  mapNoiseToLEDsUsingPalette(s); // Map the noise data to the color palette 
}

/**
 *  NOISE FUNCTIONS
 */

// This function generates 8-bit noise values for the matrix. It is taken from the FastLed library examples. 
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
}

// This function generates 8-bit noise values for the matrix. It is taken from the FastLed library examples. 
void mapNoiseToLEDsUsingPalette(uint8_t s) { 
  uint8_t dimFactor = 10;

  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < noiseHeight; y++) {

      uint8_t index = map(noise[s][kMatrixWidth - x][noiseHeight - y], 0, 255, learnHue - (255 / paletteRatio), learnHue + (255 / paletteRatio));
      uint8_t bri =   round(noise[s][x][y] / dimFactor);

      uint8_t scale = kMatrixHeight / noiseHeight;

      CRGB color = ColorFromPalette( noisePalette, index, bri);

      for (int i = 0; i < scale; i++) {
        leds[s][XY(x, constrain(y * scale + i, 0, kMatrixHeight - 1))] = color;
      }
    }
  }
}

/** 
 *  ANIMATION UTILITY FUNCTIONS 
 */

// This function calculates how far the reactionHeight, that controls how far the reaction dots should move 
void setReactionHeight(uint8_t s, uint8_t i) { 
  uint8_t heightRaw = map(currentActivity[s], 240, 255, 160, vMatrixHeight / 2);
  reactionHeight[s][i] = constrain(heightRaw, 0, vMatrixHeight / 2);
}

// The climax animation simply lights up the whole sides in the current palette color and fades out following a curve. 
void climaxAnimation(uint8_t s) { 
  float curve[] = {1, 1, 1, 0, 100};

  float y = animate(curve, climaxT[s]);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[s][XY(i, j)] = ColorFromPalette( noisePalette, learnHue + 127, 255 * y);
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

// Returns a point on a cubic bezier curve based on the incoming time value. 
float animate(float curve[], uint8_t currentTime) { 
  float y = 0;
  float t = (float) currentTime / 255.0;

  y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier algorithm

  return y;
}

// This animations slowly fades in all other animations after a climax.
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

// This function is used to advance the time (T) of an animation. 
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

// Alternate signature of the above function with a default speed. 
uint8_t animateTime(float duration, uint8_t currentTime) { 
  return animateTime(duration, currentTime, 1.0);
}

// This function takes x and y coordinates on the matrix and returns the corresponding position in the LED array. It is taken from the FastLed library examples. 
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


// This functions takes the "virtual" positions of the dots and downsamples them to the actual matrix resolution. This is done to create smooth transitions between physical LEDs. 
// If a dot is passed with a height, so that it is actually a line, the start and end points of the line and all other points than the start and end is turned on. 
// For performance reasons, only the start and end points are run through the downsampling algorithm. 
void downsampleDots(uint8_t s, uint8_t x, uint16_t y, uint16_t h, CRGB c) { 

  float posStart, posEnd, posCenter;

  posCenter = (float) y / vPixelDensity ;

  if (h > vPixelDensity) {

    posStart = (float) (y - (h / 2)) / vPixelDensity;
    posEnd = (float) (y + (h / 2)) / vPixelDensity;

    uint8_t lineLength = ceil(posEnd) - ceil(posStart);
    if (fmod(posStart, 1) == 0) {
      lineLength -= 1;
    }
    if (fmod(posEnd, 1) == 0) {
      lineLength += 1;
    }

    for (int i = 0; i < lineLength; i++) {
      uint8_t linePos = ceil(posStart) + i; //Calculate positions between start and end
      if (fmod(posStart, 1) == 0) {
        linePos += 1;
      }
      leds[s][XY(x, constrain(linePos, 0, kMatrixHeight - 2))] += CRGB(c.r, c.g, c.b);
    }

    downsample(s, x, posStart, c, 1);
    downsample(s, x, posEnd, c, 2);
  } else {
    downsample(s, x, posCenter, c, 3);
  }
}

// Alternate signature of the function above that is used when downsampling single dots. 
void downsampleDots(uint8_t s, uint8_t x, uint16_t y, CRGB c) { 
  downsampleDots(s, x, y, 1, c);
}

// Contains the actual downsampling algorithm
// It used the decimal of the passed position to calculate a ratio in which the two adjacents LEDs should be lit. 
void downsample(uint8_t s, uint8_t x, float pos, CRGB c, uint8_t loc) {
  float delta = fmod(pos, 1);

  if (loc == 1 || loc == 3) {
    uint8_t r_f = round(c.r * (1.0f - delta));
    uint8_t g_f = round(c.g * (1.0f - delta));
    uint8_t b_f = round(c.b * (1.0f - delta));
    uint8_t floorPos = floor(pos);
    leds[s][XY(x, constrain(floorPos, 0, kMatrixHeight - 2))] += CRGB(r_f, g_f, b_f);
  }
  if (loc == 2 || loc == 3 ) {
    uint8_t r_c = round(c.r * delta);
    uint8_t g_c = round(c.g * delta);
    uint8_t b_c = round(c.b * delta);
    uint8_t ceilPos = ceil(pos);
    if (delta == 0) {
      ceilPos += 1;
    }
    leds[s][XY(x, constrain(ceilPos, 0, kMatrixHeight - 2))] += CRGB(r_c, g_c, b_c);
  }
}

/**
*   I2C Functions
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
    learnHue = (uint8_t) receiveBuffer[2];
    learnAni1 = (uint8_t) receiveBuffer[3];
    learnAni2 = (uint8_t) receiveBuffer[4];
    learnAni3 = (uint8_t) receiveBuffer[5];
    learnAni4 = (uint8_t) receiveBuffer[6];
    learnAni5 = (uint8_t) receiveBuffer[7];
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
  sendBuffer[6] = sensorStates[0];
  sendBuffer[7] = sensorStates[1];

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
  sendBuffer[1] = learnHue;
  sendBuffer[2] = learnAni1;
  sendBuffer[3] = learnAni2;
  sendBuffer[4] = learnAni3;
  sendBuffer[5] = learnAni4;
  sendBuffer[6] = learnAni5;
  sendBuffer[7] = mappedInteractionTime;

  Wire.write(sendBuffer, sendBufferSize);

  totalInteractionTime = 0;
}
