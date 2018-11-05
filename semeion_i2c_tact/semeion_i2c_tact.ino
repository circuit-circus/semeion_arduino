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

/** INCLUDES

*/
#include <Tact.h>
#include <Wire.h>
#include <FastLED.h>

/** LED

*/
#define NUM_LEDS 25
#define DATA_PIN 3
#define MAX_ANI 2 //Maximum number of curves per animation
CRGB leds [NUM_LEDS];

//Each curve consists of 11 parameters: id, curve1-y1, curve1-ycontrol1, curve1-y2, curve1-ycontrol2, curve1-duration, curve2-y1, curve2-ycontrol1 etc.
float currentAnimation[11];
float incomingAnimation[11];

//Predefined animations
float aniDark[] = {0, 0.2, 0.1, 0.1, 0.2, 30.0, -99, 0, 0, 0, 0}; //-99 is used to terminate the reading of the array if all the curves are not defined.
float aniIdleHigh[] = {1, 0.8, 0.8, 1.23, 0.8, 100.0, 0.8, 0.2, 0.2, 0.8, 60.0};
float aniAppearHigh[] = {2, 0, 0.6, 1, 0.8, 50.0, -99, 0, 0, 0, 0};
float aniFadeHigh[] = {3, 0.8, 0.2, 0, 0, 50.0, -99, 0, 0, 0, 0};
float aniShock[] = {4, 0.0, 0.145, 0.185, 0.0, 100.0, -99, 0, 0, 0, 0};
// float aniMirror[] = {5, 0.5, 1.195, 0.360, 0.715, 20.0, -99, 0, 0, 0, 0};
float aniIdleLow[] = {6, 0.1, 1.195, 0.360, 0.1, 20.0, -99, 0, 0, 0, 0};
float aniClimax[] = {7, 0.0, 1.145, 1.485, 0.0, 50.0, 0.0, 1.145, 1.485, 0.0, 250.0};
float aniDisappearHide[] = {8, 0.8, 1.195, 0.360, 0.715, 20.0, -99, 0, 0, 0, 0};
float aniAppearLow[] = {9, 0.0, 0.195, 0.360, 0.715, 20.0, -99, 0, 0, 0, 0};
// float aniRandomPause[] = {10, 0.9, 1.195, 1.360, 0.715, 20.0, -99, 0, 0, 0, 0};
// float aniStimulation[] = {11, 0.0, 1.195, 1.360, 0.715, 20.0, -99, 0, 0, 0, 0};

//Animation variables
bool readyToChangeAnimation;
float t;
long startTime;
float duration;
int iterator = 1;
float lastBrightness = 0;

// Animation decision tree variables
bool shouldInteractWithHumans = false;
const uint8_t minProximity = 5;
const uint8_t maxProximity = 12;
const uint8_t closeProximity = 5;
const uint8_t touchProximity = 15;
const uint8_t shockAcc = 3;
unsigned long chargeCounter = 0;
const unsigned long chargeCounterThreshold = 500;

//Tweening
float tweenTime;
float easing = 0.1;
float targetY;
float startY;
bool isTweening;

// Timers 
unsigned long actionTimer = 0;
const unsigned long actionTimerDuration = 15000;

unsigned long shockTimer = 0;
const unsigned long shockTimerDuration = 10000;

/** TACT INITS

*/
Tact Tact(TACT_MULTI);
int SPECTRUMSTART = 73;
const int SENSOR_ID = 0;
const int SPECTRUMSTEPS = 22;
const int SPECTRUMSTEPSIZE = 1;
int spectrum[SPECTRUMSTEPS];

// Tact - Peak averaging
const uint8_t PEAK_SAMPLE_COUNT = 50;
float peakSamples[PEAK_SAMPLE_COUNT];
uint8_t peakSampleIndex = 0;
float peakTotal = 0.0f; // the running peakTotal
float peakAvrg = 0.0f; 

// Tact - Calibration
const uint8_t BASELINE_SAMPLE_COUNT = 5;
float baselineSamples[BASELINE_SAMPLE_COUNT];
uint8_t baselineSampleIndex = 0;
float baselineTotal = 0.0f;
float calibratedBaseline = 0.0f;

static float baselineTolerance = 1.75f; // How much "noise" can be tolerate from the calibratedBaseline?
static long calibrateTime = 6000;

// Tact - Meaningful values
float proximity = 0.0f;
float acceleration = 0.0f;

/** I2C INITS

*/
#define SLAVE_ADDRESS 0x08

// 10 byte data buffer
int receiveBuffer[9];

