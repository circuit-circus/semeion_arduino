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

//Animation variables
bool animationEnded;
float t;
long startTime;
float duration;
int iterator = 1;

//Tweening
float tweenTime;
float easing = 0.1;
float targetY;
float startY;
bool isTweening;

/** TACT INITS

*/
Tact Tact(TACT_MULTI);
int SPECTRUMSTART = 73;
const int SENSOR_ID = 3;
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
float distance = 0.0f;
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
    showLight(distance);
  }
}

/* Tact Sensor functions */

void runTactSensor() {
  // read Spectrum for sensor
  // the spectrum array holds as many values as defined in SPECTRUMSTEPS
  Tact.readSpectrum(SENSOR_ID, spectrum);

  // Keep track of how many samples we've gotten
  // avrgSampleIndex = avrgSampleIndex < AVRG_SAMPLE_COUNT - 1 ? avrgSampleIndex + 1 : 0;
  peakSampleIndex = peakSampleIndex < PEAK_SAMPLE_COUNT - 1 ? peakSampleIndex + 1 : 0;

  // Collect samples and calculate the MA from that
  // fillSamples();
  smoothPeaks();

  // Calibrate in the first 5 seconds
  if (millis() < calibrateTime) {
    calibratedBaseline = calculateBaseline();
    Serial.println("Calibrating");
    calculateDistAndAcc(calibratedBaseline);
  }
  // Else just keep calculating the dist and acceleration
  else {
    calculateDistAndAcc(calculateBaseline());

    // Serial.println(acceleration);
    Serial.println(distance);
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

// Find the distance and acceleration based on the two baselines
void calculateDistAndAcc(float accelerationBaseline) {
  distance = abs(peakAvrg - calibratedBaseline);
  if (distance >= baselineTolerance) {
    distance = abs(peakAvrg - calibratedBaseline) - baselineTolerance;
  }
  else {
    distance = 0.0f;
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
    writeData(distance);
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

void showLight(float d) {
  if (d > 5 && currentAnimation[0] == aniDark[0] && animationEnded) {
    memcpy(currentAnimation, aniAppearHigh, (5 * MAX_ANI + 1)*sizeof(float));
  } else if ( currentAnimation[0] == aniAppearHigh[0] && animationEnded ) {
    memcpy(currentAnimation, aniIdleHigh, (5 * MAX_ANI + 1)*sizeof(float));
  } else if ( d <= 5 && currentAnimation[0] == aniIdleHigh[0] && animationEnded) {
    memcpy(currentAnimation, aniFadeHigh, (5 * MAX_ANI + 1)*sizeof(float));
  } else if ( currentAnimation[0] == aniFadeHigh[0] && animationEnded) {
    memcpy(currentAnimation, aniDark, (5 * MAX_ANI + 1)*sizeof(float));
  }

  float b = animate(currentAnimation);

  for (int i = 0; i < 4; i++) {
    leds[i] = CHSV( 224, 187, b * 225);
  }

  FastLED.show();
  delay(10);
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
      animationEnded = true;
    }

  } else {
    animationEnded = false;
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