void setup() {
  Serial.begin(9600);

  // Start Tact toolkit
  Tact.begin();
  Tact.addSensor(SENSOR_ID, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);

  // Start I2C
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Start FastLED
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB( 0, 0, 0);
    FastLED.show();
  }

  memcpy(currentAnimation, aniDark, 11 * sizeof(float));
  startTime = millis();
}

void loop() {
  runTactSensor();
  if(millis() > calibrateTime) {
    showLight();
  }

  /*if(millis() % 500 < 100) {
    Serial.println("Anim\t\t\tInteract\t\tCharge\t\tProx");
    Serial.print(currentAnimation[0]);
    Serial.print("\t\t\t");
    Serial.print(shouldInteractWithHumans);
    Serial.print("\t\t\t");
    Serial.print(chargeCounter);
    Serial.print("\t\t");
    Serial.println(proximity);
  }*/
}

/* Tact Sensor functions */

void runTactSensor() {
  // read Spectrum for sensor
  // the spectrum array holds as many values as defined in SPECTRUMSTEPS
  Tact.readSpectrum(SENSOR_ID, spectrum);

  // Keep track of how many samples we've gotten
  // avrgSampleIndex = avrgSampleIndex < AVRG_SAMPLE_COUNT - 1 ? avrgSampleIndex + 1 : 0;
  peakSampleIndex = peakSampleIndex < PEAK_SAMPLE_COUNT - 1 ? peakSampleIndex + 1 : 0;

  // Collect samples and calculate the average from that
  smoothPeaks();

  // Calibrate in the first 5 seconds
  if (millis() < calibrateTime) {
    calibratedBaseline = calculateBaseline();
    Serial.println("Calibrating");
    calculateProxAndAcc(calibratedBaseline);
  }
  // Else just keep calculating the dist and acceleration
  else {
    calculateProxAndAcc(calculateBaseline());

    // Serial.println(acceleration);
    // Serial.println(proximity);
  }
}

void smoothPeaks() {
  // subtract the last reading:
  peakTotal -= peakSamples[peakSampleIndex];
  // read from the variable to be smoothed:
  peakSamples[peakSampleIndex] = Tact.readPeak(SENSOR_ID);
  // add the reading to the distTotal:
  peakTotal += peakSamples[peakSampleIndex];
  // advance to the next position in the array:
  peakSampleIndex++;

  // if we're at the end of the array...
  if (peakSampleIndex >= PEAK_SAMPLE_COUNT) {
    // ...wrap around to the beginning:
    peakSampleIndex = 0;
  }

  // calculate the average:
  peakAvrg = peakTotal / PEAK_SAMPLE_COUNT;
  delay(1); // delay in between reads for stability
}

float calculateBaseline() {
  // Get next sample
  baselineSamples[baselineSampleIndex] = peakAvrg;

  // Reset calibratedBaseline distTotal
  baselineTotal = 0.0f;
  for (int i = 0; i < BASELINE_SAMPLE_COUNT; i++) {
    // Add samples to calibratedBaseline distTotal
    baselineTotal += baselineSamples[i];
  }
  // calibratedBaseline is equal to the distTotal of the samples divided by the amount of samples, i.e. the average
  float bl = baselineTotal / BASELINE_SAMPLE_COUNT;

  // Keep track of amount of samples gathered
  baselineSampleIndex = baselineSampleIndex < BASELINE_SAMPLE_COUNT - 1 ? baselineSampleIndex + 1 : 0;

  return bl;
}

// Find the proximity and acceleration based on the two baselines
void calculateProxAndAcc(float accelerationBaseline) {
  proximity = abs(peakAvrg - calibratedBaseline);
  if (proximity >= baselineTolerance) {
    proximity = abs(peakAvrg - calibratedBaseline) - baselineTolerance;
  }
  else {
    proximity = 0.0f;
  }
  
  acceleration = abs(peakAvrg - accelerationBaseline);
}

/* I2C Functions */

// Read data in to buffer with the offset in first element, so we can use it in sendData
void receiveData(int byteCount) {
  int counter = 0;
  while (Wire.available()) {
    receiveBuffer[counter] = Wire.read();
    counter++;
  }
}

void sendData() {
  // If the buffer is set to 99, make some data
  if (receiveBuffer[0] == 99) {
    writeData(proximity);
  } 
  else {
    Serial.println("No function for this address");
  }
}

// Write data
void writeData(float newData) {
  char dataString[8];
  // Convert our data to a string/char[] with min length 5 
  dtostrf(newData, 5, 3, dataString);
  Wire.write(dataString);
}

/* LED Functions */

void showLight() {

  // If humans move too fast, shock them
  if(acceleration > shockAcc) {
    shouldInteractWithHumans = false;
    shockTimer = millis();
    chargeCounter = 0;
    memcpy(currentAnimation, aniShock, (5 * MAX_ANI + 1)*sizeof(float));
  }

  // If humans are either not close enough or move too fast, stop reacting to them
  if(proximity <= closeProximity - closeProximity / 2 && shouldInteractWithHumans) {
    shouldInteractWithHumans = false;
  }

  // If we're idling, we should be able to jump out of animations immediately without waiting for the anim to end
  if(currentAnimation[0] == aniIdleHigh[0] && proximity > closeProximity) {
    shouldInteractWithHumans = true;
  }

  float brightness = 0.0f;
  if(!shouldInteractWithHumans && millis() > shockTimer + shockTimerDuration) {

    if(readyToChangeAnimation) {

      // DECISION TREE START
      // 1. If we were not showing anything
      if(currentAnimation[0] == aniDark[0]) {
        // If we see a person close by, fade in
        if(proximity > closeProximity) {
          memcpy(currentAnimation, aniIdleHigh, (5 * MAX_ANI + 1)*sizeof(float));
          shouldInteractWithHumans = true;
          actionTimer = millis();
        }
        else {
          // pause(random)
        }
      }

      // 2. When we have faded in, move to idling
      else if(currentAnimation[0] == aniAppearHigh[0]) {
        memcpy(currentAnimation, aniIdleHigh, (5 * MAX_ANI + 1)*sizeof(float));
      }

      // 3. If we were idling
      else if(currentAnimation[0] == aniIdleHigh[0]) {
        // If people left and the actionTimer has gone off, fade out
        if(proximity <= closeProximity && millis() > actionTimer + actionTimerDuration) {
          memcpy(currentAnimation, aniFadeHigh, (5 * MAX_ANI + 1)*sizeof(float));
        }
      }

      // 4. At the end of shock, change to no animation 
      else if(currentAnimation[0] == aniShock[0]) {
        memcpy(currentAnimation, aniDark, (5 * MAX_ANI + 1)*sizeof(float));
      }

      // 5. At the end of fading out, change to no animation
      else if(currentAnimation[0] == aniFadeHigh[0]) {
        memcpy(currentAnimation, aniDark, (5 * MAX_ANI + 1)*sizeof(float));
      }

      // 6. If we are at a climax, go back to interacting with humans and set idle high as default
      else if(currentAnimation[0] == aniClimax[0]) {
        memcpy(currentAnimation, aniIdleHigh, (5 * MAX_ANI + 1)*sizeof(float));
        shouldInteractWithHumans = true;
        readyToChangeAnimation = true;
      }
      // DECISION TREE END

    }

    brightness = animate(currentAnimation);
  }

  // If we are interacting with humans
  // Not written as else, because if so, we wouldn't go directly from one animation to interaction
  if(shouldInteractWithHumans) {
    t = 0;
    actionTimer = millis();
    chargeCounter++;
    // If humans have been interacting for a long time, and they're real close, then show a climax
    if(chargeCounter > chargeCounterThreshold && proximity >= touchProximity) {
      memcpy(currentAnimation, aniClimax, (5 * MAX_ANI + 1)*sizeof(float));
      chargeCounter = 0;
      shouldInteractWithHumans = false;
    }
    // Being is not yet charged, so just mirror the "prox" to them in brightness
    else {
      brightness = reactToProximity();
    }
  }

  brightness = lerpFloat(lastBrightness, brightness, 0.2f);
  for (int i = 0; i < 4; i++) {
    leds[i] = CHSV( 224, 187, brightness * 225);
  }
  lastBrightness = brightness;

  FastLED.show();
  delay(10);
}

float reactToProximity() {
  float b = 0.0f;
  b = mapFloat(proximity, minProximity, maxProximity, 0.0f, 1.0f);
  b = min(1, b);
  b = max(0, b);
  return b;
}

//Returns the value of point t on the animation that is passed.
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

  } else {
    readyToChangeAnimation = false;
  }

  return y;
}

/** NOT TESTED **/
//Tweening function to tween when a new animation is started before the first one is finished.
float tweenTo(float p[], float y) {
  if (!isTweening) {
    startY = y;
    isTweening = true;
  }
  targetY = animate(p);

  float dy = targetY - startY;

  if (startY == targetY) {
    isTweening = false;
  } else {
    isTweening = true;
  }
    
  return startY += dy * easing;
}

// Utility
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float lerpFloat(float x, float y, float lerpBy) {
  return x - ((x - y) * lerpBy);
}